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

// Set this to 1 if we want to print debug messages to stderr
#define AUDIO_DEBUG 1
#include "audio.h"
// We need 'qbs' and 'mem' stuff from here. This should eventually change when things are moved to smaller, logical and self-contained files
#include "../../libqb.h"
#define STB_VORBIS_HEADER_ONLY
#include "extras/stb_vorbis.c"
#include "miniaudio.h"
#include "mutex.h"
#include <algorithm>
#include <stack>
#include <unordered_map>
#include <vector>

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
#define CLAMP(_x_, _low_, _high_) (((_x_) > (_high_)) ? (_high_) : (((_x_) < (_low_)) ? (_low_) : (_x_)))

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
void Sleep(uint32 milliseconds);                                    // There is a non-Windows implementation. However it is not declared anywhere
#endif

extern ptrszint dblock;         // Required for Play(). Did not find this declared anywhere
extern uint64 mem_lock_id;      // Another one that we need for the mem stuff
extern mem_lock *mem_lock_base; // Same as above
extern mem_lock *mem_lock_tmp;  // Same as above

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

    static const size_t DEFAULT_SIZE = 1024;  // this is almost twice the amout what miniaudio actually asks for in frameCount

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

    /// @brief This pushes a whole buffer of mono sample frames to the queue. This is mutex protected and called by the main thread
    /// @param buffer The buffer containing the sample frames. This cannot be NULL
    /// @param frames The total number of frames in the buffer
    void PushMonoSampleFrames(float *buffer, ma_uint64 frames) {
        libqb_mutex_guard lock(m); // lock the mutex before accessing the vectors
        for (ma_uint64 i = 0; i < frames; i++) {
            producer->data.push_back({buffer[i], buffer[i]});
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

    auto pRawStream = (RawStream *)pDataSource;                                                     // cast to RawStream instance pointer
    auto result = MA_SUCCESS;                                                                       // must be initialized to MA_SUCCESS
    auto maBuffer = (SampleFrame *)pFramesOut;                                                      // cast to sample frame pointer

    ma_uint64 sampleFramesCount = pRawStream->consumer->data.size() - pRawStream->consumer->cursor; // total amount of samples we need to send to miniaudio
    // Swap buffers if we do not have anything left to play
    if (!sampleFramesCount) {
        pRawStream->SwapBuffers();
        sampleFramesCount = pRawStream->consumer->data.size() - pRawStream->consumer->cursor; // get the total number of samples again
    }
    sampleFramesCount = std::min(sampleFramesCount, frameCount);                              // we'll always send lower of what miniaudio wants or what we have

    ma_uint64 sampleFramesRead = 0;                                                           // sample frame counter
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

        delete pRawStream;                    // delete the raw stream object

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

/// @brief A waveform class and wrapper around the miniaudio waveform APIs
class Waveform {
  public:
    /// @brief Various types of waveform that can be generated
    enum Type { SILENCE = 0, SAWTOOTH, TRIANGLE, SQUARE, SINE, NOISE, COUNT };

  private:
    ma_waveform_config maWaveformConfig; // miniaudio waveform configuration
    ma_waveform maWaveform;              // miniaudio waveform
    ma_noise_config maNoiseConfig;       // miniaudio noise configuration
    ma_noise maNoise;                    // miniaudio noise
    ma_result maResult;                  // result of the last miniaudio operation
    RawStream *rawStream;                // this is the RawStream where the samples data will be pushed to
    float *bufferWorking;                // this is where stuff is rendered temporarily when mixing is needed
    float *bufferFinal;                  // this is where the waveform is rendered before being pushed to RawStream
    ma_uint64 frames;                    // size of the buffer in sample frames
    Type type;                           // the current waveform type selected
    bool played;                         // was the last generated waveform played?

    /// @brief Resizes the working and final buffers if frames requested is not the same as the one already allocated
    /// @param newFrames New sample frames required
    /// @return True if the buffers were allocated correctly
    bool ResizeBuffers(ma_uint64 newFrames) {
        if (newFrames <= frames || !newFrames) {
            frames = newFrames; // no need to re-allocate if we are reducing the buffer size
            return bufferWorking != nullptr && bufferFinal != nullptr;
        }

        auto bufferSize = newFrames * SAMPLE_FRAME_SIZE(float, 1);

        float *tmpBufferWorking = (float *)realloc(bufferWorking, bufferSize);
        if (!tmpBufferWorking)
            return false;
        bufferWorking = tmpBufferWorking;

        float *tmpBufferFinal = (float *)realloc(bufferFinal, bufferSize);
        if (!tmpBufferFinal)
            return false;
        bufferFinal = tmpBufferFinal;

        if (newFrames > frames) {
            auto deltaSize = (newFrames - frames) * SAMPLE_FRAME_SIZE(float, 1);
            memset(bufferWorking + frames, 0, deltaSize);
            memset(bufferFinal + frames, 0, deltaSize);
        }

        AUDIO_DEBUG_PRINT("Buffers resized from %llu to %llu", frames, newFrames);

        frames = newFrames;

        return true;
    }

  public:
    // Delete default, copy and move constructors and assignments
    Waveform() = delete;
    Waveform(const Waveform &) = delete;
    Waveform &operator=(const Waveform &) = delete;
    Waveform &operator=(Waveform &&) = delete;
    Waveform(Waveform &&) = delete;

    /// @brief Contructors that can set few or more defaults
    /// @param pRawStream Pointer to a raw stream where the samples data will be pushed to. This cannot be NULL
    /// @param type The waveform type. See ma_waveform_type
    /// @param amplitude The amplitude of the waveform
    /// @param frequency The frequency of the waveform
    Waveform(RawStream *pRawStream, Type waveType, double amplitude, double frequency) {
        rawStream = pRawStream; // Save the raw queue object pointer
        bufferWorking = bufferFinal = nullptr;
        frames = 0;
        played = true; // because there is nothing to play yet
        maWaveformConfig =
            ma_waveform_config_init(ma_format::ma_format_f32, 1, rawStream->sampleRate, ma_waveform_type::ma_waveform_type_square, amplitude, frequency);
        maResult = ma_waveform_init(&maWaveformConfig, &maWaveform);
        AUDIO_DEBUG_CHECK(maResult == MA_SUCCESS);
        maNoiseConfig = ma_noise_config_init(ma_format::ma_format_f32, 1, ma_noise_type::ma_noise_type_white, 0, amplitude);
        maResult = ma_noise_init(&maNoiseConfig, NULL, &maNoise);
        AUDIO_DEBUG_CHECK(maResult == MA_SUCCESS);
        SetType(waveType);

        AUDIO_DEBUG_PRINT("Type = %i, amplitude = %lf, frequency = %lf", type, amplitude, frequency);
    }

    /// @brief Contructors that can set few or more defaults
    /// @param pRawStream Pointer to a raw stream where the samples data will be pushed to
    /// @param type The waveform type. See ma_waveform_type
    Waveform(RawStream *pRawStream, Type waveType) : Waveform(pRawStream, waveType, 1.0, 440.0) {}

    /// @brief Contructors that can set few or more defaults
    /// @param pRawStream Pointer to a raw stream where the samples data will be pushed to
    Waveform(RawStream *pRawStream) : Waveform(pRawStream, Type::SQUARE) {}

    /// @brief This just frees the waveform buffer and cleans up the waveform resources
    ~Waveform() {
        ma_noise_uninit(&maNoise, NULL); // destroy miniaudio noise
        ma_waveform_uninit(&maWaveform); // destroy miniaudio waveform
        free(bufferFinal);
        free(bufferWorking);

        AUDIO_DEBUG_PRINT("Waveform object destroyed");
    }

    /// @brief Generates a waveform based on member values. Subsequent calls will overwrite the buffer if mix is false
    /// @param totalDuration The duration of the sound in seconds. This can be more than realDuration if silence padding is needed at the end
    /// @param realDuration The real duration of the sound in seconds (should always be less or equal to duration)
    /// @param mix Mixes the generated waveform to the buffer instead of overwriting it
    /// @return True if successful
    bool Generate(double totalDuration, double realDuration, bool mix = false) {
        auto totalFramesRequested = (ma_uint64)(totalDuration * rawStream->sampleRate);
        auto realFramesRequested = (ma_uint64)(realDuration * rawStream->sampleRate);

        if (!totalFramesRequested || !realFramesRequested || realFramesRequested > totalFramesRequested ||
            !ResizeBuffers(mix ? (std::max)(totalFramesRequested, frames) : totalFramesRequested))
            return false;

        auto targetBuffer = mix ? bufferWorking : bufferFinal;
        auto framesGenerated = realFramesRequested;
        maResult = MA_SUCCESS;

        switch (type) {
        case Type::TRIANGLE:
        case Type::SAWTOOTH:
        case Type::SINE:
        case Type::SQUARE:
            maResult = ma_waveform_read_pcm_frames(&maWaveform, targetBuffer, realFramesRequested, &framesGenerated);
            break;

        case Type::NOISE:
            maResult = ma_noise_read_pcm_frames(&maNoise, targetBuffer, realFramesRequested, &framesGenerated);
            break;

        case Type::SILENCE:
        default:
            memset(targetBuffer, 0, realFramesRequested * SAMPLE_FRAME_SIZE(float, 1));
            break;
        }

        if (totalFramesRequested > realFramesRequested) {
            auto deltaSize = (totalFramesRequested - realFramesRequested) * SAMPLE_FRAME_SIZE(float, 1);
            memset(targetBuffer + realFramesRequested, 0, deltaSize);
        }

        if (maResult != MA_SUCCESS)
            return false;

        if (mix) {
            for (size_t i = 0; i < framesGenerated; i++)
                bufferFinal[i] += bufferWorking[i];

            AUDIO_DEBUG_PRINT("Mixed %llu frames of type %i waveform to %llu frames buffer", framesGenerated, type, frames);
        } else {
            if (totalFramesRequested == realFramesRequested)
                frames = framesGenerated;
            else
                frames = totalFramesRequested;

            AUDIO_DEBUG_PRINT("Generated %llu frames of type %i waveform to %llu frames buffer", framesGenerated, type, frames);
        }

        played = false;

        return true;
    }

    /// @brief Sets the amplitude of the waveform
    /// @param amplitude The amplitude of the waveform
    /// @return True if successful
    bool SetAmplitude(double amplitude) {
        maResult = MA_SUCCESS;

        switch (type) {
        case Type::TRIANGLE:
        case Type::SAWTOOTH:
        case Type::SINE:
        case Type::SQUARE:
            maResult = ma_waveform_set_amplitude(&maWaveform, amplitude);
            break;

        case Type::NOISE:
            maResult = ma_noise_set_amplitude(&maNoise, amplitude);
            break;
        }

        if (maResult != MA_SUCCESS)
            return false;

        AUDIO_DEBUG_PRINT("Amplitude set to %lf", amplitude);

        return true;
    }

    /// @brief Sets the frequency of the waveform
    /// @param frequency The frequency of the waveform
    /// @return True if successful
    bool SetFrequency(double frequency) {
        maResult = MA_SUCCESS;

        switch (type) {
        case Type::TRIANGLE:
        case Type::SAWTOOTH:
        case Type::SINE:
        case Type::SQUARE:
            maResult = ma_waveform_set_frequency(&maWaveform, frequency);
            break;
        }

        if (maResult != MA_SUCCESS)
            return false;

        AUDIO_DEBUG_PRINT("Frequency set to %lf", frequency);

        return true;
    }

    /// @brief Sets the waveform type
    /// @param type The waveform type. See Waveform::Type
    /// @return True if successful
    bool SetType(Type waveType) {
        maResult = MA_SUCCESS;

        switch (waveType) {
        case Type::TRIANGLE:
            maResult = ma_waveform_set_type(&maWaveform, ma_waveform_type::ma_waveform_type_triangle);
            break;

        case Type::SAWTOOTH:
            maResult = ma_waveform_set_type(&maWaveform, ma_waveform_type::ma_waveform_type_sawtooth);
            break;

        case Type::SINE:
            maResult = ma_waveform_set_type(&maWaveform, ma_waveform_type::ma_waveform_type_sine);
            break;

        case Type::SQUARE:
            maResult = ma_waveform_set_type(&maWaveform, ma_waveform_type::ma_waveform_type_square);
            break;
        }

        if (maResult != MA_SUCCESS)
            return false;

        type = waveType;

        AUDIO_DEBUG_PRINT("Waveform type set to %i", type);

        return true;
    }

    /// @brief Returns the type of waveform
    /// @return The type of waveform
    Type GetType() { return type; }

    /// @brief Sends the buffer for playback
    /// @return True if successful
    bool Play() {
        if (bufferFinal) {
            rawStream->PushMonoSampleFrames(bufferFinal, frames);

            AUDIO_DEBUG_PRINT("Sent %llu samples for playback", frames);

            played = true;
            return true;
        }
        return false;
    }

    /// @brief
    /// @return
    bool Played() { return played; }

    /// @brief Waits for any playback to complete
    void AwaitPlaybackCompletion() {
        AUDIO_DEBUG_PRINT("Waiting for playback to complete");

        auto timeMs = (ma_uint32)(rawStream->GetSampleFramesRemaining() * 1000) / rawStream->sampleRate;
        if (timeMs)
            Sleep(timeMs);
    }

    ma_uint32 GetSampleRate() { return rawStream->sampleRate; }
};

/// @brief This is an MML parser and player class that implements the QB64 MML spec and more
/// https://qb64phoenix.com/qb64wiki/index.php/PLAY
/// http://vgmpf.com/Wiki/index.php?title=Music_Macro_Language
/// https://en.wikipedia.org/wiki/Music_Macro_Language
/// https://sneslab.net/wiki/Music_Macro_Language
/// http://www.mirbsd.org/htman/i386/man4/speaker.htm
/// https://www.qbasic.net/en/reference/qb11/Statement/PLAY-006.htm
/// https://woolyss.com/chipmusic-mml.php
/// frequency = 440.0 * pow(2.0, (note + (octave - 2.0) * 12.0 - 9.0) / 12.0)
/*
const float FREQUENCY_TABLE[] = {
        0,
        //1       2         3         4         5         6         7         8         9         10        11        12
        //C       C#        D         D#        E         F         F#        G         G#        A         A#        B
        16.35f,   17.32f,   18.35f,   19.45f,   20.60f,   21.83f,   23.12f,   24.50f,   25.96f,   27.50f,   29.14f,   30.87f,   // Octave 0
        32.70f,   34.65f,   36.71f,   38.89f,   41.20f,   43.65f,   46.25f,   49.00f,   51.91f,   55.00f,   58.27f,   61.74f,   // Octave 1
        65.41f,   69.30f,   73.42f,   77.78f,   82.41f,   87.31f,   92.50f,   98.00f,   103.83f,  110.00f,  116.54f,  123.47f,  // Octave 2
        130.81f,  138.59f,  146.83f,  155.56f,  164.81f,  174.62f,  185.00f,  196.00f,  207.65f,  220.00f,  233.08f,  246.94f,  // Octave 3
        261.63f,  277.18f,  293.67f,  311.13f,  329.63f,  349.23f,  370.00f,  392.00f,  415.31f,  440.00f,  466.17f,  493.89f,  // Octave 4
        523.25f,  554.37f,  587.33f,  622.26f,  659.26f,  698.46f,  739.99f,  783.99f,  830.61f,  880.00f,  932.33f,  987.77f,  // Octave 5
        1046.51f, 1108.74f, 1174.67f, 1244.51f, 1318.52f, 1396.92f, 1479.99f, 1567.99f, 1661.23f, 1760.01f, 1864.66f, 1975.54f, // Octave 6
        2093.02f, 2217.47f, 2349.33f, 2489.03f, 2637.03f, 2793.84f, 2959.97f, 3135.98f, 3322.45f, 3520.02f, 3729.33f, 3951.09f, // Octave 7
};
*/
class MMLPlayer {
  private:
    /// @brief This struct to used to hold the current string state and also used for the state stack
    struct State {
        uint8_t *string;
        size_t length;
        size_t position;
    };

    int command;                  // the command we need to execute
    int note;                     // the current note
    int tempo;                    // the tempo of the tune (this impacts all lengths)
    int octave;                   // the current octave that we'll use for notes
    double length;                // the length of each note (1 = full, 4 = quarter etc.)
    double pause;                 // the duration of silence after a note (this eats away from the note length)
    double volume;                // the current volume
    double duration;              // the duration of the note / silence (in seconds)
    bool background;              // if this is true, then control will be returned back to the caller as soon as the MML is rendered
    bool mix;                     // the next note should be mixed to the previous one
    Waveform *waveform;           // samples are generated and pushed for playback using this waveform object pointer
    std::stack<State> stateStack; // this maintains the state stack if we need to process substrings (VARPTR$)
    State currentState;           // this is the current state. See State struct

    // These are some constants that can be tweaked to change the behavior of the parser
    // These mostly conform to the QBasic and QB64 spec.
    static constexpr auto MIN_VOLUME = 0.0;
    static constexpr auto MAX_VOLUME = 100.0;
    static constexpr auto DEFAULT_VOLUME = MAX_VOLUME / 2;
    static const auto MIN_TEMPO = 32;
    static const auto MAX_TEMPO = 255;
    static const auto DEFAULT_TEMPO = 120;
    static const auto MIN_OCTAVE = 0;
    static const auto MAX_OCTAVE = 7;
    static const auto DEFAULT_OCTAVE = 4;
    static const auto MIN_NOTE = 0;
    static const auto MAX_NOTE = 12 * (1 + MAX_OCTAVE);
    static constexpr auto MIN_LENGTH = 1.0;
    static constexpr auto MAX_LENGTH = 64.0;
    static constexpr auto DEFAULT_LENGTH = 4.0;
    static constexpr auto DEFAULT_PAUSE = 1.0 / 8.0;
    static const auto INVALID = -1;

    /// @brief Get stores the current character from the string to 'command' member skipping all whitespace characters
    /// @return True if it was successful
    bool GetCommand() {
        while (currentState.position < currentState.length) {
            command = toupper(currentState.string[currentState.position]);
            if (!isspace(command))
                return true;
            currentState.position++; // skip space characters
        }
        return false;
    }

    /// @brief Gets the next non-whitespace character from the string without changing the index
    /// @return A valid ASCII character or -1 if there is none
    int PeekNextCharacter() {
        for (auto p = currentState.position + 1; p < currentState.length; ++p) {
            auto c = toupper(currentState.string[p]);
            if (!isspace(c))
                return toupper(c);
        }
        return INVALID;
    }

    /// @brief Gets the next non-whitespace character from the string and advances the index to the character location
    /// @return A valid ASCII character or -1 if there is none
    int GetNextCharacter() {
        while (++currentState.position < currentState.length) {
            auto c = toupper(currentState.string[currentState.position]);
            if (!isspace(c))
                return toupper(c);
        }
        return INVALID;
    }

    /// @brief Returns a numeric argument for a command
    /// @return A valid positive number or -1 if there is none
    int GetCommandArgument() {
        // TODO: Implement "=" + VARPTR$(n) support for numbers
        auto number = 0;
        auto commandArgument = INVALID;
        for (auto c = PeekNextCharacter(); c != INVALID && isdigit(c); c = PeekNextCharacter()) {
            number = number * 10 + c - '0';
            commandArgument = number;
            GetNextCharacter(); // consume the character
        }
        return commandArgument;
    }

    /// @brief Counts the dots after a note or a pause and increases the duration
    void DoDots() {
        auto dots = 0;
        for (auto c = PeekNextCharacter(); c != INVALID && c == '.'; c = PeekNextCharacter()) {
            ++dots;
            GetNextCharacter(); // consume the character
        }

        auto dotDuration = duration;
        for (auto i = 0; i < dots; i++) {
            dotDuration /= 2.0;
            duration += dotDuration;
        }
    }

    /// @brief Calculates the note duration in seconds and stores it in the 'duration' member
    /// @param customLength This is optional and can be specified if a duration is required for a custom length (useful for 'note' x and P x commands)
    void CalculateDuration(double customLength = INVALID) {
        auto l = customLength == INVALID ? length : customLength;
        duration = 1.0 / (tempo / 60.0) * (4.0 / l);
    }

    /// @brief This converts a note string to a number and handles all notes (including sharps and flats) and also 'N' commands
    void GetNote() {
        static const int notes[] = {9, 11, 0, 2, 4, 5, 7};
        auto argument = PeekNextCharacter();
        if (command == 'N') {
            if (isdigit(argument)) {
                argument = GetCommandArgument();
                if (argument >= MIN_NOTE && argument <= MAX_NOTE)
                    note = argument - 1; // rest (N0) will become -1
                else
                    AUDIO_DEBUG_PRINT("Invalid note (%i) requested", argument);
            } else {
                AUDIO_DEBUG_PRINT("Expected number for note. Got '%c'", argument);
            }
        } else if (command >= 'A' && command <= 'G') {
            note = notes[command - 'A'];
            if (argument == '+' || argument == '#') {
                ++note;
                GetNextCharacter(); // consume the character
            } else if (argument == '-') {
                --note;
                GetNextCharacter(); // consume the character
            }
        } else {
            AUDIO_DEBUG_PRINT("Expected A/B/C/D/E/F/G/N. Got '%c'", command);
        }
    }

    /// @brief This is what playes the notes based on the note number
    void DoNote() {
        GetNote();

        auto argument = PeekNextCharacter();
        if (isdigit(argument)) {
            argument = GetCommandArgument();
            if (argument >= MIN_LENGTH && argument <= MAX_LENGTH)
                CalculateDuration(argument);
            else
                AUDIO_DEBUG_PRINT("Invalid length (%i) requested", argument);
        } else {
            CalculateDuration();
        }

        DoDots();

        if (note == INVALID) {
            auto waveType = waveform->GetType();
            waveform->SetType(Waveform::Type::SILENCE);
            waveform->Generate(duration, duration);
            waveform->Play();
            waveform->SetType(waveType);
        } else {
            waveform->SetFrequency(440.0 * pow(2.0, (note + (octave - 2.0) * 12.0 - 9.0) / 12.0));
            waveform->SetAmplitude(volume / (MAX_VOLUME + 1.0));
            waveform->Generate(duration, duration - (duration * pause), mix);
            if (PeekNextCharacter() != ',') // peek ahead to see if we need to mix this but don't consume the ',' yet
                waveform->Play();           // play the note only if we do not need to mix
        }

        mix = false; // unset the last mix flag as this will be set again by main loop if mix is needed
    }

    /// @brief Used for playing silence when 'P' or 'R' is encountered
    void DoSilence() {
        if (isdigit(PeekNextCharacter())) {
            auto argument = GetCommandArgument();
            if (argument >= MIN_LENGTH && argument <= MAX_LENGTH)
                CalculateDuration(argument);
            else
                AUDIO_DEBUG_PRINT("Invalid length (%i) requested", argument);
        } else {
            CalculateDuration();
        }
        DoDots();
        auto waveType = waveform->GetType();
        waveform->SetType(Waveform::Type::SILENCE);
        waveform->Generate(duration, duration);
        waveform->Play();
        waveform->SetType(waveType);
    }

    /// @brief Used to change the volume when a 'V' command is encoutered
    void DoVolume() {
        auto argument = PeekNextCharacter();
        if (isdigit(argument)) {
            argument = GetCommandArgument();
            if (argument >= MIN_VOLUME && argument <= MAX_VOLUME)
                volume = argument;
            else
                AUDIO_DEBUG_PRINT("Invalid volume (%i) requested", argument);
        } else {
            if (argument == '+') {
                ++volume;
                volume = CLAMP(volume, MIN_VOLUME, MAX_VOLUME);
            } else if (argument == '-') {
                --volume;
                volume = CLAMP(volume, MIN_VOLUME, MAX_VOLUME);
            } else {
                AUDIO_DEBUG_PRINT("Expected +, - or a number for volume. Got '%c'", argument);
                return;
            }
            GetNextCharacter(); // consume the character
        }
    }

    /// @brief Used to change the tune tempo when a 'T' command is encountered
    void DoTempo() {
        auto argument = PeekNextCharacter();
        if (isdigit(argument)) {
            argument = GetCommandArgument();
            if (argument >= MIN_TEMPO && argument <= MAX_TEMPO)
                tempo = argument;
            else
                AUDIO_DEBUG_PRINT("Invalid tempo (%i) requested", argument);
        } else {
            if (argument == '+') {
                ++tempo;
                tempo = CLAMP(tempo, MIN_TEMPO, MAX_TEMPO);
            } else if (argument == '-') {
                --tempo;
                tempo = CLAMP(tempo, MIN_TEMPO, MAX_TEMPO);
            } else {
                AUDIO_DEBUG_PRINT("Expected +, - or a number for tempo. Got '%c'", argument);
                return;
            }
            GetNextCharacter(); // consume the character
        }
    }

    /// @brief Used to change the note length when an 'L' command is encountered
    void DoLength() {
        auto argument = PeekNextCharacter();
        if (isdigit(argument)) {
            argument = GetCommandArgument();
            if (argument >= MIN_LENGTH && argument <= MAX_LENGTH)
                length = argument;
            else
                AUDIO_DEBUG_PRINT("Invalid length (%i) requested", argument);
        } else {
            if (argument == '+') {
                ++length;
                length = CLAMP(length, MIN_LENGTH, MAX_LENGTH);
            } else if (argument == '-') {
                --length;
                length = CLAMP(length, MIN_LENGTH, MAX_LENGTH);
            } else {
                AUDIO_DEBUG_PRINT("Expected +, - or a number for length. Got '%c'", argument);
                return;
            }
            GetNextCharacter(); // consume the character
        }
    }

    /// @brief Used to change the octave when an 'O' command is encountered
    void DoOctave() {
        auto argument = PeekNextCharacter();
        if (isdigit(argument)) {
            argument = GetCommandArgument();
            if (argument >= MIN_OCTAVE && argument <= MAX_OCTAVE)
                octave = argument;
            else
                AUDIO_DEBUG_PRINT("Invalid octave (%i) requested", argument);
        } else {
            if (argument == '+') {
                ++octave;
                octave = CLAMP(octave, MIN_OCTAVE, MAX_OCTAVE);
            } else if (argument == '-') {
                --octave;
                octave = CLAMP(octave, MIN_OCTAVE, MAX_OCTAVE);
            } else {
                AUDIO_DEBUG_PRINT("Expected +, - or a number for octave. Got '%c'", argument);
                return;
            }
            GetNextCharacter(); // consume the character
        }
    }

    /// @brief Used to handle all 'M' commands
    void DoTiming() {
        auto argument = PeekNextCharacter();
        switch (argument) {
        case 'B': // background
            background = true;
            break;

        case 'F': // foreground
            background = false;
            break;

        case 'L': // legato
            pause = 0.0;
            break;

        case 'N': // normal
            pause = 1.0 / 8.0;
            break;

        case 'S': // staccato
            pause = 1.0 / 4.0;
            break;

        default:
            AUDIO_DEBUG_PRINT("Expected B/F/L/N/S. Got '%c'", argument);
            return;
        }
        GetNextCharacter(); // consume the character
    }

    /// @brief Used to push the current string state to the stack when an 'X' command is encountered
    void DoSubstring() {
        // get string address
        // if (successful) >
        // currentState.position++;	// increment to the next position (important!)
        // stateStack.push(currentState); // push the current string & state to the state stack
        // currentState.string = new_string;
        // currentState.length = strlen(new_string);
        // currentState.position = 0;
        AUDIO_DEBUG_PRINT("X not implemented");
    }

    /// @brief Used to change the waveform type when a 'W' command is encountered
    void DoWaveform() {
        auto argument = PeekNextCharacter();
        if (isdigit(argument)) {
            argument = GetCommandArgument();
            if (argument > Waveform::Type::SILENCE && argument < Waveform::Type::COUNT)
                waveform->SetType((Waveform::Type)argument);
            else
                AUDIO_DEBUG_PRINT("Invalid waveform (%i) requested", argument);
        } else {
            AUDIO_DEBUG_PRINT("Expected number for waveform. Got '%c'", argument);
        }
    }

  public:
    // Delete default, copy and move constructors and assignments
    MMLPlayer() = delete;
    MMLPlayer(const MMLPlayer &) = delete;
    MMLPlayer &operator=(const MMLPlayer &) = delete;
    MMLPlayer &operator=(MMLPlayer &&) = delete;
    MMLPlayer(MMLPlayer &&) = delete;

    /// @brief The only constructor
    /// @param pWaveform A valid Waveform object pointer. This cannot be NULL
    MMLPlayer(Waveform *pWaveform) {
        command = INVALID;
        note = INVALID;
        tempo = DEFAULT_TEMPO;
        octave = DEFAULT_OCTAVE;
        length = DEFAULT_LENGTH;
        pause = DEFAULT_PAUSE;
        volume = DEFAULT_VOLUME;
        background = false;
        mix = false;
        duration = 0;
        waveform = pWaveform;
        ZERO_VARIABLE(currentState);
    }

    /// @brief Returns if music is to be played in the background
    /// @return True for background, false for foreground
    bool IsBackgroundPlayback() { return background; }

    /// @brief The is what should be called to parse and play an MML string
    /// @param mml A string containing the MML tune
    void Play(qbs *mml) {
        if (!mml || !mml->len) // exit if string is empty
            return;

        stateStack.push({mml->chr, (size_t)mml->len, 0}); // push the string to the state stack

        // Process until our state stack is empty
        while (!stateStack.empty()) {
            // Pop and use the top item in the state stack
            currentState = stateStack.top();
            stateStack.pop();

            // Parse and play each character in the MML string
            while (GetCommand()) {
                // Check the command and take appropriate action
                switch (command) {
                case 'A': // note A
                case 'B': // note B
                case 'C': // note C
                case 'D': // note D
                case 'E': // note E
                case 'F': // note F
                case 'G': // note G
                case 'N': // note 'n'
                    DoNote();
                    break;

                case 'L': // length
                    DoLength();
                    break;

                case 'M': // timing
                    DoTiming();
                    break;

                case 'O': // octave
                    DoOctave();
                    break;

                case 'P': // pause
                case 'R': // rest
                    DoSilence();
                    break;

                case 'T': // tempo
                    DoTempo();
                    break;

                case 'V': // volume
                    DoVolume();
                    break;

                case 'W': // waveform
                    DoWaveform();
                    break;

                case 'X': // substring
                    DoSubstring();
                    break;

                case '>': // octave ++
                    ++octave;
                    if (octave > MAX_OCTAVE)
                        octave = MAX_OCTAVE;
                    break;

                case '<': // octave --
                    --octave;
                    if (octave < MIN_OCTAVE)
                        octave = MIN_OCTAVE;
                    break;

                case ',': // mix
                    mix = not mix;
                    break;

                default: // unhandled stuff
                    AUDIO_DEBUG_PRINT("Command not handled '%c'", command);
                    break;
                }

                currentState.position++; // move to the next character
            }

            if (!waveform->Played()) // send any leftover samples for playback
                waveform->Play();
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
    enum Type { NONE, STATIC, RAW };

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
        type = Type::NONE;
        autoKill = false;
        ZERO_VARIABLE(maSound);
        maFlags = MA_SOUND_FLAG_NO_PITCH | MA_SOUND_FLAG_NO_SPATIALIZATION | MA_SOUND_FLAG_WAIT_INIT;
        maDecoder = nullptr;
        bufferKey = 0;
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
    Waveform *waveform;                                 // internal waveform object that we will use for Play(), Beep() & Sound()
    MMLPlayer *mmlPlayer;                               // internal MML player that we will use for Play()
    int32_t sndInternalRaw;                             // internal sound handle that we will use for the QB64 'handle-less' raw stream
    std::vector<SoundHandle *> soundHandles;            // this is the audio handle list used by the engine and by everything else
    int32_t lowestFreeHandle;                           // this is the lowest handle then was recently freed. We'll start checking for free handles from here
    bool musicBackground;                               // should 'Sound' and 'Play' work in the background or block the caller?
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
        sampleRate = 0;
        lowestFreeHandle = 0;
        sndInternal = sndInternalRaw = INVALID_SOUND_HANDLE;
        waveform = nullptr;
        mmlPlayer = nullptr;
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
            return -1;                              // We cannot return 0 here. Since 0 is a valid internal handle

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
    void FreeSoundHandle(int32_t handle) {
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

            // Save the free hanndle to lowestFreeHandle if it is lower than lowestFreeHandle
            if (handle < lowestFreeHandle)
                lowestFreeHandle = handle;

            AUDIO_DEBUG_PRINT("Sound handle %i marked as free", handle);
        }
    }
};

// This keeps track of the audio engine state
static AudioEngine audioEngine;

/// @brief This generates a sound at the specified frequency for the specified amount of time
/// @param frequency Sound frequency
/// @param lengthInClockTicks Duration in clock ticks. There are 18.2 clock ticks per second
void sub_sound(double frequency, double lengthInClockTicks) {
    if (new_error || !audioEngine.isInitialized || audioEngine.sndInternal != 0 || lengthInClockTicks == 0.0)
        return;

    if ((frequency < 37.0 && frequency != 0) || frequency > 32767.0 || lengthInClockTicks < 0.0 || lengthInClockTicks > 65535.0) {
        error(5);
        return;
    }

    // Kickstart the raw stream if it is not already
    if (!audioEngine.soundHandles[audioEngine.sndInternal]->rawStream) {
        // Initialize internal RawStream object
        audioEngine.soundHandles[audioEngine.sndInternal]->rawStream =
            RawStreamCreate(&audioEngine.maEngine, &audioEngine.soundHandles[audioEngine.sndInternal]->maSound);
        if (!audioEngine.soundHandles[audioEngine.sndInternal]->rawStream) { // failed
            AUDIO_DEBUG_PRINT("Failed to initialize RawStream object");
            return;
        }
        audioEngine.soundHandles[audioEngine.sndInternal]->type = SoundHandle::Type::RAW; // set type of sound
    }

    // Initialize internal Waveform object
    if (!audioEngine.waveform) {
        audioEngine.waveform = new Waveform(audioEngine.soundHandles[audioEngine.sndInternal]->rawStream);

        if (!audioEngine.waveform) {
            AUDIO_DEBUG_PRINT("Failed to initialize Waveform object");
            return;
        }
    }

    auto duration = lengthInClockTicks / 18.2;
    audioEngine.waveform->SetFrequency(frequency);
    audioEngine.waveform->Generate(duration, duration);
    audioEngine.waveform->Play();

    if (!audioEngine.musicBackground) // await playback to complete if we are in MF mode
        audioEngine.waveform->AwaitPlaybackCompletion();
}

/// @brief This generates a default 'beep' sound
void sub_beep() {
    sub_sound(900, 4.5);

    // We'll send a very short silence after the beep so that two successive beeps sound unique
    if (audioEngine.waveform) {
        auto duration = 0.5 / 18.2;
        auto waveType = audioEngine.waveform->GetType();
        audioEngine.waveform->SetType(Waveform::Type::SILENCE);
        audioEngine.waveform->Generate(duration, duration);
        audioEngine.waveform->Play();
        audioEngine.waveform->SetType(waveType);

        if (!audioEngine.musicBackground)
            audioEngine.waveform->AwaitPlaybackCompletion(); // await playback to complete if we are in MF mode
    }
}

/// @brief This was designed to returned the number of notes in the background music queue.
/// However, here we'll just return the number of sample frame remaining to play when Play(), Sound() or Beep() are used
/// @param ignore Well, it's ignored
/// @return Returns the number of sample frames left to play for Play(), Sound() & Beep()
int32_t func_play(int32_t ignore) {
    if (audioEngine.isInitialized && audioEngine.sndInternal == 0 && audioEngine.soundHandles[audioEngine.sndInternal]->rawStream) {
        return (int32_t)audioEngine.soundHandles[audioEngine.sndInternal]->rawStream->GetSampleFramesRemaining();
    }

    return 0;
}

/// @brief Processes and plays the MML specified in the string
/// @param str The string to play
void sub_play(qbs *str) {
    if (new_error || !audioEngine.isInitialized || audioEngine.sndInternal != 0)
        return;

    // Kickstart the raw stream if it is not already
    if (!audioEngine.soundHandles[audioEngine.sndInternal]->rawStream) {
        // Initialize internal RawStream object
        audioEngine.soundHandles[audioEngine.sndInternal]->rawStream =
            RawStreamCreate(&audioEngine.maEngine, &audioEngine.soundHandles[audioEngine.sndInternal]->maSound);
        if (!audioEngine.soundHandles[audioEngine.sndInternal]->rawStream) { // failed
            AUDIO_DEBUG_PRINT("Failed to initialize RawStream object");
            return;
        }
        audioEngine.soundHandles[audioEngine.sndInternal]->type = SoundHandle::Type::RAW; // set type of sound
    }

    // Initialize internal Waveform object
    if (!audioEngine.waveform) {
        audioEngine.waveform = new Waveform(audioEngine.soundHandles[audioEngine.sndInternal]->rawStream);

        if (!audioEngine.waveform) {
            AUDIO_DEBUG_PRINT("Failed to initialize Waveform object");
            return;
        }
    }

    // Initialize internal MMLPlayer
    if (!audioEngine.mmlPlayer) {
        audioEngine.mmlPlayer = new MMLPlayer(audioEngine.waveform);

        if (!audioEngine.mmlPlayer) {
            AUDIO_DEBUG_PRINT("Failed to initialize MMLPlayer object");
            return;
        }
    }

    audioEngine.mmlPlayer->Play(str);                                            // playback the string
    audioEngine.musicBackground = audioEngine.mmlPlayer->IsBackgroundPlayback(); // sync the background playback flag

    if (!audioEngine.musicBackground)                                            // await playback to complete if we are in MF mode
        audioEngine.waveform->AwaitPlaybackCompletion();
}

/// <summary>
/// This returns the sample rate from ma engine if ma is initialized.
/// </summary>
/// <returns>miniaudio sample rtate</returns>
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

    // Alocate a sound handle
    int32_t handle = audioEngine.AllocateSoundHandle();
    if (handle < 1) // We are not expected to open files with handle 0
        return INVALID_SOUND_HANDLE;

    // Set some handle properties
    audioEngine.soundHandles[handle]->type = SoundHandle::Type::STATIC;

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

        audioEngine.soundHandles[handle]->bufferKey = (intptr_t)fileName->chr;                                      // make a unique key and save it
        audioEngine.bufferMap.AddBuffer(fileName->chr, fileName->len, audioEngine.soundHandles[handle]->bufferKey); // make a copy of the buffer
        auto [buffer, bufferSize] = audioEngine.bufferMap.GetBuffer(audioEngine.soundHandles[handle]->bufferKey);   // get the buffer pointer and size
        audioEngine.maResult = InitializeSoundFromMemory(buffer, bufferSize, handle);                               // create the ma_sound
    } else {
        AUDIO_DEBUG_PRINT("Loading sound from file '%s'", fileNameZ->chr);
        qbs_set(fileNameZ, qbs_add(fileName, qbs_new_txt_len("\0", 1))); // s1 = filename + CHR$(0)

        // Forward the request to miniaudio to open the sound file
        audioEngine.maResult = ma_sound_init_from_file(&audioEngine.maEngine, (const char *)fileNameZ->chr, audioEngine.soundHandles[handle]->maFlags, NULL,
                                                       NULL, &audioEngine.soundHandles[handle]->maSound);
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
        // If we have a raw stream then force it to push all it's data to miniaudio
        // Note that this will take care of checking if the handle is a raw steam and other stuff
        // So it is completly safe to call it this way
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

        dst_handle = audioEngine.AllocateSoundHandle(); // alocate a sound handle
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

        dst_handle = audioEngine.AllocateSoundHandle(); // alocate a sound handle
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

            ma_vec3f v = ma_sound_get_position(&audioEngine.soundHandles[handle]->maSound);           // Get the current position in 3D space

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

    // Alocate a sound handle
    int32_t handle = audioEngine.AllocateSoundHandle();
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

    // Alocate a sound handle
    int32_t handle = audioEngine.AllocateSoundHandle();
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
    ma_format maFormat = ma_format::ma_format_unknown;
    ma_uint32 channels = 0;
    ma_uint64 sampleFrames = 0;
    ptrszint data = NULL;

    // Setup mem_block (assuming failure)
    mem_block mb = {};
    mb.lock_offset = (ptrszint)mem_lock_base;
    mb.lock_id = INVALID_MEM_LOCK;

    // Return invalid mem_block if audio is not initialized, handle is invalid or sound type is not static
    if (!audioEngine.isInitialized || !IS_SOUND_HANDLE_VALID(handle) || audioEngine.soundHandles[handle]->type != SoundHandle::Type::STATIC ||
        (targetChannel != 0 && targetChannel != 1)) {
        AUDIO_DEBUG_PRINT("Invalid handle (%i), sound type (%i) or channel (%i)", handle, audioEngine.soundHandles[handle]->type, targetChannel);
        return mb;
    }

    // Check what kind of sound we are dealing with and take appropriate path
    if (audioEngine.soundHandles[handle]->maAudioBuffer) { // we are dealing with a user created audio buffer
        AUDIO_DEBUG_PRINT("Entering ma_audio_buffer path");
        maFormat = audioEngine.soundHandles[handle]->maAudioBuffer->ref.format;
        channels = audioEngine.soundHandles[handle]->maAudioBuffer->ref.channels;
        sampleFrames = audioEngine.soundHandles[handle]->maAudioBuffer->ref.sizeInFrames;
        data = (ptrszint)audioEngine.soundHandles[handle]->maAudioBuffer->ref.pData;
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

    AUDIO_DEBUG_PRINT("Format = %u, channels = %u, frames = %llu", maFormat, channels, sampleFrames);

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

/// @brief This shuts down the audio engine and frees any resources used
void snd_un_init() {
    if (audioEngine.isInitialized) {
        // Free any MMLPlayer object if they were created
        if (audioEngine.mmlPlayer) {
            delete audioEngine.mmlPlayer;
            audioEngine.mmlPlayer = nullptr;
        }

        // Free any Waveform object if they were created and
        if (audioEngine.waveform) {
            delete audioEngine.waveform;
            audioEngine.waveform = nullptr;
        }

        // Free all sound handles here
        for (size_t handle = 0; handle < audioEngine.soundHandles.size(); handle++) {
            audioEngine.FreeSoundHandle(handle);     // let FreeSoundHandle do it's thing
            delete audioEngine.soundHandles[handle]; // now free the object created by AllocateSoundHandle()
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
                        // Well thats on the programmer. Probably they want it that way
                        if (!ma_sound_is_playing(&audioEngine.soundHandles[handle]->maSound))
                            audioEngine.FreeSoundHandle(handle);

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
