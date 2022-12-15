//----------------------------------------------------------------------------------------------------
//    ___  ___   __ _ _  ___ ___     _          _ _       ___           _
//   / _ \| _ ) / /| | || _ \ __|   /_\ _  _ __| (_)___  | __|_ _  __ _(_)_ _  ___
//  | (_) | _ \/ _ \_  _|  _/ _|   / _ \ || / _` | / _ \ | _|| ' \/ _` | | ' \/ -_)
//   \__\_\___/\___/ |_||_| |___| /_/ \_\_,_\__,_|_\___/ |___|_||_\__, |_|_||_\___|
//                                                                |___/
//
//  QB64-PE Audio Engine powered by miniaudio (https://miniaud.io/)
//
//  Copyright (c) 2022 Samuel Gomes
//  https://github.com/a740g
//
//-----------------------------------------------------------------------------------------------------

// Set this to 1 if we want to print debug messages to stderr
#define AUDIO_DEBUG 0
#include "audio.h"
#include "condvar.h"

#include <algorithm>
#include <queue>
#include <vector>

// Enable Ogg Vorbis decoding
#define STB_VORBIS_HEADER_ONLY
#include "extras/stb_vorbis.c"

// The main miniaudio header
#include "miniaudio.h"

// Although Matt says we should not be doing this, this has worked out to be ok so far
// We need 'qbs' and also the 'mem' stuff from here
// I am not using 'list' anymore and have migrated the code to use C++ vectors instead
// We'll likely keep the 'include' this way because I do not want to duplicate stuff and cause issues
// For now, we'll wait for Matt until he sorts out things to smaller and logical files
#include "../../libqb.h"

// This should be defined elsewhere (in libqb?). Since it is not, we are doing it here
#define INVALID_MEM_LOCK 1073741821
// This should be defined elsewhere (in libqb?). Since it is not, we are doing it here
#define MEM_TYPE_SOUND 5
// In QuickBASIC false means 0 and true means -1 (sad, but true XD)
#define QB_FALSE MA_FALSE
#define QB_TRUE -MA_TRUE
// This is returned to the caller if handle allocation fails with a -1
// AllocateSoundHandle() does not return 0 because it is a valid internal handle
// Handle 0 is 'handled' as a special case
#define INVALID_SOUND_HANDLE 0
// This is the string that must be passed in the requirements parameter to stream a sound from storage
#define REQUIREMENT_STRING_STREAM "STREAM"

#define SAMPLE_FRAME_SIZE(_type_, _channels_) (sizeof(_type_) * (_channels_))

// This basically checks if the handle is within vector limits and 'isUsed' is set to true
// We are relying on C's boolean short-circuit to not evaluate the last 'isUsed' if previous conditions are false
// Here we are checking > 0 because this is meant to check user handles only
#define IS_SOUND_HANDLE_VALID(_handle_)                                                                                                                        \
    ((_handle_) > 0 && (_handle_) < audioEngine.soundHandles.size() && audioEngine.soundHandles[_handle_]->isUsed &&                                           \
     !audioEngine.soundHandles[_handle_]->autoKill)

#ifdef QB64_WINDOWS
#    define ZERO_VARIABLE(_v_) ZeroMemory(&(_v_), sizeof(_v_))
#else
#    define ZERO_VARIABLE(_v_) memset(&(_v_), 0, sizeof(_v_))
#endif

// This adds our customer backend (format decoders) VTables to our ma_resource_manager_config
void AudioEngineAttachCustomBackendVTables(ma_resource_manager_config *maResourceManagerConfig);

// These are stuff that was not declared anywhere else
// We will wait for Matt to cleanup the C/C++ source file and include header files that declare this stuff
int32 func_instr(int32 start, qbs *str, qbs *substr, int32 passed); // Did not find this declared anywhere
void new_mem_lock();                                                // This is required for MemSound()
void free_mem_lock(mem_lock *lock);                                 // Same as above
#ifndef QB64_WINDOWS
void Sleep(uint32 milliseconds); // There is a non-Windows implementation. However it is not declared anywhere
#endif

extern ptrszint dblock;         // Required for Play(). Did not find this declared anywhere
extern uint64 mem_lock_id;      // Another one that we need for the mem stuff
extern mem_lock *mem_lock_base; // Same as above
extern mem_lock *mem_lock_tmp;  // Same as above

/// @brief Type of sound
enum struct SoundType {
    None,   // No sound or internal sound whose buffer is managed by the QBPE audio engine
    Static, // Static sounds that are completely managed by miniaudio
    Raw     // Raw sound stream that is managed by the QBPE audio engine
};

/// @brief A simple FP32 stereo sample frame
struct SampleFrame {
    float l;
    float r;
};

/// @brief A thread-safe queue
/// @tparam T Data type
template <class T> class SafeQueue {
  private:
    std::queue<T> q;
    libqb_mutex *m;
    libqb_condvar *c;

  public:
    /// @brief Constructor
    SafeQueue() : q() {
        m = libqb_mutex_new();
        c = libqb_condvar_new();
    }

    /// @brief Destructor
    ~SafeQueue() {
        libqb_condvar_free(c);
        libqb_mutex_free(m);
    }

    /// @brief Add an element to the queue
    /// @param t The element
    void enqueue(T t) {
        libqb_mutex_guard lock(m);
        q.push(t);
        libqb_condvar_signal(c);
    }

    /// @brief Get the front element. If the queue is empty, wait till a element is avaiable
    /// @return Returns the front element
    T dequeue() {
        libqb_mutex_guard lock(m);

        while (q.empty())
            libqb_condvar_wait(c, m); // release lock as long as the wait and reaquire it afterwards

        T val = q.front();
        q.pop();

        return val;
    }

    /// @brief Returns number of elements in the queue
    /// @return Number of elements
    std::size_t size() {
        libqb_mutex_guard lock(m);

        return q.size();
    }
};

/// @brief A miniaudiio raw audio stream datasource
struct RawStream {
    ma_data_source_base ds;        // miniaudio data source
    ma_engine *maEngine;           // pointer to a ma_engine object that was passed while creating the data source
    ma_sound *maSound;             // pointer to a ma_sound object that was passed while creating the data source
    SafeQueue<SampleFrame> stream; // raw stereo sample frame stream (which is conveniently block allocated by C++ STL)
    ma_uint32 sampleRate;          // the sample rate reported by ma_engine
};

/// @brief This is what is used by miniaudio to pull a chunk of raw sample frames to play. The samples being read is removed from the queue
/// @param pDataSource Pointer to the raw stream data source (cast to RawStream type)
/// @param pFramesOut The sample frames sent to miniaudio
/// @param frameCount The sample frame count requested by miniaudio
/// @param pFramesRead The sample frame count that was actually sent (this must not exceed frameCount)
/// @return MA_SUCCESS on success or an appropriate MA_FAILED_* value on failure
static ma_result RawStreamOnRead(ma_data_source *pDataSource, void *pFramesOut, ma_uint64 frameCount, ma_uint64 *pFramesRead) {
    if (pFramesRead)
        *pFramesRead = 0;

    if (frameCount == 0)
        return MA_INVALID_ARGS;

    if (!pDataSource)
        return MA_INVALID_ARGS;

    auto pRawStream = (RawStream *)pDataSource;
    auto result = MA_SUCCESS; // must be initialized to MA_SUCCESS
    auto buffer = (SampleFrame *)pFramesOut;
    ma_uint64 sampleFramesRead = 0;

    while (sampleFramesRead < frameCount) {
        if (pRawStream->stream.size() > 0) { // must have one or more sample frames
            *buffer = pRawStream->stream.dequeue();
            ++buffer;           // increment the buffer pointer
            ++sampleFramesRead; // increment the frame counter
        } else
            break; // no more frames to play
    }

    // To keep the stream going, play silence if there are no frames to play
    if (!sampleFramesRead) {
        while (sampleFramesRead < frameCount) {
            *buffer = {};
            ++buffer;
            ++sampleFramesRead;
        }
    }

    if (pFramesRead)
        *pFramesRead = sampleFramesRead;

    return result;
}

/// @brief This is a dummy callback function which just tells miniaudio that it succeeded
/// @param pDataSource Pointer to the raw stream data source (cast to RawStream type)
/// @param frameIndex The frame index to seek to (unused)
/// @return Always MA_SUCCESS
static ma_result RawStreamOnSeek(ma_data_source *pDataSource, ma_uint64 frameIndex) {
    // NOP. Just pretend to be successful.
    (void)pDataSource;
    (void)frameIndex;

    return MA_SUCCESS;
}

/// @brief Returns the audio format to miniaudio
/// @param pDataSource Pointer to the raw stream data source (cast to RawStream type)
/// @param pFormat The ma_format to use (we always return ma_format_f32 because that is what QB64 uses)
/// @param pChannels The number of audio channels (always 2 - stereo)
/// @param pSampleRate The sample rate of the stream (we always return the engine sample rate)
/// @param pChannelMap Sent to ma_channel_map_init_standard
/// @param channelMapCap Sent to ma_channel_map_init_standard
/// @return Always MA_SUCCESS
static ma_result RawStreamOnGetDataFormat(ma_data_source *pDataSource, ma_format *pFormat, ma_uint32 *pChannels, ma_uint32 *pSampleRate,
                                          ma_channel *pChannelMap, size_t channelMapCap) {
    auto pRawStream = (RawStream *)pDataSource;

    if (pFormat)
        *pFormat = ma_format::ma_format_f32; // QB64 SndRaw API uses FP32 samples

    if (pChannels)
        *pChannels = 2; // stereo

    if (pSampleRate)
        *pSampleRate = pRawStream->sampleRate; // we'll play at the audio engine sampling rate

    if (pChannelMap)
        ma_channel_map_init_standard(ma_standard_channel_map_default, pChannelMap, channelMapCap, 2); // stereo

    return MA_SUCCESS;
}

/// @brief Raw stream data source vtable
static ma_data_source_vtable maDataSourceVtableRaw = {
    RawStreamOnRead,          // Returns a bunch of samples from a raw sample stream queue. The samples being returned is removed from the queue
    RawStreamOnSeek,          // NOP for raw sample stream
    RawStreamOnGetDataFormat, // Returns the audio format to miniaudio
    NULL,                     // No notion of a cursor for raw sample stream
    NULL,                     // No notion of a length for raw sample stream
    NULL,                     // Cannot loop raw sample stream
    0                         // flags
};

/// @brief This creates, initializes and sets up a raw stream for playback
/// @param pmaEngine This should come from the QBPE sound engine
/// @param pmaSound This should come from a QBPE sound handle
/// @return Returns a pointer to a data souce if successful, NULL otherwise
static RawStream *RawStreamCreate(ma_engine *pmaEngine, ma_sound *pmaSound) {
    if (!pmaEngine || !pmaSound) { // these should not be NULL
        AUDIO_DEBUG_PRINT("Invalid arguments");

        return nullptr;
    }

    auto pRawStream = new RawStream; // create the data source object
    if (!pRawStream) {
        AUDIO_DEBUG_PRINT("Failed to create data source");

        return nullptr;
    }

    ZERO_VARIABLE(pRawStream->ds);

    auto dataSourceConfig = ma_data_source_config_init();
    dataSourceConfig.vtable = &maDataSourceVtableRaw; // attach the vtable to the data source

    auto result = ma_data_source_init(&dataSourceConfig, &pRawStream->ds);
    if (result != MA_SUCCESS) {
        AUDIO_DEBUG_PRINT("Error %i: failed to initialize data source", result);

        delete pRawStream;

        return nullptr;
    }

    pRawStream->maSound = pmaSound;                                // save the pointer to the ma_sound object (this is basically from a QBPE sound handle)
    pRawStream->maEngine = pmaEngine;                              // save the pointer to the ma_engine object (this should come from the QBPE sound engine)
    pRawStream->sampleRate = ma_engine_get_sample_rate(pmaEngine); // save the sample rate

    result = ma_sound_init_from_data_source(pmaEngine, &pRawStream->ds, 0, NULL, pmaSound); // attach data source to the ma_sound
    if (result != MA_SUCCESS) {
        AUDIO_DEBUG_PRINT("Error %i: failed to initalize sound from data source", result);

        delete pRawStream;

        return nullptr;
    }

    result = ma_sound_start(pmaSound); // play the ma_sound
    if (result != MA_SUCCESS) {
        AUDIO_DEBUG_PRINT("Error %i: failed to start sound playback", result);

        ma_sound_uninit(pmaSound); // delete the ma_sound object

        delete pRawStream;

        return nullptr;
    }

    AUDIO_DEBUG_PRINT("Raw sound stream created");

    return pRawStream;
}

/// @brief Stops and then frees a raw stream data source previously created with RawStreamCreate()
/// @param pRawStream Pointer to the data source object
static void RawStreamDestroy(RawStream *pRawStream) {
    if (pRawStream) {
        auto result = ma_sound_stop(pRawStream->maSound); // stop playback
        AUDIO_DEBUG_CHECK(result == MA_SUCCESS);

        ma_sound_uninit(pRawStream->maSound); // delete the ma_sound object

        delete pRawStream; // delete the raw stream object

        AUDIO_DEBUG_PRINT("Raw sound stream destroyed");
    }
}

/// @brief This pushes a sample frame at the end of the queue
/// @param pRawStream The raw stream object
/// @param l Sample frame left channel data
/// @param r Sample frame right channel data
static void RawStreamPushSampleFrame(RawStream *pRawStream, float l, float r) {
    if (pRawStream)
        pRawStream->stream.enqueue({l, r});
}

/// @brief Returns the length, in sample frames of sound queued
/// @param pRawStream The raw stream object
/// @return The length left to play in sample frames
static ma_uint64 RawStreamGetSampleFramesRemaining(RawStream *pRawStream) {
    if (pRawStream)
        return pRawStream->stream.size();

    return 0;
}

/// @brief Returns the length, in seconds of sound queued
/// @param pRawStream The raw stream object
/// @return The length left to play in seconds
static double RawStreamGetTimeRemaining(RawStream *pRawStream) {
    if (pRawStream) {
        auto sampleFramesRemaining = pRawStream->stream.size();

        // This will help us avoid situations where we can get a non-zero value even if GetSampleFramesRemaining returns 0
        return !sampleFramesRemaining ? 0 : (double)sampleFramesRemaining / (double)pRawStream->sampleRate;
    }

    return 0;
}

/// <summary>
/// Sound handle type
/// This describes every sound the system will ever play (including raw streams).
/// </summary>
struct SoundHandle {
    bool isUsed;          // Is this handle in active use?
    SoundType type;       // Type of sound (see SoundType enum class)
    bool autoKill;        // Do we need to auto-clean this sample / stream after playback is done?
    ma_sound maSound;     // miniaudio sound
    ma_uint32 maFlags;    // miniaudio flags that were used when initializing the sound
    RawStream *rawStream; // Raw sample frame queue
    void *memLockOffset;  // This is a pointer from new_mem_lock()
    uint64 memLockId;     // This is mem_lock_id created by new_mem_lock()

    SoundHandle(const SoundHandle &) = delete;      // No default copy constructor
    SoundHandle &operator=(SoundHandle &) = delete; // No assignment operator

    /// <summary>
    ///	Just initializes some important members.
    /// 'inUse' will be set to true by AllocateSoundHandle().
    /// This is done here, as well as slightly differently in AllocateSoundHandle() for safety.
    /// </summary>
    SoundHandle() {
        isUsed = false;
        type = SoundType::None;
        autoKill = false;
        rawStream = nullptr;
        ZERO_VARIABLE(maSound);
        maFlags = MA_SOUND_FLAG_NO_PITCH | MA_SOUND_FLAG_NO_SPATIALIZATION | MA_SOUND_FLAG_WAIT_INIT;
        memLockId = INVALID_MEM_LOCK;
        memLockOffset = nullptr;
    }
};

/// <summary>
///	Type will help us keep track of the audio engine state
/// </summary>
struct AudioEngine {
    bool isInitialized;                                 // This is set to true if we were able to initialize miniaudio and allocated all required resources
    bool initializationFailed;                          // This is set to true if a past initialization attempt failed
    ma_resource_manager_config maResourceManagerConfig; // miniaudio resource manager configuration
    ma_resource_manager maResourceManager;              // miniaudio resource manager
    ma_engine_config maEngineConfig;                    // miniaudio engine configuration  (will be used to pass in the resource manager)
    ma_engine maEngine;                                 // This is the primary miniaudio engine 'context'. Everything happens using this!
    ma_result maResult;                                 // This is the result of the last miniaudio operation (used for trapping errors)
    ma_uint32 sampleRate;                               // Sample rate used by the miniaudio engine
    int32_t sndInternal;                                // Internal sound handle that we will use for Play(), Beep() & Sound()
    int32_t sndInternalRaw;                             // Internal sound handle that we will use for the QB64 'handle-less' raw stream
    std::vector<SoundHandle *> soundHandles;            // This is the audio handle list used by the engine and by everything else
    int32_t lowestFreeHandle;                           // This is the lowest handle then was recently freed. We'll start checking for free handles from here
    bool musicBackground;                               // Should 'Sound' and 'Play' work in the background or block the caller?

    AudioEngine(const AudioEngine &) = delete;      // No default copy constructor
    AudioEngine &operator=(AudioEngine &) = delete; // No assignment operator

    /// <summary>
    ///	Just initializes some important members.
    /// </summary>
    AudioEngine() {
        isInitialized = initializationFailed = false;
        sampleRate = 0;
        lowestFreeHandle = 0;
        sndInternal = sndInternalRaw = INVALID_SOUND_HANDLE;
        musicBackground = false;
    }

    /// <summary>
    /// This allocates a sound handle. It will return -1 on error.
    /// Handle 0 is used internally for Beep, Sound and Play and thus cannot be used by the user.
    /// Basically, we go through the vector and find an object pointer were 'isUsed' is set as false and return the index.
    /// If such an object pointer is not found, then we add a pointer to a new object at the end of the vector and return the index.
    /// We are using pointers because miniaudio keeps using stuff from ma_sound and these cannot move in memory when the vector is resized.
    /// The handle is put-up for recycling simply by setting the 'isUsed' member to false.
    /// Note that this means the vector will keep growing until the largest handle (index) and never shrink.
    /// The choice of using a vector was simple - performance. Vector performance when using 'indexes' is next to no other.
    /// The vector will be pruned only when snd_un_init gets called.
    /// We will however, be good citizens and will also 'delete' the objects when snd_un_init gets called.
    /// This also increments 'lowestFreeHandle' to allocated handle + 1.
    /// </summary>
    /// <returns>Returns a non-negative handle if successful</returns>
    int32_t AllocateSoundHandle() {
        if (!isInitialized)
            return -1; // We cannot return 0 here. Since 0 is a valid internal handle

        size_t h, vectorSize = soundHandles.size(); // Save the vector size

        // Scan the vector starting from lowestFreeHandle
        // This will help us quickly allocate a free handle and should be a decent optimization for SndPlayCopy()
        for (h = lowestFreeHandle; h < vectorSize; h++) {
            if (!soundHandles[h]->isUsed) {
                AUDIO_DEBUG_PRINT("Recent sound handle %i recycled", h);
                break;
            }
        }

        if (h >= vectorSize) {
            // Scan through the entire vector and return a slot that is not being used
            // Ideally this should execute in extremely few (if at all) senarios
            // Also, this loop should not execute if size is 0
            for (h = 0; h < vectorSize; h++) {
                if (!soundHandles[h]->isUsed) {
                    AUDIO_DEBUG_PRINT("Sound handle %i recycled", h);
                    break;
                }
            }
        }

        if (h >= vectorSize) {
            // If we have reached here then either the vector is empty or there are no empty slots
            // Simply create a new SoundHandle at the back of the vector
            SoundHandle *newHandle = new SoundHandle;

            if (!newHandle)
                return -1; // We cannot return 0 here. Since 0 is a valid internal handle

            soundHandles.push_back(newHandle);
            size_t newVectorSize = soundHandles.size();

            // If newVectorSize == vectorSize then push_back() failed
            if (newVectorSize <= vectorSize) {
                delete newHandle;
                return -1; // We cannot return 0 here. Since 0 is a valid internal handle
            }

            h = newVectorSize - 1; // The handle is simply newVectorSize - 1

            AUDIO_DEBUG_PRINT("Sound handle %i created", h);
        }

        AUDIO_DEBUG_CHECK(soundHandles[h]->isUsed == false);

        // Initializes a sound handle that was just allocated.
        // This will set it to 'in use' after applying some defaults.
        soundHandles[h]->type = SoundType::None;
        soundHandles[h]->autoKill = false;
        soundHandles[h]->rawStream = nullptr;
        ZERO_VARIABLE(soundHandles[h]->maSound);
        // We do not use pitch shifting, so this will give a little performance boost
        // Spatialization is disabled by default but will be enabled on the fly if required
        soundHandles[h]->maFlags = MA_SOUND_FLAG_NO_PITCH | MA_SOUND_FLAG_NO_SPATIALIZATION | MA_SOUND_FLAG_WAIT_INIT;
        soundHandles[h]->memLockId = INVALID_MEM_LOCK;
        soundHandles[h]->memLockOffset = nullptr;
        soundHandles[h]->isUsed = true;

        AUDIO_DEBUG_PRINT("Sound handle %i returned", h);

        lowestFreeHandle = h + 1; // Set lowestFreeHandle to allocated handle + 1

        return (int32_t)(h);
    }

    /// <summary>
    /// The frees and unloads an open sound.
    /// If the sound is playing or looping, it will be stopped.
    /// If the sound is a stream of raw samples then it is stopped and freed.
    /// Finally the handle is invalidated and put-up for recycling.
    /// If the handle being freed is lower than 'lowestFreeHandle' then this saves the handle to 'lowestFreeHandle'.
    /// </summary>
    /// <param name="handle">A sound handle</param>
    void FreeSoundHandle(int32_t handle) {
        if (isInitialized && handle >= 0 && handle < soundHandles.size() && soundHandles[handle]->isUsed) {
            // Sound type specific cleanup
            switch (soundHandles[handle]->type) {
            case SoundType::Static:
                ma_sound_uninit(&soundHandles[handle]->maSound);

                break;

            case SoundType::Raw:
                RawStreamDestroy(soundHandles[handle]->rawStream);
                soundHandles[handle]->rawStream = nullptr;

                break;

            case SoundType::None:
                if (handle != 0)
                    AUDIO_DEBUG_PRINT("Sound type is 'None' when handle value is not 0");

                break;

            default:
                AUDIO_DEBUG_PRINT("Condition not handled"); // It should not come here
            }

            // Invalidate any memsound stuff
            if (soundHandles[handle]->memLockOffset) {
                free_mem_lock((mem_lock *)soundHandles[handle]->memLockOffset);
                soundHandles[handle]->memLockId = INVALID_MEM_LOCK;
                soundHandles[handle]->memLockOffset = nullptr;
            }

            // Now simply set the 'isUsed' member to false so that the handle can be recycled
            soundHandles[handle]->isUsed = false;
            soundHandles[handle]->type = SoundType::None;

            // Save the free hanndle to lowestFreeHandle if it is lower than lowestFreeHandle
            if (handle < lowestFreeHandle)
                lowestFreeHandle = handle;

            AUDIO_DEBUG_PRINT("Sound handle %i marked as free", handle);
        }
    }
};

// This keeps track of the audio engine state
static AudioEngine audioEngine;

/// <summary>
/// This creates 16-bit signed stereo data. The sound buffer is allocated and then returned.
/// Do we really need stereo for Play(), Sound() and Beep()?
/// </summary>
/// <param name="frequency">The sound frequency</param>
/// <param name="length">The duration of the sound in seconds</param>
/// <param name="volume">The volume of the sound (0.0 - 1.0)</param>
/// <param name="soundwave_bytes">A pointer to an integer that will receive the buffer size in bytes. This cannot be NULL</param>
/// <returns></returns>
static ma_uint8 *GenerateWaveform(double frequency, double length, double volume, ma_int32 *soundwave_bytes) {
    static ma_uint8 *data;
    static ma_int32 i;
    static ma_int16 x, lastx;
    static ma_int16 *sp;
    static double samples;
    static ma_int32 samplesi;
    static ma_int32 direction;
    static double value;
    static double volume_multiplier;
    static ma_int32 waveend;
    static double gradient;

    // calculate total number of samples required
    samples = length * audioEngine.sampleRate;
    samplesi = samples;
    if (!samplesi)
        samplesi = 1;

    *soundwave_bytes = samplesi * SAMPLE_FRAME_SIZE(ma_int16, 2);

    // Frequency equal to or above 20000 will produce silence
    // This is per QuickBASIC 4.5 behavior
    if (frequency < 20000) {
        data = (ma_uint8 *)malloc(*soundwave_bytes);
    } else {
        data = (ma_uint8 *)calloc(*soundwave_bytes, sizeof(ma_uint8));
        return data;
    }

    if (!data)
        return nullptr;

    sp = (ma_int16 *)data;

    direction = 1;
    value = 0;
    volume_multiplier = volume * 32767.0;
    waveend = 0;

    // frequency*4.0*length is the total distance value will travel (+1,-2,+1[repeated])
    // samples is the number of steps to do this in
    if (samples)
        gradient = (frequency * 4.0 * length) / samples;
    else
        gradient = 0; // avoid division by 0

    lastx = 1; // set to 1 to avoid passing initial comparison
    for (i = 0; i < samplesi; i++) {
        x = value * volume_multiplier;
        *sp++ = x;
        *sp++ = x;
        if (x > 0) {
            if (lastx <= 0) {
                waveend = i;
            }
        }
        lastx = x;
        if (direction) {
            if ((value += gradient) >= 1.0) {
                direction = 0;
                value = 2.0 - value;
            }
        } else {
            if ((value -= gradient) <= -1.0) {
                direction = 1;
                value = -2.0 - value;
            }
        }
    } // i

    if (waveend)
        *soundwave_bytes = waveend * SAMPLE_FRAME_SIZE(ma_int16, 2);

    return data;
}

/// <summary>
/// Returns the of a sound buffer in bytes.
/// </summary>
/// <param name="length">Length in seconds</param>
/// <returns>Length in bytes</returns>
static ma_int32 WaveformBufferSize(double length) {
    static ma_int32 samples;

    samples = (ma_int32)(length * audioEngine.sampleRate);
    if (!samples)
        samples = 1;

    return samples * SAMPLE_FRAME_SIZE(ma_int16, 2);
}

/// <summary>
/// This sends a buffer to a raw queue for playback.
/// Buffer required in 16-bit stereo at native frequency.
/// The buffer is freed.
/// </summary>
/// <param name="data">Sound buffer</param>
/// <param name="bytes">Length of buffer in bytes</param>
/// <param name="block">So we have to wait until playback completes</param>
/// <param name="sndRawQueue">A pointer to a raw queue object</param>
static void SendWaveformToQueue(ma_uint8 *data, ma_int32 bytes, bool block) {
    if (!data)
        return;

    // Move data into sndraw handle
    for (auto i = 0; i < bytes; i += SAMPLE_FRAME_SIZE(ma_int16, 2)) {
        RawStreamPushSampleFrame(audioEngine.soundHandles[audioEngine.sndInternal]->rawStream, (float)((ma_int16 *)(data + i))[0] / 32768.0f,
                                 (float)((ma_int16 *)(data + i))[1] / 32768.0f);
    }

    free(data); // free the sound data

    // This will wait for the block to finish (if specified)
    // We'll be good citizens and give-up our time-slices while waiting
    if (block) {
        auto time_ms = (ma_int64)(RawStreamGetTimeRemaining(audioEngine.soundHandles[audioEngine.sndInternal]->rawStream) * 950.0 - 250.0);
        if (time_ms > 0)
            Sleep(time_ms);
    }
}

/// <summary>
/// This generates a sound at the specified frequency for the specified amount of time.
/// </summary>
/// <param name="frequency">Sound frequency</param>
/// <param name="lengthInClockTicks">Duration in clock ticks. There are 18.2 clock ticks per second</param>
void sub_sound(double frequency, double lengthInClockTicks) {
    static ma_uint8 *data;
    static ma_int32 soundwave_bytes;

    if (new_error || !audioEngine.isInitialized || audioEngine.sndInternal != 0)
        return;

    if ((frequency < 37.0) && (frequency != 0))
        goto error;
    if (frequency > 32767.0)
        goto error;
    if (lengthInClockTicks < 0.0)
        goto error;
    if (lengthInClockTicks > 65535.0)
        goto error;
    if (lengthInClockTicks == 0.0)
        return;

    // Initialize the raw stream if we have not already
    if (!audioEngine.soundHandles[audioEngine.sndInternal]->rawStream) {
        audioEngine.soundHandles[audioEngine.sndInternal]->rawStream =
            RawStreamCreate(&audioEngine.maEngine, &audioEngine.soundHandles[audioEngine.sndInternal]->maSound);
        AUDIO_DEBUG_CHECK(audioEngine.soundHandles[audioEngine.sndInternal]->rawStream != nullptr);

        if (!audioEngine.soundHandles[audioEngine.sndInternal]->rawStream)
            return;

        audioEngine.soundHandles[audioEngine.sndInternal]->type = SoundType::Raw;
    }

    data = GenerateWaveform(frequency, lengthInClockTicks / 18.2, 1, &soundwave_bytes);
    SendWaveformToQueue(data, soundwave_bytes, !audioEngine.musicBackground);

    return;

error:
    error(5);
}

/// <summary>
/// This generates a default 'beep' sound.
/// </summary>
void sub_beep() { sub_sound(900, 5); }

/// <summary>
/// This was designed to returned the number of notes in the background music queue.
/// However, here we'll just return the number of sample frame remaining to play when Play(), Sound() or Beep() are used.
/// This allows programs like the following to compile and work.
///
///     Music$ = "MBT180o2P2P8L8GGGL2E-P24P8L8FFFL2D"
///     PLAY Music$
///     WHILE PLAY(0) > 5: WEND
///     PRINT "Just about done!"
/// </summary>
/// <param name="ignore">Well, it's ignored</param>
/// <returns>Returns the number of sample frames left to play for Play(), Sound() & Beep()</returns>
int32_t func_play(int32_t ignore) {
    if (audioEngine.isInitialized && audioEngine.sndInternal == 0) {
        return (int32_t)RawStreamGetSampleFramesRemaining(audioEngine.soundHandles[audioEngine.sndInternal]->rawStream);
    }

    return 0;
}

/// <summary>
/// Processes and plays the MML specified in the string.
/// Mmmmm. Spaghetti goodness.
/// Formats:
/// A[#|+|-][0-64]
/// 0-64 is like temp. Lnumber, 0 is whatever the current default is
/// </summary>
/// <param name="str">The string to play</param>
void sub_play(qbs *str) {
    static ma_int32 soundwave_bytes;
    static ma_uint8 *b, *wave, *wave2;
    static double d;
    static ma_int32 i, bytes_left, a, x, x2, x3, x4, wave_bytes, wave_base;
    static ma_int32 o = 4;
    static double t = 120; // quarter notes per minute (120/60=2 per second)
    static double l = 4;
    static double pause = 1.0 / 8.0; // ML 0.0, MN 1.0/8.0, MS 1.0/4.0
    static double length, length2;   // derived from l and t
    static double frequency;
    static double v = 50;
    static ma_int32 n;         // the semitone-intervaled note to be played
    static ma_int32 n_changed; //+,#,- applied?
    static ma_int64 number;
    static ma_int32 number_entered;
    static ma_int32 followup; // 1=play note
    static ma_int32 playit;
    static ma_int32 fullstops = 0;

    if (new_error || !audioEngine.isInitialized || audioEngine.sndInternal != 0)
        return;

    // Initialize the raw stream if we have not already
    if (!audioEngine.soundHandles[audioEngine.sndInternal]->rawStream) {
        audioEngine.soundHandles[audioEngine.sndInternal]->rawStream =
            RawStreamCreate(&audioEngine.maEngine, &audioEngine.soundHandles[audioEngine.sndInternal]->maSound);
        AUDIO_DEBUG_CHECK(audioEngine.soundHandles[audioEngine.sndInternal]->rawStream != nullptr);

        if (!audioEngine.soundHandles[audioEngine.sndInternal]->rawStream)
            return;

        audioEngine.soundHandles[audioEngine.sndInternal]->type = SoundType::Raw;
    }

    b = str->chr;
    bytes_left = str->len;
    wave = NULL;
    wave_bytes = 0;
    n_changed = 0;
    n = 0;
    number_entered = 0;
    number = 0;
    followup = 0;
    length = 1.0 / (t / 60.0) * (4.0 / l);
    playit = 0;
    wave_base = 0; // point at which new sounds will be inserted

next_byte:
    if ((bytes_left--) || followup) {

        if (bytes_left < 0) {
            i = 32;
            goto follow_up;
        }

        i = *b++;
        if (i == 32)
            goto next_byte;
        if (i >= 97 && i <= 122)
            a = i - 32;
        else
            a = i;

        if (i == 61) { //= (+VARPTR$)
            if (fullstops) {
                error(5);
                return;
            }
            if (number_entered) {
                error(5);
                return;
            }
            number_entered = 2;
            // VARPTR$ reference
            /*
               'BYTE=1
               'INTEGER=2
               'STRING=3 SUB-STRINGS must use "X"+VARPTR$(string$)
               'SINGLE=4
               'INT64=5
               'FLOAT=6
               'DOUBLE=8
               'LONG=20
               'BIT=64+n
             */
            if (bytes_left < 3) {
                error(5);
                return;
            }
            i = *b++;
            bytes_left--; // read type byte
            x = *(ma_uint16 *)b;
            b += 2;
            bytes_left -= 2; // read offset within DBLOCK
            // note: allowable _BIT type variables in VARPTR$ are all at a byte offset and are all
            //      padded until the next byte
            d = 0;
            switch (i) {
            case 1:
                d = *(char *)(dblock + x);
                break;
            case (1 + 128):
                d = *(ma_uint8 *)(dblock + x);
                break;
            case 2:
                d = *(ma_int16 *)(dblock + x);
                break;
            case (2 + 128):
                d = *(ma_uint16 *)(dblock + x);
                break;
            case 4:
                d = *(float *)(dblock + x);
                break;
            case 5:
                d = *(ma_int64 *)(dblock + x);
                break;
            case (5 + 128):
                d = *(ma_int64 *)(dblock + x); // unsigned conversion is unsupported!
                break;
            case 6:
                d = *(long double *)(dblock + x);
                break;
            case 8:
                d = *(double *)(dblock + x);
                break;
            case 20:
                d = *(ma_int32 *)(dblock + x);
                break;
            case (20 + 128):
                d = *(ma_uint32 *)(dblock + x);
                break;
            default:
                // bit type?
                if ((i & 64) == 0) {
                    error(5);
                    return;
                }
                x2 = i & 63;
                if (x2 > 56) {
                    error(5);
                    return;
                } // valid number of bits?
                // create a mask
                static ma_int64 i64num, mask, i64x;
                mask = (((ma_int64)1) << x2) - 1;
                i64num = (*(ma_int64 *)(dblock + x)) & mask;
                // signed?
                if (i & 128) {
                    mask = ((ma_int64)1) << (x2 - 1);
                    if (i64num & mask) { // top bit on?
                        mask = -1;
                        mask <<= x2;
                        i64num += mask;
                    }
                } // signed
                d = i64num;
            }
            if (d > 2147483647.0 || d < -2147483648.0) {
                error(5);
                return;
            } // out of range value!
            number = round(d);
            goto next_byte;
        }

        // read in a number
        if ((i >= 48) && (i <= 57)) {
            if (fullstops || (number_entered == 2)) {
                error(5);
                return;
            }
            if (!number_entered) {
                number = 0;
                number_entered = 1;
            }
            number = number * 10 + i - 48;
            goto next_byte;
        }

        // read fullstops
        if (i == 46) {
            if (followup != 7 && followup != 1 && followup != 4) {
                error(5);
                return;
            }
            fullstops++;
            goto next_byte;
        }

    follow_up:

        if (followup == 8) { // V...
            if (!number_entered) {
                error(5);
                return;
            }
            number_entered = 0;
            if (number > 100) {
                error(5);
                return;
            }
            v = number;
            followup = 0;
            if (bytes_left < 0)
                goto done;
        } // 8

        if (followup == 7) { // P...
            if (number_entered) {
                number_entered = 0;
                if (number < 1 || number > 64) {
                    error(5);
                    return;
                }
                length2 = 1.0 / (t / 60.0) * (4.0 / ((double)number));
            } else {
                length2 = length;
            }
            d = length2;
            for (x = 1; x <= fullstops; x++) {
                d /= 2.0;
                length2 = length2 + d;
            }
            fullstops = 0;

            soundwave_bytes = WaveformBufferSize(length2);
            if (!wave) {
                // create buffer
                wave = (ma_uint8 *)calloc(soundwave_bytes, 1);
                wave_bytes = soundwave_bytes;
                wave_base = 0;
            } else {
                // increase buffer?
                if ((wave_base + soundwave_bytes) > wave_bytes) {
                    wave = (ma_uint8 *)realloc(wave, wave_base + soundwave_bytes);
                    memset(wave + wave_base, 0, wave_base + soundwave_bytes - wave_bytes);
                    wave_bytes = wave_base + soundwave_bytes;
                }
            }
            if (i != 44) {
                wave_base += soundwave_bytes;
            }

            playit = 1;
            followup = 0;
            if (i == 44)
                goto next_byte;
            if (bytes_left < 0)
                goto done;
        } // 7

        if (followup == 6) { // T...
            if (!number_entered) {
                error(5);
                return;
            }
            number_entered = 0;
            if (number < 32 || number > 255) {
                number = 120;
            }
            t = number;
            length = 1.0 / (t / 60.0) * (4.0 / l);
            followup = 0;
            if (bytes_left < 0)
                goto done;
        } // 6

        if (followup == 5) { // M...
            if (number_entered) {
                error(5);
                return;
            }
            switch (a) {
            case 76: // L
                pause = 0;
                break;
            case 78: // N
                pause = 1.0 / 8.0;
                break;
            case 83: // S
                pause = 1.0 / 4.0;
                break;

            case 66: // MB
                if (!audioEngine.musicBackground) {
                    audioEngine.musicBackground = true;
                    if (playit) {
                        playit = 0;
                        SendWaveformToQueue(wave, wave_bytes, true);
                    }
                    wave = NULL;
                }
                break;
            case 70: // MF
                if (audioEngine.musicBackground) {
                    audioEngine.musicBackground = false;
                    // preceding MB content incorporated into MF block
                }
                break;
            default:
                error(5);
                return;
            }
            followup = 0;
            goto next_byte;
        } // 5

        if (followup == 4) { // N...
            if (!number_entered) {
                error(5);
                return;
            }
            number_entered = 0;
            if (number > 84) {
                error(5);
                return;
            }
            n = -33 + number;
            goto followup1;
            followup = 0;
            if (bytes_left < 0)
                goto done;
        } // 4

        if (followup == 3) { // O...
            if (!number_entered) {
                error(5);
                return;
            }
            number_entered = 0;
            if (number > 6) {
                error(5);
                return;
            }
            o = number;
            followup = 0;
            if (bytes_left < 0)
                goto done;
        } // 3

        if (followup == 2) { // L...
            if (!number_entered) {
                error(5);
                return;
            }
            number_entered = 0;
            if (number < 1 || number > 64) {
                error(5);
                return;
            }
            l = number;
            length = 1.0 / (t / 60.0) * (4.0 / l);
            followup = 0;
            if (bytes_left < 0)
                goto done;
        } // 2

        if (followup == 1) { // A-G...
            if (i == 45) {   //-
                if (n_changed || number_entered) {
                    error(5);
                    return;
                }
                n_changed = 1;
                n--;
                goto next_byte;
            }
            if (i == 43 || i == 35) { //+,#
                if (n_changed || number_entered) {
                    error(5);
                    return;
                }
                n_changed = 1;
                n++;
                goto next_byte;
            }
        followup1:
            if (number_entered) {
                number_entered = 0;
                if (number < 0 || number > 64) {
                    error(5);
                    return;
                }
                if (!number)
                    length2 = length;
                else
                    length2 = 1.0 / (t / 60.0) * (4.0 / ((double)number));
            } else {
                length2 = length;
            } // number_entered
            d = length2;
            for (x = 1; x <= fullstops; x++) {
                d /= 2.0;
                length2 = length2 + d;
            }
            fullstops = 0;
            // frequency=(2^(note/12))*440
            frequency = pow(2.0, ((double)n) / 12.0) * 440.0;

            // create wave
            wave2 = GenerateWaveform(frequency, length2 * (1.0 - pause), v / 100.0, &soundwave_bytes);
            if (pause > 0) {
                wave2 = (ma_uint8 *)realloc(wave2, soundwave_bytes + WaveformBufferSize(length2 * pause));
                memset(wave2 + soundwave_bytes, 0, WaveformBufferSize(length2 * pause));
                soundwave_bytes += WaveformBufferSize(length2 * pause);
            }

            if (!wave) {
                // adopt buffer
                wave = wave2;
                wave_bytes = soundwave_bytes;
                wave_base = 0;
            } else {
                // mix required?
                if (wave_base == wave_bytes)
                    x = 0;
                else
                    x = 1;
                // increase buffer?
                if ((wave_base + soundwave_bytes) > wave_bytes) {
                    wave = (ma_uint8 *)realloc(wave, wave_base + soundwave_bytes);
                    memset(wave + wave_base, 0, wave_base + soundwave_bytes - wave_bytes);
                    wave_bytes = wave_base + soundwave_bytes;
                }
                // mix or copy
                if (x) {
                    // mix
                    static ma_int16 *sp, *sp2;
                    sp = (ma_int16 *)(wave + wave_base);
                    sp2 = (ma_int16 *)wave2;
                    x2 = soundwave_bytes / 2;
                    for (x = 0; x < x2; x++) {
                        x3 = *sp2++;
                        x4 = *sp;
                        x4 += x3;
                        if (x4 > 32767)
                            x4 = 32767;
                        if (x4 < -32767)
                            x4 = -32767;
                        *sp++ = x4;
                    } // x
                } else {
                    // copy
                    memcpy(wave + wave_base, wave2, soundwave_bytes);
                } // x
                free(wave2);
            }
            if (i != 44) {
                wave_base += soundwave_bytes;
            }

            playit = 1;
            n_changed = 0;
            followup = 0;
            if (i == 44)
                goto next_byte;
            if (bytes_left < 0)
                goto done;
        } // 1

        if (a >= 65 && a <= 71) {
            // modify a to represent a semitonal note (n) interval
            switch (a) {
                //[c][ ][d][ ][e][f][ ][g][ ][a][ ][b]
                // 0  1  2  3  4  5  6  7  8  9  0  1
            case 65:
                n = 9;
                break;
            case 66:
                n = 11;
                break;
            case 67:
                n = 0;
                break;
            case 68:
                n = 2;
                break;
            case 69:
                n = 4;
                break;
            case 70:
                n = 5;
                break;
            case 71:
                n = 7;
                break;
            }
            n = n + (o - 2) * 12 - 9;
            followup = 1;
            goto next_byte;
        } // a

        if (a == 76) { // L
            followup = 2;
            goto next_byte;
        }

        if (a == 77) { // M
            followup = 5;
            goto next_byte;
        }

        if (a == 78) { // N
            followup = 4;
            goto next_byte;
        }

        if (a == 79) { // O
            followup = 3;
            goto next_byte;
        }

        if (a == 84) { // T
            followup = 6;
            goto next_byte;
        }

        if (a == 60) { //<
            o--;
            if (o < 0)
                o = 0;
            goto next_byte;
        }

        if (a == 62) { //>
            o++;
            if (o > 6)
                o = 6;
            goto next_byte;
        }

        if (a == 80) { // P
            followup = 7;
            goto next_byte;
        }

        if (a == 86) { // V
            followup = 8;
            goto next_byte;
        }

        error(5);
        return;
    } // bytes_left
done:
    if (number_entered || followup) {
        error(5);
        return;
    } // unhandled data

    if (playit) {
        SendWaveformToQueue(wave, wave_bytes, !audioEngine.musicBackground);
    } // playit
}

/// <summary>
/// This returns the sample rate from ma engine if ma is initialized.
/// </summary>
/// <returns>miniaudio sample rtate</returns>
int32_t func__sndrate() { return audioEngine.sampleRate; }

/// <summary>
/// This loads a sound file into memory and returns a LONG handle value above 0.
/// </summary>
/// <param name="fileName">The is the pathname for the sound file. This can be any format that miniaudio or a miniaudio plugin supports</param>
/// <param name="requirements">This is leftover from the old QB64-SDL days. But we use this to pass some parameters like 'stream'</param>
/// <param name="passed">How many parameters were passed?</param>
/// <returns>Returns a valid sound handle (> 0) if successful or 0 if it fails</returns>
int32_t func__sndopen(qbs *fileName, qbs *requirements, int32_t passed) {
    // Some QB strings that we'll need
    static qbs *fileNameZ = nullptr;
    static qbs *reqs = nullptr;

    if (!audioEngine.isInitialized)
        return INVALID_SOUND_HANDLE;

    if (!fileNameZ)
        fileNameZ = qbs_new(0, 0);

    if (!reqs)
        reqs = qbs_new(0, 0);

    qbs_set(fileNameZ, qbs_add(fileName, qbs_new_txt_len("\0", 1))); // s1 = filename + CHR$(0)
    if (fileNameZ->len == 1)
        return INVALID_SOUND_HANDLE; // Return INVALID_SOUND_HANDLE if file name is null length string

    // Alocate a sound handle
    int32_t handle = audioEngine.AllocateSoundHandle();
    if (handle < 1) // We are not expected to open files with handle 0
        return INVALID_SOUND_HANDLE;

    // Set some handle properties
    audioEngine.soundHandles[handle]->type = SoundType::Static;

    // Set the flags to specifiy how we want the audio file to be opened
    if (passed && requirements->len) {
        qbs_set(reqs, qbs_ucase(requirements)); // Convert tmp str to perm str
        if (func_instr(1, reqs, qbs_new_txt(REQUIREMENT_STRING_STREAM), 1))
            audioEngine.soundHandles[handle]->maFlags |= MA_SOUND_FLAG_STREAM; // Check if the user wants to stream the file
    } else {
        audioEngine.soundHandles[handle]->maFlags |= MA_SOUND_FLAG_DECODE; // Else decode and load the whole sound in memory
    }

    // Forward the request to miniaudio to open the sound file
    audioEngine.maResult = ma_sound_init_from_file(&audioEngine.maEngine, (const char *)fileNameZ->chr, audioEngine.soundHandles[handle]->maFlags, NULL, NULL,
                                                   &audioEngine.soundHandles[handle]->maSound);

    // If the sound failed to copy, then free the handle and return INVALID_SOUND_HANDLE
    if (audioEngine.maResult != MA_SUCCESS) {
        AUDIO_DEBUG_PRINT("'%s' failed to open", fileNameZ->chr);
        audioEngine.soundHandles[handle]->isUsed = false;
        return INVALID_SOUND_HANDLE;
    }

    AUDIO_DEBUG_PRINT("'%s' successfully opened", fileNameZ->chr);
    return handle;
}

/// <summary>
/// The frees and unloads an open sound.
/// If the sound is playing, it'll let it finish. Looping sounds will loop until the program is closed.
/// If the sound is a stream of raw samples then any remaining samples pending for playback will be sent to miniaudio and then the handle will be freed.
/// </summary>
/// <param name="handle">A sound handle</param>
void sub__sndclose(int32_t handle) {
    if (audioEngine.isInitialized && IS_SOUND_HANDLE_VALID(handle)) {
        // If we have a raw stream then force it to push all it's data to miniaudio
        // Note that this will take care of checking if the handle is a raw steam and other stuff
        // So it is completly safe to call it this way
        sub__sndrawdone(handle, true);

        // Simply set the autokill flag to true and let the sound loop handle disposing the sound
        audioEngine.soundHandles[handle]->autoKill = true;
    }
}

/// <summary>
/// This copies a sound to a new handle so that two or more of the same sound can be played at once.
/// </summary>
/// <param name="src_handle">A source sound handle</param>
/// <returns>A new sound handle if successful or 0 on failure</returns>
int32_t func__sndcopy(int32_t src_handle) {
    // Check for all invalid cases
    if (!audioEngine.isInitialized || !IS_SOUND_HANDLE_VALID(src_handle) || audioEngine.soundHandles[src_handle]->type != SoundType::Static)
        return INVALID_SOUND_HANDLE;

    // Alocate a sound handle
    int32_t dst_handle = audioEngine.AllocateSoundHandle();
    // Initialize the sound handle data
    if (dst_handle < 1) // We are not expected to open files with handle 0
        return INVALID_SOUND_HANDLE;

    audioEngine.soundHandles[dst_handle]->type = SoundType::Static;                                // Set some handle properties
    audioEngine.soundHandles[dst_handle]->maFlags = audioEngine.soundHandles[src_handle]->maFlags; // Copy the flags

    // Initialize a new copy of the sound
    audioEngine.maResult = ma_sound_init_copy(&audioEngine.maEngine, &audioEngine.soundHandles[src_handle]->maSound,
                                              audioEngine.soundHandles[dst_handle]->maFlags, NULL, &audioEngine.soundHandles[dst_handle]->maSound);

    // If the sound failed to copy, then free the handle and return INVALID_SOUND_HANDLE
    if (audioEngine.maResult != MA_SUCCESS) {
        audioEngine.soundHandles[dst_handle]->isUsed = false;
        return INVALID_SOUND_HANDLE;
    }

    return dst_handle;
}

/// <summary>
/// This plays a sound designated by a sound handle.
/// </summary>
/// <param name="handle">A sound handle</param>
void sub__sndplay(int32_t handle) {
    if (audioEngine.isInitialized && IS_SOUND_HANDLE_VALID(handle) && audioEngine.soundHandles[handle]->type == SoundType::Static) {
        // Reset position to zero only if we are playing and (not looping or we've reached the end of the sound)
        // This is based on the old OpenAl-soft code behavior
        if (ma_sound_is_playing(&audioEngine.soundHandles[handle]->maSound) &&
            (!ma_sound_is_looping(&audioEngine.soundHandles[handle]->maSound) || ma_sound_at_end(&audioEngine.soundHandles[handle]->maSound))) {
            audioEngine.maResult = ma_sound_seek_to_pcm_frame(&audioEngine.soundHandles[handle]->maSound, 0);
            AUDIO_DEBUG_CHECK(audioEngine.maResult == MA_SUCCESS);
        }

        // Kickstart playback
        audioEngine.maResult = ma_sound_start(&audioEngine.soundHandles[handle]->maSound);
        AUDIO_DEBUG_CHECK(audioEngine.maResult == MA_SUCCESS);

        // Stop looping the sound if it is
        if (ma_sound_is_looping(&audioEngine.soundHandles[handle]->maSound)) {
            ma_sound_set_looping(&audioEngine.soundHandles[handle]->maSound, MA_FALSE);
        }
    }
}

/// <summary>
/// This copies a sound, plays it, and automatically closes the copy.
/// </summary>
/// <param name="handle">A sound handle to copy</param>
/// <param name="volume">The volume at which the sound should be played (0.0 - 1.0)</param>
/// <param name="x">x distance values go from left (negative) to right (positive)</param>
/// <param name="y">y distance values go from below (negative) to above (positive).</param>
/// <param name="z">z distance values go from behind (negative) to in front (positive).</param>
/// <param name="passed">How many parameters were passed?</param>
void sub__sndplaycopy(int32_t src_handle, double volume, double x, double y, double z, int32_t passed) {
    // We are simply going to use sndcopy, then setup some stuff like volume and autokill and then use sndplay
    // We are not checking if the audio engine was initialized because if not we'll get an invalid handle anyway
    int32_t dst_handle = func__sndcopy(src_handle);

    // Check if we succeeded and then proceed
    if (dst_handle > 0) {
        // Set the volume if requested
        if (passed & 1)
            ma_sound_set_volume(&audioEngine.soundHandles[dst_handle]->maSound, volume);

        if (passed & 4 || passed & 8) {                                                                    // If y or z or both are passed
            ma_sound_set_spatialization_enabled(&audioEngine.soundHandles[dst_handle]->maSound, MA_TRUE);  // Enable 3D spatialization
            ma_sound_set_position(&audioEngine.soundHandles[dst_handle]->maSound, x, y, z);                // Use full 3D positioning
        } else if (passed & 2) {                                                                           // If x is passed
            ma_sound_set_spatialization_enabled(&audioEngine.soundHandles[dst_handle]->maSound, MA_FALSE); // Disable spatialization for better stereo sound
            ma_sound_set_pan_mode(&audioEngine.soundHandles[dst_handle]->maSound, ma_pan_mode_pan);        // Set true panning
            ma_sound_set_pan(&audioEngine.soundHandles[dst_handle]->maSound, x);                           // Just use stereo panning
        }

        sub__sndplay(dst_handle);                              // Play the sound
        audioEngine.soundHandles[dst_handle]->autoKill = true; // Set to auto kill
    }

    AUDIO_DEBUG_PRINT("Playing sound copy %i: volume %lf, 3D (%lf, %lf, %lf)", dst_handle, volume, x, y, z);
}

/// <summary>
/// This is a "fire and forget" style of function.
/// The engine will manage the sound handle internally.
/// When the sound finishes playing, the handle will be put up for recycling.
/// Playback starts asynchronously.
/// </summary>
/// <param name="fileName">The is the name of the file to be played</param>
/// <param name="sync">This paramater is ignored</param>
/// <param name="volume">This the sound playback volume (0 - silent ... 1 - full)</param>
/// <param name="passed">How many parameters were passed?</param>
void sub__sndplayfile(qbs *fileName, int32_t sync, double volume, int32_t passed) {
    // We need this to send requirements to SndOpen
    static qbs *reqs = nullptr;

    if (!reqs) {
        // Since this never changes, we can get away by doing this just once
        reqs = qbs_new(0, 0);
        qbs_set(reqs, qbs_new_txt(REQUIREMENT_STRING_STREAM));
    }

    // We will not wrap this in a 'if initialized' block because SndOpen will take care of that
    int32_t handle = func__sndopen(fileName, reqs, 1);

    if (handle > 0) {
        if (passed & 2)
            ma_sound_set_volume(&audioEngine.soundHandles[handle]->maSound, volume);

        sub__sndplay(handle);                              // Play the sound
        audioEngine.soundHandles[handle]->autoKill = true; // Set to auto kill
    }
}

/// <summary>
/// This pauses a sound using a sound handle.
/// </summary>
/// <param name="handle">A sound handle</param>
void sub__sndpause(int32_t handle) {
    if (audioEngine.isInitialized && IS_SOUND_HANDLE_VALID(handle) && audioEngine.soundHandles[handle]->type == SoundType::Static) {
        // Stop the sound and just leave it at that
        // miniaudio does not reset the play cursor
        audioEngine.maResult = ma_sound_stop(&audioEngine.soundHandles[handle]->maSound);
        AUDIO_DEBUG_CHECK(audioEngine.maResult == MA_SUCCESS);
    }
}

/// <summary>
/// This returns whether a sound is being played.
/// </summary>
/// <param name="handle">A sound handle</param>
/// <returns>Return true if the sound is playing. False otherwise</returns>
int32_t func__sndplaying(int32_t handle) {
    if (audioEngine.isInitialized && IS_SOUND_HANDLE_VALID(handle) && audioEngine.soundHandles[handle]->type == SoundType::Static) {
        return ma_sound_is_playing(&audioEngine.soundHandles[handle]->maSound) ? QB_TRUE : QB_FALSE;
    }

    return QB_FALSE;
}

/// <summary>
/// This checks if a sound is paused.
/// </summary>
/// <param name="handle">A sound handle</param>
/// <returns>Returns true if the sound is paused. False otherwise</returns>
int32_t func__sndpaused(int32_t handle) {
    if (audioEngine.isInitialized && IS_SOUND_HANDLE_VALID(handle) && audioEngine.soundHandles[handle]->type == SoundType::Static) {
        return !ma_sound_is_playing(&audioEngine.soundHandles[handle]->maSound) &&
                       (ma_sound_is_looping(&audioEngine.soundHandles[handle]->maSound) || !ma_sound_at_end(&audioEngine.soundHandles[handle]->maSound))
                   ? QB_TRUE
                   : QB_FALSE;
    }

    return QB_FALSE;
}

/// <summary>
/// This sets the volume of a sound loaded in memory using a sound handle.
/// New: This works for both static and raw sounds.
/// </summary>
/// <param name="handle">A sound handle</param>
/// <param name="volume">A float point value with 0 resulting in silence and anything above 1 resulting in amplification</param>
void sub__sndvol(int32_t handle, float volume) {
    if (audioEngine.isInitialized && IS_SOUND_HANDLE_VALID(handle) &&
        (audioEngine.soundHandles[handle]->type == SoundType::Static || audioEngine.soundHandles[handle]->type == SoundType::Raw)) {
        ma_sound_set_volume(&audioEngine.soundHandles[handle]->maSound, volume);
    }
}

/// <summary>
/// This is like sub__sndplay but the sound is looped.
/// </summary>
/// <param name="handle"></param>
void sub__sndloop(int32_t handle) {
    if (audioEngine.isInitialized && IS_SOUND_HANDLE_VALID(handle) && audioEngine.soundHandles[handle]->type == SoundType::Static) {
        // Reset position to zero only if we are playing and (not looping or we've reached the end of the sound)
        // This is based on the old OpenAl-soft code behavior
        if (ma_sound_is_playing(&audioEngine.soundHandles[handle]->maSound) &&
            (!ma_sound_is_looping(&audioEngine.soundHandles[handle]->maSound) || ma_sound_at_end(&audioEngine.soundHandles[handle]->maSound))) {
            audioEngine.maResult = ma_sound_seek_to_pcm_frame(&audioEngine.soundHandles[handle]->maSound, 0);
            AUDIO_DEBUG_CHECK(audioEngine.maResult == MA_SUCCESS);
        }

        // Kickstart playback
        audioEngine.maResult = ma_sound_start(&audioEngine.soundHandles[handle]->maSound);
        AUDIO_DEBUG_CHECK(audioEngine.maResult == MA_SUCCESS);

        // Start looping the sound if it is not
        if (!ma_sound_is_looping(&audioEngine.soundHandles[handle]->maSound)) {
            ma_sound_set_looping(&audioEngine.soundHandles[handle]->maSound, MA_TRUE);
        }
    }
}

/// <summary>
/// This will attempt to set the balance or 3D position of a sound.
/// Note that unlike the OpenAL code, we will do pure stereo panning if y & z are absent.
/// New: This works for both static and raw sounds.
/// </summary>
/// <param name="handle">A sound handle</param>
/// <param name="x">x distance values go from left (negative) to right (positive)</param>
/// <param name="y">y distance values go from below (negative) to above (positive).</param>
/// <param name="z">z distance values go from behind (negative) to in front (positive).</param>
/// <param name="channel">channel value 1 denotes left (mono) and 2 denotes right (stereo) channel. This has no meaning for miniaudio and is ignored</param>
/// <param name="passed">How many parameters were passed?</param>
void sub__sndbal(int32_t handle, double x, double y, double z, int32_t channel, int32_t passed) {
    if (audioEngine.isInitialized && IS_SOUND_HANDLE_VALID(handle) &&
        (audioEngine.soundHandles[handle]->type == SoundType::Static || audioEngine.soundHandles[handle]->type == SoundType::Raw)) {
        if (passed & 2 || passed & 4) {                                                               // If y or z or both are passed
            ma_sound_set_spatialization_enabled(&audioEngine.soundHandles[handle]->maSound, MA_TRUE); // Enable 3D spatialization

            ma_vec3f v = ma_sound_get_position(&audioEngine.soundHandles[handle]->maSound); // Get the current position in 3D space

            // Set the previous values of x, y, z if these were not passed
            if (!(passed & 1))
                x = v.x;
            if (!(passed & 2))
                y = v.y;
            if (!(passed & 4))
                z = v.z;

            ma_sound_set_position(&audioEngine.soundHandles[handle]->maSound, x, y, z);                // Use full 3D positioning
        } else if (passed & 1) {                                                                       // Only bother if x is passed
            ma_sound_set_spatialization_enabled(&audioEngine.soundHandles[handle]->maSound, MA_FALSE); // Disable spatialization for better stereo sound
            ma_sound_set_pan_mode(&audioEngine.soundHandles[handle]->maSound, ma_pan_mode_pan);        // Set true panning
            ma_sound_set_pan(&audioEngine.soundHandles[handle]->maSound, x);                           // Just use stereo panning
        }
    }
}

/// <summary>
/// This returns the length in seconds of a loaded sound using a sound handle.
/// </summary>
/// <param name="handle">A sound handle</param>
/// <returns>Returns the length of a sound in seconds</returns>
double func__sndlen(int32_t handle) {
    if (audioEngine.isInitialized && IS_SOUND_HANDLE_VALID(handle) && audioEngine.soundHandles[handle]->type == SoundType::Static) {
        float lengthSeconds = 0;
        audioEngine.maResult = ma_sound_get_length_in_seconds(&audioEngine.soundHandles[handle]->maSound, &lengthSeconds);
        AUDIO_DEBUG_CHECK(audioEngine.maResult == MA_SUCCESS);
        return lengthSeconds;
    }

    return 0;
}

/// <summary>
/// This returns the current playing position in seconds using a sound handle.
/// </summary>
/// <param name="handle">A sound handle</param>
/// <returns>Returns the current playing position in seconds from an open sound file</returns>
double func__sndgetpos(int32_t handle) {
    if (audioEngine.isInitialized && IS_SOUND_HANDLE_VALID(handle) && audioEngine.soundHandles[handle]->type == SoundType::Static) {
        float playCursorSeconds = 0;
        audioEngine.maResult = ma_sound_get_cursor_in_seconds(&audioEngine.soundHandles[handle]->maSound, &playCursorSeconds);
        AUDIO_DEBUG_CHECK(audioEngine.maResult == MA_SUCCESS);
        return playCursorSeconds;
    }

    return 0;
}

/// <summary>
/// This changes the current/starting playing position in seconds of a sound.
/// </summary>
/// <param name="handle">A sound handle</param>
/// <param name="seconds">The position to set in seconds</param>
void sub__sndsetpos(int32_t handle, double seconds) {
    if (audioEngine.isInitialized && IS_SOUND_HANDLE_VALID(handle) && audioEngine.soundHandles[handle]->type == SoundType::Static) {
        float lengthSeconds;
        audioEngine.maResult = ma_sound_get_length_in_seconds(&audioEngine.soundHandles[handle]->maSound, &lengthSeconds); // Get the length in seconds
        if (audioEngine.maResult != MA_SUCCESS)
            return;

        if (seconds > lengthSeconds) // If position is beyond length then simply stop playback and exit
        {
            audioEngine.maResult = ma_sound_stop(&audioEngine.soundHandles[handle]->maSound);
            AUDIO_DEBUG_CHECK(audioEngine.maResult == MA_SUCCESS);
            return;
        }

        ma_uint64 lengthSampleFrames;
        audioEngine.maResult =
            ma_sound_get_length_in_pcm_frames(&audioEngine.soundHandles[handle]->maSound, &lengthSampleFrames); // Get the total sample frames
        if (audioEngine.maResult != MA_SUCCESS)
            return;

        audioEngine.maResult = ma_sound_seek_to_pcm_frame(&audioEngine.soundHandles[handle]->maSound,
                                                          lengthSampleFrames * (seconds / lengthSeconds)); // Set the postion in PCM frames
        AUDIO_DEBUG_CHECK(audioEngine.maResult == MA_SUCCESS);
    }
}

/// <summary>
/// This stops playing a sound after it has been playing for a set number of seconds.
/// </summary>
/// <param name="handle">A sound handle</param>
/// <param name="limit">The number of seconds that the sound will play</param>
void sub__sndlimit(int32_t handle, double limit) {
    if (audioEngine.isInitialized && IS_SOUND_HANDLE_VALID(handle) && audioEngine.soundHandles[handle]->type == SoundType::Static) {
        ma_sound_set_stop_time_in_milliseconds(&audioEngine.soundHandles[handle]->maSound, limit * 1000);
    }
}

/// <summary>
/// This stops a playing or paused sound using a sound handle.
/// </summary>
/// <param name="handle">A sound handle</param>
void sub__sndstop(int32_t handle) {
    if (audioEngine.isInitialized && IS_SOUND_HANDLE_VALID(handle) && audioEngine.soundHandles[handle]->type == SoundType::Static) {
        // Stop the sound first
        audioEngine.maResult = ma_sound_stop(&audioEngine.soundHandles[handle]->maSound);
        AUDIO_DEBUG_CHECK(audioEngine.maResult == MA_SUCCESS);

        // Also reset the playback cursor to zero
        audioEngine.maResult = ma_sound_seek_to_pcm_frame(&audioEngine.soundHandles[handle]->maSound, 0);
        AUDIO_DEBUG_CHECK(audioEngine.maResult == MA_SUCCESS);
    }
}

/// <summary>
/// This function opens a new channel to fill with _SNDRAW content to manage multiple dynamically generated sounds.
/// </summary>
/// <returns>A new sound handle if successful or 0 on failure</returns>
int32_t func__sndopenraw() {
    // Return invalid handle if audio engine is not initialized
    if (!audioEngine.isInitialized)
        return INVALID_SOUND_HANDLE;

    // Alocate a sound handle
    int32_t handle = audioEngine.AllocateSoundHandle();
    if (handle < 1)
        return INVALID_SOUND_HANDLE;

    // Set some handle properties
    audioEngine.soundHandles[handle]->type = SoundType::Raw;

    // Create the raw sound object
    audioEngine.soundHandles[handle]->rawStream = RawStreamCreate(&audioEngine.maEngine, &audioEngine.soundHandles[handle]->maSound);
    if (!audioEngine.soundHandles[handle]->rawStream)
        return INVALID_SOUND_HANDLE;

    return handle;
}

/// <summary>
/// This plays sound wave sample frequencies created by a program.
/// </summary>
/// <param name="left">Left channel sample</param>
/// <param name="right">Right channel sample</param>
/// <param name="handle">A sound handle</param>
/// <param name="passed">How many parameters were passed?</param>
void sub__sndraw(float left, float right, int32_t handle, int32_t passed) {
    // Use the default raw handle if handle was not passed
    if (!(passed & 2)) {
        // Check if the default handle was created
        if (audioEngine.sndInternalRaw < 1) {
            audioEngine.sndInternalRaw = func__sndopenraw();
        }

        handle = audioEngine.sndInternalRaw;
    }

    if (audioEngine.isInitialized && IS_SOUND_HANDLE_VALID(handle) && audioEngine.soundHandles[handle]->type == SoundType::Raw) {
        if (!(passed & 1))
            right = left;

        RawStreamPushSampleFrame(audioEngine.soundHandles[handle]->rawStream, left, right);
    }
}

/// <summary>
/// This ensures that the final buffer portion is played in short sound effects even if it is incomplete.
/// </summary>
/// <param name="handle">A sound handle</param>
/// <param name="passed">How many parameters were passed?</param>
void sub__sndrawdone(int32_t handle, int32_t passed) {
    // This is NOP now because miniaudio data source automatically pulls in all the samples without us doing anything
    // As such, we need to think about the future of this function. Probably just leave it this way?
    (void)handle;
    (void)passed;
    /*
    // Use the default raw handle if handle was not passed
    if (!passed)
        handle = audioEngine.sndInternalRaw;

    if (audioEngine.isInitialized && IS_SOUND_HANDLE_VALID(handle) && audioEngine.soundHandles[handle]->type == SoundType::Raw) {
        // NOP
    }
    */
}

/// <summary>
/// This function returns the length, in seconds, of a _SNDRAW sound currently queued.
/// </summary>
/// <param name="handle">A sound handle</param>
/// <param name="passed">How many parameters were passed?</param>
/// <returns></returns>
double func__sndrawlen(int32_t handle, int32_t passed) {
    // Use the default raw handle if handle was not passed
    if (!passed)
        handle = audioEngine.sndInternalRaw;

    if (audioEngine.isInitialized && IS_SOUND_HANDLE_VALID(handle) && audioEngine.soundHandles[handle]->type == SoundType::Raw) {
        return RawStreamGetTimeRemaining(audioEngine.soundHandles[handle]->rawStream);
    }

    return 0;
}

/// <summary>
/// This function returns a _MEM value referring to a sound's raw data in memory using a designated sound handle created by the _SNDOPEN function.
/// miniaudio supports a variety of sample and channel formats. Translating all of that to basic 2 channel 16-bit format that
/// MemSound was originally supporting would require significant overhead both in terms of system resources and code.
/// For now we are just exposing the underlying PCM data directly from miniaudio. This fits rather well using the existing mem structure.
/// Mono sounds should continue to work just as it was before. Stereo and multi-channel sounds however will be required to be handled correctly
/// by the user by checking the 'elementsize' (for frame size in bytes) and 'type' (for data type) members.
/// </summary>
/// <param name="handle">A sound handle</param>
/// <param name="targetChannel">This should be 0 (for interleaved) or 1 (for mono). Anything else will result in failure</param>
/// <returns>A _MEM value that can be used to access the sound data</returns>
mem_block func__memsound(int32_t handle, int32_t targetChannel) {
    static mem_block mb;
    static ma_format maFormat;
    static ma_uint32 channels;
    static ma_uint64 sampleFrames;
    static ma_resource_manager_data_buffer *ds;

    // The sound cannot be steaming and must be completely decoded in memory
    if (!audioEngine.isInitialized || !IS_SOUND_HANDLE_VALID(handle) || audioEngine.soundHandles[handle]->type != SoundType::Static ||
        audioEngine.soundHandles[handle]->maFlags & MA_SOUND_FLAG_STREAM || !(audioEngine.soundHandles[handle]->maFlags & MA_SOUND_FLAG_DECODE))
        goto error;

    // Get the pointer to the data source
    ds = (ma_resource_manager_data_buffer *)ma_sound_get_data_source(&audioEngine.soundHandles[handle]->maSound);
    if (!ds || !ds->pNode) {
        AUDIO_DEBUG_PRINT("Data source pointer OR data source node pointer is NULL");
        goto error;
    }

    // Check if the data is one contigious buffer or a link list of decoded pages
    // We cannot have a mem object for a link list of decoded pages for obvious reasons
    if (ds->pNode->data.type != ma_resource_manager_data_supply_type::ma_resource_manager_data_supply_type_decoded) {
        AUDIO_DEBUG_PRINT("Data is not a contigious buffer. Type = %u", ds->pNode->data.type);
        goto error;
    }

    // Check the data pointer
    if (!ds->pNode->data.backend.decoded.pData) {
        AUDIO_DEBUG_PRINT("Data source data pointer is NULL");
        goto error;
    }

    AUDIO_DEBUG_PRINT("Data source data pointer = %p", ds->pNode->data.backend.decoded.pData);

    // Query the data format
    if (ma_sound_get_data_format(&audioEngine.soundHandles[handle]->maSound, &maFormat, &channels, NULL, NULL, 0) != MA_SUCCESS) {
        AUDIO_DEBUG_PRINT("Data format query failed");
        goto error;
    }

    // Do not proceed if invalid (unsupported) channel values were passed
    if (targetChannel != 0 && targetChannel != 1) {
        AUDIO_DEBUG_PRINT("Sound channels = %u, Target channel %i not supported", channels, targetChannel);
        goto error;
    }

    // Get the length in sample frames
    if (ma_sound_get_length_in_pcm_frames(&audioEngine.soundHandles[handle]->maSound, &sampleFrames) != MA_SUCCESS) {
        AUDIO_DEBUG_PRINT("PCM frames query failed");
        goto error;
    }

    AUDIO_DEBUG_PRINT("Format = %u, Channels = %u, Frames = %llu", maFormat, channels, sampleFrames);

    if (audioEngine.soundHandles[handle]->memLockOffset) {
        mb.lock_offset = (ptrszint)audioEngine.soundHandles[handle]->memLockOffset;
        mb.lock_id = audioEngine.soundHandles[handle]->memLockId;
    } else {
        new_mem_lock();
        mem_lock_tmp->type = MEM_TYPE_SOUND;
        mb.lock_offset = (ptrszint)mem_lock_tmp;
        mb.lock_id = mem_lock_id;
        audioEngine.soundHandles[handle]->memLockOffset = (void *)mem_lock_tmp;
        audioEngine.soundHandles[handle]->memLockId = mem_lock_id;
    }

    // Setup type: This was not done in the old code
    // But we are doing it here. By examing the type the user can now figure out if they have to use FP32 or integers
    if (maFormat == ma_format::ma_format_f32)
        mb.type = 4 + 256; // FP32
    else if (maFormat == ma_format::ma_format_s32)
        mb.type = 4 + 128; // Int32
    else if (maFormat == ma_format::ma_format_s16)
        mb.type = 2 + 128; // Int16
    else if (maFormat == ma_format::ma_format_u8)
        mb.type = 1 + 128 + 1024; // Int8

    mb.elementsize = ma_get_bytes_per_frame(maFormat, channels); // Set the element size. This is the size of each PCM frame in bytes
    mb.offset = (ptrszint)ds->pNode->data.backend.decoded.pData; // Setup offset
    mb.size = sampleFrames * mb.elementsize;                     // Setup size (in bytes)
    mb.sound = handle;                                           // Copy the handle
    mb.image = 0;                                                // Not needed. Set to 0

    AUDIO_DEBUG_PRINT("ElementSize = %lli, Size = %lli, Type = %lli", mb.elementsize, mb.size, mb.type);

    return mb;

error:
    mb.offset = 0;
    mb.size = 0;
    mb.lock_offset = (ptrszint)mem_lock_base;
    mb.lock_id = INVALID_MEM_LOCK;
    mb.type = 0;
    mb.elementsize = 0;
    mb.sound = 0;
    mb.image = 0;

    return mb;
}

/// <summary>
/// This initializes the QBPE audio subsystem.
/// We simply attempt to initialize and then set some globals with the results.
/// </summary>
void snd_init() {
    // Exit if engine is initialize or already initialization was attempted but failed
    if (audioEngine.isInitialized || audioEngine.initializationFailed)
        return;

    // Initialize the miniaudio resource manager
    audioEngine.maResourceManagerConfig = ma_resource_manager_config_init();
    AudioEngineAttachCustomBackendVTables(&audioEngine.maResourceManagerConfig);
    audioEngine.maResourceManagerConfig.pCustomDecodingBackendUserData = NULL; // <- pUserData parameter of each function in the decoding backend vtables

    audioEngine.maResult = ma_resource_manager_init(&audioEngine.maResourceManagerConfig, &audioEngine.maResourceManager);
    if (audioEngine.maResult != MA_SUCCESS) {
        audioEngine.initializationFailed = true;
        AUDIO_DEBUG_PRINT("Failed to initialize miniaudio resource manager");
        return;
    }

    // Once we have a resource manager we can create the engine
    audioEngine.maEngineConfig = ma_engine_config_init();
    audioEngine.maEngineConfig.pResourceManager = &audioEngine.maResourceManager;

    // Attempt to initialize with miniaudio defaults
    audioEngine.maResult = ma_engine_init(&audioEngine.maEngineConfig, &audioEngine.maEngine);
    // If failed, then set the global flag so that we don't attempt to initialize again
    if (audioEngine.maResult != MA_SUCCESS) {
        ma_resource_manager_uninit(&audioEngine.maResourceManager);
        audioEngine.initializationFailed = true;
        AUDIO_DEBUG_PRINT("miniaudio initialization failed");
        return;
    }

    // Get and save the engine sample rate. We will let miniaudio choose the device sample rate for us
    // This ensures we get the lowest latency
    // Set the resource manager decorder sample rate to the device sample rate (miniaudio engine bug?)
    audioEngine.maResourceManager.config.decodedSampleRate = audioEngine.sampleRate = ma_engine_get_sample_rate(&audioEngine.maEngine);

    // Set the initialized flag as true
    audioEngine.isInitialized = true;

    AUDIO_DEBUG_PRINT("Audio engine initialized at %uHz sample rate", audioEngine.sampleRate);

    // Reserve sound handle 0 so that nothing else can use it
    // We will use this handle internally for Play(), Beep(), Sound() etc.
    audioEngine.sndInternal = audioEngine.AllocateSoundHandle();
    AUDIO_DEBUG_CHECK(audioEngine.sndInternal == 0); // The first handle must return 0 and this is what is used by Beep and Sound
    audioEngine.soundHandles[audioEngine.sndInternal]->rawStream = nullptr;
    audioEngine.soundHandles[audioEngine.sndInternal]->type = SoundType::None;
}

/// <summary>
/// This shuts down the audio engine and frees any resources used.
/// </summary>
void snd_un_init() {
    if (audioEngine.isInitialized) {
        // Special handling for handle 0
        audioEngine.soundHandles[audioEngine.sndInternal]->type = SoundType::None;
        RawStreamDestroy(audioEngine.soundHandles[audioEngine.sndInternal]->rawStream);
        audioEngine.soundHandles[audioEngine.sndInternal]->rawStream = nullptr;

        // Free all sound handles here
        for (size_t handle = 0; handle < audioEngine.soundHandles.size(); handle++) {
            audioEngine.FreeSoundHandle(handle);     // Let FreeSoundHandle do it's thing
            delete audioEngine.soundHandles[handle]; // Now free the object created by AllocateSoundHandle()
        }

        // Now that all sounds are closed and SoundHandle objects are freed, clear the vector
        audioEngine.soundHandles.clear();

        // Invalidate internal handles
        audioEngine.sndInternal = audioEngine.sndInternalRaw = INVALID_SOUND_HANDLE;

        // Shutdown miniaudio
        ma_engine_uninit(&audioEngine.maEngine);

        // Shutdown the miniaudio resource manager
        ma_resource_manager_uninit(&audioEngine.maResourceManager);

        // Set engine initialized flag as false
        audioEngine.isInitialized = false;

        AUDIO_DEBUG_PRINT("Audio engine shutdown");
    }
}

/// <summary>
/// This is called by the QBPE library code.
/// We use this for housekeeping and other stuff.
/// </summary>
void snd_mainloop() {
    if (audioEngine.isInitialized) {
        // Scan through the whole handle vector to find anything we need to update or close
        for (size_t handle = 0; handle < audioEngine.soundHandles.size(); handle++) {
            // Only process handles that are in use
            if (audioEngine.soundHandles[handle]->isUsed) {
                // Look for stuff that is set to auto-destruct
                if (audioEngine.soundHandles[handle]->autoKill) {
                    switch (audioEngine.soundHandles[handle]->type) {
                    case SoundType::Static:
                        // Dispose the sound if it has finished playing
                        // Note that this means that temporary looping sounds will never close
                        // Well thats on the programmer. Probably they want it that way
                        if (!ma_sound_is_playing(&audioEngine.soundHandles[handle]->maSound))
                            audioEngine.FreeSoundHandle(handle);

                        break;

                    case SoundType::Raw:
                        // Close the raw stream if we have no more frames in the queue or playing
                        if (!RawStreamGetSampleFramesRemaining(audioEngine.soundHandles[handle]->rawStream))
                            audioEngine.FreeSoundHandle(handle);

                        break;

                    case SoundType::None:
                        if (handle != 0)
                            AUDIO_DEBUG_PRINT("Sound type is 'None' when handle value is not 0");

                        break;

                    default:
                        AUDIO_DEBUG_PRINT("Condition not handled"); // It should not come here
                    }
                }
            }
        }
    }
}
