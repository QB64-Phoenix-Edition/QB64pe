//----------------------------------------------------------------------------------------------------------------------
//  QB64-PE Audio Engine powered by miniaudio (https://miniaud.io/)
//
//  This implements a data source that decodes MOD, S3M, XM & IT files using libxmp-lite
//  https://github.com/libxmp/libxmp/tree/master/lite (MIT)
//----------------------------------------------------------------------------------------------------------------------

#include "../framework.h"
#define LIBXMP_STATIC 1
#include "libxmp-lite/xmp.h"

struct ma_modplay {
    // This part is for miniaudio
    ma_data_source_base ds; /* The decoder can be used independently as a data source. */
    ma_read_proc onRead;
    ma_seek_proc onSeek;
    ma_tell_proc onTell;
    void *pReadSeekTellUserData;
    ma_format format;

    // This part is format specific
    xmp_context xmpContext;                             // The player context
    xmp_frame_info xmpFrameInfo;                        // LibXMP frameinfo - used to detect loops
    DoubleBufferFrameBlock<SampleFrameI16> *frameBlock; // needed because Libxmp buffer_size (frames) can be greater than miniaudio's frameCount
    bool isReallyPlaying;                               // this holds the real playing state and is needed due to the same reason as above
};

static ma_result ma_modplay_seek_to_pcm_frame(ma_modplay *pModplay, ma_uint64 frameIndex) {
    if (pModplay == NULL) {
        return MA_INVALID_ARGS;
    }

    if (xmp_seek_time(pModplay->xmpContext, (int)((frameIndex * 1000) / MA_DEFAULT_SAMPLE_RATE)) == -XMP_ERROR_STATE) {
        return MA_INVALID_OPERATION;
    }

    return MA_SUCCESS;
}

static ma_result ma_modplay_get_data_format(ma_modplay *pModplay, ma_format *pFormat, ma_uint32 *pChannels, ma_uint32 *pSampleRate, ma_channel *pChannelMap,
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

    if (pModplay == NULL) {
        return MA_INVALID_OPERATION;
    }

    if (pFormat != NULL) {
        *pFormat = pModplay->format;
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

static ma_result ma_modplay_read_pcm_frames(ma_modplay *pModplay, void *pFramesOut, ma_uint64 frameCount, ma_uint64 *pFramesRead) {
    if (pFramesRead != NULL) {
        *pFramesRead = 0;
    }

    if (frameCount == 0 || pFramesOut == NULL || pModplay == NULL) {
        return MA_INVALID_ARGS;
    }

    ma_result result = MA_SUCCESS;
    ma_uint64 totalFramesRead = 0;

    // Only attempt to render if we are actually playing
    if (pModplay->isReallyPlaying && pModplay->frameBlock->IsWriteBlockEmpty()) {
        if (xmp_play_frame(pModplay->xmpContext) == 0) {
            xmp_get_frame_info(pModplay->xmpContext, &pModplay->xmpFrameInfo);

            if (pModplay->xmpFrameInfo.loop_count == 0 && pModplay->xmpFrameInfo.buffer && pModplay->xmpFrameInfo.buffer_size > 0) {
                ma_uint64 bufferBytes = pModplay->xmpFrameInfo.buffer_size;

                auto buffer = pModplay->frameBlock->GetWriteBlock(bufferBytes >> 2); // 16-bit stereo
                if (buffer) {
                    std::memcpy(buffer, pModplay->xmpFrameInfo.buffer, bufferBytes);
                }
            } else {
                pModplay->isReallyPlaying = false;
            }
        } else {
            pModplay->isReallyPlaying = false;
        }
    }

    totalFramesRead = pModplay->frameBlock->ReadFrames(reinterpret_cast<SampleFrameI16 *>(pFramesOut), frameCount);

    if (pModplay->frameBlock->IsEmpty() && !pModplay->isReallyPlaying) {
        result = MA_AT_END;
        audio_log_trace("Finished rendering module music");
    }

    if (pFramesRead != NULL) {
        *pFramesRead = totalFramesRead;
    }

    return result;
}

static ma_result ma_modplay_get_cursor_in_pcm_frames(ma_modplay *pModplay, ma_uint64 *pCursor) {
    if (!pCursor) {
        return MA_INVALID_ARGS;
    }

    *pCursor = 0; /* Safety. */

    if (!pModplay) {
        return MA_INVALID_ARGS;
    }

    // Get the frame information
    xmp_get_frame_info(pModplay->xmpContext, &pModplay->xmpFrameInfo);

    ma_int64 offset = ((ma_int64)pModplay->xmpFrameInfo.time * MA_DEFAULT_SAMPLE_RATE) / 1000;
    if (offset < 0) {
        return MA_INVALID_FILE;
    }

    *pCursor = (ma_uint64)offset;

    return MA_SUCCESS;
}

static ma_result ma_modplay_get_length_in_pcm_frames(ma_modplay *pModplay, ma_uint64 *pLength) {
    if (!pLength) {
        return MA_INVALID_ARGS;
    }

    *pLength = 0; /* Safety. */

    if (!pModplay) {
        return MA_INVALID_ARGS;
    }

    ma_int64 length = ((ma_int64)pModplay->xmpFrameInfo.total_time * MA_DEFAULT_SAMPLE_RATE) / 1000;
    if (length < 0) {
        return MA_INVALID_FILE;
    }

    *pLength = (ma_uint64)length;

    return MA_SUCCESS;
}

static ma_result ma_modplay_ds_read(ma_data_source *pDataSource, void *pFramesOut, ma_uint64 frameCount, ma_uint64 *pFramesRead) {
    return ma_modplay_read_pcm_frames((ma_modplay *)pDataSource, pFramesOut, frameCount, pFramesRead);
}

static ma_result ma_modplay_ds_seek(ma_data_source *pDataSource, ma_uint64 frameIndex) {
    return ma_modplay_seek_to_pcm_frame((ma_modplay *)pDataSource, frameIndex);
}

static ma_result ma_modplay_ds_get_data_format(ma_data_source *pDataSource, ma_format *pFormat, ma_uint32 *pChannels, ma_uint32 *pSampleRate,
                                               ma_channel *pChannelMap, size_t channelMapCap) {
    return ma_modplay_get_data_format((ma_modplay *)pDataSource, pFormat, pChannels, pSampleRate, pChannelMap, channelMapCap);
}

static ma_result ma_modplay_ds_get_cursor(ma_data_source *pDataSource, ma_uint64 *pCursor) {
    return ma_modplay_get_cursor_in_pcm_frames((ma_modplay *)pDataSource, pCursor);
}

static ma_result ma_modplay_ds_get_length(ma_data_source *pDataSource, ma_uint64 *pLength) {
    return ma_modplay_get_length_in_pcm_frames((ma_modplay *)pDataSource, pLength);
}

// clang-format off
static ma_data_source_vtable ma_data_source_vtable_modplay = {
    ma_modplay_ds_read,
    ma_modplay_ds_seek,
    ma_modplay_ds_get_data_format,
    ma_modplay_ds_get_cursor,
    ma_modplay_ds_get_length
};
// clang-format on

static int ma_modplay_of_callback__read(void *pUserData, unsigned char *pBufferOut, int bytesToRead) {
    ma_modplay *pModplay = (ma_modplay *)pUserData;
    ma_result result;
    size_t bytesRead;

    result = pModplay->onRead(pModplay->pReadSeekTellUserData, (void *)pBufferOut, bytesToRead, &bytesRead);

    if (result != MA_SUCCESS) {
        return -1;
    }

    return (int)bytesRead;
}

static int ma_modplay_of_callback__seek(void *pUserData, ma_int64 offset, int whence) {
    ma_modplay *pModplay = (ma_modplay *)pUserData;
    ma_result result;
    ma_seek_origin origin;

    if (whence == SEEK_SET) {
        origin = ma_seek_origin_start;
    } else if (whence == SEEK_END) {
        origin = ma_seek_origin_end;
    } else {
        origin = ma_seek_origin_current;
    }

    result = pModplay->onSeek(pModplay->pReadSeekTellUserData, offset, origin);
    if (result != MA_SUCCESS) {
        return -1;
    }

    return 0;
}

static ma_int64 ma_modplay_of_callback__tell(void *pUserData) {
    ma_modplay *pModplay = (ma_modplay *)pUserData;
    ma_result result;
    ma_int64 cursor;

    if (pModplay->onTell == NULL) {
        return -1;
    }

    result = pModplay->onTell(pModplay->pReadSeekTellUserData, &cursor);
    if (result != MA_SUCCESS) {
        return -1;
    }

    return cursor;
}

static ma_result ma_modplay_init_internal(const ma_decoding_backend_config *pConfig, ma_modplay *pModplay) {
    ma_result result;
    ma_data_source_config dataSourceConfig;

    if (pModplay == NULL) {
        return MA_INVALID_ARGS;
    }

    memset(pModplay, 0, sizeof(*pModplay));
    pModplay->format = ma_format::ma_format_s16; // We'll render 16-bit signed samples by default

    if (pConfig != NULL && pConfig->preferredFormat == ma_format::ma_format_s16) {
        pModplay->format = pConfig->preferredFormat;
    } else {
        /* Getting here means something other than s16 was specified. Just leave this unset to use the default format. */
    }

    dataSourceConfig = ma_data_source_config_init();
    dataSourceConfig.vtable = &ma_data_source_vtable_modplay;

    result = ma_data_source_init(&dataSourceConfig, &pModplay->ds);
    if (result != MA_SUCCESS) {
        return result; /* Failed to initialize the base data source. */
    }

    return MA_SUCCESS;
}

static ma_result ma_modplay_init(ma_read_proc onRead, ma_seek_proc onSeek, ma_tell_proc onTell, void *pReadSeekTellUserData,
                                 const ma_decoding_backend_config *pConfig, const ma_allocation_callbacks *pAllocationCallbacks, ma_modplay *pModplay) {
    ma_result result;

    (void)pAllocationCallbacks;

    result = ma_modplay_init_internal(pConfig, pModplay);
    if (result != MA_SUCCESS) {
        return result;
    }

    if (onRead == NULL || onSeek == NULL) {
        return MA_INVALID_ARGS; /* onRead and onSeek are mandatory. */
    }

    pModplay->onRead = onRead;
    pModplay->onSeek = onSeek;
    pModplay->onTell = onTell;
    pModplay->pReadSeekTellUserData = pReadSeekTellUserData;

    // Find the size of the file
    if (ma_modplay_of_callback__seek(pModplay, 0, SEEK_END) != 0) {
        return MA_BAD_SEEK;
    }

    // Calculate the length
    ma_int64 file_size = ma_modplay_of_callback__tell(pModplay);
    if (file_size < 1) {
        return MA_INVALID_FILE;
    }

    // Seek to the beginning of the file
    if (ma_modplay_of_callback__seek(pModplay, 0, SEEK_SET) != 0) {
        return MA_BAD_SEEK;
    }

    // Allocate some memory for the tune
    std::vector<uint8_t> tune(file_size);

    // Read the file
    if (ma_modplay_of_callback__read(pModplay, tune.data(), (int)file_size) < 1) {
        return MA_IO_ERROR;
    }

    // Check if the file is a valid module music
    xmp_test_info xmpTestInfo;
    int xmpError = xmp_test_module_from_memory(tune.data(), (long)file_size, &xmpTestInfo);
    if (xmpError != 0) {
        audio_log_warn("Not a valid music module file");
        return MA_INVALID_FILE;
    }

    pModplay->frameBlock = new DoubleBufferFrameBlock<SampleFrameI16>();
    if (!pModplay->frameBlock) {
        return MA_OUT_OF_MEMORY;
    }

    // Initialize the player
    pModplay->xmpContext = xmp_create_context();
    if (!pModplay->xmpContext) {
        return MA_OUT_OF_MEMORY;
    }

    // Load the module file
    xmpError = xmp_load_module_from_memory(pModplay->xmpContext, tune.data(), (long)file_size);
    if (xmpError != 0) {
        xmp_free_context(pModplay->xmpContext);
        pModplay->xmpContext = nullptr;
        return MA_INVALID_FILE;
    }

    // Initialize the player
    xmpError = xmp_start_player(pModplay->xmpContext, MA_DEFAULT_SAMPLE_RATE, 0);
    if (xmpError != 0) {
        xmp_release_module(pModplay->xmpContext);
        xmp_free_context(pModplay->xmpContext);
        pModplay->xmpContext = nullptr;
        return MA_INVALID_FILE;
    }

    // Set some player properties. These are not critical. So, we will not check the return values
    // These makes the sound quality much better when devices have sample rates other than 44100
    xmpError = xmp_set_player(pModplay->xmpContext, XMP_PLAYER_INTERP, XMP_INTERP_SPLINE);
    xmpError = xmp_set_player(pModplay->xmpContext, XMP_PLAYER_DSP, XMP_DSP_ALL);

    xmp_get_frame_info(pModplay->xmpContext, &pModplay->xmpFrameInfo); // Get the frame information

    pModplay->isReallyPlaying = true;
    pModplay->frameBlock->Reset();

    audio_log_trace("Loaded module music file from memory (%zu bytes)", tune.size());

    return MA_SUCCESS;
}

static ma_result ma_modplay_init_file(const char *pFilePath, const ma_decoding_backend_config *pConfig, const ma_allocation_callbacks *pAllocationCallbacks,
                                      ma_modplay *pModplay) {
    ma_result result;

    (void)pAllocationCallbacks;

    result = ma_modplay_init_internal(pConfig, pModplay);
    if (result != MA_SUCCESS) {
        return result;
    }

    // Check the file extension
    if (!filepath_has_extension(pFilePath, "it") && !filepath_has_extension(pFilePath, "xm") && !filepath_has_extension(pFilePath, "s3m") &&
        !filepath_has_extension(pFilePath, "mod")) {
        return MA_INVALID_FILE;
    }

    // Check if the file is a valid module music
    xmp_test_info xmpTestInfo;
    int xmpError = xmp_test_module(pFilePath, &xmpTestInfo);
    if (xmpError != 0) {
        audio_log_warn("Not a valid music module file");
        return MA_INVALID_FILE;
    }

    pModplay->frameBlock = new DoubleBufferFrameBlock<SampleFrameI16>();
    if (!pModplay->frameBlock) {
        return MA_OUT_OF_MEMORY;
    }

    // Initialize the player
    pModplay->xmpContext = xmp_create_context();
    if (!pModplay->xmpContext) {
        return MA_OUT_OF_MEMORY;
    }

    // Load the module file
    xmpError = xmp_load_module(pModplay->xmpContext, pFilePath);
    if (xmpError != 0) {
        xmp_free_context(pModplay->xmpContext);
        pModplay->xmpContext = nullptr;
        return MA_INVALID_FILE;
    }

    // Initialize the player
    xmpError = xmp_start_player(pModplay->xmpContext, MA_DEFAULT_SAMPLE_RATE, 0);
    if (xmpError != 0) {
        xmp_release_module(pModplay->xmpContext);
        xmp_free_context(pModplay->xmpContext);
        pModplay->xmpContext = nullptr;
        return MA_INVALID_FILE;
    }

    // Set some player properties. These are not critical. So, we will not check the return values
    // These makes the sound quality much better when devices have sample rates other than 44100
    xmpError = xmp_set_player(pModplay->xmpContext, XMP_PLAYER_INTERP, XMP_INTERP_SPLINE);
    xmpError = xmp_set_player(pModplay->xmpContext, XMP_PLAYER_DSP, XMP_DSP_ALL);

    xmp_get_frame_info(pModplay->xmpContext, &pModplay->xmpFrameInfo); // Get the frame information

    pModplay->isReallyPlaying = true;
    pModplay->frameBlock->Reset();

    audio_log_trace("Loaded module music file: %s", pFilePath);

    return MA_SUCCESS;
}

static void ma_modplay_uninit(ma_modplay *pModplay, const ma_allocation_callbacks *pAllocationCallbacks) {
    if (pModplay == NULL) {
        return;
    }

    (void)pAllocationCallbacks;

    xmp_end_player(pModplay->xmpContext);
    xmp_release_module(pModplay->xmpContext);
    xmp_free_context(pModplay->xmpContext);
    pModplay->xmpContext = nullptr;

    delete pModplay->frameBlock;
    pModplay->frameBlock = nullptr;

    ma_data_source_uninit(&pModplay->ds);

    audio_log_trace("Unloaded module music file");
}

static ma_result ma_decoding_backend_init__modplay(void *pUserData, ma_read_proc onRead, ma_seek_proc onSeek, ma_tell_proc onTell, void *pReadSeekTellUserData,
                                                   const ma_decoding_backend_config *pConfig, const ma_allocation_callbacks *pAllocationCallbacks,
                                                   ma_data_source **ppBackend) {
    ma_result result;
    ma_modplay *pModplay;

    (void)pUserData;

    pModplay = (ma_modplay *)ma_malloc(sizeof(ma_modplay), pAllocationCallbacks);
    if (pModplay == NULL) {
        return MA_OUT_OF_MEMORY;
    }

    result = ma_modplay_init(onRead, onSeek, onTell, pReadSeekTellUserData, pConfig, pAllocationCallbacks, pModplay);
    if (result != MA_SUCCESS) {
        ma_free(pModplay, pAllocationCallbacks);
        return result;
    }

    *ppBackend = pModplay;

    return MA_SUCCESS;
}

static ma_result ma_decoding_backend_init_file__modplay(void *pUserData, const char *pFilePath, const ma_decoding_backend_config *pConfig,
                                                        const ma_allocation_callbacks *pAllocationCallbacks, ma_data_source **ppBackend) {
    ma_result result;
    ma_modplay *pModplay;

    (void)pUserData;

    pModplay = (ma_modplay *)ma_malloc(sizeof(ma_modplay), pAllocationCallbacks);
    if (pModplay == NULL) {
        return MA_OUT_OF_MEMORY;
    }

    result = ma_modplay_init_file(pFilePath, pConfig, pAllocationCallbacks, pModplay);
    if (result != MA_SUCCESS) {
        ma_free(pModplay, pAllocationCallbacks);
        return result;
    }

    *ppBackend = pModplay;

    return MA_SUCCESS;
}

static void ma_decoding_backend_uninit__modplay(void *pUserData, ma_data_source *pBackend, const ma_allocation_callbacks *pAllocationCallbacks) {
    ma_modplay *pModplay = (ma_modplay *)pBackend;

    (void)pUserData;

    ma_modplay_uninit(pModplay, pAllocationCallbacks);
    ma_free(pModplay, pAllocationCallbacks);
}

// clang-format off
ma_decoding_backend_vtable ma_vtable_modplay = {
    ma_decoding_backend_init__modplay,
    ma_decoding_backend_init_file__modplay,
    NULL, /* onInitFileW() */
    NULL, /* onInitMemory() */
    ma_decoding_backend_uninit__modplay
};
// clang-format on
