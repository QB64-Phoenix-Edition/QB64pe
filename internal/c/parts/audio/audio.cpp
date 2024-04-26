//----------------------------------------------------------------------------------------------------
//    ___  ___   __ _ _  ___ ___     _          _ _       ___           _
//   / _ \| _ ) / /| | || _ \ __|   /_\ _  _ __| (_)___  | __|_ _  __ _(_)_ _  ___
//  | (_) | _ \/ _ \_  _|  _/ _|   / _ \ || / _` | / _ \ | _|| ' \/ _` | | ' \/ -_)
//   \__\_\___/\___/ |_||_| |___| /_/ \_\_,_\__,_|_\___/ |___|_||_\__, |_|_||_\___|
//                                                                |___/
//
//  QB64-PE Audio Engine powered by miniaudio (https://miniaud.io/)
//
//-----------------------------------------------------------------------------------------------------

#include "libqb-common.h"

// Set this to 1 if we want to print debug messages to stderr
#define AUDIO_DEBUG 0
#include "audio.h"

#define STB_VORBIS_HEADER_ONLY
#include "datetime.h"
#include "extras/stb_vorbis.c"
#include "filepath.h"
#include "miniaudio.h"
#include "mutex.h"
#include "error_handle.h"
#include "qbs.h"
#include "mem.h"
#include "cmem.h"

#include <string.h>
#include <math.h>
#include <algorithm>
#include <stack>
#include <unordered_map>
#include <vector>
#include <limits.h>

// This is returned to the caller if handle allocation fails with a -1
// CreateHandle() does not return 0 because it is a valid internal handle
// Handle 0 is 'handled' as a special case
#define INVALID_SOUND_HANDLE 0
// This is the string that can be passed in the requirements parameter to stream a sound from storage
#define REQUIREMENT_STRING_STREAM "STREAM"
// This is the string that can be passed in the requirements parameter to load a sound from memory
#define REQUIREMENT_STRING_MEMORY "MEMORY"

#define SAMPLE_FRAME_SIZE(_type_, _channels_) (sizeof(_type_) * (_channels_))
#define ZERO_VARIABLE(_v_) memset(&(_v_), 0, sizeof(_v_))

// This basically checks if the handle is within vector limits and 'isUsed' is set to true
// We are relying on C's boolean short-circuit to not evaluate the last 'isUsed' if previous conditions are false
// Here we are checking > 0 because this is meant to check user handles only
#define IS_SOUND_HANDLE_VALID(_handle_)                                                                                                                        \
    ((_handle_) > 0 && (_handle_) < (int32_t)audioEngine.soundHandles.size() && audioEngine.soundHandles[_handle_]->isUsed &&                                           \
     !audioEngine.soundHandles[_handle_]->autoKill)

// These attaches our customer backend (format decoders) VTables to various miniaudio structs
void AudioEngineAttachCustomBackendVTables(ma_resource_manager_config *maResourceManagerConfig);
void AudioEngineAttachCustomBackendVTables(ma_decoder_config *maDecoderConfig);


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

    static const size_t DEFAULT_SIZE = 1024; // this is almost twice the amount what miniaudio actually asks for in frameCount

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
        buffer[0].data.reserve(DEFAULT_SIZE);             // ensure we have a contiguous block to account for expansion without reallocation
        buffer[1].data.reserve(DEFAULT_SIZE);             // ensure we have a contiguous block to account for expansion without reallocation
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
        std::swap(consumer, producer); // quickly swap the Buffer pointers
    }

    /// @brief This pushes a sample frame at the end of the queue. This is mutex protected and called by the main thread
    /// @param l Sample frame left channel data
    /// @param r Sample frame right channel data
    void PushSampleFrame(float l, float r) {
        libqb_mutex_guard lock(m);        // lock the mutex before accessing the vectors
        producer->data.push_back({l, r}); // push the sample frame to the back of the producer queue
    }

    /// @brief This pushes a whole buffer of mono sample frames to the queue. This is mutex protected and called by the main thread
    /// @param buffer The buffer containing the sample frames. This cannot be NULL
    /// @param frames The total number of frames in the buffer
    /// @param panning An optional argument that controls how the buffer should be panned (-1.0 (full left) to 1.0 (full right))
    void PushMonoSampleFrames(float *buffer, ma_uint64 frames, float panning = 0.0f) {
        libqb_mutex_guard lock(m); // lock the mutex before accessing the vectors
        for (ma_uint64 i = 0; i < frames; i++) {
            producer->data.push_back({(buffer[i] * (1.0f - panning)) / 2.0f, (buffer[i] * (1.0f + panning)) / 2.0f});
        }
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
/// @return Returns a pointer to a data source if successful, NULL otherwise
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
        AUDIO_DEBUG_PRINT("Error %i: failed to initialize sound from data source", result);

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

/// @brief A class that can manage a list of buffers using unique keys
class BufferMap {
  private:
    /// @brief A buffer that is made up of a raw pointer, size and reference count
    struct Buffer {
        void *data;
        size_t size;
        size_t refCount;
    };

    std::unordered_map<intptr_t, Buffer> buffers;

  public:
    // Delete assignment operators
    BufferMap &operator=(const BufferMap &) = delete;
    BufferMap &operator=(BufferMap &&) = delete;

    /// @brief This will simply free all buffers that were allocated
    ~BufferMap() {
        for (auto &it : buffers) {
            free(it.second.data);
            AUDIO_DEBUG_PRINT("Buffer freed of size %llu", it.second.size);
        }
    }

    /// @brief Adds a buffer to the map using a unique key only if it was not added before
    /// @param data The raw data pointer. The data is copied
    /// @param size The size of the data
    /// @param key The unique key that should be used
    /// @return True if successful
    bool AddBuffer(const void *data, size_t size, intptr_t key) {
        if (data && size && key && buffers.find(key) == buffers.end()) {
            Buffer buf = {};

            buf.data = malloc(size);
            if (!buf.data)
                return false;

            buf.size = size;
            buf.refCount = 1;
            memcpy(buf.data, data, size);
            buffers.emplace(key, std::move(buf));

            AUDIO_DEBUG_PRINT("Added buffer of size %llu to map", size);
            return true;
        }

        AUDIO_DEBUG_PRINT("Failed to add buffer of size %llu", size);
        return false;
    }

    /// @brief Increments the buffer reference count
    /// @param key The unique key for the buffer
    void AddRef(intptr_t key) {
        const auto it = buffers.find(key);
        if (it != buffers.end()) {
            auto &buf = it->second;
            buf.refCount += 1;
            AUDIO_DEBUG_PRINT("Increased reference count to %llu", buf.refCount);
        } else {
            AUDIO_DEBUG_PRINT("Buffer not found");
        }
    }

    /// @brief Decrements the buffer reference count and frees the buffer if the reference count reaches zero
    /// @param key The unique key for the buffer
    void Release(intptr_t key) {
        const auto it = buffers.find(key);
        if (it != buffers.end()) {
            auto &buf = it->second;
            buf.refCount -= 1;
            AUDIO_DEBUG_PRINT("Decreased reference count to %llu", buf.refCount);

            if (buf.refCount < 1) {
                free(buf.data);
                AUDIO_DEBUG_PRINT("Buffer freed of size %llu", buf.size);
                buffers.erase(key);
            }
        } else {
            AUDIO_DEBUG_PRINT("Buffer not found");
        }
    }

    /// @brief Gets the raw pointer and size of the buffer with the given key
    /// @param key The unique key for the buffer
    /// @return An std::pair of the buffer raw pointer and size
    std::pair<const void *, size_t> GetBuffer(intptr_t key) const {
        const auto it = buffers.find(key);
        if (it == buffers.end()) {
            AUDIO_DEBUG_PRINT("Buffer not found");
            return {nullptr, 0};
        }
        const auto &buf = it->second;
        AUDIO_DEBUG_PRINT("Returning buffer of size %llu", buf.size);
        return {buf.data, buf.size};
    }
};

/// @brief This is a PSG class that handles all kinds of sound generation for BEEP, SOUND and PLAY
class PSG {
  public:
    /// @brief Various types of waveform that can be generated
    enum class WaveformType { NONE, SQUARE, SAWTOOTH, TRIANGLE, SINE, NOISE, COUNT };

    static constexpr auto PAN_LEFT = -1.0f;
    static constexpr auto PAN_RIGHT = 1.0f;
    static constexpr auto PAN_CENTER = PAN_LEFT + PAN_RIGHT;
    static constexpr auto MIN_VOLUME = 0.0;
    static constexpr auto MAX_VOLUME = 1.0;

  private:
    /// @brief This struct to used to hold the state of the MML player and also used for the state stack (i.e. when VARPTR$ substrings are used)
    struct State {
        const uint8_t *byte; // pointer to a byte in an MML string
        int32_t length;      // this needs to be signed
    };

    RawStream *rawStream;                // this is the RawStream where the samples data will be pushed to
    ma_waveform_config maWaveformConfig; // miniaudio waveform configuration
    ma_waveform maWaveform;              // miniaudio waveform
    ma_noise_config maNoiseConfig;       // miniaudio noise configuration
    ma_noise maNoise;                    // miniaudio noise
    ma_result maResult;                  // result of the last miniaudio operation
    std::vector<float> noteBuffer;       // note frames are rendered here temporarily before it is mixed to waveBuffer
    std::vector<float> waveBuffer;       // this is where the waveform is rendered / mixed before being pushed to RawStream
    ma_uint64 mixCursor;                 // this is the cursor position in waveBuffer where the next mix should happen (this can be < waveBuffer.size())
    WaveformType waveformType;           // the currently selected waveform type (applies to MML and sound)
    float volumeRampDuration;            // the volume ramping duration (this can be changed by the user)
    bool background;                     // if this is true, then control will be returned back to the caller as soon as the sound / MML is rendered
    float panning;                       // stereo pan setting for SOUND (-1.0f - 0.0f - 1.0f)
    std::stack<State> stateStack;        // this maintains the state stack if we need to process substrings (VARPTR$)
    State currentState;                  // this is the current state. See State struct
    int tempo;                           // the tempo of the MML tune (this impacts all lengths)
    int octave;                          // the current octave that we'll use for MML notes
    double length;                       // the length of each MML note (1 = full, 4 = quarter etc.)
    double pause;                        // the duration of silence after an MML note (this eats away from the note length)
    double duration;                     // the duration of a sound / MML note / silence (in seconds)
    int dots;                            // the dots after a note or a pause that increases the duration
    bool playIt;                         // flag that is set when the buffer can be played

    // These are some constants that can be tweaked to change the behavior of the PSG and MML parser
    // These mostly conform to the QBasic and QB64 spec.
    static const auto DEFAULT_WAVEFORM_TYPE = WaveformType::TRIANGLE;
    static constexpr auto DEFAULT_FREQUENCY = 440.0;
    static constexpr auto MAX_MML_VOLUME = 100.0;
    static constexpr auto DEFAULT_MML_VOLUME = MAX_MML_VOLUME / 2;
    static const auto MIN_TEMPO = 32;
    static const auto MAX_TEMPO = 255;
    static const auto DEFAULT_TEMPO = 120;
    static const auto MAX_OCTAVE = 6;
    static const auto DEFAULT_OCTAVE = 4;
    static const auto MIN_LENGTH = 1;
    static const auto MAX_LENGTH = 64;
    static constexpr auto DEFAULT_LENGTH = 4.0;
    static constexpr auto DEFAULT_PAUSE = 1.0 / 8.0;
    static constexpr auto DEFAULT_VOLUME_RAMP_DURATION = 0.01f;
    static constexpr auto BEEP_FREQUENCY = 900.0;
    static constexpr auto BEEP_WAVEFORM_DURATION = 0.2472527472527473;
    static constexpr auto BEEP_SILENCE_DURATION = 0.0274725274725275;
    static constexpr auto BEEP_DURATION = BEEP_WAVEFORM_DURATION + BEEP_SILENCE_DURATION;

    /// @brief Generates a waveform to waveBuffer starting at the mixCursor sample location.
    /// The buffer must be resized before calling this. We could have resized waveBuffer inside this.
    /// However, PLAY supports stuff like staccato etc. that needs some silence after the waveform.
    /// So it makes sense for the calling function to do the resize before calling this
    /// @param waveDuration The duration of the waveform in seconds
    /// @param mix Mixes the generated waveform to the buffer instead of overwriting it
    void GenerateWaveform(double waveDuration, bool mix = false) {
        auto neededFrames = (ma_uint64)(waveDuration * rawStream->sampleRate);

        if (!neededFrames || maWaveform.config.frequency >= 20000 || mixCursor + neededFrames > waveBuffer.size()) {
            AUDIO_DEBUG_PRINT("Not generating any waveform. Frames = %llu, frequency = %lf, cursor = %llu", neededFrames, maWaveform.config.frequency,
                              mixCursor);
            return; // nothing to do
        }

        maResult = MA_SUCCESS;
        ma_uint64 generatedFrames = neededFrames;
        noteBuffer.assign(neededFrames, 0.0f); // resize the noteBuffer vector to render the waveform and also zero (silence) everything

        // Generate to the temp buffer and then we'll mix later
        switch (waveformType) {
        case WaveformType::TRIANGLE:
        case WaveformType::SAWTOOTH:
        case WaveformType::SINE:
        case WaveformType::SQUARE:
            maResult = ma_waveform_read_pcm_frames(&maWaveform, noteBuffer.data(), neededFrames, &generatedFrames);
            break;

        case WaveformType::NOISE:
            maResult = ma_noise_read_pcm_frames(&maNoise, noteBuffer.data(), neededFrames, &generatedFrames);
            break;
        }

        if (maResult != MA_SUCCESS) {
            AUDIO_DEBUG_PRINT("maResult = %i", maResult);
            return; // something went wrong
        }

        // Apply volume ramping to the generated waveform to remove click and pops
        auto rampFrames = volumeRampDuration * rawStream->sampleRate;
        auto destination = waveBuffer.data() + mixCursor;

        if (mix) {
            // Mix the samples to the buffer
            for (size_t i = 0; i < generatedFrames; i++) {
                // Calculate the ramp factor based on the current frame position
                auto rampFactor = 1.0f;
                if (i < rampFrames) {
                    rampFactor = (float)i / rampFrames;
                } else if (i >= generatedFrames - rampFrames) {
                    rampFactor = (float)(generatedFrames - i) / rampFrames;
                }

                destination[i] += noteBuffer[i] * rampFactor; // apply the ramp factor to the sample and mix it with the destination buffer
            }

            AUDIO_DEBUG_PRINT("Waveform = %i, frames requested = %llu, frames mixed = %llu", waveformType, neededFrames, generatedFrames);
        } else {
            // Copy the samples to the buffer
            for (size_t i = 0; i < generatedFrames; i++) {
                // Calculate the ramp factor based on the current frame position
                auto rampFactor = 1.0f;
                if (i < rampFrames) {
                    rampFactor = (float)i / rampFrames;
                } else if (i >= generatedFrames - rampFrames) {
                    rampFactor = (float)(generatedFrames - i) / rampFrames;
                }

                destination[i] = noteBuffer[i] * rampFactor; // apply the ramp factor to the sample
            }

            AUDIO_DEBUG_PRINT("Waveform = %i, frames requested = %llu, frames generated = %llu", waveformType, neededFrames, generatedFrames);
        }
    }

    /// @brief Sets the frequency of the waveform
    /// @param frequency The frequency of the waveform
    void SetFrequency(double frequency) {
        maResult = ma_waveform_set_frequency(&maWaveform, frequency);

        AUDIO_DEBUG_CHECK(maResult == MA_SUCCESS);
    }

    /// @brief Sends the buffer for playback
    void PushBufferForPlayback() {
        if (!waveBuffer.empty()) {
            rawStream->PushMonoSampleFrames(waveBuffer.data(), waveBuffer.size(), panning);

            AUDIO_DEBUG_PRINT("Sent %llu samples for playback", waveBuffer.size());

            waveBuffer.clear(); // set the buffer size to zero
            mixCursor = 0;      // reset the cursor
        }
    }

    /// @brief Waits for any playback to complete
    void AwaitPlaybackCompletion() {
        if (background)
            return; // no need to wait

        auto timeSec = rawStream->GetTimeRemaining() * 0.95 - 0.25; // per original QB64 behavior

        AUDIO_DEBUG_PRINT("Waiting %f seconds for playback to complete", timeSec);

        if (timeSec > 0)
            sub__delay(timeSec); // we are using sub_delay() because ON TIMER and other events may need to be called while we are waiting

        AUDIO_DEBUG_PRINT("Playback complete");
    }

  public:
    // Delete default, copy and move constructors and assignments
    PSG() = delete;
    PSG(const PSG &) = delete;
    PSG &operator=(const PSG &) = delete;
    PSG &operator=(PSG &&) = delete;
    PSG(PSG &&) = delete;

    /// @brief The only constructor
    /// @param pRawStream A valid RawStream object pointer. This cannot be NULL
    /// @param pWaveform A valid Waveform object pointer. This cannot be NULL
    PSG(RawStream *pRawStream) {
        rawStream = pRawStream; // save the RawStream object pointer
        mixCursor = 0;
        volumeRampDuration = DEFAULT_VOLUME_RAMP_DURATION;
        background = playIt = false; // default to foreground playback
        tempo = DEFAULT_TEMPO;
        octave = DEFAULT_OCTAVE;
        length = DEFAULT_LENGTH;
        pause = DEFAULT_PAUSE;
        panning = PAN_CENTER;
        duration = 0;
        dots = 0;
        ZERO_VARIABLE(currentState);

        maWaveformConfig = ma_waveform_config_init(ma_format::ma_format_f32, 1, rawStream->sampleRate, ma_waveform_type::ma_waveform_type_square,
                                                   DEFAULT_MML_VOLUME / MAX_MML_VOLUME, DEFAULT_FREQUENCY);
        maResult = ma_waveform_init(&maWaveformConfig, &maWaveform);
        AUDIO_DEBUG_CHECK(maResult == MA_SUCCESS);
        maNoiseConfig = ma_noise_config_init(ma_format::ma_format_f32, 1, ma_noise_type::ma_noise_type_white, 0, DEFAULT_MML_VOLUME / MAX_MML_VOLUME);
        maResult = ma_noise_init(&maNoiseConfig, NULL, &maNoise);
        AUDIO_DEBUG_CHECK(maResult == MA_SUCCESS);

        SetWaveformType(DEFAULT_WAVEFORM_TYPE); // this calls the underlying miniaudio API

        AUDIO_DEBUG_PRINT("PSG initialized @ %uHz", maWaveform.config.sampleRate);
    }

    /// @brief This just frees the waveform buffer and cleans up the waveform resources
    ~PSG() {
        ma_noise_uninit(&maNoise, NULL); // destroy miniaudio noise
        ma_waveform_uninit(&maWaveform); // destroy miniaudio waveform

        AUDIO_DEBUG_PRINT("PSG destroyed");
    }

    /// @brief Sets the waveform type
    /// @param type The waveform type. See Waveform::Type
    void SetWaveformType(WaveformType waveType) {
        switch (waveType) {
        case WaveformType::TRIANGLE:
            maResult = ma_waveform_set_type(&maWaveform, ma_waveform_type::ma_waveform_type_triangle);
            break;

        case WaveformType::SAWTOOTH:
            maResult = ma_waveform_set_type(&maWaveform, ma_waveform_type::ma_waveform_type_sawtooth);
            break;

        case WaveformType::SINE:
            maResult = ma_waveform_set_type(&maWaveform, ma_waveform_type::ma_waveform_type_sine);
            break;

        case WaveformType::SQUARE:
            maResult = ma_waveform_set_type(&maWaveform, ma_waveform_type::ma_waveform_type_square);
            break;
        }

        AUDIO_DEBUG_CHECK(maResult == MA_SUCCESS);

        waveformType = waveType;

        AUDIO_DEBUG_PRINT("Waveform type set to %i", waveformType);
    }

    /// @brief Sets the amplitude of the waveform
    /// @param amplitude The amplitude of the waveform
    void SetAmplitude(double amplitude) {
        maResult = ma_waveform_set_amplitude(&maWaveform, amplitude);
        AUDIO_DEBUG_CHECK(maResult == MA_SUCCESS);
        maResult = ma_noise_set_amplitude(&maNoise, amplitude);
        AUDIO_DEBUG_CHECK(maResult == MA_SUCCESS);

        AUDIO_DEBUG_PRINT("Amplitude set to %lf", amplitude);
    }

    /// @brief Set the PSG panning value
    /// @param value A number between -1.0 to 1.0. Where 0.0 is center
    void SetPanning(float value) {
        panning = value;

        AUDIO_DEBUG_PRINT("Panning set to %f", panning);
    }

    /// @brief Plays a typical retro PC speaker BEEP sound. The volume, waveform and background mode can be changed using PLAY
    void Beep() {
        SetFrequency(BEEP_FREQUENCY);
        waveBuffer.assign((size_t)(BEEP_DURATION * rawStream->sampleRate), 0.0f);
        GenerateWaveform(BEEP_WAVEFORM_DURATION);
        PushBufferForPlayback();
        AwaitPlaybackCompletion(); // await playback to complete if we are in MF mode
    }

    /// @brief Emulates a PC speaker sound. The volume, waveform and background mode can be changed using PLAY
    void Sound(double frequency, double lengthInClockTicks) {
        SetFrequency(frequency);
        auto soundDuration = lengthInClockTicks / 18.2;
        waveBuffer.assign((size_t)(soundDuration * rawStream->sampleRate), 0.0f);
        GenerateWaveform(soundDuration);
        PushBufferForPlayback();
        AwaitPlaybackCompletion(); // await playback to complete if we are in MF mode
    }

    /// @brief This is an MML parser that implements the QB64 MML spec and more
    /// https://qb64phoenix.com/qb64wiki/index.php/PLAY
    /// http://vgmpf.com/Wiki/index.php?title=Music_Macro_Language
    /// https://en.wikipedia.org/wiki/Music_Macro_Language
    /// https://sneslab.net/wiki/Music_Macro_Language
    /// http://www.mirbsd.org/htman/i386/man4/speaker.htm
    /// https://www.qbasic.net/en/reference/qb11/Statement/PLAY-006.htm
    /// https://woolyss.com/chipmusic-mml.php
    /// frequency = 440.0 * pow(2.0, (note + (octave - 2.0) * 12.0 - 9.0) / 12.0)
    // const float FREQUENCY_TABLE[] = {
    //	0,
    //	//1       2         3         4         5         6         7         8         9         10        11        12
    //	//C       C#        D         D#        E         F         F#        G         G#        A         A#        B
    //	16.35f,   17.32f,   18.35f,   19.45f,   20.60f,   21.83f,   23.12f,   24.50f,   25.96f,   27.50f,   29.14f,   30.87f,   // Octave 0
    //	32.70f,   34.65f,   36.71f,   38.89f,   41.20f,   43.65f,   46.25f,   49.00f,   51.91f,   55.00f,   58.27f,   61.74f,   // Octave 1
    //	65.41f,   69.30f,   73.42f,   77.78f,   82.41f,   87.31f,   92.50f,   98.00f,   103.83f,  110.00f,  116.54f,  123.47f,  // Octave 2
    //	130.81f,  138.59f,  146.83f,  155.56f,  164.81f,  174.62f,  185.00f,  196.00f,  207.65f,  220.00f,  233.08f,  246.94f,  // Octave 3
    //	261.63f,  277.18f,  293.67f,  311.13f,  329.63f,  349.23f,  370.00f,  392.00f,  415.31f,  440.00f,  466.17f,  493.89f,  // Octave 4
    //	523.25f,  554.37f,  587.33f,  622.26f,  659.26f,  698.46f,  739.99f,  783.99f,  830.61f,  880.00f,  932.33f,  987.77f,  // Octave 5
    //	1046.51f, 1108.74f, 1174.67f, 1244.51f, 1318.52f, 1396.92f, 1479.99f, 1567.99f, 1661.23f, 1760.01f, 1864.66f, 1975.54f, // Octave 6
    //	2093.02f, 2217.47f, 2349.33f, 2489.03f, 2637.03f, 2793.84f, 2959.97f, 3135.98f, 3322.45f, 3520.02f, 3729.33f, 3951.09f, // Octave 7
    // };
    /// @param mml A string containing the MML tune
    void Play(const qbs *mml) {
        if (!mml || !mml->len) // exit if string is empty
            return;

        auto currentChar = 0;
        auto processedChar = 0;
        auto numberEntered = 0;
        int64_t number = 0;
        bool noteShifted = false;
        auto noteOffset = 0;
        auto followUp = 0;
        auto noDotDuration = 1.0 / (tempo / 60.0) * (4.0 / length);

        playIt = false;

        stateStack.push({mml->chr, mml->len}); // push the string to the state stack

        // Process until our state stack is empty
        while (!stateStack.empty()) {
            // Pop and use the top item in the state stack
            currentState = stateStack.top();
            stateStack.pop();

            while ((currentState.length--) || followUp) {
                if (currentState.length < 0) {
                    currentChar = ' ';
                    goto follow_up;
                }

                currentChar = *currentState.byte++;
                if (isspace(currentChar))
                    continue;

                processedChar = toupper(currentChar);

                if (processedChar == 'X') { // "X" + VARPTR$()
                    // A minimum of 3 bytes is need to read the address
                    if (currentState.length < 3) {
                        error(QB_ERROR_ILLEGAL_FUNCTION_CALL);
                        return;
                    }

                    // Read type byte
                    currentChar = *currentState.byte++;
                    currentState.length--;

                    // Read offset within DBLOCK
                    auto offset = *(uint16_t *)currentState.byte;
                    currentState.byte += 2;
                    currentState.length -= 2;

                    stateStack.push(currentState); // push the current state to the stack

                    // Set new state
                    currentState.byte = &cmem[1280] + (cmem[1280 + offset + 3] * 256 + cmem[1280 + offset + 2]);
                    currentState.length = cmem[1280 + offset + 1] * 256 + cmem[1280 + offset + 0];

                    continue;
                } else if (currentChar == '=') { // "=" + VARPTR$()
                    if (dots) {
                        error(QB_ERROR_ILLEGAL_FUNCTION_CALL);
                        return;
                    }

                    if (numberEntered) {
                        error(QB_ERROR_ILLEGAL_FUNCTION_CALL);
                        return;
                    }

                    numberEntered = 2;

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

                    if (currentState.length < 3) {
                        error(QB_ERROR_ILLEGAL_FUNCTION_CALL);
                        return;
                    }

                    currentChar = *currentState.byte++; // read type byte
                    currentState.length--;

                    auto x = *(uint16_t *)currentState.byte; // read offset within DBLOCK
                    currentState.byte += 2;
                    currentState.length -= 2;

                    // note: allowable _BIT type variables in VARPTR$ are all at a byte offset and are all
                    //      padded until the next byte
                    int64_t d = 0;

                    switch (currentChar) {
                    case 1:
                        d = *(char *)(dblock + x);
                        break;
                    case (1 + 128):
                        d = *(uint8_t *)(dblock + x);
                        break;
                    case 2:
                        d = *(int16_t *)(dblock + x);
                        break;
                    case (2 + 128):
                        d = *(uint16_t *)(dblock + x);
                        break;
                    case 4:
                        d = *(float *)(dblock + x);
                        break;
                    case 5:
                        d = *(int64_t *)(dblock + x);
                        break;
                    case (5 + 128):
                        d = *(int64_t *)(dblock + x); // unsigned conversion is unsupported!
                        break;
                    case 6:
                        d = *(long double *)(dblock + x);
                        break;
                    case 8:
                        d = *(double *)(dblock + x);
                        break;
                    case 20:
                        d = *(int32_t *)(dblock + x);
                        break;
                    case (20 + 128):
                        d = *(uint32_t *)(dblock + x);
                        break;
                    default:
                        // bit type?
                        if ((currentChar & 64) == 0) {
                            error(QB_ERROR_ILLEGAL_FUNCTION_CALL);
                            return;
                        }

                        auto x2 = currentChar & 63;

                        if (x2 > 56) {
                            error(QB_ERROR_ILLEGAL_FUNCTION_CALL);
                            return;
                        } // valid number of bits?

                        // create a mask
                        auto mask = (((int64_t)1) << x2) - 1;
                        auto i64num = (*(int64_t *)(dblock + x)) & mask;

                        // signed?
                        if (currentChar & 128) {
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
                        error(QB_ERROR_ILLEGAL_FUNCTION_CALL); // out of range value!
                        return;
                    }

                    number = llround(d);

                    continue;
                } else if (currentChar >= '0' && currentChar <= '9') {
                    if (dots || numberEntered == 2) {
                        error(QB_ERROR_ILLEGAL_FUNCTION_CALL);
                        return;
                    }

                    if (!numberEntered) {
                        number = 0;
                        numberEntered = 1;
                    }

                    number = number * 10 + currentChar - 48;

                    continue;
                } else if (currentChar == '.') {
                    if (followUp != 7 && followUp != 1 && followUp != 4) {
                        error(QB_ERROR_ILLEGAL_FUNCTION_CALL);
                        return;
                    }

                    dots++;

                    continue;
                }

            follow_up:
                if (followUp == 10) { // Q...
                    if (!numberEntered) {
                        error(QB_ERROR_ILLEGAL_FUNCTION_CALL);
                        return;
                    }

                    numberEntered = 0;

                    if (number > 100) { // 0 - 100 ms
                        error(QB_ERROR_ILLEGAL_FUNCTION_CALL);
                        return;
                    }

                    volumeRampDuration = (float)number / 1000.0f;
                    followUp = 0;

                    if (currentState.length < 0)
                        break;
                } else if (followUp == 9) { // @...
                    if (!numberEntered) {
                        error(QB_ERROR_ILLEGAL_FUNCTION_CALL);
                        return;
                    }

                    numberEntered = 0;

                    if ((WaveformType)number <= WaveformType::NONE || (WaveformType)number >= WaveformType::COUNT) {
                        error(QB_ERROR_ILLEGAL_FUNCTION_CALL);
                        return;
                    }

                    SetWaveformType((WaveformType)number);

                    followUp = 0;

                    if (currentState.length < 0)
                        break;
                } else if (followUp == 8) { // V...
                    if (!numberEntered) {
                        error(QB_ERROR_ILLEGAL_FUNCTION_CALL);
                        return;
                    }

                    numberEntered = 0;

                    if (number > MAX_MML_VOLUME) {
                        error(QB_ERROR_ILLEGAL_FUNCTION_CALL);
                        return;
                    }

                    SetAmplitude(number / MAX_MML_VOLUME);

                    followUp = 0;

                    if (currentState.length < 0)
                        break;
                } else if (followUp == 7) { // P...
                    if (numberEntered) {
                        numberEntered = 0;
                        if (number < 1 || number > 64) {
                            error(QB_ERROR_ILLEGAL_FUNCTION_CALL);
                            return;
                        }
                        duration = 1.0 / (tempo / 60.0) * (4.0 / ((double)number));
                    } else {
                        duration = noDotDuration;
                    }

                    auto dotDuration = duration;

                    for (auto i = 0; i < dots; i++) {
                        dotDuration /= 2.0;
                        duration += dotDuration;
                    }

                    dots = 0;

                    auto noteFrames = (ma_uint64)(duration * rawStream->sampleRate);

                    if ((mixCursor + noteFrames) > waveBuffer.size()) {
                        waveBuffer.resize(mixCursor + noteFrames, 0.0f);
                    }

                    if (currentChar != ',') {
                        mixCursor += noteFrames;
                    }

                    playIt = true;
                    followUp = 0;

                    if (currentChar == ',')
                        continue;

                    if (currentState.length < 0)
                        break;
                } else if (followUp == 6) { // T...
                    if (!numberEntered) {
                        error(QB_ERROR_ILLEGAL_FUNCTION_CALL);
                        return;
                    }

                    numberEntered = 0;

                    if (number < MIN_TEMPO || number > MAX_TEMPO) {
                        number = 120;
                    }

                    tempo = number;
                    noDotDuration = 1.0 / (tempo / 60.0) * (4.0 / length);
                    followUp = 0;

                    if (currentState.length < 0)
                        break;
                } else if (followUp == 5) { // M...
                    if (numberEntered) {
                        error(QB_ERROR_ILLEGAL_FUNCTION_CALL);
                        return;
                    }

                    switch (processedChar) {
                    case 'L': // legato
                        pause = 0.0;
                        break;
                    case 'N': // normal
                        pause = 1.0 / 8.0;
                        break;
                    case 'S': // staccato
                        pause = 1.0 / 4.0;
                        break;
                    case 'B': // background
                        if (!background) {
                            if (playIt) { // play pending buffer in foreground before we switch to background
                                playIt = false;
                                PushBufferForPlayback();
                                AwaitPlaybackCompletion();
                            }
                            background = true;
                        }
                        break;
                    case 'F': // foreground
                        background = false;
                        break;
                    default:
                        error(QB_ERROR_ILLEGAL_FUNCTION_CALL);
                        return;
                    }

                    followUp = 0;

                    continue;
                } else if (followUp == 4) { // N...
                    if (!numberEntered) {
                        error(QB_ERROR_ILLEGAL_FUNCTION_CALL);
                        return;
                    }

                    numberEntered = 0;

                    if (number > 84) {
                        error(QB_ERROR_ILLEGAL_FUNCTION_CALL);
                        return;
                    }

                    noteOffset = -45 + number;

                    goto follow_up_1;
                } else if (followUp == 3) { // O...
                    if (!numberEntered) {
                        error(QB_ERROR_ILLEGAL_FUNCTION_CALL);
                        return;
                    }

                    numberEntered = 0;

                    if (number > MAX_OCTAVE) {
                        error(QB_ERROR_ILLEGAL_FUNCTION_CALL);
                        return;
                    }

                    octave = number;

                    followUp = 0;

                    if (currentState.length < 0)
                        break;
                } else if (followUp == 2) { // L...
                    if (!numberEntered) {
                        error(QB_ERROR_ILLEGAL_FUNCTION_CALL);
                        return;
                    }

                    numberEntered = 0;

                    if (number < MIN_LENGTH || number > MAX_LENGTH) {
                        error(QB_ERROR_ILLEGAL_FUNCTION_CALL);
                        return;
                    }

                    length = number;
                    noDotDuration = 1.0 / (tempo / 60.0) * (4.0 / length);
                    followUp = 0;

                    if (currentState.length < 0)
                        break;
                } else if (followUp == 1) { // A-G...
                    if (currentChar == '-') {
                        if (noteShifted || numberEntered) {
                            error(QB_ERROR_ILLEGAL_FUNCTION_CALL);
                            return;
                        }

                        noteShifted = true;
                        noteOffset--;

                        continue;
                    }
                    if (currentChar == '+' || currentChar == '#') {
                        if (noteShifted || numberEntered) {
                            error(QB_ERROR_ILLEGAL_FUNCTION_CALL);
                            return;
                        }

                        noteShifted = true;
                        noteOffset++;

                        continue;
                    }

                follow_up_1:
                    if (numberEntered) {
                        numberEntered = 0;

                        if (number < 0 || number > 64) {
                            error(QB_ERROR_ILLEGAL_FUNCTION_CALL);
                            return;
                        }

                        if (!number)
                            duration = noDotDuration;
                        else
                            duration = 1.0 / (tempo / 60.0) * (4.0 / ((double)number));
                    } else {
                        duration = noDotDuration;
                    }

                    auto dotDuration = duration;

                    for (auto i = 0; i < dots; i++) {
                        dotDuration /= 2.0;
                        duration += dotDuration;
                    }

                    dots = 0;

                    SetFrequency(pow(2.0, ((double)noteOffset) / 12.0) * 440.0);

                    auto noteFrames = (ma_uint64)(duration * rawStream->sampleRate);

                    if (mixCursor + noteFrames > waveBuffer.size()) {
                        waveBuffer.resize(mixCursor + noteFrames, 0.0f);
                    }

                    if (noteOffset > -45) // this ensures that we correctly handle N0 as rest
                        GenerateWaveform(duration * (1.0 - pause), mixCursor != waveBuffer.size());

                    if (currentChar != ',') {
                        mixCursor += noteFrames;
                    }

                    playIt = true;
                    noteShifted = false;
                    followUp = 0;

                    if (currentChar == ',')
                        continue;

                    if (currentState.length < 0)
                        break;
                }

                if (processedChar >= 'A' && processedChar <= 'G') {
                    switch (processedChar) {
                    case 'A':
                        noteOffset = 9;
                        break;
                    case 'B':
                        noteOffset = 11;
                        break;
                    case 'C':
                        noteOffset = 0;
                        break;
                    case 'D':
                        noteOffset = 2;
                        break;
                    case 'E':
                        noteOffset = 4;
                        break;
                    case 'F':
                        noteOffset = 5;
                        break;
                    case 'G':
                        noteOffset = 7;
                        break;
                    }
                    noteOffset = noteOffset + (octave - 2) * 12 - 9;
                    followUp = 1;
                    continue;
                } else if (processedChar == 'L') { // length
                    followUp = 2;
                    continue;
                } else if (processedChar == 'M') { // timing
                    followUp = 5;
                    continue;
                } else if (processedChar == 'N') { // note 'n'
                    followUp = 4;
                    continue;
                } else if (processedChar == 'O') { // octave
                    followUp = 3;
                    continue;
                } else if (processedChar == 'T') { // tempo
                    followUp = 6;
                    continue;
                } else if (processedChar == '<') { // octave --
                    --octave;
                    if (octave < 0)
                        octave = 0;
                    continue;
                } else if (processedChar == '>') { // octave ++
                    ++octave;
                    if (octave > 6)
                        octave = 6;
                    continue;
                } else if (processedChar == 'P' || processedChar == 'R') { // rest
                    followUp = 7;
                    continue;
                } else if (processedChar == 'V') { // volume
                    followUp = 8;
                    continue;
                } else if (processedChar == '@') { // waveform
                    followUp = 9;
                    continue;
                } else if (processedChar == 'Q') { // vol-ramp
                    followUp = 10;
                    continue;
                }

                error(QB_ERROR_ILLEGAL_FUNCTION_CALL);
                return;
            }

            if (numberEntered || followUp) {
                error(QB_ERROR_ILLEGAL_FUNCTION_CALL); // unhandled data
                return;
            }

            if (playIt) {
                PushBufferForPlayback();
                AwaitPlaybackCompletion();
            }
        }
    }
};

/// <summary>
/// Sound handle type
/// This describes every sound the system will ever play (including raw streams).
/// </summary>
struct SoundHandle {
    /// @brief Type of sound.
    /// NONE: No sound or internal sound whose buffer is managed by the QBPE audio engine.
    /// STATIC: Static sounds that are completely managed by miniaudio.
    /// RAW: Raw sound stream that is managed by the QBPE audio engine
    enum class Type { NONE, STATIC, RAW };

    bool isUsed;                                // Is this handle in active use?
    Type type;                                  // Type of sound (see Type enum above)
    bool autoKill;                              // Do we need to auto-clean this sample / stream after playback is done?
    ma_sound maSound;                           // miniaudio sound
    ma_uint32 maFlags;                          // miniaudio flags that were used when initializing the sound
    ma_decoder_config maDecoderConfig;          // miniaudio decoder configuration
    ma_decoder *maDecoder;                      // this is used for files that are loaded directly from memory
    intptr_t bufferKey;                         // a key that will uniquely identify the data the decoder will use
    ma_audio_buffer_config maAudioBufferConfig; // miniaudio buffer configuration
    ma_audio_buffer *maAudioBuffer;             // this is used for user created audio buffers (memory is managed by miniaudio)
    RawStream *rawStream;                       // Raw sample frame queue
    void *memLockOffset;                        // This is a pointer from new_mem_lock()
    uint64_t memLockId;                           // This is mem_lock_id created by new_mem_lock()

    // Delete copy and move constructors and assignments
    SoundHandle(const SoundHandle &) = delete;
    SoundHandle &operator=(const SoundHandle &) = delete;
    SoundHandle(SoundHandle &&) = delete;
    SoundHandle &operator=(SoundHandle &&) = delete;

    /// <summary>
    ///	Just initializes some important members.
    /// 'inUse' will be set to true by CreateHandle().
    /// This is done here, as well as slightly differently in CreateHandle() for safety.
    /// </summary>
    SoundHandle() {
        isUsed = false;
        type = Type::NONE;
        autoKill = false;
        ZERO_VARIABLE(maSound);
        maFlags = MA_SOUND_FLAG_NO_PITCH | MA_SOUND_FLAG_NO_SPATIALIZATION | MA_SOUND_FLAG_WAIT_INIT;
        ZERO_VARIABLE(maDecoderConfig);
        maDecoder = nullptr;
        bufferKey = 0;
        ZERO_VARIABLE(maAudioBufferConfig);
        maAudioBuffer = nullptr;
        rawStream = nullptr;
        memLockOffset = nullptr;
        memLockId = INVALID_MEM_LOCK;
    }
};

/// <summary>
///	Type will help us keep track of the audio engine state
/// </summary>
struct AudioEngine {
    bool isInitialized;                                 // this is set to true if we were able to initialize miniaudio and allocated all required resources
    bool initializationFailed;                          // this is set to true if a past initialization attempt failed
    ma_resource_manager_config maResourceManagerConfig; // miniaudio resource manager configuration
    ma_resource_manager maResourceManager;              // miniaudio resource manager
    ma_engine_config maEngineConfig;                    // miniaudio engine configuration (will be used to pass in the resource manager)
    ma_engine maEngine;                                 // this is the primary miniaudio engine 'context'. Everything happens using this!
    ma_result maResult;                                 // this is the result of the last miniaudio operation (used for trapping errors)
    ma_uint32 sampleRate;                               // sample rate used by the miniaudio engine
    int32_t sndInternal;                                // internal sound handle that we will use for Play(), Beep() & Sound()
    PSG *psg;                                           // PSG object that we will use to generate sound for Play(), Beep() & Sound()
    int32_t sndInternalRaw;                             // internal sound handle that we will use for the QB64 'handle-less' raw stream
    std::vector<SoundHandle *> soundHandles;            // this is the audio handle list used by the engine and by everything else
    int32_t lowestFreeHandle;                           // this is the lowest handle then was recently freed. We'll start checking for free handles from here
    BufferMap bufferMap;                                // this is used to keep track of and manage memory used by 'in-memory' sound files

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
        ZERO_VARIABLE(maResourceManagerConfig);
        ZERO_VARIABLE(maResourceManager);
        ZERO_VARIABLE(maEngineConfig);
        ZERO_VARIABLE(maEngine);
        maResult = ma_result::MA_SUCCESS;
        sampleRate = 0;
        sndInternal = sndInternalRaw = -1; // should not use INVALID_SOUND_HANDLE here
        psg = nullptr;
        lowestFreeHandle = 0;
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
    int32_t CreateHandle() {
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
            // Ideally this should execute in extremely few (if at all) scenarios
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
        soundHandles[h]->type = SoundHandle::Type::NONE;
        soundHandles[h]->autoKill = false;
        ZERO_VARIABLE(soundHandles[h]->maSound);
        // We do not use pitch shifting, so this will give a little performance boost
        // Spatialization is disabled by default but will be enabled on the fly if required
        soundHandles[h]->maFlags = MA_SOUND_FLAG_NO_PITCH | MA_SOUND_FLAG_NO_SPATIALIZATION | MA_SOUND_FLAG_WAIT_INIT;
        soundHandles[h]->maDecoder = nullptr;
        soundHandles[h]->bufferKey = 0;
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
    void ReleaseHandle(int32_t handle) {
        if (isInitialized && handle >= 0 && handle < soundHandles.size() && soundHandles[handle]->isUsed) {
            // Sound type specific cleanup
            switch (soundHandles[handle]->type) {
            case SoundHandle::Type::STATIC:
                ma_sound_uninit(&soundHandles[handle]->maSound);

                break;

            case SoundHandle::Type::RAW:
                RawStreamDestroy(soundHandles[handle]->rawStream);
                soundHandles[handle]->rawStream = nullptr;

                break;

            case SoundHandle::Type::NONE:
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
                bufferMap.Release(soundHandles[handle]->bufferKey);
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
            soundHandles[handle]->type = SoundHandle::Type::NONE;

            // Save the free handle to lowestFreeHandle if it is lower than lowestFreeHandle
            if (handle < lowestFreeHandle)
                lowestFreeHandle = handle;

            AUDIO_DEBUG_PRINT("Sound handle %i marked as free", handle);
        }
    }
};

// This keeps track of the audio engine state
static AudioEngine audioEngine;

/// @brief Initializes the PSG object and it's RawStream object. This only happens once. Subsequent calls to this will return true
/// @return Returns true if both objects were successfully created
static bool InitializePSG() {
    if (!audioEngine.isInitialized || audioEngine.sndInternal != 0)
        return false;

    // Kickstart the raw stream and PSG if it is not already
    if (!audioEngine.soundHandles[audioEngine.sndInternal]->rawStream) {
        audioEngine.soundHandles[audioEngine.sndInternal]->rawStream =
            RawStreamCreate(&audioEngine.maEngine, &audioEngine.soundHandles[audioEngine.sndInternal]->maSound);
        if (!audioEngine.soundHandles[audioEngine.sndInternal]->rawStream) {
            AUDIO_DEBUG_PRINT("Failed to create rawStream object for PSG");
            return false;
        }
        audioEngine.soundHandles[audioEngine.sndInternal]->type = SoundHandle::Type::RAW;

        if (!audioEngine.psg) {
            audioEngine.psg = new PSG(audioEngine.soundHandles[audioEngine.sndInternal]->rawStream);
            if (!audioEngine.psg) {
                AUDIO_DEBUG_PRINT("Failed to create PSG object");
                RawStreamDestroy(audioEngine.soundHandles[audioEngine.sndInternal]->rawStream);
                audioEngine.soundHandles[audioEngine.sndInternal]->rawStream = nullptr;
                audioEngine.soundHandles[audioEngine.sndInternal]->type = SoundHandle::Type::NONE;

                return false;
            }
        }
    }

    return (audioEngine.soundHandles[audioEngine.sndInternal]->rawStream && audioEngine.psg);
}

/// @brief This generates a sound at the specified frequency for the specified amount of time
/// @param frequency Sound frequency
/// @param lengthInClockTicks Duration in clock ticks. There are 18.2 clock ticks per second
void sub_sound(double frequency, double lengthInClockTicks, double volume, double panning, int32_t waveform, int32_t passed) {
    if (is_error_pending() || lengthInClockTicks == 0.0 || !InitializePSG())
        return;

    if ((frequency < 37.0 && frequency != 0) || frequency > 32767.0 || lengthInClockTicks < 0.0 || lengthInClockTicks > 65535.0) {
        error(QB_ERROR_ILLEGAL_FUNCTION_CALL);
        return;
    }

    if (passed & 1) {
        if (volume < PSG::MIN_VOLUME || volume > PSG::MAX_VOLUME) {
            error(QB_ERROR_ILLEGAL_FUNCTION_CALL);
            return;
        }
        audioEngine.psg->SetAmplitude(volume);
    }

    if (passed & 2) {
        if (panning < PSG::PAN_LEFT || panning > PSG::PAN_RIGHT) {
            error(QB_ERROR_ILLEGAL_FUNCTION_CALL);
            return;
        }
        audioEngine.psg->SetPanning((float)panning);
    }

    if (passed & 4) {
        if ((PSG::WaveformType)waveform <= PSG::WaveformType::NONE || (PSG::WaveformType)waveform >= PSG::WaveformType::COUNT) {
            error(QB_ERROR_ILLEGAL_FUNCTION_CALL);
            return;
        }
        audioEngine.psg->SetWaveformType((PSG::WaveformType)waveform);
    }

    audioEngine.psg->Sound(frequency, lengthInClockTicks);
}

/// @brief This generates a default 'beep' sound
void sub_beep() {
    if (is_error_pending() || !InitializePSG())
        return;

    audioEngine.psg->Beep();
}

/// @brief This was designed to returned the number of notes in the background music queue.
/// However, here we'll just return the number of sample frame remaining to play when Play(), Sound() or Beep() are used
/// @param ignore Well, it's ignored
/// @return Returns the number of sample frames left to play for Play(), Sound() & Beep()
int32_t func_play(int32_t ignore) {
    if (audioEngine.isInitialized && audioEngine.sndInternal == 0 && audioEngine.soundHandles[audioEngine.sndInternal]->rawStream) {
        if (ignore)
            return lround(audioEngine.soundHandles[audioEngine.sndInternal]->rawStream->GetTimeRemaining());
        else
            return (int32_t)audioEngine.soundHandles[audioEngine.sndInternal]->rawStream->GetSampleFramesRemaining();
    }

    return 0;
}

/// @brief Processes and plays the MML specified in the string
/// @param str The string to play
void sub_play(const qbs *str) {
    if (is_error_pending() || !InitializePSG())
        return;

    audioEngine.psg->Play(str);
}

/// <summary>
/// This returns the sample rate from ma engine if ma is initialized.
/// </summary>
/// <returns>miniaudio sample rate</returns>
int32_t func__sndrate() { return audioEngine.sampleRate; }

/// @brief Creates a ma_decoder and ma_sound from a memory buffer for a valid sound handle
/// @param buffer A raw pointer to the sound file in memory
/// @param size The size of the file in memory
/// @param handle A valid sound handle
/// @return MA_SUCCESS if successful. Else, a valid ma_result
static ma_result InitializeSoundFromMemory(const void *buffer, size_t size, int32_t handle) {
    if (!IS_SOUND_HANDLE_VALID(handle) || audioEngine.soundHandles[handle]->maDecoder || !buffer || !size)
        return MA_INVALID_ARGS;

    audioEngine.soundHandles[handle]->maDecoder = new ma_decoder(); // allocate and zero memory
    if (!audioEngine.soundHandles[handle]->maDecoder) {
        AUDIO_DEBUG_PRINT("Failed to allocate memory for miniaudio decoder");
        return MA_OUT_OF_MEMORY;
    }

    // Setup the decoder & attach the custom backed vtables
    audioEngine.soundHandles[handle]->maDecoderConfig = ma_decoder_config_init_default();
    AudioEngineAttachCustomBackendVTables(&audioEngine.soundHandles[handle]->maDecoderConfig);
    audioEngine.soundHandles[handle]->maDecoderConfig.sampleRate = audioEngine.sampleRate;

    audioEngine.maResult = ma_decoder_init_memory(buffer, size, &audioEngine.soundHandles[handle]->maDecoderConfig,
                                                  audioEngine.soundHandles[handle]->maDecoder); // initialize the decoder
    if (audioEngine.maResult != MA_SUCCESS) {
        delete audioEngine.soundHandles[handle]->maDecoder;
        audioEngine.soundHandles[handle]->maDecoder = nullptr;
        AUDIO_DEBUG_PRINT("Error %i: failed to initialize miniaudio decoder", audioEngine.maResult);
        return audioEngine.maResult;
    }

    // Finally, load the sound as a data source
    audioEngine.maResult = ma_sound_init_from_data_source(&audioEngine.maEngine, audioEngine.soundHandles[handle]->maDecoder,
                                                          audioEngine.soundHandles[handle]->maFlags, NULL, &audioEngine.soundHandles[handle]->maSound);

    if (audioEngine.maResult != MA_SUCCESS) {
        ma_decoder_uninit(audioEngine.soundHandles[handle]->maDecoder);
        delete audioEngine.soundHandles[handle]->maDecoder;
        audioEngine.soundHandles[handle]->maDecoder = nullptr;
        AUDIO_DEBUG_PRINT("Error %i: failed to initialize sound", audioEngine.maResult);
        return audioEngine.maResult;
    }

    return MA_SUCCESS;
}

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

    // Allocate a sound handle
    int32_t handle = audioEngine.CreateHandle();
    if (handle < 1) // We are not expected to open files with handle 0
        return INVALID_SOUND_HANDLE;

    // Set some handle properties
    audioEngine.soundHandles[handle]->type = SoundHandle::Type::STATIC;

    // Prepare the requirements string
    if (passed && requirements->len)
        qbs_set(reqs, qbs_ucase(requirements)); // Convert tmp str to perm str

    // Set the flags to specify how we want the audio file to be opened
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

        audioEngine.soundHandles[handle]->bufferKey = (intptr_t)fileName->chr;                                      // make a unique key and save it
        audioEngine.bufferMap.AddBuffer(fileName->chr, fileName->len, audioEngine.soundHandles[handle]->bufferKey); // make a copy of the buffer
        auto [buffer, bufferSize] = audioEngine.bufferMap.GetBuffer(audioEngine.soundHandles[handle]->bufferKey);   // get the buffer pointer and size
        audioEngine.maResult = InitializeSoundFromMemory(buffer, bufferSize, handle);                               // create the ma_sound
    } else {
        AUDIO_DEBUG_PRINT("Loading sound from file '%s'", fileNameZ->chr);
        qbs_set(fileNameZ, qbs_add(fileName, qbs_new_txt_len("\0", 1))); // s1 = filename + CHR$(0)

        // Forward the request to miniaudio to open the sound file
        audioEngine.maResult = ma_sound_init_from_file(&audioEngine.maEngine, filepath_fix_directory(fileNameZ), audioEngine.soundHandles[handle]->maFlags,
                                                       NULL, NULL, &audioEngine.soundHandles[handle]->maSound);
    }

    // If the sound failed to initialize, then free the handle and return INVALID_SOUND_HANDLE
    if (audioEngine.maResult != MA_SUCCESS) {
        AUDIO_DEBUG_PRINT("Error %i: failed to open sound", audioEngine.maResult);
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
        // If we have a raw stream then force it to push all its data to miniaudio
        // Note that this will take care of checking if the handle is a raw steam and other stuff
        // So it is completely safe to call it this way
        sub__sndrawdone(handle, true);

        if (audioEngine.soundHandles[handle]->type == SoundHandle::Type::RAW)
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
    if (!audioEngine.isInitialized || !IS_SOUND_HANDLE_VALID(src_handle) || audioEngine.soundHandles[src_handle]->type != SoundHandle::Type::STATIC)
        return INVALID_SOUND_HANDLE;

    int32_t dst_handle = INVALID_SOUND_HANDLE;

    // Miniaudio will not copy sounds attached to ma_audio_buffers so we'll handle the duplication ourselves
    // Sadly, since this involves a buffer copy there may be a delay before the sound can play (especially if the sound is lengthy)
    // The delay may be noticeable when _SNDPLAYCOPY is used multiple times on the a _SNDNEW sound
    if (audioEngine.soundHandles[src_handle]->maAudioBuffer) {
        AUDIO_DEBUG_PRINT("Doing custom sound copy for ma_audio_buffer");

        auto frames = audioEngine.soundHandles[src_handle]->maAudioBuffer->ref.sizeInFrames;
        auto channels = audioEngine.soundHandles[src_handle]->maAudioBuffer->ref.channels;
        auto format = audioEngine.soundHandles[src_handle]->maAudioBuffer->ref.format;

        // First create a new _SNDNEW sound with the same properties at the source
        dst_handle = func__sndnew(frames, channels, CHAR_BIT * ma_get_bytes_per_sample(format));
        if (dst_handle < 1)
            return INVALID_SOUND_HANDLE;

        // Next memcopy the samples from the source to the dest
        memcpy((void *)audioEngine.soundHandles[dst_handle]->maAudioBuffer->ref.pData, audioEngine.soundHandles[src_handle]->maAudioBuffer->ref.pData,
               frames * ma_get_bytes_per_frame(format, channels)); // naughty const void* casting, but should be OK
    } else if (audioEngine.soundHandles[src_handle]->maDecoder) {
        AUDIO_DEBUG_PRINT("Doing custom sound copy for ma_decoder");

        dst_handle = audioEngine.CreateHandle(); // allocate a sound handle
        if (dst_handle < 1)
            return INVALID_SOUND_HANDLE;

        audioEngine.soundHandles[dst_handle]->type = SoundHandle::Type::STATIC;                                       // set some handle properties
        audioEngine.soundHandles[dst_handle]->maFlags = audioEngine.soundHandles[src_handle]->maFlags;                // copy the flags
        audioEngine.soundHandles[dst_handle]->bufferKey = audioEngine.soundHandles[src_handle]->bufferKey;            // copy the BufferMap unique key
        audioEngine.bufferMap.AddRef(audioEngine.soundHandles[dst_handle]->bufferKey);                                // increase the reference count
        auto [buffer, bufferSize] = audioEngine.bufferMap.GetBuffer(audioEngine.soundHandles[dst_handle]->bufferKey); // get the buffer pointer and size
        audioEngine.maResult = InitializeSoundFromMemory(buffer, bufferSize, dst_handle);                             // create the ma_sound

        if (audioEngine.maResult != MA_SUCCESS) {
            audioEngine.bufferMap.Release(audioEngine.soundHandles[dst_handle]->bufferKey);
            audioEngine.soundHandles[dst_handle]->isUsed = false;
            AUDIO_DEBUG_PRINT("Error %i: failed to copy sound", audioEngine.maResult);

            return INVALID_SOUND_HANDLE;
        }
    } else {
        AUDIO_DEBUG_PRINT("Doing regular miniaudio sound copy");

        dst_handle = audioEngine.CreateHandle(); // allocate a sound handle
        if (dst_handle < 1)
            return INVALID_SOUND_HANDLE;

        audioEngine.soundHandles[dst_handle]->type = SoundHandle::Type::STATIC;                        // set some handle properties
        audioEngine.soundHandles[dst_handle]->maFlags = audioEngine.soundHandles[src_handle]->maFlags; // copy the flags

        // Initialize a new copy of the sound
        audioEngine.maResult = ma_sound_init_copy(&audioEngine.maEngine, &audioEngine.soundHandles[src_handle]->maSound,
                                                  audioEngine.soundHandles[dst_handle]->maFlags, NULL, &audioEngine.soundHandles[dst_handle]->maSound);

        // If the sound failed to copy, then free the handle and return INVALID_SOUND_HANDLE
        if (audioEngine.maResult != MA_SUCCESS) {
            audioEngine.soundHandles[dst_handle]->isUsed = false;
            AUDIO_DEBUG_PRINT("Error %i: failed to copy sound", audioEngine.maResult);

            return INVALID_SOUND_HANDLE;
        }
    }

    return dst_handle;
}

/// <summary>
/// This plays a sound designated by a sound handle.
/// </summary>
/// <param name="handle">A sound handle</param>
void sub__sndplay(int32_t handle) {
    if (audioEngine.isInitialized && IS_SOUND_HANDLE_VALID(handle) && audioEngine.soundHandles[handle]->type == SoundHandle::Type::STATIC) {
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

        AUDIO_DEBUG_PRINT("Playing sound %i", handle);
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
    auto dst_handle = func__sndcopy(src_handle);

    AUDIO_DEBUG_PRINT("Source handle = %i, destination handle = %i", src_handle, dst_handle);

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

        AUDIO_DEBUG_PRINT("Playing sound copy %i: volume %lf, 3D (%lf, %lf, %lf)", dst_handle, volume, x, y, z);
    }
}

/// <summary>
/// This is a "fire and forget" style of function.
/// The engine will manage the sound handle internally.
/// When the sound finishes playing, the handle will be put up for recycling.
/// Playback starts asynchronously.
/// </summary>
/// <param name="fileName">The is the name of the file to be played</param>
/// <param name="sync">This parameter is ignored</param>
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
    if (audioEngine.isInitialized && IS_SOUND_HANDLE_VALID(handle) && audioEngine.soundHandles[handle]->type == SoundHandle::Type::STATIC) {
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
    if (audioEngine.isInitialized && IS_SOUND_HANDLE_VALID(handle) && audioEngine.soundHandles[handle]->type == SoundHandle::Type::STATIC) {
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
    if (audioEngine.isInitialized && IS_SOUND_HANDLE_VALID(handle) && audioEngine.soundHandles[handle]->type == SoundHandle::Type::STATIC) {
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
        (audioEngine.soundHandles[handle]->type == SoundHandle::Type::STATIC || audioEngine.soundHandles[handle]->type == SoundHandle::Type::RAW)) {
        ma_sound_set_volume(&audioEngine.soundHandles[handle]->maSound, volume);
    }
}

/// <summary>
/// This is like sub__sndplay but the sound is looped.
/// </summary>
/// <param name="handle"></param>
void sub__sndloop(int32_t handle) {
    if (audioEngine.isInitialized && IS_SOUND_HANDLE_VALID(handle) && audioEngine.soundHandles[handle]->type == SoundHandle::Type::STATIC) {
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
        (audioEngine.soundHandles[handle]->type == SoundHandle::Type::STATIC || audioEngine.soundHandles[handle]->type == SoundHandle::Type::RAW)) {
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
    if (audioEngine.isInitialized && IS_SOUND_HANDLE_VALID(handle) && audioEngine.soundHandles[handle]->type == SoundHandle::Type::STATIC) {
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
    if (audioEngine.isInitialized && IS_SOUND_HANDLE_VALID(handle) && audioEngine.soundHandles[handle]->type == SoundHandle::Type::STATIC) {
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
    if (audioEngine.isInitialized && IS_SOUND_HANDLE_VALID(handle) && audioEngine.soundHandles[handle]->type == SoundHandle::Type::STATIC) {
        float lengthSeconds = 0;
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
                                                          lengthSampleFrames * (seconds / lengthSeconds)); // Set the position in PCM frames
        AUDIO_DEBUG_CHECK(audioEngine.maResult == MA_SUCCESS);
    }
}

/// <summary>
/// This stops playing a sound after it has been playing for a set number of seconds.
/// </summary>
/// <param name="handle">A sound handle</param>
/// <param name="limit">The number of seconds that the sound will play</param>
void sub__sndlimit(int32_t handle, double limit) {
    if (audioEngine.isInitialized && IS_SOUND_HANDLE_VALID(handle) && audioEngine.soundHandles[handle]->type == SoundHandle::Type::STATIC) {
        ma_sound_set_stop_time_in_milliseconds(&audioEngine.soundHandles[handle]->maSound, limit * 1000);
    }
}

/// <summary>
/// This stops a playing or paused sound using a sound handle.
/// </summary>
/// <param name="handle">A sound handle</param>
void sub__sndstop(int32_t handle) {
    if (audioEngine.isInitialized && IS_SOUND_HANDLE_VALID(handle) && audioEngine.soundHandles[handle]->type == SoundHandle::Type::STATIC) {
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

    // Allocate a sound handle
    int32_t handle = audioEngine.CreateHandle();
    if (handle < 1)
        return INVALID_SOUND_HANDLE;

    // Set some handle properties
    audioEngine.soundHandles[handle]->type = SoundHandle::Type::RAW;

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

    if (audioEngine.isInitialized && IS_SOUND_HANDLE_VALID(handle) && audioEngine.soundHandles[handle]->type == SoundHandle::Type::RAW) {
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

    if (audioEngine.isInitialized && IS_SOUND_HANDLE_VALID(handle) && audioEngine.soundHandles[handle]->type == SoundHandle::Type::RAW) {
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

    if (audioEngine.isInitialized && IS_SOUND_HANDLE_VALID(handle) && audioEngine.soundHandles[handle]->type == SoundHandle::Type::RAW) {
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

    // Allocate a sound handle
    int32_t handle = audioEngine.CreateHandle();
    if (handle < 1)
        return INVALID_SOUND_HANDLE;

    // Set some handle properties
    audioEngine.soundHandles[handle]->type = SoundHandle::Type::STATIC;

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
        ma_audio_buffer_uninit_and_free(audioEngine.soundHandles[handle]->maAudioBuffer);
        audioEngine.soundHandles[handle]->maAudioBuffer = nullptr;
        audioEngine.soundHandles[handle]->isUsed = false;
        return INVALID_SOUND_HANDLE;
    }

    AUDIO_DEBUG_PRINT("Frames = %i, channels = %i, bits = %i, ma_format = %i, pointer = %p", audioEngine.soundHandles[handle]->maAudioBuffer->ref.sizeInFrames,
                      audioEngine.soundHandles[handle]->maAudioBuffer->ref.channels, bits, audioEngine.soundHandles[handle]->maAudioBuffer->ref.format,
                      audioEngine.soundHandles[handle]->maAudioBuffer->ref.pData);

    return handle;
}

/// @brief This function returns a _MEM value referring to a sound's raw data in memory using a designated sound handle created by the _SNDOPEN function.
/// miniaudio supports a variety of sample and channel formats. Translating all of that to basic 2 channel 16-bit format that
/// MemSound was originally supporting would require significant overhead both in terms of system resources and code.
/// For now we are just exposing the underlying PCM data directly from miniaudio. This fits rather well using the existing mem structure.
/// Mono sounds should continue to work just as it was before. Stereo and multi-channel sounds however will be required to be handled correctly
/// by the user by checking the 'elementsize' (for frame size in bytes) and 'type' (for data type) members.
/// @param handle A sound handle
/// @param targetChannel This should be 0 (for interleaved) or 1 (for mono). Anything else will result in failure
/// @param passed What arguments were passed?
/// @return A _MEM value that can be used to access the sound data
mem_block func__memsound(int32_t handle, int32_t targetChannel, int32_t passed) {
    ma_format maFormat = ma_format::ma_format_unknown;
    ma_uint32 channels = 0;
    ma_uint64 sampleFrames = 0;
    intptr_t data = NULL;

    // Setup mem_block (assuming failure)
    mem_block mb = {};
    mb.lock_offset = (intptr_t)mem_lock_base;
    mb.lock_id = INVALID_MEM_LOCK;

    // Return invalid mem_block if audio is not initialized, handle is invalid or sound type is not static
    if (!audioEngine.isInitialized || !IS_SOUND_HANDLE_VALID(handle) || audioEngine.soundHandles[handle]->type != SoundHandle::Type::STATIC) {
        AUDIO_DEBUG_PRINT("Invalid handle (%i) or sound type (%i)", handle, audioEngine.soundHandles[handle]->type);
        return mb;
    }

    // Simply return an "empty" mem_block if targetChannel is not 0 or 1
    if (passed && targetChannel != 0 && targetChannel != 1) {
        AUDIO_DEBUG_PRINT("Invalid channel (%i)", targetChannel);
        return mb;
    }

    // Check what kind of sound we are dealing with and take appropriate path
    if (audioEngine.soundHandles[handle]->maAudioBuffer) { // we are dealing with a user created audio buffer
        AUDIO_DEBUG_PRINT("Entering ma_audio_buffer path");
        maFormat = audioEngine.soundHandles[handle]->maAudioBuffer->ref.format;
        channels = audioEngine.soundHandles[handle]->maAudioBuffer->ref.channels;
        sampleFrames = audioEngine.soundHandles[handle]->maAudioBuffer->ref.sizeInFrames;
        data = (intptr_t)audioEngine.soundHandles[handle]->maAudioBuffer->ref.pData;
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

        // Check if the data is one contiguous buffer or a link list of decoded pages
        // We cannot have a mem object for a link list of decoded pages for obvious reasons
        if (ds->pNode->data.type != ma_resource_manager_data_supply_type::ma_resource_manager_data_supply_type_decoded) {
            AUDIO_DEBUG_PRINT("Data is not a contiguous buffer. Type = %u", ds->pNode->data.type);
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

        data = (intptr_t)ds->pNode->data.backend.decoded.pData;
    }

    AUDIO_DEBUG_PRINT("Format = %u, channels = %u, frames = %llu", maFormat, channels, sampleFrames);

    // Setup type: This was not done in the old code
    // But we are doing it here. By examining the type the user can now figure out if they have to use FP32 or integers
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
        mb.lock_offset = (intptr_t)audioEngine.soundHandles[handle]->memLockOffset;
        mb.lock_id = audioEngine.soundHandles[handle]->memLockId;
    } else {
        AUDIO_DEBUG_PRINT("Returning new mem_lock");
        new_mem_lock();
        mem_lock_tmp->type = MEM_TYPE_SOUND;
        mb.lock_offset = (intptr_t)mem_lock_tmp;
        mb.lock_id = mem_lock_id;
        audioEngine.soundHandles[handle]->memLockOffset = (void *)mem_lock_tmp;
        audioEngine.soundHandles[handle]->memLockId = mem_lock_id;
    }

    mb.elementsize = ma_get_bytes_per_frame(maFormat, channels); // Set the element size. This is the size of each PCM frame in bytes
    mb.offset = data;                                            // Setup offset
    mb.size = sampleFrames * mb.elementsize;                     // Setup size (in bytes)
    mb.sound = handle;                                           // Copy the handle
    mb.image = 0;                                                // Not needed. Set to 0

    AUDIO_DEBUG_PRINT("ElementSize = %lli, size = %lli, type = %lli, pointer = %p", mb.elementsize, mb.size, mb.type, mb.offset);

    return mb;
}

/// @brief This initializes the audio subsystem. We simply attempt to initialize and then set some globals with the results
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
    // Set the resource manager decoder sample rate to the device sample rate (miniaudio engine bug?)
    audioEngine.maResourceManager.config.decodedSampleRate = audioEngine.sampleRate = ma_engine_get_sample_rate(&audioEngine.maEngine);

    // Set the initialized flag as true
    audioEngine.isInitialized = true;

    AUDIO_DEBUG_PRINT("Audio engine initialized @ %uHz", audioEngine.sampleRate);

    // Reserve sound handle 0 so that nothing else can use it
    // We will use this handle internally for Play(), Beep(), Sound() etc.
    audioEngine.sndInternal = audioEngine.CreateHandle();
    AUDIO_DEBUG_CHECK(audioEngine.sndInternal == 0); // The first handle must return 0 and this is what is used by Beep and Sound
}

/// @brief This shuts down the audio engine and frees any resources used
void snd_un_init() {
    if (audioEngine.isInitialized) {
        // Free any PSG object if they were created
        if (audioEngine.psg) {
            delete audioEngine.psg;
            audioEngine.psg = nullptr;
        }

        // Free all sound handles here
        for (size_t handle = 0; handle < audioEngine.soundHandles.size(); handle++) {
            audioEngine.ReleaseHandle(handle);       // let ReleaseHandle do its thing
            delete audioEngine.soundHandles[handle]; // now free the object created by CreateHandle()
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

/// @brief This is called by the QB64-PE internally at ~60Hz. We use this for housekeeping and other stuff.
void snd_mainloop() {
    if (audioEngine.isInitialized) {
        // Scan through the whole handle vector to find anything we need to update or close
        for (size_t handle = 0; handle < audioEngine.soundHandles.size(); handle++) {
            // Only process handles that are in use
            if (audioEngine.soundHandles[handle]->isUsed) {
                // Look for stuff that is set to auto-destruct
                if (audioEngine.soundHandles[handle]->autoKill) {
                    switch (audioEngine.soundHandles[handle]->type) {
                    case SoundHandle::Type::STATIC:
                    case SoundHandle::Type::RAW:
                        // Dispose the sound if it has finished playing
                        // Note that this means that temporary looping sounds will never close
                        // Well that's on the programmer. Probably they want it that way
                        if (!ma_sound_is_playing(&audioEngine.soundHandles[handle]->maSound))
                            audioEngine.ReleaseHandle(handle);

                        break;

                    case SoundHandle::Type::NONE:
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
