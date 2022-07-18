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
// Again, we will wait for Matt to cleanup the C/C++ source file and include header files that declare this stuff when those become available
qbs *qbs_new_txt_len(const char *txt, int32 len);
int32 func_instr(int32 start, qbs *str, qbs *substr, int32 passed);
//-----------------------------------------------------------------------------------------------------

//-----------------------------------------------------------------------------------------------------
// CONSTANTS
//-----------------------------------------------------------------------------------------------------
// In QuickBASIC false means 0 and true means -1 (sad, but true)
#define QB_FALSE MA_FALSE
#define QB_TRUE -MA_TRUE

// This is returned to the caller if handle allocation fails. We do not return 0 becuase 0 is a valid vector index
// Technically all handles < 1 are invalid as far as the user is concerned (see IS_SOUND_HANDLE_VALID)
#define INVALID_SOUND_HANDLE -1
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
// STRUCTURES
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
/// This is a queue of type SampleFrameNode.
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
    ma_bool8 isUsed;                   // Is this handle in active use
    SoundType type;                    // Type of sound (see SoundType enum)
    ma_bool8 autoKill;                 // Do we need to auto-clean this sample / stream after playback is done?
    ma_bool8 isInternal;               // Is this a sound handle that is used internally by the audio engine?
    ma_sound maSound;                  // miniaudio sound
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
    int32 sndInternal;             // Internal sound handle that we will use for Beep() & Sound(). TODO: Do we really need this? See Beep() and Sound()
    std::vector<SoundHandle *> soundHandles; // This is the audio handle list used by the engine and by everything else
} AudioEngine;
//-----------------------------------------------------------------------------------------------------

//-----------------------------------------------------------------------------------------------------
// GLOBAL VARIABLES
//-----------------------------------------------------------------------------------------------------
// This keeps track of the audio engine state
static AudioEngine audioEngine = {0};
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
static int32 AllocateSoundHandle() {
    size_t vectorSize = audioEngine.soundHandles.size(); // Save the vector size

    // Scan through the vector and return a slot that is not being used
    // This loop should not execute if size is 0
    for (size_t i = 0; i < vectorSize; i++) {
        if (!audioEngine.soundHandles[i]->isUsed)
            return (int32)i;
    }

    // If we have reached here then either the vector is empty or there are no empty slots
    // Simply create a new SoundHandle at the back of the vector and return it's position
    audioEngine.soundHandles.push_back(new SoundHandle);
    size_t newVectorSize = audioEngine.soundHandles.size();

    // If newVectorSize == vectorSize then emplace_back() failed
    if (newVectorSize <= vectorSize)
        return INVALID_SOUND_HANDLE;

    audioEngine.soundHandles[newVectorSize - 1]->isUsed = MA_FALSE; // Set the isUsed flag to false

    return (int32)(newVectorSize - 1); // Return newVectorSize - 1 as the handle
}

/// <summary>
/// Initializes a sound handle that was just allocated.
/// The handle must not be 'in use'.
/// This will set it to 'in use' after applying some defaults.
/// Note that this allows us to initiaze the internal sound handle (0)
/// </summary>
/// <param name="handle">A sound handle</param>
/// <returns>True if sound handle is not in use</returns>
static ma_bool8 InitializeSoundHandle(int32 handle) {
    if (handle >= 0 && handle < audioEngine.soundHandles.size() && !audioEngine.soundHandles[handle]->isUsed) {
        audioEngine.soundHandles[handle]->type = SOUND_NONE;
        audioEngine.soundHandles[handle]->autoKill = MA_FALSE;
        audioEngine.soundHandles[handle]->sampleFrameQueue = {0};
        audioEngine.soundHandles[handle]->maSound = {0};
        // We do not use pitch shifting, so this will give a little performance boost
        audioEngine.soundHandles[handle]->maFlags = MA_SOUND_FLAG_NO_PITCH | MA_SOUND_FLAG_WAIT_INIT;
        audioEngine.soundHandles[handle]->isUsed = MA_TRUE;

        return MA_TRUE;
    }

    return MA_FALSE;
}

/// <summary>
/// This generates a sound at the specified frequency for the specified amount of time.
/// Returns immediately after sending the data for playback.
/// </summary>
/// <param name="frequency">Sound frequency</param>
/// <param name="lengthInClockTicks">Duration in clock ticks. There are 18.2 clock ticks per second</param>
void sub_sound(double frequency, double lengthInClockTicks) {
    // TODO: miniaudio has waveform API that can be used to generare various kind of sound waves
    // We can use that or just put this through sndraw just like the OpenAL code did
    // Hmmm... decisions, decisions... XD
}

/// <summary>
/// This generates a default 'beep' sound.
/// Sends the sound samples for playback and immediately returns.
/// This is in-line with the behavior of 'Sound' and 'Play'.
/// </summary>
void sub_beep() { sub_sound(783.99, 0.25); }

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
int32 func__sndrate() { return audioEngine.isInitialized ? ma_engine_get_sample_rate(&audioEngine.maEngine) : 0; }

/// <summary>
/// This loads a sound file into memory and returns a LONG handle value above 0.
/// </summary>
/// <param name="fileName">The is the pathname for the sound file. This can be any format the miniaudio support or a format supported by a miniaudio
/// plugin</param> <param name="requirements">The is leftover from the old QB64-SDL days. But we use this to pass some parameters like 'stream'</param> <param
/// name="passed">How many parameters were passed?</param> <returns>Returns a valid sound handle (> 0) if successful or 0 if it fails</returns>
int32 func__sndopen(qbs *fileName, qbs *requirements, int32 passed) {
    if (!audioEngine.isInitialized)
        return INVALID_SOUND_HANDLE;

    // Some QB strings that we'll need
    static qbs *fileNameZ = NULL;
    if (!fileNameZ)
        fileNameZ = qbs_new(0, 0);
    static qbs *req = NULL;
    if (!req)
        req = qbs_new(0, 0);

    qbs_set(fileNameZ, qbs_add(fileName, qbs_new_txt_len("\0", 1))); // s1 = filename + CHR$(0)
    if (fileNameZ->len == 1)
        return INVALID_SOUND_HANDLE; // Return INVALID_SOUND_HANDLE if file name is null length string

    // Alocate a sound handle
    int32 handle = AllocateSoundHandle();
    // Initialize the sound handle data
    if (!InitializeSoundHandle(handle) || handle <= 0) // We are not expected to open files with handle 0
        return INVALID_SOUND_HANDLE;

    // Set some handle properties
    audioEngine.soundHandles[handle]->type = SOUND_STATIC;

    // Set the flags to specifiy how we want the audio file to be opened
    // TODO: case insensitive compare
    if (passed && requirements->len) {
        qbs_set(req, qbs_ucase(requirements)); // Convert tmp str to perm str
        if (func_instr(1, req, qbs_new_txt("STREAM"), 1))
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
        audioEngine.soundHandles[handle]->isUsed = MA_FALSE;

        return INVALID_SOUND_HANDLE;
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
        // Sound type specific cleanup
        switch (audioEngine.soundHandles[handle]->type) {
        case SOUND_STATIC:
            ma_sound_uninit(&audioEngine.soundHandles[handle]->maSound);
            break;

        case SOUND_RAW:
            // TODO:
            break;

        default:
            assert(MA_TRUE); // It should not come here
        }

        // Now simply set the 'isUsed' member to false
        audioEngine.soundHandles[handle]->isUsed = MA_FALSE;
    }
}

/// <summary>
/// This copies a sound to a new handle so that two or more of the same sound can be played at once.
/// </summary>
/// <param name="src_handle">A source sound handle</param>
/// <returns>A new sound handle if successful or 0 on failure</returns>
int32 func__sndcopy(int32 src_handle) {
    // Check for all invalid cases
    if (!audioEngine.isInitialized || !IS_SOUND_HANDLE_VALID(src_handle) || audioEngine.soundHandles[src_handle]->type != SOUND_STATIC)
        return INVALID_SOUND_HANDLE;

    // Alocate a sound handle
    int32 dst_handle = AllocateSoundHandle();
    // Initialize the sound handle data
    if (!InitializeSoundHandle(dst_handle) || dst_handle <= 0) // We are not expected to open files with handle 0
        return INVALID_SOUND_HANDLE;

    audioEngine.soundHandles[dst_handle]->type = SOUND_STATIC;                                     // Set some handle properties
    audioEngine.soundHandles[dst_handle]->maFlags = audioEngine.soundHandles[src_handle]->maFlags; // Copy the flags

    // Initialize a new copy of the sound
    audioEngine.maResult = ma_sound_init_copy(&audioEngine.maEngine, &audioEngine.soundHandles[src_handle]->maSound,
                                              audioEngine.soundHandles[dst_handle]->maFlags, NULL, &audioEngine.soundHandles[dst_handle]->maSound);

    // If the sound failed to copy, then free the handle and return INVALID_SOUND_HANDLE
    if (audioEngine.maResult != MA_SUCCESS) {
        audioEngine.soundHandles[dst_handle]->isUsed = MA_FALSE;

        return INVALID_SOUND_HANDLE;
    }

    return dst_handle;
}

/// <summary>
/// This plays a sound designated by a sound handle
/// </summary>
/// <param name="handle">A sound handle</param>
void sub__sndplay(int32 handle) {
    if (audioEngine.isInitialized && IS_SOUND_HANDLE_VALID(handle) && audioEngine.soundHandles[handle]->type == SOUND_STATIC) {
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
/// This copies a sound, plays it, and automatically closes the copy
/// </summary>
/// <param name="handle">A sound handle to copy</param>
/// <param name="volume">The volume at which the sound should be played (0.0 - 1.0)</param>
/// <param name="passed">How many parameters were passed?</param>
void sub__sndplaycopy(int32 src_handle, double volume, int32 passed) {
    // We are simply going to use sndcopy, then setup some stuff like volume and autokill and then use sndplay
    int32 dst_handle = func__sndcopy(src_handle);

    // Check if we succeeded and then proceed
    if (dst_handle > 0) {
        // Set the volume if requested
        if (passed)
            ma_sound_set_volume(&audioEngine.soundHandles[dst_handle]->maSound, volume);

        sub__sndplay(dst_handle);                                 // Play the sound
        audioEngine.soundHandles[dst_handle]->autoKill = MA_TRUE; // Set to auto kill
    }
}

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
        return; // Exit if file name is null length string

    // TODO:
    //	Implement volume
    //	Emulate QB64 path behavior?
    if (audioEngine.isInitialized) {
        ma_engine_play_sound(&audioEngine.maEngine, (const char *)fileNameZ->chr, NULL);
    }
}

/// <summary>
/// This pauses a sound using a sound handle
/// </summary>
/// <param name="handle">A sound handle</param>
void sub__sndpause(int32 handle) {
    if (audioEngine.isInitialized && IS_SOUND_HANDLE_VALID(handle) && audioEngine.soundHandles[handle]->type == SOUND_STATIC) {
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
    if (audioEngine.isInitialized && IS_SOUND_HANDLE_VALID(handle) && audioEngine.soundHandles[handle]->type == SOUND_STATIC) {
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
    if (audioEngine.isInitialized && IS_SOUND_HANDLE_VALID(handle) && audioEngine.soundHandles[handle]->type == SOUND_STATIC) {
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
    if (audioEngine.isInitialized && IS_SOUND_HANDLE_VALID(handle) && audioEngine.soundHandles[handle]->type == SOUND_STATIC) {
        ma_sound_set_volume(&audioEngine.soundHandles[handle]->maSound, volume);
    }
}

/// <summary>
/// This is like sub__sndplay but the sound is looped
/// </summary>
/// <param name="handle"></param>
void sub__sndloop(int32 handle) {
    if (audioEngine.isInitialized && IS_SOUND_HANDLE_VALID(handle) && audioEngine.soundHandles[handle]->type == SOUND_STATIC) {
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
///
/// </summary>
/// <param name="handle"></param>
/// <param name="x"></param>
/// <param name="y"></param>
/// <param name="z"></param>
/// <param name="channel"></param>
/// <param name="passed"></param>
void sub__sndbal(int32 handle, double x, double y, double z, int32 channel, int32 passed) {
    // TODO: This should be easy because miniaudio support stereo panning and also OpenAL style positional audio
}

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
    if (audioEngine.isInitialized && IS_SOUND_HANDLE_VALID(handle) && audioEngine.soundHandles[handle]->type == SOUND_STATIC) {
        // Stop the sound first
        audioEngine.maResult = ma_sound_stop(&audioEngine.soundHandles[handle]->maSound);
        assert(audioEngine.maResult == MA_SUCCESS);

        // Also reset the playback cursor to zero
        audioEngine.maResult = ma_sound_seek_to_pcm_frame(&audioEngine.soundHandles[handle]->maSound, 0);
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
mem_block func__memsound(int32 i, int32 targetChannel) {
    // TODO: Out of all the functions, this is probably going to be the most difficult or kludgy or both
    //  Since we want miniaudio to manage all audio resouces, audio data is stored how miniaudio wants in memory
    //  In miniaudio's case, stereo audio data is store as iterleaved in memory. Apparently, this is true for many audio libs out there
    //  This is completely unlike what the old OpenAL code was doing
    //  One way to implement this is to have two temp buffers - one for left and one for right and let the user use those
    //  When the user is done using and closes the mem object, then the interleave data should be created from the buffers for miniaudio (yuck!)
    //  The second way is to simply expose the miniaudio's interleaved buffer to the user. This will break compatibility. But then, how many people really use
    //  this stuff?
}

/// <summary>
///
/// </summary>
/// <param name="size"></param>
/// <param name="rate"></param>
/// <param name="channels"></param>
/// <param name="bits"></param>
/// <param name="passed"></param>
/// <returns></returns>
int32 func__newsound(int32 size, int32 rate, int32 channels, int32 bits, int32 passed) {
    // This is a new addition to the QBPE Audio API
    // This allows a user to create a empty sound buffer with a given specification
    // The user then can fill the buffer with whatever they want and play it
    // This obviously needs to be greenlit by the QBPE maintainers

    return 0;
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
        audioEngine.initializationFailed = MA_TRUE;
        return;
    }

    // Allocate a sound handle
    // Not that this should always be 0
    // We will use this handle internally for Beep(), Sound() etc.
    audioEngine.sndInternal = AllocateSoundHandle();
    assert(audioEngine.sndInternal == 0); // The first handle must return 0 and this is what is used by Beep and Sound
    if (!InitializeSoundHandle(audioEngine.sndInternal))
        assert(MA_TRUE);                                                 // It should not come here
    audioEngine.soundHandles[audioEngine.sndInternal]->type = SOUND_RAW; // Set the type as raw

    // Set the initialized flag as true
    audioEngine.isInitialized = MA_TRUE;
}

/// <summary>
/// This shuts down the audio engine and frees any resources used.
/// </summary>
void snd_un_init() {
    if (audioEngine.isInitialized) {
        // Free all sound handles here
        for (size_t handle = 0; handle < audioEngine.soundHandles.size(); handle++) {
            // Perform and sound type specific cleanup
            switch (audioEngine.soundHandles[handle]->type) {
            case SOUND_STATIC:
                // TODO:
                break;

            case SOUND_RAW:
                // TODO:
                break;

            default:
                assert(MA_TRUE); // It should not come here
            }

            sub__sndclose(handle);                   // Let SndClose do it's thing
            delete audioEngine.soundHandles[handle]; // Now free the object created by AllocateSoundHandle()
        }

        // Now that all sounds are closed and SoundHandle objects are freed, clear the vector
        audioEngine.soundHandles.clear();

        // Shutdown miniaudio
        ma_engine_uninit(&audioEngine.maEngine);

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
        for (size_t handle = 0; handle < audioEngine.soundHandles.size(); handle++) {
            // We are only looking for stuff that is set to auto-destruct
            if (audioEngine.soundHandles[handle]->isUsed && audioEngine.soundHandles[handle]->autoKill) {
                switch (audioEngine.soundHandles[handle]->type) {
                case SOUND_STATIC:
                    // Dispose the sound if it has finished playing
                    // Note that this means that temporary looping sounds will never close
                    // Well thats on the programmer. Probably they want it that way
                    if (!ma_sound_is_playing(&audioEngine.soundHandles[handle]->maSound)) {
                        sub__sndclose(handle);
                    }
                    break;

                case SOUND_RAW:
                    // TODO:
                    break;

                default:
                    assert(MA_TRUE); // It should not come here
                }

                // TODO: more common housekeeping here
            }
        }
    }
}
//-----------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------
