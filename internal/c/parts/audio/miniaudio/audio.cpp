//----------------------------------------------------------------------------------------------------
//    ___  ___ ___ ___     _          _ _       ___           _
//   / _ \| _ ) _ \ __|   /_\ _  _ __| (_)___  | __|_ _  __ _(_)_ _  ___
//  | (_) | _ \  _/ _|   / _ \ || / _` | / _ \ | _|| ' \/ _` | | ' \/ -_)
//   \__\_\___/_| |___| /_/ \_\_,_\__,_|_\___/ |___|_||_\__, |_|_||_\___|
//                                                      |___/
//
//	QBPE Audio Engine powered by miniaudio (https://miniaud.io/)
//
//	Copyright (c) 2022 Samuel Gomes
//	https://github.com/a740g
//
//-----------------------------------------------------------------------------------------------------

//-----------------------------------------------------------------------------------------------------
// HEADER FILES
//-----------------------------------------------------------------------------------------------------
#include "audio.h"
#include <algorithm>
#include <vector>
// Enable Ogg Vorbis decoding
#define STB_VORBIS_HEADER_ONLY
#include "extras/stb_vorbis.c"
// PulseAudio has serious stuttering issues in ChromeOS Linux (Crostini) and possibly others
// This may be due to this - https://github.com/mackron/miniaudio/issues/427
// And https://wiki.archlinux.org/title/PulseAudio/Troubleshooting#Glitches,_skips_or_crackling
// We'll have to look at this closely later. If this is fixed, then remove this define from here & miniaudio_impl.cpp
#define MA_NO_PULSEAUDIO
// The main miniaudio header
#include "miniaudio.h"
//-----------------------------------------------------------------------------------------------------

//-----------------------------------------------------------------------------------------------------
// FORWARD DECLARATIONS
//-----------------------------------------------------------------------------------------------------
// These are stuff that was not declared anywhere else
// Again, we will wait for Matt to cleanup the C/C++ source file and include header files that declare this stuff
qbs *qbs_new_txt_len(const char *txt, int32 len);                   // Not declared in libqb.h
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
//-----------------------------------------------------------------------------------------------------

//-----------------------------------------------------------------------------------------------------
// CONSTANTS
//-----------------------------------------------------------------------------------------------------
// Set this to 1 if we want to print debug messages to stderr
#define AE_DEBUG 0

// This should be defined elsewhere in libqb. Since it is not, we are doing it here
#define INVALID_MEM_LOCK 1073741821

// This should be defined elsewhere in libqb. Since it is not, we are doing it here
#define MEM_TYPE_SOUND 5

// In QuickBASIC false means 0 and true means -1 (sad, but true XD)
#define QB_FALSE MA_FALSE
#define QB_TRUE -MA_TRUE

// This is returned to the caller if handle allocation fails. We do not return 0 becuase 0 is a valid vector index
// Technically all handles < 1 are invalid as far as the user is concerned (see IS_SOUND_HANDLE_VALID)
#define INVALID_SOUND_HANDLE -1

// This is the string that must be passed in the requirements parameter to stream a sound from storage
#define REQUIREMENT_STRING_STREAM "STREAM"
//-----------------------------------------------------------------------------------------------------

//-----------------------------------------------------------------------------------------------------
// MACROS
//-----------------------------------------------------------------------------------------------------
#define SAMPLE_FRAME_SIZE_BY_TYPE(_channels_, _type_) ((_channels_) * (sizeof(_type_) / sizeof(ma_uint8)))
#define SAMPLE_FRAME_SIZE_BY_FORMAT(_channels_, _format_) ((_channels_) * (ma_get_bytes_per_sample(_format_)))

// This basically checks if the handle is within vector limits and 'isUsed' is set to true
// We are relying on C's boolean short-circuit to not evaluate the last 'isUsed' if previous conditions are false
// Here we are checking > 0 because this is meant to check user handles only
#define IS_SOUND_HANDLE_VALID(_handle_)                                                                                                                        \
    ((_handle_) > 0 && (_handle_) < audioEngine.soundHandles.size() && audioEngine.soundHandles[_handle_]->isUsed &&                                           \
     !audioEngine.soundHandles[_handle_]->autoKill)

#ifdef QB64_WINDOWS
#    define ZERO_VARIABLE(_v_) ZeroMemory(&(_v_), sizeof(_v_))
#else
#    define ZERO_VARIABLE(_v_) memset(&(_v_), NULL, sizeof(_v_))
#endif

#if defined(AE_DEBUG) && AE_DEBUG > 0
#    ifdef _MSC_VER
#        define DEBUG_PRINT(_fmt_, ...) fprintf(stderr, "DEBUG: %s:%d:%s(): " _fmt_ "\n", __FILE__, __LINE__, __func__, __VA_ARGS__)
#    else
#        define DEBUG_PRINT(_fmt_, _args_...) fprintf(stderr, "DEBUG: %s:%d:%s(): " _fmt_ "\n", __FILE__, __LINE__, __func__, ##_args_)
#    endif
#    define DEBUG_CHECK(_exp_)                                                                                                                                 \
        if (!(_exp_))                                                                                                                                          \
        DEBUG_PRINT("Condition (%s) failed", #_exp_)
#else
#    ifdef _MSC_VER
#        define DEBUG_PRINT(_fmt_, ...) // Don't do anything in release builds
#    else
#        define DEBUG_PRINT(_fmt_, _args_...) // Don't do anything in release builds
#    endif
#    define DEBUG_CHECK(_exp_) // Don't do anything in release builds
#endif
//-----------------------------------------------------------------------------------------------------

//-----------------------------------------------------------------------------------------------------
// STRUCTURES, CLASSES & ENUMERATIONS
//-----------------------------------------------------------------------------------------------------
/// <summary>
/// Type of sound
/// </summary>
enum struct SoundType {
    None,   // No sound or internal sound whose buffer is managed by the QBPE audio engine
    Static, // Static sounds that are completely managed by miniaudio
    Raw     // Raw sound stream that is managed by the QBPE audio engine
};

/// <summary>
/// This struct encapsulates a sample frame block and it's management.
/// We choose these 'classes' to be barebones and completely transparent for performance reasons.
/// </summary>
struct SampleFrameBlockNode {
    SampleFrameBlockNode *next; // Next block node in the chain
    ma_uint32 samples;          // Size of the block in 'samples'. See below
    ma_uint32 offset;           // Where is the write cursor in the buffer (in samples!)?
    float *buffer;              // The actual sample frame block buffer
    bool force;                 // When this is set, the buffer will be processed even when if it is not full

    SampleFrameBlockNode() = delete;                                  // No default constructor
    SampleFrameBlockNode(const SampleFrameBlockNode &) = delete;      // No default copy constructor
    SampleFrameBlockNode &operator=(SampleFrameBlockNode &) = delete; // No assignment operator

    /// <summary>
    /// The constructor parameter is in sample frames.
    /// For a stereo sample frame we'll need (sample frames * 2) samples.
    /// Each sample is sizeof(float) bytes.
    /// </summary>
    /// <param name="sampleFrames">Number of sample frames needed</param>
    SampleFrameBlockNode(ma_uint32 sampleFrames) {
        next = nullptr;                // Set this to null. This will managed by the 'Queue' struct
        samples = sampleFrames << 1;   // 2 channels (stereo)
        offset = 0;                    // Set the write cursor to 0
        force = false;                 // Set the force flag to false by default
        buffer = new float[samples](); // Allocate a zeroed float buffer of size floats. Ah, Creative Silence!
    }

    /// <summary>
    /// Free the sample frame block that was allocated.
    /// </summary>
    ~SampleFrameBlockNode() { delete[] buffer; }

    /// <summary>
    /// Pushes a sample frame in the block and increments the offset.
    /// miniaudio expects it's stereo PCM data interleaved (LRLR format).
    /// No clipping is required because miniaudio does that for us (sweet!)
    /// </summary>
    /// <param name="l">Left floating point sample</param>
    /// <param name="r">Right floating point sample</param>
    /// <returns>Return true if operation was succcessful. False if block is full</returns>
    bool PushSampleFrame(float l, float r) {
        if (buffer && offset < samples) {
            buffer[offset] = l;
            ++offset;
            buffer[offset] = r;
            ++offset;

            return true;
        }

        return false;
    }

    /// <summary>
    /// Check if the buffer is completely filled.
    /// </summary>
    /// <returns>Returns true if buffer is full</returns>
    bool IsBufferFull() { return offset >= samples || force; }
};

/// <summary>
/// This is a light weight queue of type SampleFrameBlockNode and also the guts of SndRaw.
/// We could have used some std container but I wanted something really lean, simple and transparent.
/// </summary>
struct SampleFrameBlockQueue {
    SampleFrameBlockNode *first;           // First sample frame block
    SampleFrameBlockNode *last;            // Last sample frame block
    size_t blockCount;                     // Sample frame block count
    size_t frameCount;                     // Number of sample frames we have in the queue
    ma_uint32 sampleRate;                  // The sample rate reported by ma_engine
    ma_uint32 blockSampleFrames;           // How many sample frames do we need per 'block'. See below
    float *buffer;                         // This is the ping-pong buffer where the samples block will be 'streamed' to
    ma_uint32 bufferSampleFrames;          // Size of the ping-pong buffer in *samples frames*
    bool updateFlag;                       // We will only update the buffer with fresh samples when this flag is not equal to the check condition
    ma_uint32 bufferUpdatePosition;        // The position (in samples) in the buffer where we should be copying a sample block
    ma_uint32 sampleFramesPlaying;         // The number of sample frames that was sent for playback
    ma_uint64 maEngineTime;                // miniaudio engine time use for correct length calculation
    ma_sound *maSound;                     // Pointer to a ma_sound object that was passed in the constructor
    ma_engine *maEngine;                   // Pointer to a ma_engine object
    ma_audio_buffer maBuffer;              // miniaudio buffer object
    ma_audio_buffer_config maBufferConfig; // miniaudio buffer configuration
    ma_result maResult;                    // This is the result of the last miniaudio operation (used for trapping errors)

    SampleFrameBlockQueue() = delete;                                   // No default constructor
    SampleFrameBlockQueue(const SampleFrameBlockQueue &) = delete;      // No default copy constructor
    SampleFrameBlockQueue &operator=(SampleFrameBlockQueue &) = delete; // No assignment operator

    /// <summary>
    /// This initialized the queue and calculates the sample frames per block
    /// </summary>
    /// <param name="pmaEngine">A pointer to a miniaudio engine object</param>
    /// <param name="pmaSound">A pointer to a miniaudio sound object</param>
    SampleFrameBlockQueue(ma_engine *pmaEngine, ma_sound *pmaSound) {
        first = last = nullptr;
        blockCount = frameCount = maEngineTime = sampleFramesPlaying = bufferUpdatePosition = 0;
        maSound = pmaSound;                               // Save the pointer to the ma_sound object (this is basically from a QBPE sound handle)
        maEngine = pmaEngine;                             // Save the pointer to the ma_engine object (this should come from the QBPE sound engine)
        sampleRate = ma_engine_get_sample_rate(maEngine); // Save the sample rate

        // We can get away with '>> 4' because the sound loop function is called @ ~60Hz
        // This should work even on entry level systems. Tested on AMD A6-9200 (230.4 GFLOPS), Crostini Linux
        // Also note that the nodes will allocates twice this to account for 2 channels
        blockSampleFrames = sampleRate >> 4;

        bufferSampleFrames = blockSampleFrames * 2;   // We want the playback buffer twice the size of a block to do a proper ping-pong
        buffer = new float[bufferSampleFrames * 2](); // Allocate a zeroed float buffer of bufferSizeSampleFrames * 2 floats (2 is for 2 channels - stereo)
        updateFlag = false;                           // Set this to false because we want the initial check to fail

        if (buffer) {
            // Setup the ma buffer
            maBufferConfig = ma_audio_buffer_config_init(ma_format::ma_format_f32, 2, bufferSampleFrames, buffer, NULL);
            maResult = ma_audio_buffer_init(&maBufferConfig, &maBuffer);
            DEBUG_CHECK(maResult == MA_SUCCESS);

            // Create a ma_sound from the ma_buffer
            maResult = ma_sound_init_from_data_source(maEngine, &maBuffer, 0, NULL, maSound);
            DEBUG_CHECK(maResult == MA_SUCCESS);

            // Play the ma_sound
            maResult = ma_sound_start(maSound);
            DEBUG_CHECK(maResult == MA_SUCCESS);

            // Set the buffer to loop forever
            ma_sound_set_looping(maSound, MA_TRUE);
        }

        DEBUG_PRINT("Raw sound stream created with %u sample frame block size", blockSampleFrames);
    }

    /// <summary>
    /// This simply pops all sample blocks
    /// </summary>
    ~SampleFrameBlockQueue() {
        if (buffer) {
            // Stop playback
            maResult = ma_sound_stop(maSound);
            DEBUG_CHECK(maResult == MA_SUCCESS);

            // Delete the ma_sound object
            ma_sound_uninit(maSound);

            // Delete the ma_buffer object
            ma_audio_buffer_uninit(&maBuffer);
        }

        while (PopSampleFrameBlock())
            ;

        delete[] buffer;

        DEBUG_PRINT("Raw sound stream closed");
    }

    /// <summary>
    /// This pushes a sample frame into the queue.
    /// If there are no sample frame blocks then it creates one.
    /// If the last sample frame block is full it creates a new one and links it to the chain.
    /// Note that in QBPE all samples frames are assumed to be stereo.
    /// Mono sample frames are simply simulated by playing the same data from left and right.
    /// No clipping is required because miniaudio does that for us (sweet!)
    /// </summary>
    /// <param name="l">The left sample</param>
    /// <param name="r">The right sample</param>
    /// <returns>Returns true if operation was successful</returns>
    bool PushSampleFrame(float l, float r) {
        // Attempt to push the frame into the last node if one exists
        // If successfull return true
        if (last && last->PushSampleFrame(l, r)) {
            ++frameCount; // Increment the frame count
            return true;
        }

        // If we reached here, then it means that either there are no nodes or the last one is full
        // Simply create a new node and then link it to the chain
        SampleFrameBlockNode *node = new SampleFrameBlockNode(blockSampleFrames);

        // Return false if memory allocation failed or we're mot able to save the sample frame
        if (!node || !node->PushSampleFrame(l, r)) {
            delete node;
            return false; // Ignore the sample frame and exit silently
        }

        if (last)
            last->next = node; // Add the node to the last node if we have nodes in the queue
        else
            first = node; // Else this is the first node

        last = node;  // The last item in the queue is node
        ++blockCount; // Increase the frame block count
        ++frameCount; // Increment the frame count

        return true;
    }

    /// <summary>
    /// This pops a sample frame block from the front of the queue.
    /// The sample frame block can be accessed before popping using the 'first' member.
    /// Popping a block frees and invalidates the memory it was using. So, pop a block only when we are sure that we do not need it.
    /// </summary>
    /// <returns>Returns true if we were able to pop. False means the queue is empty</returns>
    bool PopSampleFrameBlock() {
        // Only if the queue has some sample frame blocks then...
        if (blockCount) {
            SampleFrameBlockNode *node = first; // Set node to the first frame in the queue

            --blockCount;                    // Decrement the block count now so that we know what to do with 'last'
            frameCount -= node->offset >> 1; // Decrease frame count by number of sample frames written in the block (/ 2 for channels)
            first = node->next;              // Detach the node. If this is the last node then 'first' will be NULL cause node->next is NULL

            if (!blockCount)
                last = nullptr; // This means that node was the last node

            delete node; // Free the node

            return true;
        }

        return false;
    }

    /// <summary>
    /// Returns the length, in sample frames of sound queued.
    /// </summary>
    /// <returns>The length left to play in sample frames</returns>
    ma_uint64 GetSampleFramesRemaining() {
        // Calculate the time difference (ma_engine time is really just of a sum of sample frames sent to the device)
        ma_uint64 maEngineDeltaTime = ma_engine_get_time(maEngine) - maEngineTime;

        // Decrement the delta from the sample frames that are playing
        // Using std::min here is probably risky since these are all unsigned types
        sampleFramesPlaying = maEngineDeltaTime > sampleFramesPlaying ? 0 : (ma_uint32)(sampleFramesPlaying - maEngineDeltaTime);

        // Add this to the frames in the queue
        return sampleFramesPlaying + frameCount;
    }

    /// <summary>
    /// Returns the length, in seconds of sound queued.
    /// </summary>
    /// <returns>The length left to play in seconds</returns>
    double GetTimeRemaining() {
        ma_uint64 sampleFramesRemaining = GetSampleFramesRemaining();

        // This will help us avoid situations where we can get a non-zero value even if GetSampleFramesRemaining returns 0
        if (!sampleFramesRemaining)
            return 0;
        else
            return (double)sampleFramesRemaining / sampleRate;
    }

    /// <summary>
    /// Check if everything is ready to go
    /// </summary>
    /// <returns>Returns true if everything is a go</returns>
    bool IsSetupValid() { return buffer && maEngine && maSound && maResult == MA_SUCCESS; }

    /// <summary>
    /// This keeps the ping-pong (ring? whatever...) buffer fed and the sound stream going
    /// </summary>
    void Update() {
        // Figure out which pcm frame of the buffer is miniaudio playing
        ma_uint64 readCursor;
        maResult = ma_sound_get_cursor_in_pcm_frames(maSound, &readCursor);
        DEBUG_CHECK(maResult == MA_SUCCESS);

        bool checkCondition = readCursor < blockSampleFrames; // Since buffer sample frame size = blockSampleFrames * 2

        // Only proceed to update if our flag is not the same as our condition
        if (checkCondition != updateFlag) {
            // The line below does two sneaky things that deserve explanation
            //	1. We are using bufferSampleFrames which is set to exactly halfway through the buffer since we are using stereo (see constructor)
            //	2. The boolean condition above will always be 1 if the read cursor is in the lower-half and hence push the position to the top-half
            //  3. Obviously, this means that if the condition is 0 then position will be set to the lower-half
            bufferUpdatePosition = checkCondition * bufferSampleFrames; // This line basically toggles the buffer copy position

            // Check if we have any blocks in the queue and stream only if the block is full
            if (blockCount && first->IsBufferFull()) {
                // We check this here so that even if the buffer is not allocated, the block object will be popped off
                if (first->buffer) {
                    // Simply copy the first block in the queue
                    std::copy(first->buffer, first->buffer + first->samples, buffer + bufferUpdatePosition);
                }

                // Save the number of samples frames sent for playback and the current time for correct time calculation
                sampleFramesPlaying = blockSampleFrames;
                maEngineTime = ma_engine_get_time(maEngine);

                // And then pop it off
                PopSampleFrameBlock();
            } else { // Else we'll stream silence
                // We are using bufferSampleFrames here for the same reason as the explanation above
                std::fill(buffer + bufferUpdatePosition, buffer + bufferUpdatePosition + bufferSampleFrames, NULL);
            }

            updateFlag = checkCondition; // Save our check condition to our flag
        }
    }
};

/// <summary>
/// Sound handle type
/// This describes every sound the system will ever play (including raw streams).
/// </summary>
struct SoundHandle {
    bool isUsed;                     // Is this handle in active use?
    SoundType type;                  // Type of sound (see SoundType enum class)
    bool autoKill;                   // Do we need to auto-clean this sample / stream after playback is done?
    ma_sound maSound;                // miniaudio sound
    ma_uint32 maFlags;               // miniaudio flags that were used when initializing the sound
    SampleFrameBlockQueue *rawQueue; // Raw sample frame queue
    void *memLockOffset;             // This is a pointer from new_mem_lock()
    uint64 memLockId;                // This is mem_lock_id created by new_mem_lock()
};

/// <summary>
///	Type will help us keep track of the audio engine state
/// </summary>
struct AudioEngine {
    bool isInitialized;                      // This is set to true if we were able to initialize miniaudio and allocated all required resources
    bool initializationFailed;               // This is set to true if a past initialization attempt failed
    ma_engine maEngine;                      // This is the primary miniaudio engine 'context'. Everything happens using this!
    ma_result maResult;                      // This is the result of the last miniaudio operation (used for trapping errors)
    ma_uint32 sampleRate;                    // Sample rate used by the miniaudio engine
    int32 sndInternal;                       // Internal sound handle that we will use for Play(), Beep() & Sound()
    int32 sndInternalRaw;                    // Internal sound handle that we will use for the QB64 'handle-less' raw stream
    std::vector<SoundHandle *> soundHandles; // This is the audio handle list used by the engine and by everything else

    AudioEngine(const AudioEngine &) = delete;      // No default copy constructor
    AudioEngine &operator=(AudioEngine &) = delete; // No assignment operator

    /// <summary>
    ///	Just initializes some important members
    /// </summary>
    AudioEngine() {
        isInitialized = initializationFailed = false;
        sampleRate = 0;
        sndInternal = sndInternalRaw = INVALID_SOUND_HANDLE;
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
    /// All this means that a sloppy programmer may be able to grow the vector and eventually the system may run out of memory and crash.
    /// But that's ok. Sloppy programmers (like me) must be punished until they learn! XD
    /// BTW, int32 was a bad choice for handles. Oh well...
    /// </summary>
    /// <returns>Returns a non-negative handle if successful</returns>
    int32 AllocateSoundHandle() {
        if (!isInitialized)
            return INVALID_SOUND_HANDLE;

        size_t h, vectorSize = soundHandles.size(); // Save the vector size

        // Scan through the vector and return a slot that is not being used
        // This loop should not execute if size is 0
        for (h = 0; h < vectorSize; h++) {
            if (!soundHandles[h]->isUsed) {
                DEBUG_PRINT("Sound handle %i recycled", h);
                break;
            }
        }

        if (h >= vectorSize) {
            // If we have reached here then either the vector is empty or there are no empty slots
            // Simply create a new SoundHandle at the back of the vector
            SoundHandle *newHandle = new SoundHandle;

            if (!newHandle)
                return INVALID_SOUND_HANDLE;

            soundHandles.push_back(newHandle);
            size_t newVectorSize = soundHandles.size();

            // If newVectorSize == vectorSize then push_back() failed
            if (newVectorSize <= vectorSize) {
                delete newHandle;
                return INVALID_SOUND_HANDLE;
            }

            h = newVectorSize - 1; // The handle is simply newsize - 1

            DEBUG_PRINT("Sound handle %i created", h);
        }

        // Initializes a sound handle that was just allocated.
        // This will set it to 'in use' after applying some defaults.
        soundHandles[h]->type = SoundType::None;
        soundHandles[h]->autoKill = false;
        soundHandles[h]->rawQueue = nullptr;
        ZERO_VARIABLE(soundHandles[h]->maSound);
        // We do not use pitch shifting, so this will give a little performance boost
        // Spatialization is disabled by default but will be enabled on the fly if required
        soundHandles[h]->maFlags = MA_SOUND_FLAG_NO_PITCH | MA_SOUND_FLAG_NO_SPATIALIZATION | MA_SOUND_FLAG_WAIT_INIT;
        soundHandles[h]->memLockId = INVALID_MEM_LOCK;
        soundHandles[h]->memLockOffset = nullptr;
        soundHandles[h]->isUsed = true;

        DEBUG_PRINT("Sound handle %i returned", h);

        return (int32)(h);
    }

    /// <summary>
    /// The frees and unloads an open sound.
    /// If the sound is playing or looping, it will be stopped.
    /// If the sound is a stream of raw samples then it is stopped and freed.
    /// Finally the handle is invalidated and put-up for recycling.
    /// </summary>
    /// <param name="handle">A sound handle</param>
    void FreeSoundHandle(int32 handle) {
        if (isInitialized && handle >= 0 && handle < soundHandles.size() && soundHandles[handle]->isUsed) {
            // Sound type specific cleanup
            switch (soundHandles[handle]->type) {
            case SoundType::Static:
                ma_sound_uninit(&soundHandles[handle]->maSound);

                break;

            case SoundType::Raw:
                delete soundHandles[handle]->rawQueue;
                soundHandles[handle]->rawQueue = nullptr;

                break;

            case SoundType::None:
                if (handle != 0)
                    DEBUG_PRINT("Sound type is 'None' when handle value is not 0");

                break;

            default:
                DEBUG_PRINT("Condition not handled"); // It should not come here
            }

            // Now simply set the 'isUsed' member to false
            soundHandles[handle]->isUsed = false;

            if (soundHandles[handle]->memLockOffset) {
                free_mem_lock((mem_lock *)soundHandles[handle]->memLockOffset);
                soundHandles[handle]->memLockId = INVALID_MEM_LOCK;
                soundHandles[handle]->memLockOffset = nullptr;
            }

            DEBUG_PRINT("Sound handle %i marked as free", handle);
        }
    }
};
//-----------------------------------------------------------------------------------------------------

//-----------------------------------------------------------------------------------------------------
// GLOBAL VARIABLES
//-----------------------------------------------------------------------------------------------------
// This keeps track of the audio engine state
static AudioEngine audioEngine;
//-----------------------------------------------------------------------------------------------------

//-----------------------------------------------------------------------------------------------------
// FUNCTIONS
//-----------------------------------------------------------------------------------------------------
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

    *soundwave_bytes = samplesi * SAMPLE_FRAME_SIZE_BY_TYPE(2, ma_int16);
    data = (ma_uint8 *)malloc(*soundwave_bytes);
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
        *soundwave_bytes = waveend * SAMPLE_FRAME_SIZE_BY_TYPE(2, ma_int16);

    return (ma_uint8 *)data;
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

    return samples * SAMPLE_FRAME_SIZE_BY_TYPE(2, ma_int16);
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
    static ma_int32 i;
    static ma_int64 time_ms;

    // Move data into sndraw handle
    for (i = 0; i < bytes; i += 4) {
        audioEngine.soundHandles[audioEngine.sndInternal]->rawQueue->PushSampleFrame((float)((ma_int16 *)(data + i))[0] / 32768.0f,
                                                                                     (float)((ma_int16 *)(data + i))[1] / 32768.0f);
    }

    free(data); // free the sound data

    // This will push any unfinished block for playback
    if (audioEngine.soundHandles[audioEngine.sndInternal]->rawQueue->last)
        audioEngine.soundHandles[audioEngine.sndInternal]->rawQueue->last->force = true;

    if (block) {
        time_ms = (ma_int64)audioEngine.soundHandles[audioEngine.sndInternal]->rawQueue->GetTimeRemaining() * 950.0 - 250.0;
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

    audioEngine.soundHandles[audioEngine.sndInternal]->type = SoundType::Raw; // This will start processing handle 0 as a raw stream

    data = GenerateWaveform(frequency, lengthInClockTicks / 18.2, 1, &soundwave_bytes);
    SendWaveformToQueue(data, soundwave_bytes, true);

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
/// However, here we just return the number of sample frame remaining to play when Play(), Sound() or Beep() are used.
/// This allows programs like the following to compile and work.
///
///     Music$ = "MBT180o2P2P8L8GGGL2E-P24P8L8FFFL2D"
///     PLAY Music$
///     WHILE PLAY(0) > 5: WEND
///     PRINT "Just about done!"
/// </summary>
/// <param name="ignore">Well, it's ignored</param>
/// <returns>Returns the number of sample frames left to play for Play(), Sound() & Beep()</returns>
int32 func_play(int32 ignore) {
    if (audioEngine.isInitialized && audioEngine.sndInternal == 0) {
        // This will push any unfinished block for playback
        if (audioEngine.soundHandles[audioEngine.sndInternal]->rawQueue->last)
            audioEngine.soundHandles[audioEngine.sndInternal]->rawQueue->last->force = true;

        return (int32)audioEngine.soundHandles[audioEngine.sndInternal]->rawQueue->GetSampleFramesRemaining();
    }

    return 0;
}

/// <summary>
/// Processes and plays the MML specified in the string.
/// Ummm. Spaghetti goodness.
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
    static double mb = 0;
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

    audioEngine.soundHandles[audioEngine.sndInternal]->type = SoundType::Raw; // This will start processing handle 0 as a raw stream

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
                if (!mb) {
                    mb = 1;
                    if (playit) {
                        playit = 0;
                        SendWaveformToQueue(wave, wave_bytes, true);
                    }
                    wave = NULL;
                }
                break;
            case 70: // MF
                if (mb) {
                    mb = 0;
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
        if (mb) {
            SendWaveformToQueue(wave, wave_bytes, false);
        } else {
            SendWaveformToQueue(wave, wave_bytes, true);
        }
    } // playit
}

/// <summary>
/// This returns the sample rate from ma engine if ma is initialized.
/// </summary>
/// <returns>miniaudio sample rtate</returns>
int32 func__sndrate() { return audioEngine.sampleRate; }

/// <summary>
/// This loads a sound file into memory and returns a LONG handle value above 0.
/// </summary>
/// <param name="fileName">The is the pathname for the sound file. This can be any format the miniaudio or a miniaudio plugin</param>
/// <param name="requirements">This is leftover from the old QB64-SDL days. But we use this to pass some parameters like 'stream'</param>
/// <param name="passed">How many parameters were passed?</param>
/// <returns>Returns a valid sound handle (> 0) if successful or 0 if it fails</returns>
int32 func__sndopen(qbs *fileName, qbs *requirements, int32 passed) {
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
    int32 handle = audioEngine.AllocateSoundHandle();
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
        audioEngine.soundHandles[handle]->isUsed = false;

        return INVALID_SOUND_HANDLE;
    }

    return handle;
}

/// <summary>
/// The frees and unloads an open sound.
/// If the sound is playing, it'll let it finish. Looping sounds will loop until the program is closed.
/// If the sound is a stream of raw samples then any remaining samples pending for playback will be sent to miniaudio and then the handle will be freed.
/// </summary>
/// <param name="handle">A sound handle</param>
void sub__sndclose(int32 handle) {
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
int32 func__sndcopy(int32 src_handle) {
    // Check for all invalid cases
    if (!audioEngine.isInitialized || !IS_SOUND_HANDLE_VALID(src_handle) || audioEngine.soundHandles[src_handle]->type != SoundType::Static)
        return INVALID_SOUND_HANDLE;

    // Alocate a sound handle
    int32 dst_handle = audioEngine.AllocateSoundHandle();
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
void sub__sndplay(int32 handle) {
    if (audioEngine.isInitialized && IS_SOUND_HANDLE_VALID(handle) && audioEngine.soundHandles[handle]->type == SoundType::Static) {
        // Reset position to zero only if we are playing and (not looping or we've reached the end of the sound)
        // This is based on the old OpenAl-soft code behavior
        if (ma_sound_is_playing(&audioEngine.soundHandles[handle]->maSound) &&
            (!ma_sound_is_looping(&audioEngine.soundHandles[handle]->maSound) || ma_sound_at_end(&audioEngine.soundHandles[handle]->maSound))) {
            audioEngine.maResult = ma_sound_seek_to_pcm_frame(&audioEngine.soundHandles[handle]->maSound, 0);
            DEBUG_CHECK(audioEngine.maResult == MA_SUCCESS);
        }

        // Kickstart playback
        audioEngine.maResult = ma_sound_start(&audioEngine.soundHandles[handle]->maSound);
        DEBUG_CHECK(audioEngine.maResult == MA_SUCCESS);

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
/// <param name="passed">How many parameters were passed?</param>
void sub__sndplaycopy(int32 src_handle, double volume, int32 passed) {
    // We are simply going to use sndcopy, then setup some stuff like volume and autokill and then use sndplay
    // We are not checking if the audio engine was initialized because if not we'll get an invalid handle anyway
    int32 dst_handle = func__sndcopy(src_handle);

    // Check if we succeeded and then proceed
    if (dst_handle > 0) {
        // Set the volume if requested
        if (passed)
            ma_sound_set_volume(&audioEngine.soundHandles[dst_handle]->maSound, volume);

        sub__sndplay(dst_handle);                              // Play the sound
        audioEngine.soundHandles[dst_handle]->autoKill = true; // Set to auto kill
    }
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
void sub__sndplayfile(qbs *fileName, int32 sync, double volume, int32 passed) {
    // We need this to send requirements to SndOpen
    static qbs *reqs = nullptr;

    if (!reqs) {
        // Since this never changes, we can get away by doing this just once
        reqs = qbs_new(0, 0);
        qbs_set(reqs, qbs_new_txt(REQUIREMENT_STRING_STREAM));
    }

    // We will not wrap this in a 'if initialized' block because SndOpen will take care of that
    int32 handle = func__sndopen(fileName, reqs, 1);

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
void sub__sndpause(int32 handle) {
    if (audioEngine.isInitialized && IS_SOUND_HANDLE_VALID(handle) && audioEngine.soundHandles[handle]->type == SoundType::Static) {
        // Stop the sound and just leave it at that
        // miniaudio does not reset the play cursor
        audioEngine.maResult = ma_sound_stop(&audioEngine.soundHandles[handle]->maSound);
        DEBUG_CHECK(audioEngine.maResult == MA_SUCCESS);
    }
}

/// <summary>
/// This returns whether a sound is being played.
/// </summary>
/// <param name="handle">A sound handle</param>
/// <returns>Return true if the sound is playing. False otherwise</returns>
int32 func__sndplaying(int32 handle) {
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
int32 func__sndpaused(int32 handle) {
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
void sub__sndvol(int32 handle, float volume) {
    if (audioEngine.isInitialized && IS_SOUND_HANDLE_VALID(handle) &&
        (audioEngine.soundHandles[handle]->type == SoundType::Static || audioEngine.soundHandles[handle]->type == SoundType::Raw)) {
        ma_sound_set_volume(&audioEngine.soundHandles[handle]->maSound, volume);
    }
}

/// <summary>
/// This is like sub__sndplay but the sound is looped.
/// </summary>
/// <param name="handle"></param>
void sub__sndloop(int32 handle) {
    if (audioEngine.isInitialized && IS_SOUND_HANDLE_VALID(handle) && audioEngine.soundHandles[handle]->type == SoundType::Static) {
        // Reset position to zero only if we are playing and (not looping or we've reached the end of the sound)
        // This is based on the old OpenAl-soft code behavior
        if (ma_sound_is_playing(&audioEngine.soundHandles[handle]->maSound) &&
            (!ma_sound_is_looping(&audioEngine.soundHandles[handle]->maSound) || ma_sound_at_end(&audioEngine.soundHandles[handle]->maSound))) {
            audioEngine.maResult = ma_sound_seek_to_pcm_frame(&audioEngine.soundHandles[handle]->maSound, 0);
            DEBUG_CHECK(audioEngine.maResult == MA_SUCCESS);
        }

        // Kickstart playback
        audioEngine.maResult = ma_sound_start(&audioEngine.soundHandles[handle]->maSound);
        DEBUG_CHECK(audioEngine.maResult == MA_SUCCESS);

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
void sub__sndbal(int32 handle, double x, double y, double z, int32 channel, int32 passed) {
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

            ma_sound_set_position(&audioEngine.soundHandles[handle]->maSound, x, y, z); // Use full 3D positioning
        } else {
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
double func__sndlen(int32 handle) {
    if (audioEngine.isInitialized && IS_SOUND_HANDLE_VALID(handle) && audioEngine.soundHandles[handle]->type == SoundType::Static) {
        float lengthSeconds = 0;
        audioEngine.maResult = ma_sound_get_length_in_seconds(&audioEngine.soundHandles[handle]->maSound, &lengthSeconds);
        DEBUG_CHECK(audioEngine.maResult == MA_SUCCESS);

        return lengthSeconds;
    }

    return 0;
}

/// <summary>
/// This returns the current playing position in seconds using a sound handle.
/// </summary>
/// <param name="handle">A sound handle</param>
/// <returns>Returns the current playing position in seconds from an open sound file</returns>
double func__sndgetpos(int32 handle) {
    if (audioEngine.isInitialized && IS_SOUND_HANDLE_VALID(handle) && audioEngine.soundHandles[handle]->type == SoundType::Static) {
        float playCursorSeconds = 0;
        audioEngine.maResult = ma_sound_get_cursor_in_seconds(&audioEngine.soundHandles[handle]->maSound, &playCursorSeconds);
        DEBUG_CHECK(audioEngine.maResult == MA_SUCCESS);

        return playCursorSeconds;
    }

    return 0;
}

/// <summary>
/// This changes the current/starting playing position in seconds of a sound.
/// </summary>
/// <param name="handle">A sound handle</param>
/// <param name="seconds">The position to set in seconds</param>
void sub__sndsetpos(int32 handle, double seconds) {
    if (audioEngine.isInitialized && IS_SOUND_HANDLE_VALID(handle) && audioEngine.soundHandles[handle]->type == SoundType::Static) {
        float lengthSeconds;
        audioEngine.maResult = ma_sound_get_length_in_seconds(&audioEngine.soundHandles[handle]->maSound, &lengthSeconds); // Get the length in seconds
        if (audioEngine.maResult != MA_SUCCESS)
            return;

        if (seconds > lengthSeconds) // If position is beyond length then simply stop playback and exit
        {
            audioEngine.maResult = ma_sound_stop(&audioEngine.soundHandles[handle]->maSound);
            DEBUG_CHECK(audioEngine.maResult == MA_SUCCESS);
            return;
        }

        ma_uint64 lengthSampleFrames;
        audioEngine.maResult =
            ma_sound_get_length_in_pcm_frames(&audioEngine.soundHandles[handle]->maSound, &lengthSampleFrames); // Get the total sample frames
        if (audioEngine.maResult != MA_SUCCESS)
            return;

        audioEngine.maResult = ma_sound_seek_to_pcm_frame(&audioEngine.soundHandles[handle]->maSound,
                                                          lengthSampleFrames * (seconds / lengthSeconds)); // Set the postion in PCM frames
        DEBUG_CHECK(audioEngine.maResult == MA_SUCCESS);
    }
}

/// <summary>
/// This stops playing a sound after it has been playing for a set number of seconds.
/// </summary>
/// <param name="handle">A sound handle</param>
/// <param name="limit">The number of seconds that the sound will play</param>
void sub__sndlimit(int32 handle, double limit) {
    if (audioEngine.isInitialized && IS_SOUND_HANDLE_VALID(handle) && audioEngine.soundHandles[handle]->type == SoundType::Static) {
        ma_sound_set_stop_time_in_milliseconds(&audioEngine.soundHandles[handle]->maSound, limit * 1000);
    }
}

/// <summary>
/// This stops a playing or paused sound using a sound handle.
/// </summary>
/// <param name="handle">A sound handle</param>
void sub__sndstop(int32 handle) {
    if (audioEngine.isInitialized && IS_SOUND_HANDLE_VALID(handle) && audioEngine.soundHandles[handle]->type == SoundType::Static) {
        // Stop the sound first
        audioEngine.maResult = ma_sound_stop(&audioEngine.soundHandles[handle]->maSound);
        DEBUG_CHECK(audioEngine.maResult == MA_SUCCESS);

        // Also reset the playback cursor to zero
        audioEngine.maResult = ma_sound_seek_to_pcm_frame(&audioEngine.soundHandles[handle]->maSound, 0);
        DEBUG_CHECK(audioEngine.maResult == MA_SUCCESS);
    }
}

/// <summary>
/// This function opens a new channel to fill with _SNDRAW content to manage multiple dynamically generated sounds.
/// </summary>
/// <returns>A new sound handle if successful or 0 on failure</returns>
int32 func__sndopenraw() {
    // Return invalid handle if audio engine is not initialized
    if (!audioEngine.isInitialized)
        return INVALID_SOUND_HANDLE;

    // Alocate a sound handle
    int32 handle = audioEngine.AllocateSoundHandle();
    if (handle < 1)
        return INVALID_SOUND_HANDLE;

    // Set some handle properties
    audioEngine.soundHandles[handle]->type = SoundType::Raw;

    // Create the raw sound object
    audioEngine.soundHandles[handle]->rawQueue = new SampleFrameBlockQueue(&audioEngine.maEngine, &audioEngine.soundHandles[handle]->maSound);
    if (!audioEngine.soundHandles[handle]->rawQueue)
        return INVALID_SOUND_HANDLE;

    // Check if everything was setup correctly
    if (!audioEngine.soundHandles[handle]->rawQueue->IsSetupValid()) {
        delete audioEngine.soundHandles[handle]->rawQueue;
        audioEngine.soundHandles[handle]->rawQueue = nullptr;

        return INVALID_SOUND_HANDLE;
    }

    return handle;
}

/// <summary>
/// This plays sound wave sample frequencies created by a program.
/// </summary>
/// <param name="left">Left channel sample</param>
/// <param name="right">Right channel sample</param>
/// <param name="handle">A sound handle</param>
/// <param name="passed">How many parameters were passed?</param>
void sub__sndraw(float left, float right, int32 handle, int32 passed) {
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

        audioEngine.soundHandles[handle]->rawQueue->PushSampleFrame(left, right);
    }
}

/// <summary>
/// This ensures that the final buffer portion is played in short sound effects even if it is incomplete.
/// </summary>
/// <param name="handle">A sound handle</param>
/// <param name="passed">How many parameters were passed?</param>
void sub__sndrawdone(int32 handle, int32 passed) {
    // Use the default raw handle if handle was not passed
    if (!passed)
        handle = audioEngine.sndInternalRaw;

    if (audioEngine.isInitialized && IS_SOUND_HANDLE_VALID(handle) && audioEngine.soundHandles[handle]->type == SoundType::Raw) {
        // Set the last block's force flag to true
        if (audioEngine.soundHandles[handle]->rawQueue->last) {
            audioEngine.soundHandles[handle]->rawQueue->last->force = true;
        }
    }
}

/// <summary>
/// This function returns the length, in seconds, of a _SNDRAW sound currently queued.
/// </summary>
/// <param name="handle">A sound handle</param>
/// <param name="passed">How many parameters were passed?</param>
/// <returns></returns>
double func__sndrawlen(int32 handle, int32 passed) {
    // Use the default raw handle if handle was not passed
    if (!passed)
        handle = audioEngine.sndInternalRaw;

    if (audioEngine.isInitialized && IS_SOUND_HANDLE_VALID(handle) && audioEngine.soundHandles[handle]->type == SoundType::Raw) {
        return audioEngine.soundHandles[handle]->rawQueue->GetTimeRemaining();
    }

    return 0;
}

/// <summary>
/// This function returns a _MEM value referring to a sound's raw data in memory using a designated sound handle created by the _SNDOPEN function.
/// miniaudio supports a variety of sample and channel formats. Translating all of that to basic 2 channel 16-bit formats that
/// MemSound was originally supporting would require significant overhead both in terms of system resources and code.
/// For now we are just exposing the underlying PCM data directly from miniaudio. This fits rather well using the existing mem structure.
/// Mono sounds should continue to work just as it was before. Stereo and multi-channel sounds however will be required to be handled correctly
/// by the user by checking the 'elementsize' member - which is set to the size (in bytes) of each PCM frame.
/// </summary>
/// <param name="handle">A sound handle</param>
/// <param name="targetChannel">This is ignored with miniaudio as it uses interleaved samples</param>
/// <returns>A _MEM value that can be used to access the sound data</returns>
mem_block func__memsound(int32 handle, int32 targetChannel) {
    static mem_block mb;
    static ma_data_source *pcmData;
    static ma_format maFormat;
    static ma_uint32 channels;
    static ma_uint64 sampleFrames;

    if (new_error || !audioEngine.isInitialized || !IS_SOUND_HANDLE_VALID(handle) || audioEngine.soundHandles[handle]->type != SoundType::Static ||
        audioEngine.soundHandles[handle]->maFlags & MA_SOUND_FLAG_STREAM || !(audioEngine.soundHandles[handle]->maFlags & MA_SOUND_FLAG_DECODE))
        goto error;

    // Get the pointer to the data source
    // TODO: The hope is that this will have the raw frames without any header data. But this needs to be tested thoroughly!
    pcmData = ma_sound_get_data_source(&audioEngine.soundHandles[handle]->maSound);
    if (!pcmData)
        goto error;

    // Query the data format
    if (ma_sound_get_data_format(&audioEngine.soundHandles[handle]->maSound, &maFormat, &channels, NULL, NULL, NULL) != MA_SUCCESS)
        goto error;

    // Get the length in sample frames
    if (ma_sound_get_length_in_pcm_frames(&audioEngine.soundHandles[handle]->maSound, &sampleFrames) != MA_SUCCESS)
        goto error;

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

    // Setup type (TODO: do we really need to do this?)
    if (maFormat == ma_format::ma_format_f32)
        mb.type = 4; // FP32
    else
        mb.type = 1; // Integer
    if (maFormat == ma_format::ma_format_u8)
        mb.type |= 2; // Unsigned

    mb.elementsize = SAMPLE_FRAME_SIZE_BY_FORMAT(channels, maFormat); // Set the element size. This is the size of each PCM frame in bytes
    mb.offset = (ptrszint)pcmData;                                    // Setup offset
    mb.size = sampleFrames * mb.elementsize;                          // Setup size (in bytes)
    mb.sound = handle;                                                // Copy the handle
    mb.image = 0;                                                     // Not needed. Set to 0

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

    // Attempt to initialize with miniaudio defaults
    audioEngine.maResult = ma_engine_init(NULL, &audioEngine.maEngine);

    // If failed, then set the global flag so that we don't attempt to initialize again
    if (audioEngine.maResult != MA_SUCCESS) {
        audioEngine.initializationFailed = true;
        DEBUG_PRINT("miniaudio initialization failed");

        return;
    }

    // Set the initialized flag as true
    audioEngine.isInitialized = true;

    // Get and save the engine sample rate
    // We will let miniaudio choose the device sample rate for us. This ensures we get the lowest latency and resampling artifacts
    audioEngine.sampleRate = ma_engine_get_sample_rate(&audioEngine.maEngine);

    DEBUG_PRINT("Audio engine initialized at %uHz sample rate", audioEngine.sampleRate);

    // Reserve sound handle 0 so that nothing else can use it
    // We will use this handle internally for Play(), Beep(), Sound() etc.
    audioEngine.sndInternal = audioEngine.AllocateSoundHandle();
    DEBUG_CHECK(audioEngine.sndInternal == 0); // The first handle must return 0 and this is what is used by Beep and Sound

    // Just do a basic setup and mark the type as 'none'
    // If Play(), Sound(), Beep() are called, those will mark it as 'raw'
    audioEngine.soundHandles[audioEngine.sndInternal]->rawQueue =
        new SampleFrameBlockQueue(&audioEngine.maEngine, &audioEngine.soundHandles[audioEngine.sndInternal]->maSound);
    audioEngine.soundHandles[audioEngine.sndInternal]->type = SoundType::None;
}

/// <summary>
/// This shuts down the audio engine and frees any resources used.
/// </summary>
void snd_un_init() {
    if (audioEngine.isInitialized) {
        // Special handling for handle 0
        audioEngine.soundHandles[audioEngine.sndInternal]->type = SoundType::None;
        delete audioEngine.soundHandles[audioEngine.sndInternal]->rawQueue;
        audioEngine.soundHandles[audioEngine.sndInternal]->rawQueue = nullptr;

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

        // Set engine initialized flag as false
        audioEngine.isInitialized = false;

        DEBUG_PRINT("Audio engine shutdown");
    }
}

/// <summary>
/// This is called by the QBPE library code.
/// We use this for housekeeping and other stuff.
/// </summary>
void snd_mainloop() {
    /*
#ifdef AE_DEBUG
    static int frameCounter = 0;
    static double frameTime = 0;

    ++frameCounter;

    double currentTime = func_timer(0.001, true);
    if (currentTime - frameTime > 1) {
        DEBUG_PRINT("Sound loop FPS = %i", frameCounter);
        frameTime = currentTime;
        frameCounter = 0;
    }
#endif
    */

    if (audioEngine.isInitialized) {
        // Scan through the whole handle to find anything we need to update or close
        for (size_t handle = 0; handle < audioEngine.soundHandles.size(); handle++) {
            // Only process handles that are in use
            if (audioEngine.soundHandles[handle]->isUsed) {
                // Keep raw audio streams going
                if (audioEngine.soundHandles[handle]->type == SoundType::Raw)
                    audioEngine.soundHandles[handle]->rawQueue->Update();

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
                        if (!audioEngine.soundHandles[handle]->rawQueue->GetSampleFramesRemaining())
                            audioEngine.FreeSoundHandle(handle);

                        break;

                    case SoundType::None:
                        if (handle != 0)
                            DEBUG_PRINT("Sound type is 'None' when handle value is not 0");

                        break;

                    default:
                        DEBUG_PRINT("Condition not handled"); // It should not come here
                    }
                }
            }
        }
    }
}
//-----------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------
