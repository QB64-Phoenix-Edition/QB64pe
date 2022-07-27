//----------------------------------------------------------------------------------------------------
//    ___  ___ ___ ___     _          _ _       ___           _
//   / _ \| _ ) _ \ __|   /_\ _  _ __| (_)___  | __|_ _  __ _(_)_ _  ___
//  | (_) | _ \  _/ _|   / _ \ || / _` | / _ \ | _|| ' \/ _` | | ' \/ -_)
//   \__\_\___/_| |___| /_/ \_\_,_\__,_|_\___/ |___|_||_\__, |_|_||_\___|
//                                                      |___/
//
//	QBPE Audio engine powered by miniaudio (https://miniaud.io/)
//
//	Copyright (c) 2022 Samuel Gomes
//	https://github.com/a740g
//
//-----------------------------------------------------------------------------------------------------

//-----------------------------------------------------------------------------------------------------
// HEADER FILES
//-----------------------------------------------------------------------------------------------------
#include "audio.h"
#include <assert.h>
#include <vector>
// Enable Ogg Vorbis decoding
#define STB_VORBIS_HEADER_ONLY
#include "extras/stb_vorbis.c"
// The main miniaudio header
#include "miniaudio.h"
//-----------------------------------------------------------------------------------------------------

//-----------------------------------------------------------------------------------------------------
// FORWARD DECLARATIONS
//-----------------------------------------------------------------------------------------------------
// These are stuff that was not declared anywhere else
// Again, we will wait for Matt to cleanup the C/C++ source file and include header files that declare this stuff
qbs *qbs_new_txt_len(const char *txt, int32 len);
int32 func_instr(int32 start, qbs *str, qbs *substr, int32 passed);

#ifndef QB64_WINDOWS
void Sleep(uint32 milliseconds);
#endif
//-----------------------------------------------------------------------------------------------------

//-----------------------------------------------------------------------------------------------------
// CONSTANTS
//-----------------------------------------------------------------------------------------------------
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
// This basically checks if the handle is within sh vector limits and 'isUsed' is set to true
// We are relying on C's boolean short-circuit to not evaluate the last 'isUsed' if previous conditions are false
// Here we are checking > 0 because this is meant to check user handles only
#define IS_SOUND_HANDLE_VALID(_handle_) ((_handle_) > 0 && (_handle_) < audioEngine.soundHandles.size() && audioEngine.soundHandles[_handle_]->isUsed)
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
    ma_uint32 size;             // Size of the block in 'samples'. See below
    ma_uint32 offset;           // Where is the write cursor in the buffer?
    float *buffer;              // The actual sample frame block buffer

    SampleFrameBlockNode(const SampleFrameBlockNode &) = delete;      // No default copy constructor
    SampleFrameBlockNode &operator=(SampleFrameBlockNode &) = delete; // No assignment operator

    /// <summary>
    /// The constructor parameter is in sample frames.
    /// For a stereo sample frame we'll need (sample frames * 2) samples.
    /// Each sample is sizeof(float) bytes.
    /// </summary>
    /// <param name="sampleFrames">Number of sample frames needed</param>
    SampleFrameBlockNode(ma_uint32 sampleFrames) {
        next = nullptr;             // Set this to null. This will managed by the 'Queue' struct
        size = sampleFrames << 1;   // 2 channels (stereo)
        offset = 0;                 // Set the write cursor to 0
        buffer = new float[size](); // Allocate a zeroed float buffer of size floats. Ah, Creative Silence!
    }

    /// <summary>
    /// Free the sample frame block that was allocated.
    /// </summary>
    ~SampleFrameBlockNode() { delete[] buffer; }

    /// <summary>
    /// Pushes a sample frame in the block and increments the offset.
    /// miniaudio expects it's stereo PCM data interleaved (LRLR format).
    /// </summary>
    /// <param name="l">Left floating point sample</param>
    /// <param name="r">Right floating point sample</param>
    /// <returns>Return true if operation was succcessful. False if block is full</returns>
    bool PushSampleFrame(float l, float r) {
        if (buffer && offset < size) {
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
    bool IsBufferFull() { return buffer && offset >= size; }
};

/// <summary>
/// This is a light weight queue of type SampleFrameBlockNode and also the guts of SndRaw.
/// We could have used some std container too but I wanted something really lean, simple and transparent.
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
    ma_sound *maSound;                     // Pointer to a sound object that was passed in the constructor
    ma_engine *maEngine;                   // Pointer to a ma engine object
    ma_audio_buffer maBuffer;              // miniaudio buffer object
    ma_audio_buffer_config maBufferConfig; // miniaudio buffer configuration
    ma_result maResult;                    // This is the result of the last miniaudio operation (used for trapping errors)

    SampleFrameBlockQueue(const SampleFrameBlockQueue &) = delete;      // No default copy constructor
    SampleFrameBlockQueue &operator=(SampleFrameBlockQueue &) = delete; // No assignment operator

    /// <summary>
    /// This initialized the queue and calculates the sample frames per block
    /// </summary>
    /// <param name="pmaEngine">A pointer to a miniaudio engine object</param>
    /// <param name="pmaSound">A pointer to a miniaudio sound object</param>
    SampleFrameBlockQueue(ma_engine *pmaEngine, ma_sound *pmaSound) {
        first = last = nullptr;
        blockCount = frameCount = 0;
        maSound = pmaSound;                               // Save the pointer to the ma_sound object (this is basically from a QBPE sound handle)
        maEngine = pmaEngine;                             // Save the pointer to the ma_engine object (this should come from the QBPE sound engine)
        sampleRate = ma_engine_get_sample_rate(maEngine); // Save the sample rate
        // We'll have to see if this is a good number or if we can decrease it
        // Note the node will allocates twice this to account for 2 channels
        // This is a key member. Setting this to large value will cause latency issues
        // Setting this to a tiny value may cause clicks, pops or gaps in the stream
        blockSampleFrames = sampleRate >> 4;
        bufferSampleFrames = blockSampleFrames * 2;   // We want the playback buffer twice the size of a block to do a proper ping-pong
        buffer = new float[bufferSampleFrames * 2](); // Allocate a zeroed float buffer of bufferSizeSampleFrames * 2 floats (2 here is for 2 channels - stereo)
        bufferUpdatePosition = 0;                     // This will be set to the correct position by checking updateFlag
        updateFlag = false;                           // Set this to false because we want the initial check to fail

        if (buffer) {
            // Setup the ma buffer
            maBufferConfig = ma_audio_buffer_config_init(ma_format_f32, 2, bufferSampleFrames, buffer, NULL);
            maResult = ma_audio_buffer_init(&maBufferConfig, &maBuffer);
            assert(maResult == MA_SUCCESS);

            // Create a ma_sound from the ma_buffer
            maResult = ma_sound_init_from_data_source(maEngine, &maBuffer, 0, NULL, maSound);
            assert(maResult == MA_SUCCESS);
        }
    }

    /// <summary>
    /// This simply pops all sample blocks
    /// </summary>
    ~SampleFrameBlockQueue() {
        while (PopSampleFrameBlock())
            ;

        if (buffer) {
            // Stop playback
            maResult = ma_sound_stop(maSound);
            assert(maResult == MA_SUCCESS);

            // Delete the ma_sound object
            ma_sound_uninit(maSound);

            // Delete the ma_buffer object
            ma_audio_buffer_uninit(&maBuffer);
        }

        delete[] buffer;
    }

    /// <summary>
    /// This pushes a sample frame into the queue.
    /// If there are no sample frame blocks then it creates one.
    /// If the last sample frame block is full it creates a new one and links it to the chain.
    /// Note that in QBPE all samples frames are assumed to be stereo.
    /// Mono sample frames are simply simulated by playing the same data from left and right.
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

        // Kickstart buffer playback and looping if the first block was created
        if (1 == blockCount) {
            // Play the ma sound
            maResult = ma_sound_start(maSound);
            assert(maResult == MA_SUCCESS);

            // Set the buffer to loop
            ma_sound_set_looping(maSound, MA_TRUE);
        }

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

            --blockCount;                    // Decrement the frame count now so that we know what to do with 'last'
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
    size_t GetSampleFramesRemaining() {
        size_t bufferSampleFramesRemaining = 0;

        if (ma_sound_is_playing(maSound)) {
            // Figure out which pcm frame of the buffer is miniaudio playing
            ma_uint64 readCursor;
            maResult = ma_sound_get_cursor_in_pcm_frames(maSound, &readCursor);
            assert(maResult == MA_SUCCESS);

            // Figure out how many frames we have remaining in the buffer
            bufferSampleFramesRemaining = readCursor < blockSampleFrames ? blockSampleFrames - readCursor : bufferSampleFrames - readCursor;
        }

        // Add this to the frames in the queue
        return bufferSampleFramesRemaining + frameCount;
    }

    /// <summary>
    /// Returns the length, in seconds of sound queued.
    /// </summary>
    /// <returns>The length left to play in seconds</returns>
    double GetTimeRemaining() { return (double)GetSampleFramesRemaining() / sampleRate; }

    /// <summary>
    /// Check if everything is ready to go
    /// </summary>
    /// <returns>Returns true if everything is a go</returns>
    bool IsSetupValid() { return buffer && maEngine && maSound && maResult == MA_SUCCESS; }

    /// <summary>
    /// This keeps the ping-pong buffer fed and the sound stream going
    /// </summary>
    /// <param name="purge">If this flag is set then we will push any incomplete blocks to the sound buffer</param>
    void Update(bool purge) {
        // Only bother if the buffer is actually playing
        if (ma_sound_is_playing(maSound)) {
            // Figure out which pcm frame of the buffer is miniaudio playing
            ma_uint64 readCursor;
            maResult = ma_sound_get_cursor_in_pcm_frames(maSound, &readCursor);
            assert(maResult == MA_SUCCESS);

            bool checkCondition = readCursor < blockSampleFrames; // Since buffer sample frame size = blockSampleFrames * 2

            // Only proceed to update if our flag is not the same as our condition
            if (checkCondition != updateFlag) {
                // The line below does two sneaky things that deserve explanation
                //	1. We are using bufferSampleFrames which is set to exactly halfway through the buffer since we are using stereo (see constructor)
                //	2. The boolean condition above will always be 1 if the read cursor is in the lower-half and hence push the position to the top-half
                bufferUpdatePosition = checkCondition * bufferSampleFrames; // This line basically toggles the buffer copy position

                if (blockCount) // Check if we have any blocks in the queue
                {
                    if (first->IsBufferFull() || (purge && 1 == blockCount)) // We will stream the block only if it is full or the purge flag is set
                    {
                        // Simply copy the first block in the queue
                        if (first->buffer)
                            std::copy(first->buffer, first->buffer + first->size, buffer + bufferUpdatePosition);

                        // And then pop it off
                        PopSampleFrameBlock();
                    }
                } else {
                    // Stop looping the buffer if this was the last block and read cursor is in the lower half
                    if (checkCondition)
                        ma_sound_set_looping(maSound, MA_FALSE);
                }

                updateFlag = checkCondition; // Save our check condition to our flag
            }
        }
    }
};

/// <summary>
/// Sound handle type
/// This describes every sound the system will ever play (including raw streams).
/// </summary>
struct SoundHandle {
    bool isUsed;                     // Is this handle in active use
    SoundType type;                  // Type of sound (see SoundType enum)
    bool autoKill;                   // Do we need to auto-clean this sample / stream after playback is done?
    ma_sound maSound;                // miniaudio sound
    ma_uint32 maFlags;               // miniaudio flags that were used when initializing the sound
    SampleFrameBlockQueue *rawQueue; // Raw sample frame queue
};

/// <summary>
///	Type will help us keep track of the audio engine state
/// </summary>
struct AudioEngine {
    bool isInitialized;                      // This is set to true if we were able to initialize miniaudio and allocated all required resources
    bool initializationFailed;               // This is set to true if a past initialization attempt failed
    ma_engine maEngine;                      // This is the primary miniaudio "engine" context. Everything happens using this!
    ma_result maResult;                      // This is the result of the last miniaudio operation (used for trapping errors)
    ma_uint32 sampleRate;                    // Sample rate used by the miniaudio engine
    int32 sndInternal;                       // Internal sound handle that we will use for Beep() & Sound()
    int32 sndInternalRaw;                    // Internal sound handle that we will use for 'handle-less' raw streames
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
    /// This allocates a sound handle.
    /// It will return -1 on error.
    /// Handle 0 is used internally for Beep, Sound and Play and thus cannot be used by the user.
    /// Basically, we go through the vector and find an object pointer were 'isUsed' is set as false and return the index.
    /// If such an object pointer is not found, then we add a pointer to a new object at the end of the vector and return the index.
    /// We are using pointers because miniaudio keeps using stuff from ma_sound and these cannot move in memory when the vector is resized.
    /// The handle is put-up for recycling simply by setting the 'isUsed' member to false.
    /// Note that this means the vector will keep growing until the largest handle (index) and never shrink.
    /// The choice of using a vector was simple - performance. Vector performance when using 'indexes' is next to no other.
    /// It's just like using an array. Only that the array is dynamic. The vector will be pruned only when snd_un_init gets called.
    /// We will however, be good citizens and will also 'delete' the objects when snd_un_init gets called.
    /// All this means that a sloppy programmer may be able to grow the vector and eventually the system may run out of memory and crash.
    /// But that's ok. Sloppy programmers (like me) must be punished until they learn! XD
    /// BTW, int32 was a bad choice for handles and obviously 64-bit size_t is way larger. Oh well...
    /// </summary>
    /// <returns>Returns a non-negative handle if successful</returns>
    int32 AllocateSoundHandle() {
        if (!isInitialized)
            return INVALID_SOUND_HANDLE;

        size_t h, vectorSize = soundHandles.size(); // Save the vector size

        // Scan through the vector and return a slot that is not being used
        // This loop should not execute if size is 0
        for (h = 0; h < vectorSize; h++) {
            if (!soundHandles[h]->isUsed)
                break;
        }

        if (h >= vectorSize) {
            // If we have reached here then either the vector is empty or there are no empty slots
            // Simply create a new SoundHandle at the back of the vector
            soundHandles.push_back(new SoundHandle);
            size_t newVectorSize = soundHandles.size();

            // If newVectorSize == vectorSize then push_back() failed
            if (newVectorSize <= vectorSize)
                return INVALID_SOUND_HANDLE;

            h = newVectorSize - 1; // The handle is simply newsize - 1
        }

        // Check if the new operator above was successfull
        if (!soundHandles[h]) {
            soundHandles.pop_back();

            return INVALID_SOUND_HANDLE;
        }

        // Initializes a sound handle that was just allocated.
        // This will set it to 'in use' after applying some defaults.
        soundHandles[h]->type = SoundType::None;
        soundHandles[h]->autoKill = false;
        soundHandles[h]->rawQueue = nullptr;
        soundHandles[h]->maSound = {};
        // We do not use pitch shifting, so this will give a little performance boost
        soundHandles[h]->maFlags = MA_SOUND_FLAG_NO_PITCH | MA_SOUND_FLAG_NO_SPATIALIZATION | MA_SOUND_FLAG_WAIT_INIT;
        soundHandles[h]->isUsed = true;

        return (int32)(h); // Return newVectorSize - 1 as the handle
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
                    assert(false && "Sound type is 'None' when handle value is not 0");

                break;

            default:
                assert(false && "Condition not handled"); // It should not come here
            }

            // Now simply set the 'isUsed' member to false
            soundHandles[handle]->isUsed = false;
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
/// This generates a sound at the specified frequency for the specified amount of time.
/// </summary>
/// <param name="frequency">Sound frequency</param>
/// <param name="lengthInClockTicks">Duration in clock ticks. There are 18.2 clock ticks per second</param>
void sub_sound(double frequency, double lengthInClockTicks) {
    // We'll allocate a buffer just once
    // This buffer will be resized as required and will be freed by the system when the program ends
    // This is much faster then allocating and freening multiple times if we have a good crt
    static float *waveformBuffer = nullptr;

    // Exit if frequency or length is 0 or if the audio engine is not initialized
    if (frequency <= 0 || lengthInClockTicks <= 0 || !audioEngine.isInitialized || audioEngine.sndInternal != 0 ||
        audioEngine.soundHandles[audioEngine.sndInternal]->type != SoundType::None)
        return;

    // Calculate the sample frames (duration) of the sound
    ma_uint64 sampleFrames = (ma_uint64)((double)audioEngine.sampleRate * lengthInClockTicks / 18.2);

    // Allocate the 'sample frame' number of bytes for the waveform buffer
    float *tmp = (float *)realloc(waveformBuffer, sampleFrames * sizeof(float));

    // Exit if memory allocation failed
    if (!tmp)
        return;

    // Save the buffer pointer now that it is successfully allocated
    waveformBuffer = tmp;

    // Create a sine wave
    ma_waveform_config maWaveConfig = ma_waveform_config_init(ma_format_f32, 1, audioEngine.sampleRate, ma_waveform_type_sine, 1, frequency);
    ma_waveform maSineWave;
    audioEngine.maResult = ma_waveform_init(&maWaveConfig, &maSineWave);
    assert(audioEngine.maResult == MA_SUCCESS);
    audioEngine.maResult = ma_waveform_read_pcm_frames(&maSineWave, waveformBuffer, sampleFrames, NULL); // Generate the waveform
    assert(audioEngine.maResult == MA_SUCCESS);

    // Setup the ma buffer
    ma_audio_buffer_config maBufferConfig = ma_audio_buffer_config_init(ma_format_f32, 1, sampleFrames, waveformBuffer, NULL);
    ma_audio_buffer maBuffer;
    audioEngine.maResult = ma_audio_buffer_init(&maBufferConfig, &maBuffer);
    assert(audioEngine.maResult == MA_SUCCESS);

    // Create a ma sound from the ma buffer
    audioEngine.maResult =
        ma_sound_init_from_data_source(&audioEngine.maEngine, &maBuffer, 0, NULL, &audioEngine.soundHandles[audioEngine.sndInternal]->maSound);
    assert(audioEngine.maResult == MA_SUCCESS);

    // Play the ma sound
    audioEngine.maResult = ma_sound_start(&audioEngine.soundHandles[audioEngine.sndInternal]->maSound);
    assert(audioEngine.maResult == MA_SUCCESS);

    // Wait for the sound to end
    // This also blocks the caller and correctly implements original QuickBASIC behavior
    while (ma_sound_is_playing(&audioEngine.soundHandles[audioEngine.sndInternal]->maSound))
        Sleep(0);

    ma_sound_uninit(&audioEngine.soundHandles[audioEngine.sndInternal]->maSound);
    ma_audio_buffer_uninit(&maBuffer);
    ma_waveform_uninit(&maSineWave);
}

/// <summary>
/// This generates a default 'beep' sound.
/// </summary>
void sub_beep() { sub_sound(900, 5); }

/// <summary>
/// Processes and plays the MML specified in the string.
/// Returns immediately after it is completed submitting all samples for playback.
/// </summary>
/// <param name="str">The string to play</param>
void sub_play(qbs *str) {
    // TODO: We'll probably implement this uing sndraw like the old OpenAL code
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
    if (handle <= 0) // We are not expected to open files with handle 0
        return INVALID_SOUND_HANDLE;

    // Set some handle properties
    audioEngine.soundHandles[handle]->type = SoundType::Static;

    // Set the flags to specifiy how we want the audio file to be opened
    // TODO: case insensitive compare
    if (passed && requirements->len) {
        qbs_set(reqs, qbs_ucase(requirements)); // Convert tmp str to perm str
        if (func_instr(1, reqs, qbs_new_txt(REQUIREMENT_STRING_STREAM), 1))
            audioEngine.soundHandles[handle]->maFlags |= MA_SOUND_FLAG_STREAM; // Check if the user wants to stream the file
    } else {
        audioEngine.soundHandles[handle]->maFlags |= MA_SOUND_FLAG_DECODE; // Else decode and load the whole sound in memory
    }

    // Forward the request to miniaudio to open the sound file
    // TODO: Check if we need to emulate QB64 path behavior or if this will just work
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
    if (dst_handle <= 0) // We are not expected to open files with handle 0
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
            assert(audioEngine.maResult == MA_SUCCESS);
        }

        // Kickstart playback
        audioEngine.maResult = ma_sound_start(&audioEngine.soundHandles[handle]->maSound);
        assert(audioEngine.maResult == MA_SUCCESS);

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
        assert(audioEngine.maResult == MA_SUCCESS);
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
/// </summary>
/// <param name="handle">A sound handle</param>
/// <param name="volume">A float point value with 0 resulting in silence and anything above 1 resulting in amplification</param>
void sub__sndvol(int32 handle, float volume) {
    // TODO: We should enable this for raw pcm sounds too
    if (audioEngine.isInitialized && IS_SOUND_HANDLE_VALID(handle) && audioEngine.soundHandles[handle]->type == SoundType::Static) {
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
            assert(audioEngine.maResult == MA_SUCCESS);
        }

        // Kickstart playback
        audioEngine.maResult = ma_sound_start(&audioEngine.soundHandles[handle]->maSound);
        assert(audioEngine.maResult == MA_SUCCESS);

        // Start looping the sound if it is not
        if (!ma_sound_is_looping(&audioEngine.soundHandles[handle]->maSound)) {
            ma_sound_set_looping(&audioEngine.soundHandles[handle]->maSound, MA_TRUE);
        }
    }
}

/// <summary>
/// This will attempt to set the balance or 3D position of a sound.
/// Note that unlike the OpenAL code, we will do pure stereo panning if y & z are absent.
/// </summary>
/// <param name="handle">A sound handle</param>
/// <param name="x">x distance values go from left (negative) to right (positive)</param>
/// <param name="y">y distance values go from below (negative) to above (positive).</param>
/// <param name="z">z distance values go from behind (negative) to in front (positive).</param>
/// <param name="channel">channel value 1 denotes left (mono) and 2 denotes right (stereo) channel. This has no meaning for miniaudio and is ignored</param>
/// <param name="passed">How many parameters were passed?</param>
void sub__sndbal(int32 handle, double x, double y, double z, int32 channel, int32 passed) {
    // TODO: We should enable this for raw pcm sounds too
    if (audioEngine.isInitialized && IS_SOUND_HANDLE_VALID(handle) && audioEngine.soundHandles[handle]->type == SoundType::Static) {
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
        assert(audioEngine.maResult == MA_SUCCESS);

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
        assert(audioEngine.maResult == MA_SUCCESS);

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
            assert(audioEngine.maResult == MA_SUCCESS);
            return;
        }

        ma_uint64 lengthSampleFrames;
        audioEngine.maResult =
            ma_sound_get_length_in_pcm_frames(&audioEngine.soundHandles[handle]->maSound, &lengthSampleFrames); // Get the total sample frames
        if (audioEngine.maResult != MA_SUCCESS)
            return;

        audioEngine.maResult = ma_sound_seek_to_pcm_frame(&audioEngine.soundHandles[handle]->maSound,
                                                          lengthSampleFrames * (seconds / lengthSeconds)); // Set the postion in PCM frames
        assert(audioEngine.maResult == MA_SUCCESS);
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
        assert(audioEngine.maResult == MA_SUCCESS);

        // Also reset the playback cursor to zero
        audioEngine.maResult = ma_sound_seek_to_pcm_frame(&audioEngine.soundHandles[handle]->maSound, 0);
        assert(audioEngine.maResult == MA_SUCCESS);
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
    if (handle <= 0) // We are not expected to open files with handle 0
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
        if (audioEngine.sndInternalRaw <= 0) {
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
        while (audioEngine.soundHandles[handle]->rawQueue->GetSampleFramesRemaining()) {
            audioEngine.soundHandles[handle]->rawQueue->Update(true);
            Sleep(0);
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
/// </summary>
/// <param name="handle">A sound handle</param>
/// <param name="targetChannel">The channel data is required. This is ignored with miniaudio as it uses interleaved samples</param>
/// <returns>A _MEM value that can be used to access the sound data</returns>
mem_block func__memsound(int32 handle, int32 targetChannel) {
    // TODO: Out of all the functions, this is probably going to be the most difficult or kludgy or both
    //  Since we want miniaudio to manage all audio resouces, audio data is stored how miniaudio wants in memory
    //  In miniaudio's case, stereo audio data is store as interleaved in memory. Apparently, this is true for many audio libs out there
    //  This is completely unlike what the old OpenAL code was doing
    //  One way to implement this is to have two temp buffers - one for left and one for right and let the user use those
    //  When the user is done using and closes the mem object, then the interleave data should be created from the buffers for miniaudio (yuck!)
    //  The second way is to simply expose the miniaudio's interleaved buffer to the user. This will break compatibility for stereo sound.
    //  But then, how many people really use this stuff?
}

/// <summary>
/// This is a new addition to the QBPE Audio API.
/// It returns a _MEM value referring to a newly created sound's raw data in memory with the given specification.
/// The user can then fill the buffer with whatever they want (using _MEMSOUND) and play it.
/// This obviously needs to be greenlit by the QBPE maintainers.
/// </summary>
/// <param name="frames">The number of sample frames required</param>
/// <param name="rate">The sample rate of the sound</param>
/// <param name="channels">The number of sound channels</param>
/// <param name="bits">The bit depth of the sound</param>
/// <param name="passed">How many parameters were passed?</param>
/// <returns>A new sound handle if successful or 0 on failure</returns>
int32 func__newsound(int32 frames, int32 rate, int32 channels, int32 bits, int32 passed) { return 0; }

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
        return;
    }

    // Set the initialized flag as true
    audioEngine.isInitialized = true;

    // Get and save the engine sample rate
    audioEngine.sampleRate = ma_engine_get_sample_rate(&audioEngine.maEngine);

    // Allocate a sound handle
    // Note that this should always be 0
    // We will use this handle internally for Beep(), Sound() etc.
    audioEngine.sndInternal = audioEngine.AllocateSoundHandle();
    assert(audioEngine.sndInternal == 0); // The first handle must return 0 and this is what is used by Beep and Sound
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

        // Set engine initialized flag as false
        audioEngine.isInitialized = false;
    }
}

/// <summary>
/// This is called by the QBPE library code.
/// We use this for housekeeping and other stuff.
/// </summary>
void snd_mainloop() {
    if (audioEngine.isInitialized) {
        // Scan through the whole handle to find anything we need to update or close
        for (size_t handle = 0; handle < audioEngine.soundHandles.size(); handle++) {
            // Only process handles that are in use
            if (audioEngine.soundHandles[handle]->isUsed) {
                // Keep raw audio streams going
                if (audioEngine.soundHandles[handle]->type == SoundType::Raw)
                    audioEngine.soundHandles[handle]->rawQueue->Update(true); // TODO: We should not be setting this to true

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
                            assert(false && "Sound type is 'None' when handle value is not 0");

                        break;

                    default:
                        assert(false && "Condition not handled"); // It should not come here
                    }
                }
            }
        }
    }
}
//-----------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------
