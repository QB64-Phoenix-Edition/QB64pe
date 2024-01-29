//--------------------------------------------------------------------------------------------------
//    ___  ___   __ _ _  ___ ___     _          _ _       ___           _
//   / _ \| _ ) / /| | || _ \ __|   /_\ _  _ __| (_)___  | __|_ _  __ _(_)_ _  ___
//  | (_) | _ \/ _ \_  _|  _/ _|   / _ \ || / _` | / _ \ | _|| ' \/ _` | | ' \/ -_)
//   \__\_\___/\___/ |_||_| |___| /_/ \_\_,_\__,_|_\___/ |___|_||_\__, |_|_||_\___|
//                                                                |___/
//
//  QB64-PE Audio Engine powered by miniaudio (https://miniaud.io/)
//
//  This implements a data source that decodes Amiga AHX and HLV formats
//
//  https://github.com/pete-gordon/hivelytracker (BSD 3-Clause)
//
//--------------------------------------------------------------------------------------------------

#include "../miniaudio.h"
#include "audio.h"
#include "filepath.h"
#include <cstring>

#include "hivelytracker/hvl_replay.h"

#include "vtables.h"

constexpr auto MAX_HIVELY_FRAMES = 10 * 60 * 50; // maximum *hively* frames before timeout

struct ma_hively {
    // This part is for miniaudio
    ma_data_source_base ds; /* The decoder can be used independently as a data source. */
    ma_read_proc onRead;
    ma_seek_proc onSeek;
    ma_tell_proc onTell;
    void *pReadSeekTellUserData;
    ma_format format;

    // This part is format specific
    hvl_tune *player;               // player context
    ma_uint64 lengthInSampleFrames; // total length of the tune in sample frames
    ma_int16 *buffer;               // render buffer (16-bit stereo)
    ma_uint64 bufferSamples;        // total number of samples in the buffer
    ma_uint64 bufferReadCursor;     // where is the buffer read cursor (in samples)
};

static ma_result ma_hively_seek_to_pcm_frame(ma_hively *pmaHively, ma_uint64 frameIndex) {
    if (pmaHively == NULL) {
        return MA_INVALID_ARGS;
    }

    // We can only reset the player to the beginning
    if (frameIndex == 0) {
        if (!hvl_InitSubsong(pmaHively->player, 0))
            return MA_INVALID_OPERATION;

        pmaHively->player->ht_SongEndReached = 0;

        return MA_SUCCESS;
    }

    return MA_INVALID_OPERATION; // Anything else is not seekable
}

static ma_result ma_hively_get_data_format(ma_hively *pmaHively, ma_format *pFormat, ma_uint32 *pChannels, ma_uint32 *pSampleRate, ma_channel *pChannelMap,
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

    if (pmaHively == NULL) {
        return MA_INVALID_OPERATION;
    }

    if (pFormat != NULL) {
        *pFormat = pmaHively->format;
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

static ma_result ma_hively_read_pcm_frames(ma_hively *pmaHively, void *pFramesOut, ma_uint64 frameCount, ma_uint64 *pFramesRead) {
    if (pFramesRead != NULL) {
        *pFramesRead = 0;
    }

    if (frameCount == 0) {
        return MA_INVALID_ARGS;
    }

    if (pmaHively == NULL) {
        return MA_INVALID_ARGS;
    }

    auto result = MA_SUCCESS; /* Must be initialized to MA_SUCCESS. */
    ma_uint64 totalFramesRead = 0;
    auto buffer = (ma_int16 *)pFramesOut;

    if (pmaHively->bufferReadCursor >= pmaHively->bufferSamples) {
        // We are out of samples so reset the cursor and render some
        pmaHively->bufferReadCursor = 0;
        hvl_DecodeFrame(pmaHively->player, (int8 *)pmaHively->buffer, ((int8 *)pmaHively->buffer) + 2, 4);
    }

    while (totalFramesRead < frameCount) {
        if (pmaHively->bufferReadCursor >= pmaHively->bufferSamples) // break out of the loop if we finished the block
            break;

        // Left channel sample
        *buffer = pmaHively->buffer[pmaHively->bufferReadCursor];
        ++buffer;
        pmaHively->bufferReadCursor++;

        // Right channel sample
        *buffer = pmaHively->buffer[pmaHively->bufferReadCursor];
        ++buffer;
        pmaHively->bufferReadCursor++;

        ++totalFramesRead;
    }

    // Are we done with the tune?
    if (pmaHively->player->ht_SongEndReached)
        result = MA_AT_END;

    if (pFramesRead != NULL) {
        *pFramesRead = totalFramesRead;
    }

    return result;
}

static ma_result ma_hively_get_cursor_in_pcm_frames(ma_hively *pmaHively, ma_uint64 *pCursor) {
    if (!pCursor) {
        return MA_INVALID_ARGS;
    }

    *pCursor = 0; /* Safety. */

    if (!pmaHively) {
        return MA_INVALID_ARGS;
    }

    return MA_NOT_IMPLEMENTED;
}

static ma_result ma_hively_get_length_in_pcm_frames(ma_hively *pmaHively, ma_uint64 *pLength) {
    if (!pLength) {
        return MA_INVALID_ARGS;
    }

    *pLength = 0; /* Safety. */

    if (!pmaHively) {
        return MA_INVALID_ARGS;
    }

    if (pmaHively->lengthInSampleFrames < 1) {
        return MA_INVALID_FILE;
    }

    *pLength = pmaHively->lengthInSampleFrames;

    return MA_SUCCESS;
}

static ma_result ma_hively_ds_read(ma_data_source *pDataSource, void *pFramesOut, ma_uint64 frameCount, ma_uint64 *pFramesRead) {
    return ma_hively_read_pcm_frames((ma_hively *)pDataSource, pFramesOut, frameCount, pFramesRead);
}

static ma_result ma_hively_ds_seek(ma_data_source *pDataSource, ma_uint64 frameIndex) {
    return ma_hively_seek_to_pcm_frame((ma_hively *)pDataSource, frameIndex);
}

static ma_result ma_hively_ds_get_data_format(ma_data_source *pDataSource, ma_format *pFormat, ma_uint32 *pChannels, ma_uint32 *pSampleRate,
                                              ma_channel *pChannelMap, size_t channelMapCap) {
    return ma_hively_get_data_format((ma_hively *)pDataSource, pFormat, pChannels, pSampleRate, pChannelMap, channelMapCap);
}

static ma_result ma_hively_ds_get_cursor(ma_data_source *pDataSource, ma_uint64 *pCursor) {
    return ma_hively_get_cursor_in_pcm_frames((ma_hively *)pDataSource, pCursor);
}

static ma_result ma_hively_ds_get_length(ma_data_source *pDataSource, ma_uint64 *pLength) {
    return ma_hively_get_length_in_pcm_frames((ma_hively *)pDataSource, pLength);
}

/// @brief HivelyTracker data source vtable
static ma_data_source_vtable ma_data_source_vtable_hively = {
    ma_hively_ds_read,            // Decodes and returns multiple frames of audio
    ma_hively_ds_seek,            // Can only support seeking to position 0
    ma_hively_ds_get_data_format, // Returns the audio format to miniaudio
    ma_hively_ds_get_cursor,      // Not supported
    ma_hively_ds_get_length,      // Returns the precalculated length
    NULL,                         // onSetLooping: NOP
    0                             // flags: none
};

static int ma_hively_of_callback__read(void *pUserData, unsigned char *pBufferOut, int bytesToRead) {
    ma_hively *pmaHively = (ma_hively *)pUserData;
    ma_result result;
    size_t bytesRead;

    result = pmaHively->onRead(pmaHively->pReadSeekTellUserData, (void *)pBufferOut, bytesToRead, &bytesRead);

    if (result != MA_SUCCESS) {
        return -1;
    }

    return (int)bytesRead;
}

static int ma_hively_of_callback__seek(void *pUserData, ma_int64 offset, int whence) {
    ma_hively *pmaHively = (ma_hively *)pUserData;
    ma_result result;
    ma_seek_origin origin;

    if (whence == SEEK_SET) {
        origin = ma_seek_origin_start;
    } else if (whence == SEEK_END) {
        origin = ma_seek_origin_end;
    } else {
        origin = ma_seek_origin_current;
    }

    result = pmaHively->onSeek(pmaHively->pReadSeekTellUserData, offset, origin);
    if (result != MA_SUCCESS) {
        return -1;
    }

    return 0;
}

static ma_int64 ma_hively_of_callback__tell(void *pUserData) {
    ma_hively *pmaHively = (ma_hively *)pUserData;
    ma_result result;
    ma_int64 cursor;

    if (pmaHively->onTell == NULL) {
        return -1;
    }

    result = pmaHively->onTell(pmaHively->pReadSeekTellUserData, &cursor);
    if (result != MA_SUCCESS) {
        return -1;
    }

    return cursor;
}

static ma_result ma_hively_init_internal(const ma_decoding_backend_config *pConfig, ma_hively *pmaHively) {
    ma_result result;
    ma_data_source_config dataSourceConfig;

    if (pmaHively == NULL) {
        return MA_INVALID_ARGS;
    }

    memset(pmaHively, 0, sizeof(*pmaHively));
    pmaHively->format = ma_format::ma_format_s16; // We'll render 16-bit signed samples

    if (pConfig != NULL && pConfig->preferredFormat == ma_format::ma_format_s16) {
        pmaHively->format = pConfig->preferredFormat;
    } else {
        /* Getting here means something other than s16 was specified. Just leave this unset to use the default format. */
    }

    dataSourceConfig = ma_data_source_config_init();
    dataSourceConfig.vtable = &ma_data_source_vtable_hively;

    result = ma_data_source_init(&dataSourceConfig, &pmaHively->ds);
    if (result != MA_SUCCESS) {
        return result; /* Failed to initialize the base data source. */
    }

    return MA_SUCCESS;
}

// This help us calculate the total frame size of the tune
// Note that this must be called before rendering the tune as it actually "plays" it to a dummy buffer to calculate the length
static ma_uint64 ma_hively_get_length_in_pcm_frames_internal(ma_hively *pmaHively) {
    ma_uint64 totalFramesRead = 0;

    auto frame = 0;

    while (frame < MAX_HIVELY_FRAMES) {
        if (pmaHively->player->ht_SongEndReached)
            break;

        hvl_DecodeFrame(pmaHively->player, (int8 *)pmaHively->buffer, ((int8 *)pmaHively->buffer) + 2, 4);

        totalFramesRead += pmaHively->bufferSamples >> 1; // divide by 2 for 2 channels
        ++frame;
    }

    // Reset playback position
    hvl_InitSubsong(pmaHively->player, 0);
    pmaHively->player->ht_SongEndReached = 0;

    return totalFramesRead; // Return the total frames rendered
}

static ma_result ma_hively_init(ma_read_proc onRead, ma_seek_proc onSeek, ma_tell_proc onTell, void *pReadSeekTellUserData,
                                const ma_decoding_backend_config *pConfig, const ma_allocation_callbacks *pAllocationCallbacks, ma_hively *pmaHively) {
    ma_result result;

    (void)pAllocationCallbacks;

    result = ma_hively_init_internal(pConfig, pmaHively);
    if (result != MA_SUCCESS) {
        return result;
    }

    if (onRead == NULL || onSeek == NULL) {
        return MA_INVALID_ARGS; /* onRead and onSeek are mandatory. */
    }

    pmaHively->onRead = onRead;
    pmaHively->onSeek = onSeek;
    pmaHively->onTell = onTell;
    pmaHively->pReadSeekTellUserData = pReadSeekTellUserData;

    // Find the size of the file
    if (ma_hively_of_callback__seek(pmaHively, 0, SEEK_END) != 0) {
        return MA_BAD_SEEK;
    }

    // Calculate the length
    ma_int64 file_size = ma_hively_of_callback__tell(pmaHively);
    if (file_size < 1) {
        return MA_INVALID_FILE;
    }

    // Seek to the beginning of the file
    if (ma_hively_of_callback__seek(pmaHively, 0, SEEK_SET) != 0) {
        return MA_BAD_SEEK;
    }

    // Allocate some memory for the tune
    ma_uint8 *tune = new ma_uint8[file_size];
    if (tune == nullptr) {
        return MA_OUT_OF_MEMORY;
    }

    // Read the file
    if (ma_hively_of_callback__read(pmaHively, tune, (int)file_size) < 1) {
        delete[] tune;
        return MA_IO_ERROR;
    }

    hvl_InitReplayer(); // we'll initialize the re-player here

    // Ok, we have the tune in memory, now loads it
    pmaHively->player = hvl_ParseTune(tune, file_size, MA_DEFAULT_SAMPLE_RATE, 3);
    if (!pmaHively->player || !hvl_InitSubsong(pmaHively->player, 0)) {
        if (pmaHively->player)
            hvl_FreeTune(pmaHively->player);
        pmaHively->player = nullptr;
    }

    // Free the memory now that we don't need it anymore
    delete[] tune;

    if (pmaHively->player == nullptr) {
        // This means our loader failed
        return MA_INVALID_FILE;
    }

    // Calculate the buffer size and then allocate memory
    pmaHively->bufferSamples = (MA_DEFAULT_SAMPLE_RATE * 2) / 50;
    pmaHively->buffer = new ma_int16[pmaHively->bufferSamples];
    if (!pmaHively->buffer) {
        hvl_FreeTune(pmaHively->player);
        pmaHively->player = nullptr;

        return MA_OUT_OF_MEMORY;
    }

    // Calculate the sample frames
    pmaHively->lengthInSampleFrames = ma_hively_get_length_in_pcm_frames_internal(pmaHively);

    return MA_SUCCESS;
}

static ma_result ma_hively_init_file(const char *pFilePath, const ma_decoding_backend_config *pConfig, const ma_allocation_callbacks *pAllocationCallbacks,
                                     ma_hively *pmaHively) {
    ma_result result;

    (void)pAllocationCallbacks;

    result = ma_hively_init_internal(pConfig, pmaHively);
    if (result != MA_SUCCESS) {
        return result;
    }

    // Check the file extension
    if (filepath_has_extension(pFilePath, "hvl") || filepath_has_extension(pFilePath, "ahx")) {
        hvl_InitReplayer(); // we'll initialize the re-player here

        pmaHively->player = hvl_LoadTune(pFilePath, MA_DEFAULT_SAMPLE_RATE, 3);
        if (!pmaHively->player || !hvl_InitSubsong(pmaHively->player, 0)) {
            if (pmaHively->player)
                hvl_FreeTune(pmaHively->player);
            pmaHively->player = nullptr;

            return MA_INVALID_FILE;
        }

        // Calculate the buffer size and then allocate memory
        pmaHively->bufferSamples = (MA_DEFAULT_SAMPLE_RATE * 2) / 50;
        pmaHively->buffer = new ma_int16[pmaHively->bufferSamples];
        if (!pmaHively->buffer) {
            hvl_FreeTune(pmaHively->player);
            pmaHively->player = nullptr;

            return MA_OUT_OF_MEMORY;
        }
    } else {
        return MA_INVALID_FILE;
    }

    // Calculate the sample frames
    pmaHively->lengthInSampleFrames = ma_hively_get_length_in_pcm_frames_internal(pmaHively);

    return MA_SUCCESS;
}

static void ma_hively_uninit(ma_hively *pmaHively, const ma_allocation_callbacks *pAllocationCallbacks) {
    if (pmaHively == NULL) {
        return;
    }

    (void)pAllocationCallbacks;

    // Free all resources
    pmaHively->lengthInSampleFrames = 0;
    pmaHively->bufferSamples = 0;
    pmaHively->bufferReadCursor = 0;
    hvl_FreeTune(pmaHively->player);
    pmaHively->player = nullptr;
    delete[] pmaHively->buffer;
    pmaHively->buffer = nullptr;

    ma_data_source_uninit(&pmaHively->ds);
}

static ma_result ma_decoding_backend_init__hively(void *pUserData, ma_read_proc onRead, ma_seek_proc onSeek, ma_tell_proc onTell, void *pReadSeekTellUserData,
                                                  const ma_decoding_backend_config *pConfig, const ma_allocation_callbacks *pAllocationCallbacks,
                                                  ma_data_source **ppBackend) {
    ma_result result;
    ma_hively *pmaHively;

    (void)pUserData;

    pmaHively = (ma_hively *)ma_malloc(sizeof(ma_hively), pAllocationCallbacks);
    if (pmaHively == NULL) {
        return MA_OUT_OF_MEMORY;
    }

    result = ma_hively_init(onRead, onSeek, onTell, pReadSeekTellUserData, pConfig, pAllocationCallbacks, pmaHively);
    if (result != MA_SUCCESS) {
        ma_free(pmaHively, pAllocationCallbacks);
        return result;
    }

    *ppBackend = pmaHively;

    return MA_SUCCESS;
}

static ma_result ma_decoding_backend_init_file__hively(void *pUserData, const char *pFilePath, const ma_decoding_backend_config *pConfig,
                                                       const ma_allocation_callbacks *pAllocationCallbacks, ma_data_source **ppBackend) {
    ma_result result;
    ma_hively *pmaHively;

    (void)pUserData;

    pmaHively = (ma_hively *)ma_malloc(sizeof(ma_hively), pAllocationCallbacks);
    if (pmaHively == NULL) {
        return MA_OUT_OF_MEMORY;
    }

    result = ma_hively_init_file(pFilePath, pConfig, pAllocationCallbacks, pmaHively);
    if (result != MA_SUCCESS) {
        ma_free(pmaHively, pAllocationCallbacks);
        return result;
    }

    *ppBackend = pmaHively;

    return MA_SUCCESS;
}

static void ma_decoding_backend_uninit__hively(void *pUserData, ma_data_source *pBackend, const ma_allocation_callbacks *pAllocationCallbacks) {
    ma_hively *pmaHively = (ma_hively *)pBackend;

    (void)pUserData;

    ma_hively_uninit(pmaHively, pAllocationCallbacks);
    ma_free(pmaHively, pAllocationCallbacks);
}

// clang-format off
ma_decoding_backend_vtable ma_vtable_hively = {
    ma_decoding_backend_init__hively,
    ma_decoding_backend_init_file__hively,
    NULL, /* onInitFileW() */
    NULL, /* onInitMemory() */
    ma_decoding_backend_uninit__hively
};
// clang-format on
