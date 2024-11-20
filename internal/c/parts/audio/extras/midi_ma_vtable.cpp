//----------------------------------------------------------------------------------------------------------------------
//  QB64-PE Audio Engine powered by miniaudio (https://miniaud.io/)
//
//  This implements a data source that decodes MIDI files using:
//  foo_midi (heavily modified) from https://github.com/stuerp/foo_midi (MIT license)
//  libmidi (modified) https://github.com/stuerp/libmidi (MIT license)
//  Opal (refactored) from https://www.3eality.com/productions/reality-adlib-tracker (Public Domain)
//  primesynth (heavily modified) from https://github.com/mosmeh/primesynth (MIT license)
//  stb_vorbis.c from https://github.com/nothings/stb (Public Domain)
//  TinySoundFont from https://github.com/schellingb/TinySoundFont (MIT license)
//  ymfmidi (heavily modified) from https://github.com/devinacker/ymfmidi (BSD-3-Clause license)
//----------------------------------------------------------------------------------------------------------------------

#include "../framework.h"
#include "foo_midi/InstrumentBankManager.h"
#include "foo_midi/MIDIPlayer.h"
#include "foo_midi/OpalPlayer.h"
#include "foo_midi/PSPlayer.h"
#include "foo_midi/TSFPlayer.h"
#ifdef _WIN32
#    include "foo_midi/VSTiPlayer.h"
#endif
#include "libmidi/MIDIContainer.h"
#include "libmidi/MIDIProcessor.h"

struct ma_midi {
    // This part is for miniaudio
    ma_data_source_base ds; /* The decoder can be used independently as a data source. */
    ma_read_proc onRead;
    ma_seek_proc onSeek;
    ma_tell_proc onTell;
    void *pReadSeekTellUserData;
    ma_format format;

    // This part is format specific
    MIDIPlayer *sequencer;       // foo_midi sequencer
    midi_container_t *container; // foo_midi - libmidi container
    uint32_t trackNumber;        // the MIDI track number to played (this is automatically set to the first playable track)
    ma_int64 totalTime;          // total duration of the MIDI song in frames
    bool isPlaying;              // this holds the playing state
#ifdef _WIN32
    DoubleBufferFrameBlock *frameBlock; // only needed when a player cannot do variable frame size rendering (e.g. VSTiPlayer)
    bool isReallyPlaying;               // this holds the real playing state and is needed due to the same reason as above
#endif
};

static ma_result ma_midi_seek_to_pcm_frame(ma_midi *pMIDI, ma_uint64 frameIndex) {
    if (pMIDI == NULL) {
        return MA_INVALID_ARGS;
    }

    // We can only reset the player to the beginning
    pMIDI->sequencer->Seek(uint32_t(frameIndex));

    return MA_SUCCESS;
}

static ma_result ma_midi_get_data_format(ma_midi *pMIDI, ma_format *pFormat, ma_uint32 *pChannels, ma_uint32 *pSampleRate, ma_channel *pChannelMap,
                                         size_t channelMapCap) {
    /* Defaults for safety. */
    if (pFormat != NULL) {
        *pFormat = ma_format_unknown;
    }
    if (pChannels != NULL) {
        *pChannels = 0;
    }
    if (pSampleRate != NULL) {
        *pSampleRate = 0;
    }
    if (pChannelMap != NULL) {
        memset(pChannelMap, 0, sizeof(*pChannelMap) * channelMapCap);
    }

    if (pMIDI == NULL) {
        return MA_INVALID_OPERATION;
    }

    if (pFormat != NULL) {
        *pFormat = pMIDI->format;
    }

    if (pChannels != NULL) {
        *pChannels = 2; // Stereo
    }

    if (pSampleRate != NULL) {
        *pSampleRate = MA_DEFAULT_SAMPLE_RATE;
    }

    if (pChannelMap != NULL) {
        ma_channel_map_init_standard(ma_standard_channel_map_default, pChannelMap, channelMapCap, 2);
    }

    return MA_SUCCESS;
}

static ma_result ma_midi_read_pcm_frames(ma_midi *pMIDI, void *pFramesOut, ma_uint64 frameCount, ma_uint64 *pFramesRead) {
    if (pFramesRead != NULL) {
        *pFramesRead = 0;
    }

    if (frameCount == 0 || pFramesOut == NULL || pMIDI == NULL) {
        return MA_INVALID_ARGS;
    }

    auto result = MA_SUCCESS; // Must be initialized to MA_SUCCESS
    ma_uint64 totalFramesRead = 0;

#ifdef _WIN32
    const auto fixedFrames = pMIDI->sequencer->GetSampleBlockSize();

    if (fixedFrames) {
        // Only attempt to render if we are actually playing
        if (pMIDI->isReallyPlaying) {
            auto dest = pMIDI->frameBlock->GetWriteBlock(fixedFrames);
            if (dest)
                pMIDI->isReallyPlaying = pMIDI->sequencer->Play(dest, fixedFrames) > 0;
        }

        // Get partial data from the frame block
        totalFramesRead = pMIDI->frameBlock->ReadFrames(reinterpret_cast<float *>(pFramesOut), frameCount);

        // Set the isPlaying flag to true if we still have some data in the buffers
        pMIDI->isPlaying = !pMIDI->frameBlock->IsEmpty();
    } else
#endif
    {
        totalFramesRead = pMIDI->sequencer->Play(reinterpret_cast<float *>(pFramesOut), frameCount);
        pMIDI->isPlaying = totalFramesRead > 0;
    }

    // Signal end of stream if we have reached the end
    if (!pMIDI->isPlaying) {
        result = MA_AT_END;
        AUDIO_DEBUG_PRINT("Reached end of stream");
    }

    if (pFramesRead != NULL) {
        *pFramesRead = totalFramesRead;
    }

    return result;
}

static ma_result ma_midi_get_cursor_in_pcm_frames(ma_midi *pMIDI, ma_uint64 *pCursor) {
    if (pCursor == NULL || pMIDI == NULL) {
        return MA_INVALID_ARGS;
    }

    *pCursor = 0; /* Safety. */

    auto offset = ma_int64(pMIDI->sequencer->GetFramePosition());
    if (offset < 0) {
        return MA_INVALID_FILE;
    }

    *pCursor = ma_uint64(offset);

    return MA_SUCCESS;
}

static ma_result ma_midi_get_length_in_pcm_frames(ma_midi *pMIDI, ma_uint64 *pLength) {
    if (pLength == NULL || pMIDI == NULL) {
        return MA_INVALID_ARGS;
    }

    *pLength = 0; /* Safety. */

    ma_int64 length = pMIDI->totalTime;
    if (length < 0) {
        return MA_INVALID_FILE;
    }

    *pLength = ma_uint64(length);

    return MA_SUCCESS;
}

static ma_result ma_midi_ds_read(ma_data_source *pDataSource, void *pFramesOut, ma_uint64 frameCount, ma_uint64 *pFramesRead) {
    return ma_midi_read_pcm_frames((ma_midi *)pDataSource, pFramesOut, frameCount, pFramesRead);
}

static ma_result ma_midi_ds_seek(ma_data_source *pDataSource, ma_uint64 frameIndex) {
    return ma_midi_seek_to_pcm_frame((ma_midi *)pDataSource, frameIndex);
}

static ma_result ma_midi_ds_get_data_format(ma_data_source *pDataSource, ma_format *pFormat, ma_uint32 *pChannels, ma_uint32 *pSampleRate,
                                            ma_channel *pChannelMap, size_t channelMapCap) {
    return ma_midi_get_data_format((ma_midi *)pDataSource, pFormat, pChannels, pSampleRate, pChannelMap, channelMapCap);
}

static ma_result ma_midi_ds_get_cursor(ma_data_source *pDataSource, ma_uint64 *pCursor) {
    return ma_midi_get_cursor_in_pcm_frames((ma_midi *)pDataSource, pCursor);
}

static ma_result ma_midi_ds_get_length(ma_data_source *pDataSource, ma_uint64 *pLength) {
    return ma_midi_get_length_in_pcm_frames((ma_midi *)pDataSource, pLength);
}

// clang-format off
static ma_data_source_vtable ma_data_source_vtable_midi = {
    ma_midi_ds_read, ma_midi_ds_seek,
    ma_midi_ds_get_data_format,
    ma_midi_ds_get_cursor,
    ma_midi_ds_get_length
};
// clang-format on

static int ma_midi_of_callback__read(void *pUserData, unsigned char *pBufferOut, int bytesToRead) {
    ma_midi *pMIDI = (ma_midi *)pUserData;
    ma_result result;
    size_t bytesRead;

    result = pMIDI->onRead(pMIDI->pReadSeekTellUserData, (void *)pBufferOut, bytesToRead, &bytesRead);

    if (result != MA_SUCCESS) {
        return -1;
    }

    return (int)bytesRead;
}

static int ma_midi_of_callback__seek(void *pUserData, ma_int64 offset, int whence) {
    ma_midi *pMIDI = (ma_midi *)pUserData;
    ma_result result;
    ma_seek_origin origin;

    if (whence == SEEK_SET) {
        origin = ma_seek_origin_start;
    } else if (whence == SEEK_END) {
        origin = ma_seek_origin_end;
    } else {
        origin = ma_seek_origin_current;
    }

    result = pMIDI->onSeek(pMIDI->pReadSeekTellUserData, offset, origin);
    if (result != MA_SUCCESS) {
        return -1;
    }

    return 0;
}

static ma_int64 ma_midi_of_callback__tell(void *pUserData) {
    ma_midi *pMIDI = (ma_midi *)pUserData;
    ma_result result;
    ma_int64 cursor;

    if (pMIDI->onTell == NULL) {
        return -1;
    }

    result = pMIDI->onTell(pMIDI->pReadSeekTellUserData, &cursor);
    if (result != MA_SUCCESS) {
        return -1;
    }

    return cursor;
}

static ma_result ma_midi_init_internal(const ma_decoding_backend_config *pConfig, ma_midi *pMIDI) {
    ma_result result;
    ma_data_source_config dataSourceConfig;

    if (pMIDI == NULL) {
        return MA_INVALID_ARGS;
    }

    memset(pMIDI, 0, sizeof(*pMIDI));
    pMIDI->format = ma_format::ma_format_f32; // We'll render 32-bit floating point samples

    if (pConfig != NULL && pConfig->preferredFormat == ma_format::ma_format_f32) {
        pMIDI->format = pConfig->preferredFormat;
    } else {
        /* Getting here means something other than f32 was specified. Just leave this unset to use the default format. */
    }

    dataSourceConfig = ma_data_source_config_init();
    dataSourceConfig.vtable = &ma_data_source_vtable_midi;

    result = ma_data_source_init(&dataSourceConfig, &pMIDI->ds);
    if (result != MA_SUCCESS) {
        return result; /* Failed to initialize the base data source. */
    }

    return MA_SUCCESS;
}

/// @brief Common cleanup routine. Assumes pMIDI is valid
/// @param pMIDI Valid pointer to a ma_midi object
static void ma_midi_uninit_common(ma_midi *pMIDI) {
    AUDIO_DEBUG_PRINT("Deleting foo_midi objects");

    delete pMIDI->container;
    pMIDI->container = nullptr;
    AUDIO_DEBUG_PRINT("foo_midi container deleted");

    delete pMIDI->sequencer;
    pMIDI->sequencer = nullptr;
    AUDIO_DEBUG_PRINT("foo_midi sequencer deleted");

#ifdef _WIN32
    delete pMIDI->frameBlock;
    pMIDI->frameBlock = nullptr;
    AUDIO_DEBUG_PRINT("foo_midi frameBlock deleted");
#endif

    AUDIO_DEBUG_PRINT("foo_midi objects deleted");
}

/// @brief Common init routine for memory and file based MIDI. This does not check any of the parameters and expects them to be valid.
/// @param pMIDI Valid pointer to a ma_midi object
/// @param tune The tune to load
/// @return Result code (MA_SUCCESS on success)
static auto ma_midi_init_common(ma_midi *pMIDI, const std::vector<uint8_t> &tune, const char *pFilePath) {
    AUDIO_DEBUG_CHECK(pMIDI != NULL);
    AUDIO_DEBUG_CHECK(pMIDI->container == nullptr);
    AUDIO_DEBUG_CHECK(pMIDI->sequencer == nullptr);
    AUDIO_DEBUG_PRINT("Tune size: %llu", tune.size());

    // Create the synthesizer based on the sound bank
    switch (g_InstrumentBankManager.GetType()) {
    case InstrumentBankManager::Type::Primesynth:
        pMIDI->sequencer = new PSPlayer(&g_InstrumentBankManager);
        AUDIO_DEBUG_PRINT("Using primesynth");
        break;

    case InstrumentBankManager::Type::TinySoundFont:
        pMIDI->sequencer = new TSFPlayer(&g_InstrumentBankManager);
        AUDIO_DEBUG_PRINT("Using TinySoundFont");
        break;

#ifdef _WIN32
    case InstrumentBankManager::Type::VSTi:
        pMIDI->sequencer = new VSTiPlayer(&g_InstrumentBankManager);
        AUDIO_DEBUG_PRINT("Using VSTi");
        break;
#endif

    default:
        AUDIO_DEBUG_PRINT("Unknown synth type. Falling back to OpalPlayer");
        g_InstrumentBankManager.SetPath(nullptr);
        [[fallthrough]];

    case InstrumentBankManager::Type::Opal:
        pMIDI->sequencer = new OpalPlayer(&g_InstrumentBankManager);
        AUDIO_DEBUG_PRINT("Using OpalPlayer");
        break;
    }

    if (!pMIDI->sequencer) {
        AUDIO_DEBUG_PRINT("Failed to create sequencer instance");
        return MA_INVALID_FILE; // failure here will be mostly due to bad sound bank
    }

    // Set sample rate
    pMIDI->sequencer->SetSampleRate(MA_DEFAULT_SAMPLE_RATE);
    AUDIO_DEBUG_PRINT("Sample rate set to %d", MA_DEFAULT_SAMPLE_RATE);

    // Create the MIDI container object
    pMIDI->container = new midi_container_t();
    if (!pMIDI->container) {
        ma_midi_uninit_common(pMIDI);
        return MA_OUT_OF_MEMORY;
    }

    try {
        if (!midi_processor_t::Process(tune, pFilePath, *pMIDI->container)) {
            ma_midi_uninit_common(pMIDI);
            AUDIO_DEBUG_PRINT("Not a valid MIDI file");
            return MA_INVALID_FILE;
        }
    } catch (std::exception &e) {
        AUDIO_DEBUG_PRINT("Exception: %s", e.what());
        ma_midi_uninit_common(pMIDI);
        return MA_INVALID_FILE;
    }

    auto trackCount = pMIDI->container->GetTrackCount();
    if (!trackCount) {
        ma_midi_uninit_common(pMIDI);
        return MA_INVALID_FILE;
    }

    bool hasDuration = false;
    pMIDI->trackNumber = 0;

    // Probe and check the track number we can play
    for (uint32_t i = 0; i < trackCount; i++) {
        pMIDI->totalTime = (ma_int64(pMIDI->container->GetDuration(i, true)) * MA_DEFAULT_SAMPLE_RATE) / 1000; // convert this to frames
        AUDIO_DEBUG_PRINT("MIDI track %u, duration: %lld frames", i, pMIDI->totalTime);

        if (pMIDI->totalTime) {
            hasDuration = true;
            pMIDI->trackNumber = i;
            break;
        }
    }

    if (!hasDuration) {
        ma_midi_uninit_common(pMIDI);
        return MA_INVALID_FILE;
    }

#ifdef _WIN32
    pMIDI->frameBlock = new DoubleBufferFrameBlock();
    if (!pMIDI->frameBlock) {
        ma_midi_uninit_common(pMIDI);
        return MA_OUT_OF_MEMORY;
    }
#endif

    // Detect all possible loop information
    pMIDI->container->DetectLoops(true, true, true, true, true);

    try {
        if (pMIDI->sequencer->Load(*pMIDI->container, pMIDI->trackNumber, LoopType::NeverLoop, 0)) {
            // Set play state flags
            pMIDI->isPlaying = true;
#ifdef _WIN32
            pMIDI->isReallyPlaying = true;
            pMIDI->frameBlock->Reset();
#endif
        }
    } catch (std::exception &e) {
        AUDIO_DEBUG_PRINT("Exception: %s", e.what());
        ma_midi_uninit_common(pMIDI);
        return MA_INVALID_FILE;
    }

    AUDIO_DEBUG_PRINT("MIDI initialized");

    return MA_SUCCESS;
}

static ma_result ma_midi_init(ma_read_proc onRead, ma_seek_proc onSeek, ma_tell_proc onTell, void *pReadSeekTellUserData,
                              const ma_decoding_backend_config *pConfig, const ma_allocation_callbacks *pAllocationCallbacks, ma_midi *pMIDI) {
    ma_result result;

    (void)pAllocationCallbacks;

    result = ma_midi_init_internal(pConfig, pMIDI);
    if (result != MA_SUCCESS) {
        return result;
    }

    if (onRead == NULL || onSeek == NULL) {
        return MA_INVALID_ARGS; /* onRead and onSeek are mandatory. */
    }

    pMIDI->onRead = onRead;
    pMIDI->onSeek = onSeek;
    pMIDI->onTell = onTell;
    pMIDI->pReadSeekTellUserData = pReadSeekTellUserData;

    // Seek to the end of the file
    if (ma_midi_of_callback__seek(pMIDI, 0, SEEK_END) != 0) {
        return MA_BAD_SEEK;
    }

    // Calculate the length
    ma_int64 file_size = ma_midi_of_callback__tell(pMIDI);
    if (file_size < 1) {
        return MA_INVALID_FILE;
    }

    // See to the beginning of the file
    if (ma_midi_of_callback__seek(pMIDI, 0, SEEK_SET) != 0) {
        return MA_BAD_SEEK;
    }

    // Allocate some memory for the tune
    std::vector<uint8_t> tune(file_size);

    // Read the file
    if (ma_midi_of_callback__read(pMIDI, &tune[0], (int)file_size) < 1) {
        return MA_IO_ERROR;
    }

    return ma_midi_init_common(pMIDI, tune, "");
}

static ma_result ma_midi_init_file(const char *pFilePath, const ma_decoding_backend_config *pConfig, const ma_allocation_callbacks *pAllocationCallbacks,
                                   ma_midi *pMIDI) {
    ma_result result;

    (void)pAllocationCallbacks;

    result = ma_midi_init_internal(pConfig, pMIDI);
    if (result != MA_SUCCESS) {
        return result;
    }

    // Check the file extension (mus, hmi, hmp, hmq, kar, lds, mds, mids, rcp, r36, g18, g36, rmi, mid, midi, xfm, xmi)
    if (!filepath_has_extension(pFilePath, "mus") && !filepath_has_extension(pFilePath, "hmi") && !filepath_has_extension(pFilePath, "hmp") &&
        !filepath_has_extension(pFilePath, "hmq") && !filepath_has_extension(pFilePath, "kar") && !filepath_has_extension(pFilePath, "lds") &&
        !filepath_has_extension(pFilePath, "mds") && !filepath_has_extension(pFilePath, "mids") && !filepath_has_extension(pFilePath, "rcp") &&
        !filepath_has_extension(pFilePath, "r36") && !filepath_has_extension(pFilePath, "g18") && !filepath_has_extension(pFilePath, "g36") &&
        !filepath_has_extension(pFilePath, "rmi") && !filepath_has_extension(pFilePath, "mid") && !filepath_has_extension(pFilePath, "midi") &&
        !filepath_has_extension(pFilePath, "xfm") && !filepath_has_extension(pFilePath, "xmi")) {
        return MA_INVALID_FILE;
    }

    auto tune = AudioEngine_LoadFile<std::vector<uint8_t>>(pFilePath);
    if (tune.empty()) {
        return MA_INVALID_FILE;
    }

    return ma_midi_init_common(pMIDI, tune, pFilePath);
}

static void ma_midi_uninit(ma_midi *pMIDI, const ma_allocation_callbacks *pAllocationCallbacks) {
    if (pMIDI == NULL) {
        return;
    }

    (void)pAllocationCallbacks;

    ma_midi_uninit_common(pMIDI);
    ma_data_source_uninit(&pMIDI->ds);

    AUDIO_DEBUG_PRINT("MIDI uninitialized");
}

static ma_result ma_decoding_backend_init__midi(void *pUserData, ma_read_proc onRead, ma_seek_proc onSeek, ma_tell_proc onTell, void *pReadSeekTellUserData,
                                                const ma_decoding_backend_config *pConfig, const ma_allocation_callbacks *pAllocationCallbacks,
                                                ma_data_source **ppBackend) {
    ma_result result;
    ma_midi *pMIDI;

    (void)pUserData;

    pMIDI = (ma_midi *)ma_malloc(sizeof(ma_midi), pAllocationCallbacks);
    if (pMIDI == NULL) {
        return MA_OUT_OF_MEMORY;
    }

    result = ma_midi_init(onRead, onSeek, onTell, pReadSeekTellUserData, pConfig, pAllocationCallbacks, pMIDI);
    if (result != MA_SUCCESS) {
        ma_free(pMIDI, pAllocationCallbacks);
        return result;
    }

    *ppBackend = pMIDI;

    return MA_SUCCESS;
}

static ma_result ma_decoding_backend_init_file__midi(void *pUserData, const char *pFilePath, const ma_decoding_backend_config *pConfig,
                                                     const ma_allocation_callbacks *pAllocationCallbacks, ma_data_source **ppBackend) {
    ma_result result;
    ma_midi *pMIDI;

    (void)pUserData;

    pMIDI = (ma_midi *)ma_malloc(sizeof(ma_midi), pAllocationCallbacks);
    if (pMIDI == NULL) {
        return MA_OUT_OF_MEMORY;
    }

    result = ma_midi_init_file(pFilePath, pConfig, pAllocationCallbacks, pMIDI);
    if (result != MA_SUCCESS) {
        ma_free(pMIDI, pAllocationCallbacks);
        return result;
    }

    *ppBackend = pMIDI;

    return MA_SUCCESS;
}

static void ma_decoding_backend_uninit__midi(void *pUserData, ma_data_source *pBackend, const ma_allocation_callbacks *pAllocationCallbacks) {
    ma_midi *pMIDI = (ma_midi *)pBackend;

    (void)pUserData;

    ma_midi_uninit(pMIDI, pAllocationCallbacks);
    ma_free(pMIDI, pAllocationCallbacks);
}

// clang-format off
ma_decoding_backend_vtable ma_vtable_midi = {
    ma_decoding_backend_init__midi,
    ma_decoding_backend_init_file__midi,
    NULL, /* onInitFileW() */
    NULL, /* onInitMemory() */
    ma_decoding_backend_uninit__midi
};
// clang-format on
