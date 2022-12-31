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

#include <vector>
#define STB_VORBIS_HEADER_ONLY
#include "extras/stb_vorbis.c"
#include "miniaudio.h"
// Set this to 1 if we want to print debug messages to stderr
#define AUDIO_DEBUG 0
#include "audio.h"
#include "mutex.h"
// We need 'qbs' and 'mem' stuff from here. This should eventually change when things are moved to smaller, logical and self-contained files
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
// This is the string that can be passed in the requirements parameter to stream a sound from storage
#define REQUIREMENT_STRING_STREAM "STREAM"
// This is the string that can be passed in the requirements parameter to load a sound from memory
#define REQUIREMENT_STRING_MEMORY "MEMORY"

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

// These attaches our customer backend (format decoders) VTables to various miniaudio structs
void AudioEngineAttachCustomBackendVTables(ma_resource_manager_config *maResourceManagerConfig);
void AudioEngineAttachCustomBackendVTables(ma_decoder_config *maDecoderConfig);

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

/// @brief A miniaudiio raw audio stream datasource
struct RawStream {
    ma_data_source_base maDataSource;         // miniaudio data source (this must be the first member of our struct)
    ma_data_source_config maDataSourceConfig; // config struct for the data source
    ma_engine *maEngine;                      // pointer to a ma_engine object that was passed while creating the data source
    ma_sound *maSound;                        // pointer to a ma_sound object that was passed while creating the data source
    ma_uint32 sampleRate;                     // the sample rate reported by ma_engine
    struct Buffer {                           // we'll give this a name that we'll use below
        std::vector<SampleFrame> data;        // this holds the actual sample frames
        size_t cursor;                        // the read cursor (in frames) in the stream
    } buffer[2];                              // we need two of these to do a proper ping-pong
    Buffer *consumer;                         // this is what the miniaudio thread will use to pull data from
    Buffer *producer;                         // this is what the main thread will use to push data to
    libqb_mutex *m;                           // we'll use a mutex to give exclusive access to resources used by both threads
    bool stop;                                // set this to true to stop supply of samples completely (including silent samples)

    static const size_t DEFAULT_SIZE = 1024; // this is almost twice the amout what miniaudio actually asks for in frameCount

    // Delete default, copy and move constructors and assignments
    RawStream() = delete;
    RawStream(const RawStream &) = delete;
    RawStream &operator=(const RawStream &) = delete;
    RawStream &operator=(RawStream &&) = delete;
    RawStream(RawStream &&) = delete;

    /// @brief This is use to setup the vectors, mutex and set some defaults
    RawStream(ma_engine *pmaEngine, ma_sound *pmaSound) {
        maSound = pmaSound;                               // Save the pointer to the ma_sound object (this is basically from a QBPE sound handle)
        maEngine = pmaEngine;                             // Save the pointer to the ma_engine object (this should come from the QBPE sound engine)
        sampleRate = ma_engine_get_sample_rate(maEngine); // Save the sample rate
        buffer[0].cursor = buffer[1].cursor = 0;          // reset the cursors
        buffer[0].data.reserve(DEFAULT_SIZE);             // ensure we have a contigious block to account for expansion without reallocation
        buffer[1].data.reserve(DEFAULT_SIZE);             // ensure we have a contigious block to account for expansion without reallocation
        consumer = &buffer[0];                            // set default consumer
        producer = &buffer[1];                            // set default producer
        stop = false;                                     // by default we will send silent samples to keep the playback going
        m = libqb_mutex_new();
    }

    /// @brief We use this to destroy the mutex
    ~RawStream() { libqb_mutex_free(m); }

    /// @brief This swaps the consumer and producer Buffers. This is mutex protected and called by the miniaudio thread
    void SwapBuffers() {
        libqb_mutex_guard lock(m);     // lock the mutex before accessing the vectors
        consumer->cursor = 0;          // reset the cursor
        consumer->data.clear();        // clear the consumer vector
        std::swap(consumer, producer); // quicky swap the Buffer pointers
    }

    /// @brief This pushes a sample frame at the end of the queue. This is mutex protected and called by the main thread
    /// @param l Sample frame left channel data
    /// @param r Sample frame right channel data
    void PushSampleFrame(float l, float r) {
        libqb_mutex_guard lock(m);        // lock the mutex before accessing the vectors
        producer->data.push_back({l, r}); // push the sample frame to the back of the producer queue
    }

    /// @brief Returns the length, in sample frames of sound queued
    /// @return The length left to play in sample frames
    ma_uint64 GetSampleFramesRemaining() {
        libqb_mutex_guard lock(m);                                                                      // lock the mutex before accessing the vectors
        return (consumer->data.size() - consumer->cursor) + (producer->data.size() - producer->cursor); // sum of producer and consumer sample frames
    }

    /// @brief Returns the length, in seconds of sound queued
    /// @return The length left to play in seconds
    double GetTimeRemaining() { return (double)GetSampleFramesRemaining() / (double)sampleRate; }
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

    auto pRawStream = (RawStream *)pDataSource; // cast to RawStream instance pointer
    auto result = MA_SUCCESS;                   // must be initialized to MA_SUCCESS
    auto maBuffer = (SampleFrame *)pFramesOut;  // cast to sample frame pointer

    ma_uint64 sampleFramesCount = pRawStream->consumer->data.size() - pRawStream->consumer->cursor; // total amount of samples we need to send to miniaudio
    // Swap buffers if we do not have anything left to play
    if (!sampleFramesCount) {
        pRawStream->SwapBuffers();
        sampleFramesCount = pRawStream->consumer->data.size() - pRawStream->consumer->cursor; // get the total number of samples again
    }
    sampleFramesCount = std::min(sampleFramesCount, frameCount); // we'll always send lower of what miniaudio wants or what we have

    ma_uint64 sampleFramesRead = 0; // sample frame counter
    // Now send the samples to miniaudio
    while (sampleFramesRead < sampleFramesCount) {
        maBuffer[sampleFramesRead] = pRawStream->consumer->data[pRawStream->consumer->cursor];
        ++sampleFramesRead;             // increment the frame counter
        pRawStream->consumer->cursor++; // increment the read cursor
    }

    // To keep the stream going, play silence if there are no frames to play
    if (!sampleFramesRead && !pRawStream->stop) {
        while (sampleFramesRead < frameCount) {
            maBuffer[sampleFramesRead] = {};
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
static ma_data_source_vtable rawStreamDataSourceVtable = {
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

    auto pRawStream = new RawStream(pmaEngine, pmaSound); // create the data source object
    if (!pRawStream) {
        AUDIO_DEBUG_PRINT("Failed to create data source");

        return nullptr;
    }

    ZERO_VARIABLE(pRawStream->maDataSource);

    pRawStream->maDataSourceConfig = ma_data_source_config_init();
    pRawStream->maDataSourceConfig.vtable = &rawStreamDataSourceVtable; // attach the vtable to the data source

    auto result = ma_data_source_init(&pRawStream->maDataSourceConfig, &pRawStream->maDataSource);
    if (result != MA_SUCCESS) {
        AUDIO_DEBUG_PRINT("Error %i: failed to initialize data source", result);

        delete pRawStream;

        return nullptr;
    }

    result = ma_sound_init_from_data_source(pmaEngine, &pRawStream->maDataSource, MA_SOUND_FLAG_NO_PITCH | MA_SOUND_FLAG_NO_SPATIALIZATION, NULL,
                                            pmaSound); // attach data source to the ma_sound
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

/// <summary>
/// Sound handle type
/// This describes every sound the system will ever play (including raw streams).
/// </summary>
struct SoundHandle {
    bool isUsed;                                // Is this handle in active use?
    SoundType type;                             // Type of sound (see SoundType enum class)
    bool autoKill;                              // Do we need to auto-clean this sample / stream after playback is done?
    ma_sound maSound;                           // miniaudio sound
    ma_uint32 maFlags;                          // miniaudio flags that were used when initializing the sound
    ma_decoder_config maDecoderConfig;          // miniaudio decoder configuration
    ma_decoder *maDecoder;                      // this is used for files that are loaded directly from memory
    ma_audio_buffer_config maAudioBufferConfig; // miniaudio buffer configuration
    ma_audio_buffer *maAudioBuffer;             // this is used for user created audio buffers (memory is managed by miniaudio)
    RawStream *rawStream;                       // Raw sample frame queue
    void *memLockOffset;                        // This is a pointer from new_mem_lock()
    uint64 memLockId;                           // This is mem_lock_id created by new_mem_lock()

    // Delete copy and move constructors and assignments
    SoundHandle(const SoundHandle &) = delete;
    SoundHandle &operator=(const SoundHandle &) = delete;
    SoundHandle(SoundHandle &&) = delete;
    SoundHandle &operator=(SoundHandle &&) = delete;

    /// <summary>
    ///	Just initializes some important members.
    /// 'inUse' will be set to true by AllocateSoundHandle().
    /// This is done here, as well as slightly differently in AllocateSoundHandle() for safety.
    /// </summary>
    SoundHandle() {
        isUsed = false;
        type = SoundType::None;
        autoKill = false;
        ZERO_VARIABLE(maSound);
        maFlags = MA_SOUND_FLAG_NO_PITCH | MA_SOUND_FLAG_NO_SPATIALIZATION | MA_SOUND_FLAG_WAIT_INIT;
        maDecoder = nullptr;
        maAudioBuffer = nullptr;
        rawStream = nullptr;
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
    ma_engine_config maEngineConfig;                    // miniaudio engine configuration (will be used to pass in the resource manager)
    ma_engine maEngine;                                 // This is the primary miniaudio engine 'context'. Everything happens using this!
    ma_result maResult;                                 // This is the result of the last miniaudio operation (used for trapping errors)
    ma_uint32 sampleRate;                               // Sample rate used by the miniaudio engine
    int32_t sndInternal;                                // Internal sound handle that we will use for Play(), Beep() & Sound()
    int32_t sndInternalRaw;                             // Internal sound handle that we will use for the QB64 'handle-less' raw stream
    std::vector<SoundHandle *> soundHandles;            // This is the audio handle list used by the engine and by everything else
    int32_t lowestFreeHandle;                           // This is the lowest handle then was recently freed. We'll start checking for free handles from here
    bool musicBackground;                               // Should 'Sound' and 'Play' work in the background or block the caller?

    // Delete copy and move constructors and assignments
    AudioEngine(const AudioEngine &) = delete;
    AudioEngine &operator=(const AudioEngine &) = delete;
    AudioEngine &operator=(AudioEngine &&) = delete;
    AudioEngine(AudioEngine &&) = delete;

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
        ZERO_VARIABLE(soundHandles[h]->maSound);
        // We do not use pitch shifting, so this will give a little performance boost
        // Spatialization is disabled by default but will be enabled on the fly if required
        soundHandles[h]->maFlags = MA_SOUND_FLAG_NO_PITCH | MA_SOUND_FLAG_NO_SPATIALIZATION | MA_SOUND_FLAG_WAIT_INIT;
        soundHandles[h]->maDecoder = nullptr;
        soundHandles[h]->maAudioBuffer = nullptr;
        soundHandles[h]->rawStream = nullptr;
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

            // Free any initialized miniaudio decoder
            if (soundHandles[handle]->maDecoder) {
                ma_decoder_uninit(soundHandles[handle]->maDecoder);
                delete soundHandles[handle]->maDecoder;
                soundHandles[handle]->maDecoder = nullptr;
                AUDIO_DEBUG_PRINT("Decoder uninitialized");
            }

            // Free any initialized audio buffer
            if (soundHandles[handle]->maAudioBuffer) {
                ma_audio_buffer_uninit_and_free(soundHandles[handle]->maAudioBuffer);
                soundHandles[handle]->maAudioBuffer = nullptr;
                AUDIO_DEBUG_PRINT("Audio buffer uninitialized & freed");
            }

            // Invalidate any memsound stuff
            if (soundHandles[handle]->memLockOffset) {
                free_mem_lock((mem_lock *)soundHandles[handle]->memLockOffset);
                soundHandles[handle]->memLockId = INVALID_MEM_LOCK;
                soundHandles[handle]->memLockOffset = nullptr;
                AUDIO_DEBUG_PRINT("MemSound stuff invalidated");
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

/// @brief This creates a mono FP32 waveform based on frequency, length and volume
/// @param frequency The sound frequency
/// @param length The duration of the sound in seconds
/// @param volume The volume of the sound (0.0 - 1.0)
/// @param soundwave_bytes A pointer to an integer that will receive the buffer size in bytes. This cannot be NULL
/// @return A buffer containing the floating point waveform
static float *GenerateWaveform(double frequency, double length, double volume, int *soundwave_bytes) {
    // Calculate the number of samples
    auto samples = length * audioEngine.sampleRate;
    auto samplesi = (int)samples;
    if (!samplesi)
        samplesi = 1;

    // Calculate the number of bytes in the waveform data
    *soundwave_bytes = samplesi * SAMPLE_FRAME_SIZE(float, 1);

    // Allocate memory for the waveform data
    auto data = (float *)malloc(*soundwave_bytes);
    if (!data)
        return nullptr;

    // Set all bytes to 0 (silence) if frequency is >= 20000. This is per QuickBASIC 4.5 behavior
    if (frequency >= 20000) {
        memset(data, 0, *soundwave_bytes);
        return data;
    }

    // Generate the waveform
    auto direction = 1;
    auto value = 0.0;
    // frequency * 4.0 * length is the total distance value will travel (+1,-2,+1[repeated])
    // samples is the number of steps to do this in
    auto gradient = samples ? (frequency * 4.0 * length) / samples : 0.0;
    auto waveend = 0;
    auto lastx = 1.0f; // set to 1 to avoid passing initial comparison

    for (int i = 0; i < samplesi; i++) {
        auto x = (float)(value * volume);
        data[i] = x;

        if (x > 0 && lastx <= 0)
            waveend = i;

        lastx = x;

        // Update value and direction
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
    }

    // Update reduced size if waveend is non-zero
    if (waveend)
        *soundwave_bytes = waveend * SAMPLE_FRAME_SIZE(float, 1);

    return data;
}

/// @brief Returns the of a sound buffer in bytes
/// @param length Length in seconds
/// @return Length in bytes
static int WaveformBufferSize(double length) {
    auto samples = (int)(length * audioEngine.sampleRate);
    if (!samples)
        samples = 1;

    return samples * SAMPLE_FRAME_SIZE(float, 1);
}

/// @brief This sends a FP32 mono buffer to a raw stream for playback and then frees the buffer
/// @param data FP32 mono sound buffer
/// @param bytes Length of buffer in bytes
/// @param block If this is true the function is wait until the wavefrom has finished playing
static void SendWaveformToQueue(float *data, int bytes, bool block) {
    if (!data)
        return;

    auto samples = bytes / SAMPLE_FRAME_SIZE(float, 1);

    // Move data into sndraw handle
    for (auto i = 0; i < samples; i++) {
        audioEngine.soundHandles[audioEngine.sndInternal]->rawStream->PushSampleFrame(data[i], data[i]);
    }

    free(data); // free the sound data

    // This will wait for the block to finish (if specified)
    // We'll be good citizens and give-up our time-slices while waiting
    if (block) {
        auto time_ms = (audioEngine.soundHandles[audioEngine.sndInternal]->rawStream->GetSampleFramesRemaining() * 1000) / audioEngine.sampleRate;
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
    if (new_error || !audioEngine.isInitialized || audioEngine.sndInternal != 0 || lengthInClockTicks == 0.0)
        return;

    if ((frequency < 37.0 && frequency != 0) || frequency > 32767.0 || lengthInClockTicks < 0.0 || lengthInClockTicks > 65535.0) {
        error(5);
        return;
    }

    // Kickstart the raw stream if it is not already
    if (!audioEngine.soundHandles[audioEngine.sndInternal]->rawStream) {
        audioEngine.soundHandles[audioEngine.sndInternal]->rawStream =
            RawStreamCreate(&audioEngine.maEngine, &audioEngine.soundHandles[audioEngine.sndInternal]->maSound);
        AUDIO_DEBUG_CHECK(audioEngine.soundHandles[audioEngine.sndInternal]->rawStream != nullptr);
        if (!audioEngine.soundHandles[audioEngine.sndInternal]->rawStream)
            return;
        audioEngine.soundHandles[audioEngine.sndInternal]->type = SoundType::Raw;
    }

    int soundwave_bytes;
    auto data = GenerateWaveform(frequency, lengthInClockTicks / 18.2, 1, &soundwave_bytes);
    SendWaveformToQueue(data, soundwave_bytes, !audioEngine.musicBackground);
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
        return (int32_t)audioEngine.soundHandles[audioEngine.sndInternal]->rawStream->GetSampleFramesRemaining();
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
    static int soundwave_bytes;
    static uint8_t *b, *wave, *wave2;
    static double d;
    static int i, bytes_left, a, x, x2, wave_bytes, wave_base;
    static int o = 4;
    static double t = 120; // quarter notes per minute (120/60=2 per second)
    static double l = 4;
    static double pause = 1.0 / 8.0; // ML 0.0, MN 1.0/8.0, MS 1.0/4.0
    static double length, length2;   // derived from l and t
    static double frequency;
    static double v = 50;
    static int n;         // the semitone-intervaled note to be played
    static int n_changed; //+,#,- applied?
    static int64_t number;
    static int number_entered;
    static int followup; // 1=play note
    static bool playit;
    static int fullstops = 0;

    if (new_error || !audioEngine.isInitialized || audioEngine.sndInternal != 0)
        return;

    // Kickstart the raw stream if it is not already
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
    playit = false;
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
                auto mask = (((int64_t)1) << x2) - 1;
                auto i64num = (*(int64_t *)(dblock + x)) & mask;
                // signed?
                if (i & 128) {
                    mask = ((int64_t)1) << (x2 - 1);
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

            playit = true;
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
                        playit = false;
                        SendWaveformToQueue((float *)wave, wave_bytes, true);
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
            if (number == 0)
                number = 125; // #217: this will generate a frequency > 44k and will force GenerateWaveform() to generate silence
            n = -45 + number; // #217: fixes incorrect octave
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
            wave2 = (ma_uint8 *)GenerateWaveform(frequency, length2 * (1.0 - pause), v / 100.0, &soundwave_bytes);
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
                    auto sp = (float *)(wave + wave_base);
                    auto sp2 = (float *)wave2;
                    auto samples = soundwave_bytes / SAMPLE_FRAME_SIZE(float, 1);

                    for (x = 0; x < samples; x++) {
                        sp[x] += sp2[x];
                    }
                } else {
                    // copy
                    memcpy(wave + wave_base, wave2, soundwave_bytes);
                } // x
                free(wave2);
            }
            if (i != 44) {
                wave_base += soundwave_bytes;
            }

            playit = true;
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
        SendWaveformToQueue((float *)wave, wave_bytes, !audioEngine.musicBackground);
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

    if (!audioEngine.isInitialized || !fileName->len)
        return INVALID_SOUND_HANDLE;

    if (!fileNameZ)
        fileNameZ = qbs_new(0, 0);

    if (!reqs)
        reqs = qbs_new(0, 0);

    // Alocate a sound handle
    int32_t handle = audioEngine.AllocateSoundHandle();
    if (handle < 1) // We are not expected to open files with handle 0
        return INVALID_SOUND_HANDLE;

    // Set some handle properties
    audioEngine.soundHandles[handle]->type = SoundType::Static;

    // Prepare the requirements string
    if (passed && requirements->len)
        qbs_set(reqs, qbs_ucase(requirements)); // Convert tmp str to perm str

    // Set the flags to specifiy how we want the audio file to be opened
    if (passed && requirements->len && func_instr(1, reqs, qbs_new_txt(REQUIREMENT_STRING_STREAM), 1)) {
        audioEngine.soundHandles[handle]->maFlags |= MA_SOUND_FLAG_STREAM; // Check if the user wants to stream the file
        AUDIO_DEBUG_PRINT("Sound will stream");
    } else {
        audioEngine.soundHandles[handle]->maFlags |= MA_SOUND_FLAG_DECODE; // Else decode and load the whole sound in memory
        AUDIO_DEBUG_PRINT("Sound will be fully decoded");
    }

    // Load the file from file or memory based on the requirements string
    if (passed && requirements->len && func_instr(1, reqs, qbs_new_txt(REQUIREMENT_STRING_MEMORY), 1)) {
        // Configure a miniaudio decoder to load the sound from memory
        AUDIO_DEBUG_PRINT("Loading sound from memory");

        audioEngine.soundHandles[handle]->maDecoder = new ma_decoder(); // allocate and zero memory
        if (!audioEngine.soundHandles[handle]->maDecoder) {
            AUDIO_DEBUG_PRINT("Failed to allocate memory for miniaudio decoder");
            audioEngine.soundHandles[handle]->isUsed = false;
            return INVALID_SOUND_HANDLE;
        }

        // Setup the decoder & attach the custom backed vtables
        audioEngine.soundHandles[handle]->maDecoderConfig = ma_decoder_config_init_default();
        AudioEngineAttachCustomBackendVTables(&audioEngine.soundHandles[handle]->maDecoderConfig);
        audioEngine.soundHandles[handle]->maDecoderConfig.sampleRate = audioEngine.sampleRate;

        audioEngine.maResult = ma_decoder_init_memory(fileName->chr, fileName->len, &audioEngine.soundHandles[handle]->maDecoderConfig,
                                                      audioEngine.soundHandles[handle]->maDecoder); // initialize the decoder
        if (audioEngine.maResult != MA_SUCCESS) {
            delete audioEngine.soundHandles[handle]->maDecoder;
            audioEngine.soundHandles[handle]->maDecoder = nullptr;
            AUDIO_DEBUG_PRINT("Failed to initialize miniaudio decoder");
            audioEngine.soundHandles[handle]->isUsed = false;
            return INVALID_SOUND_HANDLE;
        }

        // Finally, load the sound as a data source
        audioEngine.maResult = ma_sound_init_from_data_source(&audioEngine.maEngine, audioEngine.soundHandles[handle]->maDecoder,
                                                              audioEngine.soundHandles[handle]->maFlags, NULL, &audioEngine.soundHandles[handle]->maSound);
    } else {
        AUDIO_DEBUG_PRINT("Loading sound from file '%s'", fileNameZ->chr);
        qbs_set(fileNameZ, qbs_add(fileName, qbs_new_txt_len("\0", 1))); // s1 = filename + CHR$(0)

        // Forward the request to miniaudio to open the sound file
        audioEngine.maResult = ma_sound_init_from_file(&audioEngine.maEngine, (const char *)fileNameZ->chr, audioEngine.soundHandles[handle]->maFlags, NULL,
                                                       NULL, &audioEngine.soundHandles[handle]->maSound);
    }

    // If the sound failed to initialize, then free the handle and return INVALID_SOUND_HANDLE
    if (audioEngine.maResult != MA_SUCCESS) {
        if (audioEngine.soundHandles[handle]->maDecoder) {
            delete audioEngine.soundHandles[handle]->maDecoder;
            audioEngine.soundHandles[handle]->maDecoder = nullptr;
        }
        AUDIO_DEBUG_PRINT("Error %i: '%s' failed to open", audioEngine.maResult, fileNameZ->chr);
        audioEngine.soundHandles[handle]->isUsed = false;
        return INVALID_SOUND_HANDLE;
    }

    AUDIO_DEBUG_PRINT("Sound successfully loaded");
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

        if (audioEngine.soundHandles[handle]->type == SoundType::Raw)
            audioEngine.soundHandles[handle]->rawStream->stop = true; // Signal miniaudio thread that we are going to end playback

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

        audioEngine.soundHandles[handle]->rawStream->PushSampleFrame(left, right);
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
        return audioEngine.soundHandles[handle]->rawStream->GetTimeRemaining();
    }

    return 0;
}

/// <summary>
/// This returns a sound handle to a newly created sound's raw data in memory with the given specification.
/// The user can then fill the buffer with whatever they want (using _MEMSOUND) and play it.
/// This is basically the sound equivalent of _NEWIMAGE.
/// </summary>
/// <param name="frames">The number of sample frames required</param>
/// <param name="channels">The number of sound channels. This can be 1 (mono) or 2 (stereo)/param>
/// <param name="bits">The bit depth of the sound. This can be 8 (unsigned 8-bit), 16 (signed 16-bit) or 32 (FP32)</param>
/// <returns>A new sound handle if successful or 0 on failure</returns>
int32_t func__sndnew(int32_t frames, int32_t channels, int32_t bits) {
    if (!audioEngine.isInitialized || frames <= 0) {
        AUDIO_DEBUG_CHECK(frames > 0);
        return INVALID_SOUND_HANDLE;
    }

    // Validate all parameters
    if ((channels != 1 && channels != 2) || (bits != 16 && bits != 32 && bits != 8)) {
        AUDIO_DEBUG_PRINT("Invalid channels (%i) or bits (%i)", channels, bits);
        return INVALID_SOUND_HANDLE;
    }

    // Alocate a sound handle
    int32_t handle = audioEngine.AllocateSoundHandle();
    if (handle < 1)
        return INVALID_SOUND_HANDLE;

    // Set some handle properties
    audioEngine.soundHandles[handle]->type = SoundType::Static;

    // Setup the ma_audio_buffer
    audioEngine.soundHandles[handle]->maAudioBufferConfig = ma_audio_buffer_config_init(
        (bits == 32 ? ma_format::ma_format_f32 : (bits == 16 ? ma_format::ma_format_s16 : ma_format::ma_format_u8)), channels, frames, NULL, NULL);

    // This currently has no effect. Sample rate always defaults to engine sample rate
    // Sample rate support for audio buffer is coming in miniaudio version 0.12
    // Once we have support, we can add sample rate as an optional 4th parameter
    // audioEngine.soundHandles[handle]->maAudioBufferConfig.sampleRate = audioEngine.sampleRate;

    // Allocate and initialize ma_audio_buffer
    audioEngine.maResult =
        ma_audio_buffer_alloc_and_init(&audioEngine.soundHandles[handle]->maAudioBufferConfig, &audioEngine.soundHandles[handle]->maAudioBuffer);
    if (audioEngine.maResult != MA_SUCCESS) {
        AUDIO_DEBUG_PRINT("Error %i: failed to initialize audio buffer", audioEngine.maResult);
        audioEngine.soundHandles[handle]->isUsed = false;
        return INVALID_SOUND_HANDLE;
    }

    // Create a ma_sound from the ma_audio_buffer
    audioEngine.maResult = ma_sound_init_from_data_source(&audioEngine.maEngine, audioEngine.soundHandles[handle]->maAudioBuffer,
                                                          audioEngine.soundHandles[handle]->maFlags, NULL, &audioEngine.soundHandles[handle]->maSound);
    if (audioEngine.maResult != MA_SUCCESS) {
        AUDIO_DEBUG_PRINT("Error %i: failed to initialize data source", audioEngine.maResult);
        audioEngine.soundHandles[handle]->isUsed = false;
        ma_audio_buffer_uninit_and_free(audioEngine.soundHandles[handle]->maAudioBuffer);
        audioEngine.soundHandles[handle]->maAudioBuffer = nullptr;
        return INVALID_SOUND_HANDLE;
    }

    AUDIO_DEBUG_PRINT("Frames = %i, Channels = %i, Bits = %i", frames, channels, bits);

    return handle;
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
    ma_format maFormat;
    ma_uint32 channels;
    ma_uint64 sampleFrames;
    ptrszint data;

    // Setup mem_block (assuming failure)
    mem_block mb = {};
    mb.lock_offset = (ptrszint)mem_lock_base;
    mb.lock_id = INVALID_MEM_LOCK;

    // Return invalid mem_block if audio is not initialized, handle is invalid or sound type is not static
    if (!audioEngine.isInitialized || !IS_SOUND_HANDLE_VALID(handle) || audioEngine.soundHandles[handle]->type != SoundType::Static ||
        (targetChannel != 0 && targetChannel != 1)) {
        AUDIO_DEBUG_PRINT("Invalid handle (%i), sound type (%i) or channel (%i)", handle, audioEngine.soundHandles[handle]->type, targetChannel);
        return mb;
    }

    // Check what kind of sound we are dealing with and take appropriate path
    if (audioEngine.soundHandles[handle]->maAudioBuffer) { // we are dealing with a user created audio buffer
        AUDIO_DEBUG_PRINT("Entering ma_audio_buffer path");
        maFormat = audioEngine.soundHandles[handle]->maAudioBufferConfig.format;
        channels = audioEngine.soundHandles[handle]->maAudioBufferConfig.channels;
        sampleFrames = audioEngine.soundHandles[handle]->maAudioBufferConfig.sizeInFrames;
        data = (ptrszint)&audioEngine.soundHandles[handle]->maAudioBuffer->_pExtraData[0];
    } else { // we are dealing with a sound loaded from file or memory
        AUDIO_DEBUG_PRINT("Entering ma_resource_manager_data_buffer path");

        // The sound cannot be steaming and must be completely decoded in memory
        if (audioEngine.soundHandles[handle]->maFlags & MA_SOUND_FLAG_STREAM || !(audioEngine.soundHandles[handle]->maFlags & MA_SOUND_FLAG_DECODE)) {
            AUDIO_DEBUG_PRINT("Sound data is not completely decoded");
            return mb;
        }

        // Get the pointer to the data source
        auto ds = (ma_resource_manager_data_buffer *)ma_sound_get_data_source(&audioEngine.soundHandles[handle]->maSound);
        if (!ds || !ds->pNode) {
            AUDIO_DEBUG_PRINT("Data source pointer OR data source node pointer is NULL");
            return mb;
        }

        // Check if the data is one contigious buffer or a link list of decoded pages
        // We cannot have a mem object for a link list of decoded pages for obvious reasons
        if (ds->pNode->data.type != ma_resource_manager_data_supply_type::ma_resource_manager_data_supply_type_decoded) {
            AUDIO_DEBUG_PRINT("Data is not a contigious buffer. Type = %u", ds->pNode->data.type);
            return mb;
        }

        // Check the data pointer
        if (!ds->pNode->data.backend.decoded.pData) {
            AUDIO_DEBUG_PRINT("Data source data pointer is NULL");
            return mb;
        }

        // Query the data format
        if (ma_sound_get_data_format(&audioEngine.soundHandles[handle]->maSound, &maFormat, &channels, NULL, NULL, 0) != MA_SUCCESS) {
            AUDIO_DEBUG_PRINT("Data format query failed");
            return mb;
        }

        // Get the length in sample frames
        if (ma_sound_get_length_in_pcm_frames(&audioEngine.soundHandles[handle]->maSound, &sampleFrames) != MA_SUCCESS) {
            AUDIO_DEBUG_PRINT("PCM frames query failed");
            return mb;
        }

        data = (ptrszint)ds->pNode->data.backend.decoded.pData;
    }

    AUDIO_DEBUG_PRINT("Format = %u, Channels = %u, Frames = %llu", maFormat, channels, sampleFrames);

    // Setup type: This was not done in the old code
    // But we are doing it here. By examing the type the user can now figure out if they have to use FP32 or integers
    switch (maFormat) {
    case ma_format::ma_format_f32:
        mb.type = 4 + 256; // FP32
        break;

    case ma_format::ma_format_s32:
        mb.type = 4 + 128; // signed int32
        break;

    case ma_format::ma_format_s16:
        mb.type = 2 + 128; // signed int16
        break;

    case ma_format::ma_format_u8:
        mb.type = 1 + 128 + 1024; // unsigned int8
        break;

    default:
        AUDIO_DEBUG_PRINT("Unsupported audio format");
        return mb;
    }

    if (audioEngine.soundHandles[handle]->memLockOffset) {
        AUDIO_DEBUG_PRINT("Returning previously created mem_lock");
        mb.lock_offset = (ptrszint)audioEngine.soundHandles[handle]->memLockOffset;
        mb.lock_id = audioEngine.soundHandles[handle]->memLockId;
    } else {
        AUDIO_DEBUG_PRINT("Returning new mem_lock");
        new_mem_lock();
        mem_lock_tmp->type = MEM_TYPE_SOUND;
        mb.lock_offset = (ptrszint)mem_lock_tmp;
        mb.lock_id = mem_lock_id;
        audioEngine.soundHandles[handle]->memLockOffset = (void *)mem_lock_tmp;
        audioEngine.soundHandles[handle]->memLockId = mem_lock_id;
    }

    mb.elementsize = ma_get_bytes_per_frame(maFormat, channels); // Set the element size. This is the size of each PCM frame in bytes
    mb.offset = data;                                            // Setup offset
    mb.size = sampleFrames * mb.elementsize;                     // Setup size (in bytes)
    mb.sound = handle;                                           // Copy the handle
    mb.image = 0;                                                // Not needed. Set to 0

    AUDIO_DEBUG_PRINT("ElementSize = %lli, Size = %lli, Type = %lli", mb.elementsize, mb.size, mb.type);

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
}

/// <summary>
/// This shuts down the audio engine and frees any resources used.
/// </summary>
void snd_un_init() {
    if (audioEngine.isInitialized) {
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
                    case SoundType::Raw:
                        // Dispose the sound if it has finished playing
                        // Note that this means that temporary looping sounds will never close
                        // Well thats on the programmer. Probably they want it that way
                        if (!ma_sound_is_playing(&audioEngine.soundHandles[handle]->maSound))
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
