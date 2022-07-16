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

// HACK for: internal\c/os.h: declaration does not declare anything [-fpermissive]
#undef int64
#undef int32
#undef uint32
#undef int16
#undef uint16
#undef int8
#undef uint8

//-----------------------------------------------------------------------------------------------------
// HEADER FILES
//-----------------------------------------------------------------------------------------------------
// Enables Vorbis decoding
#define STB_VORBIS_HEADER_ONLY
#include "extras/stb_vorbis.c"
// The main miniaudio header
#define MA_API static
#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"
// The stb_vorbis implementation must come after the implementation of miniaudio
#undef STB_VORBIS_HEADER_ONLY
#include "extras/stb_vorbis.c"
//-----------------------------------------------------------------------------------------------------

//-----------------------------------------------------------------------------------------------------
// CONSTANTS
//-----------------------------------------------------------------------------------------------------
#define QB_FALSE MA_FALSE
#define QB_TRUE -MA_TRUE
//-----------------------------------------------------------------------------------------------------

//-----------------------------------------------------------------------------------------------------
// MACROS
//-----------------------------------------------------------------------------------------------------
// This basically checks if the handle is within the list limits
#define IS_SOUND_HANDLE_VALID(_handle_) ((_handle_) > 0 && (_handle_) <= audioEngine.soundHandles->indexes)
#define CLEAR_VARIABLE(_v_) MA_ZERO_MEMORY(&(_v_), sizeof(_v_))
//-----------------------------------------------------------------------------------------------------

//-----------------------------------------------------------------------------------------------------
// STRUCTURE TYPES
//-----------------------------------------------------------------------------------------------------
/// <summary>
/// This is a node in a queue that can hold a single floating point sample frame.
/// </summary>
typedef struct SampleFrameNode {
    struct SampleFrameNode *next;
    float leftSample;
    float rightSample;
} SampleFrameNode;

/// <summary>
/// This is a queue of type FloatNode.
/// </summary>
typedef struct {
    SampleFrameNode *first;
    SampleFrameNode *last;
    size_t sampleFrames;
} SampleFrameQueue;

/// <summary>
/// Type of sound
/// </summary>
typedef enum {
    SOUND_NONE,   // No sound
    SOUND_STATIC, // Static sounds that are completely managed by miniaudio
    SOUND_RAW     // Raw sound stream that is managed outside miniaudio by the audio engine
} SoundType;

/// <summary>
/// Sound handle type
/// This describes every sound the system will ever play (including raw streams).
/// </summary>
typedef struct {
    SoundType type;                    // Type of sound (see SoundType enum)
    ma_bool8 autoKill;                 // Do we need to auto-clean this sample / stream after playback is done?
    ma_bool8 isInternal;               // Is this a sound handle that is used internally by the audio engine
    ma_sound maSound;                  // miniaudio sound type
    ma_uint32 maFlags;                 // miniaudio flags that were used when initializing the sound
    SampleFrameQueue sampleFrameQueue; // Raw sample frame queue
} SoundHandle;

/// <summary>
///	Type will help us keep track of the audio engine state
/// </summary>
typedef struct {
    ma_bool8 isInitialized;        // This is set to true if we were able to initialize miniaudio and allocated all required resources
    ma_bool8 initializationFailed; // This is set to true if a past initialization attempt failed
    ma_engine maEngine;            // This is the primary miniaudio "engine" context. Everything happens using this!
    ma_result maResult;            // This is the result of the last miniaudio operation (used for trapping errors)
    int32 sndInternal;             // Internal sound handle that we will use for Beep() & Sound()
    list *soundHandles;            // This is the audio handle list used by the engine and by everything else
} AudioEngine;
//-----------------------------------------------------------------------------------------------------

//-----------------------------------------------------------------------------------------------------
// GLOBAL VARIABLES
//-----------------------------------------------------------------------------------------------------
// This is the global audio engine state
static AudioEngine audioEngine = {0}; // Zero the whole structure
//-----------------------------------------------------------------------------------------------------

//-----------------------------------------------------------------------------------------------------
// FUNCTIONS
//-----------------------------------------------------------------------------------------------------
/// <summary>
/// This pushes a sample frame at the back of the queue.
/// Note that in QBPE all samples frames are assumed to be stereo.
/// Mono sample frames are simply simulated by playing the same data from left and right.
/// For this to work correctly the queue (q) must zeroed out by the user before use!
/// </summary>
/// <param name="q">A pointer to the queue</param>
/// <param name="l">The left sample</param>
/// <param name="r">The right sample</param>
/// <returns>Returns true if operation was successful</returns>
static ma_bool8 PushSampleFrameBack(SampleFrameQueue *q, float l, float r) {
    // Attempt to allocate a node
    SampleFrameNode *node = (SampleFrameNode *)malloc(sizeof(SampleFrameNode));

    // Return false if memory allocation failed
    if (!node)
        return MA_FALSE;

    // Save the data
    node->leftSample = l;
    node->rightSample = r;
    node->next = NULL; // Set the next node for node to NULL

    if (q->sampleFrames)
        q->last->next = node; // Add the node to the last node if we have samples frames in the queue
    else
        q->first = node; // Else this is the first node

    q->last = node;    // The last item in the queue is node
    q->sampleFrames++; // Increase the frame count

    return MA_TRUE;
}

/// <summary>
/// This pops a sample frame at the front of the queue.
/// The sample frame can be accessed before popping with q->first.leftSample and q->first.rightSample.
/// </summary>
/// <param name="q">A pointer to the queue</param>
/// <param name="l">Pointer to left sample</param>
/// <param name="r">Pointer to right sample</param>
/// <returns>Returns true if we were able to pop. False means the queue is empty</returns>
static ma_bool8 PopSampleFrameFront(SampleFrameQueue *q, float *l, float *r) {
    // Only if the queue has some sample frames then...
    if (q->sampleFrames) {
        SampleFrameNode *node = q->first; // Set node to the first frame in the queue

        q->sampleFrames--;     // Decrement the frame count now so that we know what to do with 'last'
        q->first = node->next; // Detach the node. If this is the last node then 'first' will be NULL cause node->next is NULL

        if (!q->sampleFrames)
            q->last = NULL; // This means that node was the last node

        // Retrieve the left and right samples
        *l = node->leftSample;
        *r = node->rightSample;

        // Free the node
        free(node);

        return MA_TRUE;
    }

    return MA_FALSE;
}

/// <summary>
/// Initializes a sound handle that was just allocated.
/// The handle must not be in use.
/// This will set it to "in use" after applying some defaults.
/// </summary>
/// <param name="handle">A sound handle</param>
static void InitializeSoundHandle(SoundHandle *snd) {
    snd->type = SOUND_NONE;
    snd->autoKill = MA_FALSE;
    snd->isInternal = MA_FALSE;
    CLEAR_VARIABLE(snd->sampleFrameQueue);
    CLEAR_VARIABLE(snd->maSound);
    snd->maFlags = MA_SOUND_FLAG_NO_PITCH | MA_SOUND_FLAG_WAIT_INIT; // We do not use pitch shifting, so this will give a little performance boost
}

/// <summary>
/// This generates a default 'beep' sound.
/// Sends the sound samples for playback and immediately returns.
/// This is in-line with the behavior of 'Sound' and 'Play'.
/// </summary>
void sub_beep() {}

/// <summary>
/// This generates a sound at the specified frequency for the specified amount of time.
/// Returns immediately after sending the data for playback.
/// </summary>
/// <param name="frequency">Sound frequency</param>
/// <param name="lengthInClockTicks">Duration in clock ticks. There are 18.2 clock ticks per second</param>
void sub_sound(double frequency, double lengthInClockTicks) {}

/// <summary>
/// Processes and plays the MML specified in the string.
/// Returns immediately after it is completed submitting all samples for playback.
/// </summary>
/// <param name="str">The string to play</param>
void sub_play(qbs *str) {}

/// <summary>
/// This returns the sample rate from ma engine if ma is initialized.
/// </summary>
/// <returns>miniaudio sample rtate</returns>
int32 func__sndrate() { return audioEngine.isInitialized ? ma_engine_get_sample_rate(&audioEngine.maEngine) : 0; }

/// <summary>
/// This loads a sound file into memory and returns a LONG handle value above 0.
/// </summary>
/// <param name="fileName">The is the pathname for the sound file. This can be any format the miniaudio support or a format supported by a miniaudio
/// plugin</param> <param name="requirements">The is leftover from the old QB64-SDL days. But we use this to pass some parameters like 'stream'</param> <param
/// name="passed">How many parameters were passed?</param> <returns>Returns a valid sound handle (> 0) if successful or 0 if it fails</returns>
int32 func__sndopen(qbs *fileName, qbs *requirements, int32 passed) {
    if (!audioEngine.isInitialized)
        return 0;

    // Some QB strings that we'll need
    static qbs *fileNameZ = NULL;
    if (!fileNameZ)
        fileNameZ = qbs_new(0, 0);
    static qbs *req = NULL;
    if (!req)
        req = qbs_new(0, 0);

    qbs_set(fileNameZ, qbs_add(fileName, qbs_new_txt_len("\0", 1))); // s1 = filename + CHR$(0)
    if (fileNameZ->len == 1)
        return 0; // Return 0 if file name is null length string

    // Alocate a sound handle
    int32 handle = list_add(audioEngine.soundHandles);
    SoundHandle *snd = (SoundHandle *)list_get(audioEngine.soundHandles, handle);
    // Return failure if we were not able to create the handle
    if (!snd) {
        list_remove(audioEngine.soundHandles, handle);
        return 0; // Return 0 if we were not able to allocate a handle
    }

    // Initialize the sound handle data
    InitializeSoundHandle(snd);

    // Set some handle properties
    snd->type = SOUND_STATIC;

    // Set the flags to specifiy how we want the audio file to be opened
    // TODO: case insensitive compare
    if (passed && requirements->len) {
        qbs_set(req, qbs_ucase(requirements)); // Convert tmp str to perm str
        if (func_instr(1, req, qbs_new_txt("STREAM"), 1))
            snd->maFlags |= MA_SOUND_FLAG_STREAM; // Check if the user wants to stream the file
    } else {
        snd->maFlags |= MA_SOUND_FLAG_DECODE; // Else decode and load the whole sound in memory
    }

    // Forward the request to miniaudio to open the sound file
    // TODO: Emulate QB64 path behavior?
    audioEngine.maResult = ma_sound_init_from_file(&audioEngine.maEngine, (const char *)fileNameZ->chr, snd->maFlags, NULL, NULL, &snd->maSound);

    // If the sound failed to load free the handle and return 0
    if (audioEngine.maResult != MA_SUCCESS) {
        list_remove(audioEngine.soundHandles, handle);

        return 0;
    }

    return handle;
}

/// <summary>
/// The frees and unloads an open sound.
/// If the sound is playing or looping, it will be stopped.
/// If the sound is a stream of raw samples then any remaining samples pending for playback will be sent to miniaudio and then the handle will be freed.
/// </summary>
/// <param name="handle">A sound handle</param>
void sub__sndclose(int32 handle) {
    if (audioEngine.isInitialized && IS_SOUND_HANDLE_VALID(handle)) {
        SoundHandle *snd = (SoundHandle *)list_get(audioEngine.soundHandles, handle);
        if (!snd)
            return;

        // Sound type specific cleanup
        switch (snd->type) {
        case SOUND_STATIC:
            ma_sound_uninit(&snd->maSound);
            break;

        case SOUND_RAW:
            // TODO:
            break;

        default:
            assert(MA_TRUE); // It should not come here
        }

        // Free the handle
        list_remove(audioEngine.soundHandles, handle);
    }
}

/// <summary>
/// This copies a sound to a new handle so that two or more of the same sound can be played at once.
/// </summary>
/// <param name="src_handle">A source sound handle</param>
/// <returns>A new sound handle if successful or 0 on failure</returns>
int32 func__sndcopy(int32 src_handle) {
    // Check for all invalid cases
    if (!audioEngine.isInitialized || !IS_SOUND_HANDLE_VALID(src_handle))
        return 0;

    SoundHandle *src_snd = (SoundHandle *)list_get(audioEngine.soundHandles, src_handle);
    if (!src_snd || src_snd->type != SOUND_STATIC)
        return 0;

    // Alocate a sound handle
    int32 dst_handle = list_add(audioEngine.soundHandles);
    SoundHandle *dst_snd = (SoundHandle *)list_get(audioEngine.soundHandles, dst_handle);
    if (!dst_snd) {
        list_remove(audioEngine.soundHandles, dst_handle);
        return 0; // Return 0 if we were not able to allocate a handle
    }

    // Initialize the sound handle data
    InitializeSoundHandle(dst_snd);

    dst_snd->type = SOUND_STATIC;        // Set some handle properties
    dst_snd->maFlags = src_snd->maFlags; // Copy the flags

    // Initialize a new copy of the sound
    audioEngine.maResult = ma_sound_init_copy(&audioEngine.maEngine, &src_snd->maSound, dst_snd->maFlags, NULL, &dst_snd->maSound);

    // If the sound failed to copy, then free the handle and return 0
    if (audioEngine.maResult != MA_SUCCESS) {
        list_remove(audioEngine.soundHandles, dst_handle);

        return 0;
    }

    return dst_handle;
}

/// <summary>
/// This plays a sound designated by a sound handle
/// </summary>
/// <param name="handle">A sound handle</param>
void sub__sndplay(int32 handle) {
    if (audioEngine.isInitialized && IS_SOUND_HANDLE_VALID(handle)) {
        SoundHandle *snd = (SoundHandle *)list_get(audioEngine.soundHandles, handle);
        if (!snd || snd->type != SOUND_STATIC)
            return;

        // Reset position to zero only if we are not looping or we've reached the end of the sound
        if (!ma_sound_is_looping(&snd->maSound) || ma_sound_at_end(&snd->maSound)) {
            audioEngine.maResult = ma_sound_seek_to_pcm_frame(&snd->maSound, 0);
            assert(audioEngine.maResult == MA_SUCCESS);
        }

        // Kickstart playback
        audioEngine.maResult = ma_sound_start(&snd->maSound);
        assert(audioEngine.maResult == MA_SUCCESS);

        // Stop looping the sound if it is
        if (ma_sound_is_looping(&snd->maSound)) {
            ma_sound_set_looping(&snd->maSound, MA_FALSE);
        }
    }
}

/// <summary>
///
/// </summary>
/// <param name="handle"></param>
/// <param name="volume"></param>
/// <param name="passed"></param>
void sub__sndplaycopy(int32 handle, double volume, int32 passed) {}

/// <summary>
/// This is a "fire and forget" style of function.
/// The engine will manage the ma_sound object internally.
/// When the sound finishes playing, it'll be put up for recycling.
/// Playback starts asynchronously.
/// </summary>
/// <param name="fileName">The is the name of the file to be played</param>
/// <param name="sync">This paramater is ignored</param>
/// <param name="volume">This the sound playback volume (0 - silent ... 1 - full)</param>
/// <param name="passed">How many parameters were passed?</param>
void sub__sndplayfile(qbs *fileName, int32 sync, double volume, int32 passed) {
    // Some QB strings that we'll need
    static qbs *fileNameZ = NULL;
    if (!fileNameZ)
        fileNameZ = qbs_new(0, 0);

    qbs_set(fileNameZ, qbs_add(fileName, qbs_new_txt_len("\0", 1))); // s1 = filename + CHR$(0)
    if (fileNameZ->len == 1)
        return; // Return 0 if file name is null length string

    // TODO:
    //	Implement volume
    //	Emulate QB64 path behavior
    if (audioEngine.isInitialized) {
        ma_engine_play_sound(&audioEngine.maEngine, (const char *)fileNameZ->chr, NULL);
    }
}

/// <summary>
/// This pauses a sound using a sound handle
/// </summary>
/// <param name="handle">A sound handle</param>
void sub__sndpause(int32 handle) {
    if (audioEngine.isInitialized && IS_SOUND_HANDLE_VALID(handle)) {
        SoundHandle *snd = (SoundHandle *)list_get(audioEngine.soundHandles, handle);
        if (!snd || snd->type != SOUND_STATIC)
            return;

        // Stop the sound and just leave it at that
        // miniaudio does not reset the play cursor
        audioEngine.maResult = ma_sound_stop(&snd->maSound);
        assert(audioEngine.maResult == MA_SUCCESS);
    }
}

/// <summary>
/// This returns whether a sound is being played.
/// </summary>
/// <param name="handle">A sound handle</param>
/// <returns>Return true if the sound is playing. False otherwise</returns>
int32 func__sndplaying(int32 handle) {
    if (audioEngine.isInitialized && IS_SOUND_HANDLE_VALID(handle)) {
        SoundHandle *snd = (SoundHandle *)list_get(audioEngine.soundHandles, handle);
        if (!snd || snd->type != SOUND_STATIC)
            return QB_FALSE;

        return ma_sound_is_playing(&snd->maSound) ? QB_TRUE : QB_FALSE;
    }

    return QB_FALSE;
}

/// <summary>
/// This checks if a sound is paused.
/// </summary>
/// <param name="handle">A sound handle</param>
/// <returns>Returns true if the sound is paused. False otherwise</returns>
int32 func__sndpaused(int32 handle) {
    if (audioEngine.isInitialized && IS_SOUND_HANDLE_VALID(handle)) {
        SoundHandle *snd = (SoundHandle *)list_get(audioEngine.soundHandles, handle);
        if (!snd || snd->type != SOUND_STATIC)
            return QB_FALSE;

        return !ma_sound_is_playing(&snd->maSound) && (ma_sound_is_looping(&snd->maSound) || !ma_sound_at_end(&snd->maSound)) ? QB_TRUE : QB_FALSE;
    }

    return QB_FALSE;
}

/// <summary>
/// This sets the volume of a sound loaded in memory using a sound handle.
/// </summary>
/// <param name="handle">A sound handle</param>
/// <param name="volume">A float point value with 0 resulting in silence and anything above 1 resulting in amplification</param>
void sub__sndvol(int32 handle, float volume) {
    if (audioEngine.isInitialized && IS_SOUND_HANDLE_VALID(handle)) {
        SoundHandle *snd = (SoundHandle *)list_get(audioEngine.soundHandles, handle);
        if (!snd || snd->type != SOUND_STATIC)
            return;

        ma_sound_set_volume(&snd->maSound, volume);
    }
}

/// <summary>
/// This is like sub__sndplay but the sound is looped
/// </summary>
/// <param name="handle"></param>
void sub__sndloop(int32 handle) {
    if (audioEngine.isInitialized && IS_SOUND_HANDLE_VALID(handle)) {
        SoundHandle *snd = (SoundHandle *)list_get(audioEngine.soundHandles, handle);
        if (!snd || snd->type != SOUND_STATIC)
            return;

        // Reset cursor to zero only if we are not looping or we've reached the end of the sound
        if (!ma_sound_is_looping(&snd->maSound) || ma_sound_at_end(&snd->maSound)) {
            audioEngine.maResult = ma_sound_seek_to_pcm_frame(&snd->maSound, 0);
            assert(audioEngine.maResult == MA_SUCCESS);
        }

        // Kickstart playback
        audioEngine.maResult = ma_sound_start(&snd->maSound);
        assert(audioEngine.maResult == MA_SUCCESS);

        // Start looping the sound if it is not
        if (!ma_sound_is_looping(&snd->maSound)) {
            ma_sound_set_looping(&snd->maSound, MA_TRUE);
        }
    }
}

/// <summary>
///
/// </summary>
/// <param name="handle"></param>
/// <param name="x"></param>
/// <param name="y"></param>
/// <param name="z"></param>
/// <param name="channel"></param>
/// <param name="passed"></param>
void sub__sndbal(int32 handle, double x, double y, double z, int32 channel, int32 passed) {}

/// <summary>
///
/// </summary>
/// <param name="handle"></param>
/// <returns></returns>
double func__sndlen(int32 handle) { return 0; }

/// <summary>
///
/// </summary>
/// <param name="handle"></param>
/// <returns></returns>
double func__sndgetpos(int32 handle) { return 0; }

/// <summary>
///
/// </summary>
/// <param name="handle"></param>
/// <param name="seconds"></param>
void sub__sndsetpos(int32 handle, double seconds) {}

/// <summary>
///
/// </summary>
/// <param name="handle"></param>
/// <param name="limit"></param>
void sub__sndlimit(int32 handle, double limit) {}

/// <summary>
/// This stops a playing or paused sound using a sound handle.
/// </summary>
/// <param name="handle">A sound handle</param>
void sub__sndstop(int32 handle) {
    if (audioEngine.isInitialized && IS_SOUND_HANDLE_VALID(handle)) {
        SoundHandle *snd = (SoundHandle *)list_get(audioEngine.soundHandles, handle);
        if (!snd || snd->type != SOUND_STATIC)
            return;

        // Stop the sound first
        audioEngine.maResult = ma_sound_stop(&snd->maSound);
        assert(audioEngine.maResult == MA_SUCCESS);

        // Also reset the playback cursor to zero
        audioEngine.maResult = ma_sound_seek_to_pcm_frame(&snd->maSound, 0);
        assert(audioEngine.maResult == MA_SUCCESS);
    }
}

/// <summary>
///
/// </summary>
/// <returns></returns>
int32 func__sndopenraw() { return 0; }

/// <summary>
///
/// </summary>
/// <param name="left"></param>
/// <param name="right"></param>
/// <param name="handle"></param>
/// <param name="passed"></param>
void sub__sndraw(float left, float right, int32 handle, int32 passed) {}

/// <summary>
///
/// </summary>
/// <param name="handle"></param>
/// <param name="passed"></param>
void sub__sndrawdone(int32 handle, int32 passed) {}

/// <summary>
///
/// </summary>
/// <param name="handle"></param>
/// <param name="passed"></param>
/// <returns></returns>
double func__sndrawlen(int32 handle, int32 passed) { return 0; }

/// <summary>
///
/// </summary>
/// <param name="i"></param>
/// <param name="targetChannel"></param>
/// <returns></returns>
mem_block func__memsound(int32 i, int32 targetChannel) {}

/// <summary>
///
/// </summary>
/// <param name="size"></param>
/// <param name="rate"></param>
/// <param name="channels"></param>
/// <param name="bits"></param>
/// <param name="passed"></param>
/// <returns></returns>
int32 func__newsound(int32 size, int32 rate, int32 channels, int32 bits, int32 passed) { return 0; }

/// <summary>
/// This initializes the QBPE audio subsystem.
/// We simply attempt to initialize and then set some globals with the results.
/// </summary>
void snd_init() {
    // Exit if engine is initialize or already initialization was attempted but failed
    if (audioEngine.isInitialized || audioEngine.initializationFailed)
        return;

    // Setup the sound handle list
    audioEngine.soundHandles = list_new(sizeof(SoundHandle));

    if (!audioEngine.soundHandles) {
        audioEngine.initializationFailed = MA_TRUE;
        return;
    }

    // Attempt to initialize with miniaudio defaults
    audioEngine.maResult = ma_engine_init(NULL, &audioEngine.maEngine);

    // If failed, then set the global flag so that we don't attempt to initialize again
    if (audioEngine.maResult != MA_SUCCESS) {
        list_destroy(audioEngine.soundHandles);
        audioEngine.initializationFailed = MA_TRUE;
        return;
    }

    // Initialize the sound handle list
    // TODO: Use QB64 list_new() in the final version
    //	Allocate a sound handle to use for Beep(), Sound()...
    audioEngine.sndInternal = -1;

    // Set the initialized flag as true
    audioEngine.isInitialized = MA_TRUE;
}

/// <summary>
/// This shuts down the audio engine and frees any resources used.
/// </summary>
void snd_un_init() {
    if (audioEngine.isInitialized) {
        // Free more stuff here?
        // TODO: Free all internal handles here

        // Shutdown miniaudio
        ma_engine_uninit(&audioEngine.maEngine);

        // Destroy the sound handle list
        // NOTE: All audio data should have to be destroyed before the call to ma_engine_uninit!
        list_destroy(audioEngine.soundHandles);

        // Set global flag as false
        audioEngine.isInitialized = FALSE;
    }
}

/// <summary>
/// This is called by the QBPE library code.
/// We use this for housekeeping and other stuff.
/// </summary>
void snd_mainloop() {
    if (audioEngine.isInitialized) {
        // Scan through the list and find anything that we need to stop and close
        for (int32 listIndex = 1; listIndex <= audioEngine.soundHandles->indexes; listIndex++) {
            SoundHandle *snd;
            snd = (SoundHandle *)list_get(audioEngine.soundHandles, listIndex);
            if (snd) {
                // We are only looking for stuff that is set to auto-destruct
                if (snd->autoKill) {
                    switch (snd->type) {
                    case SOUND_STATIC:
                        // TODO:
                        break;

                    case SOUND_RAW:
                        // TODO:
                        break;

                    default:
                        assert(MA_TRUE); // It should not come here
                    }

                    // TODO: more common housekeeping here. Free handles and stuff
                }
            }
        }
    }
}
//-----------------------------------------------------------------------------------------------------
// HACK for: internal\c/os.h: error: declaration does not declare anything [-fpermissive]
#define int64 long long int
//-----------------------------------------------------------------------------------------------------
