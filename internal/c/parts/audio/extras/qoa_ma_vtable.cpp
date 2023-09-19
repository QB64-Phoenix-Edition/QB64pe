//----------------------------------------------------------------------------------------------------
//    ___  ___   __ _ _  ___ ___     _          _ _       ___           _
//   / _ \| _ ) / /| | || _ \ __|   /_\ _  _ __| (_)___  | __|_ _  __ _(_)_ _  ___
//  | (_) | _ \/ _ \_  _|  _/ _|   / _ \ || / _` | / _ \ | _|| ' \/ _` | | ' \/ -_)
//   \__\_\___/\___/ |_||_| |___| /_/ \_\_,_\__,_|_\___/ |___|_||_\__, |_|_||_\___|
//                                                                |___/
//
//  QB64-PE Audio Engine powered by miniaudio (https://miniaud.io/)
//
//  This implements a data source that decodes QOA files
//  https://qoaformat.org/
//
//-----------------------------------------------------------------------------------------------------

#include "../miniaudio.h"
#include "audio.h"
#include "filepath.h"
#include "libqb-common.h"
#include <algorithm>
#include <cstdio>
#include <cstring>

// Although, QOA files use lossy compression, they can be quite large (like ADPCM compressed audio)
// We certainly do not want to load these files in memory in one go
// So, we'll simply exclude the stdio one-shot read/write APIs
#define QOA_NO_STDIO
#define QOA_IMPLEMENTATION
#include "qoa.h"

#include "vtables.h"

struct ma_qoa {
    // This part is for miniaudio
    ma_data_source_base ds; /* The decoder can be used independently as a data source. */
    ma_read_proc onRead;
    ma_seek_proc onSeek;
    ma_tell_proc onTell;
    void *pReadSeekTellUserData;
    ma_format format;

    // This part is format specific
    qoa_desc info;
    FILE *file;
    ma_uint64 first_frame_pos;
    ma_uint64 sample_pos;
    size_t buffer_len;
    ma_uint8 *buffer;
    ma_uint64 sample_data_pos;
    size_t sample_data_len;
    ma_int16 *sample_data;
};

template <class T> T clamp(T x, T lo, T hi) { return std::max(std::min(x, hi), lo); }

static ma_result ma_qoa_seek_to_pcm_frame(ma_qoa *pQOA, ma_uint64 frameIndex) {
    if (pQOA == NULL) {
        return MA_INVALID_ARGS;
    }

    auto qoaFrame = clamp<ma_uint64>(frameIndex / QOA_FRAME_LEN, 0, pQOA->info.samples / QOA_FRAME_LEN);

    pQOA->sample_pos = qoaFrame * QOA_FRAME_LEN;
    pQOA->sample_data_len = 0;
    pQOA->sample_data_pos = 0;

    auto offset = pQOA->first_frame_pos + qoaFrame * qoa_max_frame_size(&pQOA->info);

    if (pQOA->file) {
        if (fseek(pQOA->file, offset, SEEK_SET) != 0)
            return MA_BAD_SEEK;
    } else {
        if (pQOA->onSeek(pQOA->pReadSeekTellUserData, offset, ma_seek_origin::ma_seek_origin_start) != MA_SUCCESS)
            return MA_BAD_SEEK;
    }

    return MA_SUCCESS;
}

static ma_result ma_qoa_get_data_format(ma_qoa *pQOA, ma_format *pFormat, ma_uint32 *pChannels, ma_uint32 *pSampleRate, ma_channel *pChannelMap,
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

    if (pQOA == NULL) {
        return MA_INVALID_OPERATION;
    }

    if (pFormat != NULL) {
        *pFormat = pQOA->format;
    }

    if (pChannels != NULL) {
        *pChannels = pQOA->info.channels;
    }

    if (pSampleRate != NULL) {
        *pSampleRate = pQOA->info.samplerate;
    }

    if (pChannelMap != NULL) {
        ma_channel_map_init_standard(ma_standard_channel_map_default, pChannelMap, channelMapCap, 2);
    }

    return MA_SUCCESS;
}

static ma_result ma_qoa_read_pcm_frames(ma_qoa *pQOA, void *pFramesOut, ma_uint64 frameCount, ma_uint64 *pFramesRead) {
    if (pFramesRead != NULL) {
        *pFramesRead = 0;
    }

    if (frameCount == 0) {
        return MA_INVALID_ARGS;
    }

    if (pQOA == NULL) {
        return MA_INVALID_ARGS;
    }

    auto result = MA_SUCCESS; // Must be initialized to MA_SUCCESS

    auto src_index = pQOA->sample_data_pos * pQOA->info.channels;
    ma_uint64 dst_index = 0, totalFramesRead = 0;
    for (ma_uint64 i = 0; i < frameCount; i++) {
        /* Do we have to decode more samples? */
        if (pQOA->sample_data_len - pQOA->sample_data_pos == 0) {
            pQOA->buffer_len = 0;
            if (pQOA->file)
                pQOA->buffer_len = fread(pQOA->buffer, 1, qoa_max_frame_size(&pQOA->info), pQOA->file);
            else
                pQOA->onRead(pQOA->pReadSeekTellUserData, pQOA->buffer, qoa_max_frame_size(&pQOA->info), &pQOA->buffer_len);

            unsigned int frame_len;
            qoa_decode_frame(pQOA->buffer, pQOA->buffer_len, &pQOA->info, pQOA->sample_data, &frame_len);
            pQOA->sample_data_pos = 0;
            pQOA->sample_data_len = frame_len;

            if (!frame_len) {
                result = MA_AT_END;
                break;
            }

            src_index = 0;
        }

        auto sample_data = reinterpret_cast<ma_int16 *>(pFramesOut);

        for (auto c = 0; c < pQOA->info.channels; c++)
            sample_data[dst_index++] = pQOA->sample_data[src_index++];

        pQOA->sample_data_pos++;
        pQOA->sample_pos++;

        ++totalFramesRead;
    }

    if (pFramesRead != NULL) {
        *pFramesRead = totalFramesRead;
    }

    return result;
}

static ma_result ma_qoa_get_cursor_in_pcm_frames(ma_qoa *pQOA, ma_uint64 *pCursor) {
    if (!pCursor) {
        return MA_INVALID_ARGS;
    }

    *pCursor = 0; /* Safety. */

    if (!pQOA) {
        return MA_INVALID_ARGS;
    }

    // Get the frame information
    *pCursor = pQOA->sample_pos;

    return MA_SUCCESS;
}

static ma_result ma_qoa_get_length_in_pcm_frames(ma_qoa *pQOA, ma_uint64 *pLength) {
    if (!pLength) {
        return MA_INVALID_ARGS;
    }

    *pLength = 0; /* Safety. */

    if (!pQOA) {
        return MA_INVALID_ARGS;
    }

    *pLength = pQOA->info.samples;

    return MA_SUCCESS;
}

static ma_result ma_qoa_ds_read(ma_data_source *pDataSource, void *pFramesOut, ma_uint64 frameCount, ma_uint64 *pFramesRead) {
    return ma_qoa_read_pcm_frames((ma_qoa *)pDataSource, pFramesOut, frameCount, pFramesRead);
}

static ma_result ma_qoa_ds_seek(ma_data_source *pDataSource, ma_uint64 frameIndex) { return ma_qoa_seek_to_pcm_frame((ma_qoa *)pDataSource, frameIndex); }

static ma_result ma_qoa_ds_get_data_format(ma_data_source *pDataSource, ma_format *pFormat, ma_uint32 *pChannels, ma_uint32 *pSampleRate,
                                           ma_channel *pChannelMap, size_t channelMapCap) {
    return ma_qoa_get_data_format((ma_qoa *)pDataSource, pFormat, pChannels, pSampleRate, pChannelMap, channelMapCap);
}

static ma_result ma_qoa_ds_get_cursor(ma_data_source *pDataSource, ma_uint64 *pCursor) {
    return ma_qoa_get_cursor_in_pcm_frames((ma_qoa *)pDataSource, pCursor);
}

static ma_result ma_qoa_ds_get_length(ma_data_source *pDataSource, ma_uint64 *pLength) {
    return ma_qoa_get_length_in_pcm_frames((ma_qoa *)pDataSource, pLength);
}

// clang-format off
static ma_data_source_vtable ma_data_source_vtable_qoa = {
    ma_qoa_ds_read,
    ma_qoa_ds_seek,
    ma_qoa_ds_get_data_format,
    ma_qoa_ds_get_cursor,
    ma_qoa_ds_get_length
};
// clang-format on

static int ma_qoa_of_callback__read(void *pUserData, unsigned char *pBufferOut, int bytesToRead) {
    ma_qoa *pQOA = (ma_qoa *)pUserData;
    ma_result result;
    size_t bytesRead;

    result = pQOA->onRead(pQOA->pReadSeekTellUserData, (void *)pBufferOut, bytesToRead, &bytesRead);

    if (result != MA_SUCCESS) {
        return -1;
    }

    return (int)bytesRead;
}

static int ma_qoa_of_callback__seek(void *pUserData, ma_int64 offset, int whence) {
    ma_qoa *pQOA = (ma_qoa *)pUserData;
    ma_result result;
    ma_seek_origin origin;

    if (whence == SEEK_SET) {
        origin = ma_seek_origin_start;
    } else if (whence == SEEK_END) {
        origin = ma_seek_origin_end;
    } else {
        origin = ma_seek_origin_current;
    }

    result = pQOA->onSeek(pQOA->pReadSeekTellUserData, offset, origin);
    if (result != MA_SUCCESS) {
        return -1;
    }

    return 0;
}

static ma_int64 ma_qoa_of_callback__tell(void *pUserData) {
    ma_qoa *pQOA = (ma_qoa *)pUserData;
    ma_result result;
    ma_int64 cursor;

    if (pQOA->onTell == NULL) {
        return -1;
    }

    result = pQOA->onTell(pQOA->pReadSeekTellUserData, &cursor);
    if (result != MA_SUCCESS) {
        return -1;
    }

    return cursor;
}

static ma_result ma_qoa_init_internal(const ma_decoding_backend_config *pConfig, ma_qoa *pQOA) {
    ma_result result;
    ma_data_source_config dataSourceConfig;

    if (pQOA == NULL) {
        return MA_INVALID_ARGS;
    }

    memset(pQOA, 0, sizeof(*pQOA));
    pQOA->format = ma_format::ma_format_s16; // We'll render 16-bit signed samples by default (QOA native format)

    if (pConfig != NULL && pConfig->preferredFormat == ma_format::ma_format_s16) {
        pQOA->format = pConfig->preferredFormat;
    } else {
        /* Getting here means something other than s16 was specified. Just leave this unset to use the default format. */
    }

    dataSourceConfig = ma_data_source_config_init();
    dataSourceConfig.vtable = &ma_data_source_vtable_qoa;

    result = ma_data_source_init(&dataSourceConfig, &pQOA->ds);
    if (result != MA_SUCCESS) {
        return result; /* Failed to initialize the base data source. */
    }

    return MA_SUCCESS;
}

static ma_result ma_qoa_init(ma_read_proc onRead, ma_seek_proc onSeek, ma_tell_proc onTell, void *pReadSeekTellUserData,
                             const ma_decoding_backend_config *pConfig, const ma_allocation_callbacks *pAllocationCallbacks, ma_qoa *pQOA) {
    (void)pAllocationCallbacks;

    auto result = ma_qoa_init_internal(pConfig, pQOA);
    if (result != MA_SUCCESS) {
        return result;
    }

    if (onRead == NULL || onSeek == NULL) {
        return MA_INVALID_ARGS; /* onRead and onSeek are mandatory. */
    }

    pQOA->onRead = onRead;
    pQOA->onSeek = onSeek;
    pQOA->onTell = onTell;
    pQOA->pReadSeekTellUserData = pReadSeekTellUserData;

    // Find the size of the file
    if (ma_qoa_of_callback__seek(pQOA, 0, SEEK_END) != 0) {
        return MA_BAD_SEEK;
    }

    // Calculate the length
    auto file_size = ma_qoa_of_callback__tell(pQOA);
    if (file_size < 1) {
        return MA_INVALID_FILE;
    }

    // Seek to the beginning of the file
    if (ma_qoa_of_callback__seek(pQOA, 0, SEEK_SET) != 0) {
        return MA_BAD_SEEK;
    }

    /* Read and decode the file header */
    ma_uint8 header[QOA_MIN_FILESIZE];
    if (ma_qoa_of_callback__read(pQOA, header, QOA_MIN_FILESIZE) < 1) {
        return MA_IO_ERROR;
    }

    pQOA->first_frame_pos = qoa_decode_header(header, QOA_MIN_FILESIZE, &pQOA->info);
    if (!pQOA->first_frame_pos) {
        return MA_INVALID_FILE;
    }

    /* Rewind the file back to beginning of the first frame */
    if (ma_qoa_of_callback__seek(pQOA, pQOA->first_frame_pos, SEEK_SET) != 0) {
        return MA_BAD_SEEK;
    }

    /* Allocate memory for the sample data for one frame and a buffer to hold one frame of encoded data. */
    pQOA->sample_data = (ma_int16 *)malloc(pQOA->info.channels * QOA_FRAME_LEN * sizeof(short) * 2); // NOTE: must be freed when stream closes!
    if (!pQOA->sample_data) {
        return MA_OUT_OF_MEMORY;
    }

    pQOA->buffer = (ma_uint8 *)malloc(qoa_max_frame_size(&pQOA->info)); // NOTE: must be freed when stream closes!
    if (!pQOA->buffer) {
        free(pQOA->sample_data);
        pQOA->sample_data = nullptr;
        return MA_OUT_OF_MEMORY;
    }

    return MA_SUCCESS;
}

static ma_result ma_qoa_init_file(const char *pFilePath, const ma_decoding_backend_config *pConfig, const ma_allocation_callbacks *pAllocationCallbacks,
                                  ma_qoa *pQOA) {
    ma_result result;

    (void)pAllocationCallbacks;

    result = ma_qoa_init_internal(pConfig, pQOA);
    if (result != MA_SUCCESS) {
        return result;
    }

    // Check the file extension
    if (!filepath_has_extension(pFilePath, "qoa")) {
        return MA_INVALID_FILE;
    }

    pQOA->file = fopen(pFilePath, "rb");
    if (!pQOA->file) {
        return MA_INVALID_FILE;
    }

    /* Read and decode the file header */
    ma_uint8 header[QOA_MIN_FILESIZE];
    if (!fread(header, QOA_MIN_FILESIZE, 1, pQOA->file)) {
        fclose(pQOA->file);
        pQOA->file = nullptr;
        return MA_IO_ERROR;
    }

    pQOA->first_frame_pos = qoa_decode_header(header, QOA_MIN_FILESIZE, &pQOA->info);
    if (!pQOA->first_frame_pos) {
        fclose(pQOA->file);
        pQOA->file = nullptr;
        return MA_INVALID_FILE;
    }

    /* Rewind the file back to beginning of the first frame */
    if (fseek(pQOA->file, pQOA->first_frame_pos, SEEK_SET) != 0) {
        fclose(pQOA->file);
        pQOA->file = nullptr;
        return MA_BAD_SEEK;
    }

    /* Allocate memory for the sample data for one frame and a buffer to hold one frame of encoded data. */
    pQOA->sample_data = (ma_int16 *)malloc(pQOA->info.channels * QOA_FRAME_LEN * sizeof(short) * 2); // NOTE: must be freed when stream closes!
    if (!pQOA->sample_data) {
        fclose(pQOA->file);
        pQOA->file = nullptr;
        return MA_OUT_OF_MEMORY;
    }

    pQOA->buffer = (ma_uint8 *)malloc(qoa_max_frame_size(&pQOA->info)); // NOTE: must be freed when stream closes!
    if (!pQOA->buffer) {
        free(pQOA->sample_data);
        pQOA->sample_data = nullptr;
        fclose(pQOA->file);
        pQOA->file = nullptr;
        return MA_OUT_OF_MEMORY;
    }

    return MA_SUCCESS;
}

static void ma_qoa_uninit(ma_qoa *pQOA, const ma_allocation_callbacks *pAllocationCallbacks) {
    if (pQOA == NULL)
        return;

    (void)pAllocationCallbacks;

    if (pQOA->file)
        fclose(pQOA->file);

    free(pQOA->buffer);
    free(pQOA->sample_data);

    ma_data_source_uninit(&pQOA->ds);
}

static ma_result ma_decoding_backend_init__qoa(void *pUserData, ma_read_proc onRead, ma_seek_proc onSeek, ma_tell_proc onTell, void *pReadSeekTellUserData,
                                               const ma_decoding_backend_config *pConfig, const ma_allocation_callbacks *pAllocationCallbacks,
                                               ma_data_source **ppBackend) {
    ma_result result;
    ma_qoa *pQOA;

    (void)pUserData;

    pQOA = (ma_qoa *)ma_malloc(sizeof(ma_qoa), pAllocationCallbacks);
    if (pQOA == NULL) {
        return MA_OUT_OF_MEMORY;
    }

    result = ma_qoa_init(onRead, onSeek, onTell, pReadSeekTellUserData, pConfig, pAllocationCallbacks, pQOA);
    if (result != MA_SUCCESS) {
        ma_free(pQOA, pAllocationCallbacks);
        return result;
    }

    *ppBackend = pQOA;

    return MA_SUCCESS;
}

static ma_result ma_decoding_backend_init_file__qoa(void *pUserData, const char *pFilePath, const ma_decoding_backend_config *pConfig,
                                                    const ma_allocation_callbacks *pAllocationCallbacks, ma_data_source **ppBackend) {
    ma_result result;
    ma_qoa *pQOA;

    (void)pUserData;

    pQOA = (ma_qoa *)ma_malloc(sizeof(ma_qoa), pAllocationCallbacks);
    if (pQOA == NULL) {
        return MA_OUT_OF_MEMORY;
    }

    result = ma_qoa_init_file(pFilePath, pConfig, pAllocationCallbacks, pQOA);
    if (result != MA_SUCCESS) {
        ma_free(pQOA, pAllocationCallbacks);
        return result;
    }

    *ppBackend = pQOA;

    return MA_SUCCESS;
}

static void ma_decoding_backend_uninit__qoa(void *pUserData, ma_data_source *pBackend, const ma_allocation_callbacks *pAllocationCallbacks) {
    ma_qoa *pQOA = (ma_qoa *)pBackend;

    (void)pUserData;

    ma_qoa_uninit(pQOA, pAllocationCallbacks);
    ma_free(pQOA, pAllocationCallbacks);
}

// clang-format off
ma_decoding_backend_vtable ma_vtable_qoa = {
    ma_decoding_backend_init__qoa,
    ma_decoding_backend_init_file__qoa,
    NULL, /* onInitFileW() */
    NULL, /* onInitMemory() */
    ma_decoding_backend_uninit__qoa
};
// clang-format on
