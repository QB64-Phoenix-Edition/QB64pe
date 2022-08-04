//----------------------------------------------------------------------------------------------------
//    ___  ___ ___ ___     _          _ _       ___           _
//   / _ \| _ ) _ \ __|   /_\ _  _ __| (_)___  | __|_ _  __ _(_)_ _  ___
//  | (_) | _ \  _/ _|   / _ \ || / _` | / _ \ | _|| ' \/ _` | | ' \/ -_)
//   \__\_\___/_| |___| /_/ \_\_,_\__,_|_\___/ |___|_||_\__, |_|_||_\___|
//                                                      |___/
//
//	QBPE Audio Engine powered by miniaudio (https://miniaud.io/)
//
//	This implements a data source that decodes Reality Adlib Tracker 2 tunes
//
//	Copyright (c) 2022 Samuel Gomes
//	https://github.com/a740g
//
//-----------------------------------------------------------------------------------------------------

#pragma once

#include "radv2/opal.cpp"
#define RAD_DETECT_REPEATS 1
#include "radv2/player20.cpp"
#include "radv2/validate20.cpp"

struct ma_radv2 {
    // This part is for miniaudio
    ma_data_source_base ds; /* The decoder can be used independently as a data source. */
    ma_read_proc onRead;
    ma_seek_proc onSeek;
    ma_tell_proc onTell;
    void *pReadSeekTellUserData;
    ma_format format;

    // This part if format specific
    RADPlayer *player;
    Opal *adlib;
    void *tune;
    uint32_t totalTime;
    int sampleCount;
    int sampleUpdate;
};

static ma_result ma_radv2_seek_to_pcm_frame(ma_radv2 *pRadv2, ma_uint64 frameIndex) {
    // We do not have any seeking support in RADv2 player
    return MA_NOT_IMPLEMENTED;
}

static ma_result ma_radv2_get_data_format(ma_radv2 *pRadv2, ma_format *pFormat, ma_uint32 *pChannels, ma_uint32 *pSampleRate, ma_channel *pChannelMap,
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
        MA_ZERO_MEMORY(pChannelMap, sizeof(*pChannelMap) * channelMapCap);
    }

    if (!pRadv2) {
        return MA_INVALID_OPERATION;
    }

    if (pFormat != NULL) {
        *pFormat = pRadv2->format;
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

static ma_result ma_radv2_read_pcm_frames(ma_radv2 *pRadv2, void *pFramesOut, ma_uint64 frameCount, ma_uint64 *pFramesRead) {
    if (pFramesRead != NULL) {
        *pFramesRead = 0;
    }

    if (frameCount == 0) {
        return MA_INVALID_ARGS;
    }

    if (!pRadv2) {
        return MA_INVALID_ARGS;
    }

    /* We always use floating point format. */
    ma_result result = MA_SUCCESS; /* Must be initialized to MA_SUCCESS. */
    ma_uint64 totalFramesRead = 0;
    ma_format format;
    ma_uint32 channels;

    ma_radv2_get_data_format(pRadv2, &format, &channels, NULL, NULL, 0);

    bool repeat = false;
    int16_t *buffer;

    while (totalFramesRead < frameCount && pRadv2->player->GetPlayTimeInSeconds() < pRadv2->totalTime) {
        // Set to the correct buffer offset
        buffer = (int16_t *)ma_offset_pcm_frames_ptr(pFramesOut, totalFramesRead, format, channels);
        // Get the left and right sample
        pRadv2->adlib->Sample(&buffer[0], &buffer[1]);
        // Increment the frame counter
        ++totalFramesRead;

        // Time to update player?
        pRadv2->sampleCount++;
        if (pRadv2->sampleCount >= pRadv2->sampleUpdate) {
            pRadv2->sampleCount = 0;
            repeat = pRadv2->player->Update();
        }

        if (repeat) {
            result = MA_AT_END;
            break;
        }
    }

    if (pFramesRead != NULL) {
        *pFramesRead = totalFramesRead;
    }

    if (result == MA_SUCCESS && totalFramesRead == 0) {
        result = MA_AT_END;
    }

    return result;
}

static ma_result ma_radv2_get_cursor_in_pcm_frames(ma_radv2 *pRadv2, ma_uint64 *pCursor) {
    // Since there is no seeking support, we'll not implement this as well
    return MA_NOT_IMPLEMENTED;
}

static ma_result ma_radv2_get_length_in_pcm_frames(ma_radv2 *pRadv2, ma_uint64 *pLength) {
    if (!pLength) {
        return MA_INVALID_ARGS;
    }

    *pLength = 0; /* Safety. */

    if (!pRadv2) {
        return MA_INVALID_ARGS;
    }

    // Total time in seconds * Opal sample rate
    ma_uint64 length = (ma_uint64)pRadv2->totalTime * MA_DEFAULT_SAMPLE_RATE;
    if (length < 0) {
        return MA_NOT_IMPLEMENTED; // This will allow miniaudio to poll the player and get whatever samples it can
    }

    *pLength = (ma_uint64)length;

    return MA_SUCCESS;
}

static ma_result ma_radv2_ds_read(ma_data_source *pDataSource, void *pFramesOut, ma_uint64 frameCount, ma_uint64 *pFramesRead) {
    return ma_radv2_read_pcm_frames((ma_radv2 *)pDataSource, pFramesOut, frameCount, pFramesRead);
}

static ma_result ma_radv2_ds_seek(ma_data_source *pDataSource, ma_uint64 frameIndex) { return ma_radv2_seek_to_pcm_frame((ma_radv2 *)pDataSource, frameIndex); }

static ma_result ma_radv2_ds_get_data_format(ma_data_source *pDataSource, ma_format *pFormat, ma_uint32 *pChannels, ma_uint32 *pSampleRate,
                                             ma_channel *pChannelMap, size_t channelMapCap) {
    return ma_radv2_get_data_format((ma_radv2 *)pDataSource, pFormat, pChannels, pSampleRate, pChannelMap, channelMapCap);
}

static ma_result ma_radv2_ds_get_cursor(ma_data_source *pDataSource, ma_uint64 *pCursor) {
    return ma_radv2_get_cursor_in_pcm_frames((ma_radv2 *)pDataSource, pCursor);
}

static ma_result ma_radv2_ds_get_length(ma_data_source *pDataSource, ma_uint64 *pLength) {
    return ma_radv2_get_length_in_pcm_frames((ma_radv2 *)pDataSource, pLength);
}

static ma_data_source_vtable ma_data_source_vtable_radv2 = {ma_radv2_ds_read, ma_radv2_ds_seek, ma_radv2_ds_get_data_format, ma_radv2_ds_get_cursor,
                                                            ma_radv2_ds_get_length};

static int ma_radv2_of_callback__read(void *pUserData, unsigned char *pBufferOut, int bytesToRead) {
    ma_radv2 *pRadv2 = (ma_radv2 *)pUserData;
    ma_result result;
    size_t bytesRead;

    result = pRadv2->onRead(pRadv2->pReadSeekTellUserData, (void *)pBufferOut, bytesToRead, &bytesRead);

    if (result != MA_SUCCESS) {
        return -1;
    }

    return (int)bytesRead;
}

static int ma_radv2_of_callback__seek(void *pUserData, ma_int64 offset, int whence) {
    ma_radv2 *pRadv2 = (ma_radv2 *)pUserData;
    ma_result result;
    ma_seek_origin origin;

    if (whence == SEEK_SET) {
        origin = ma_seek_origin_start;
    } else if (whence == SEEK_END) {
        origin = ma_seek_origin_end;
    } else {
        origin = ma_seek_origin_current;
    }

    result = pRadv2->onSeek(pRadv2->pReadSeekTellUserData, offset, origin);
    if (result != MA_SUCCESS) {
        return -1;
    }

    return 0;
}

static ma_int64 ma_radv2_of_callback__tell(void *pUserData) {
    ma_radv2 *pRadv2 = (ma_radv2 *)pUserData;
    ma_result result;
    ma_int64 cursor;

    if (pRadv2->onTell == NULL) {
        return -1;
    }

    result = pRadv2->onTell(pRadv2->pReadSeekTellUserData, &cursor);
    if (result != MA_SUCCESS) {
        return -1;
    }

    return cursor;
}

static ma_result ma_radv2_init_internal(const ma_decoding_backend_config *pConfig, ma_radv2 *pRadv2) {
    ma_result result;
    ma_data_source_config dataSourceConfig;

    if (!pRadv2) {
        return MA_INVALID_ARGS;
    }

    MA_ZERO_OBJECT(pRadv2);
    pRadv2->format = ma_format_s16; // RADv2 Opal outputs 16-bit signed samples by default

    dataSourceConfig = ma_data_source_config_init();
    dataSourceConfig.vtable = &ma_data_source_vtable_radv2;

    result = ma_data_source_init(&dataSourceConfig, &pRadv2->ds);
    if (result != MA_SUCCESS) {
        return result; /* Failed to initialize the base data source. */
    }

    return MA_SUCCESS;
}

static ma_result ma_radv2_init(ma_read_proc onRead, ma_seek_proc onSeek, ma_tell_proc onTell, void *pReadSeekTellUserData,
                               const ma_decoding_backend_config *pConfig, const ma_allocation_callbacks *pAllocationCallbacks, ma_radv2 *pRadv2) {
    ma_result result;

    (void)pAllocationCallbacks; /* Can't seem to find a way to configure memory allocations in libopus. */

    result = ma_radv2_init_internal(pConfig, pRadv2);
    if (result != MA_SUCCESS) {
        return result;
    }

    if (onRead == NULL || onSeek == NULL) {
        return MA_INVALID_ARGS; /* onRead and onSeek are mandatory. */
    }

    pRadv2->onRead = onRead;
    pRadv2->onSeek = onSeek;
    pRadv2->onTell = onTell;
    pRadv2->pReadSeekTellUserData = pReadSeekTellUserData;

    // Create the RADv2 Player objects
    pRadv2->player = new RADPlayer();
    if (!pRadv2->player) {
        return MA_OUT_OF_MEMORY;
    }

    // Create the RADv2 Opal object
    pRadv2->adlib = new Opal(MA_DEFAULT_SAMPLE_RATE);
    if (!pRadv2->adlib) {
        delete pRadv2->player;
        pRadv2->player = nullptr;
        return MA_OUT_OF_MEMORY;
    }

    // Find the size of the file
    if (ma_radv2_of_callback__seek(pRadv2, 0, SEEK_END) != 0) {
        delete pRadv2->adlib;
        delete pRadv2->player;
        pRadv2->adlib = nullptr;
        pRadv2->player = nullptr;
        return MA_BAD_SEEK;
    }

    // Calculate the length
    ma_int64 file_size = ma_radv2_of_callback__tell(pRadv2);
    if (file_size < 1) {
        delete pRadv2->adlib;
        delete pRadv2->player;
        pRadv2->adlib = nullptr;
        pRadv2->player = nullptr;
        return MA_INVALID_FILE;
    }

    // Allocate some memory for the tune
    pRadv2->tune = new uint8_t[file_size];
    if (!pRadv2->tune) {
        delete pRadv2->adlib;
        delete pRadv2->player;
        pRadv2->adlib = nullptr;
        pRadv2->player = nullptr;
        return MA_OUT_OF_MEMORY;
    }

    // See to the beginning of the file
    if (ma_radv2_of_callback__seek(pRadv2, 0, SEEK_SET) != 0) {
        delete[] pRadv2->tune;
        delete pRadv2->adlib;
        delete pRadv2->player;
        pRadv2->tune = nullptr;
        pRadv2->adlib = nullptr;
        pRadv2->player = nullptr;
        return MA_BAD_SEEK;
    }

    // Read the file
    if (ma_radv2_of_callback__read(pRadv2, (unsigned char *)pRadv2->tune, file_size) < 1) {
        delete[] pRadv2->tune;
        delete pRadv2->adlib;
        delete pRadv2->player;
        pRadv2->tune = nullptr;
        pRadv2->adlib = nullptr;
        pRadv2->player = nullptr;
        return MA_IO_ERROR;
    }

    // Check if the file is valid
    if (RADValidate(pRadv2->tune, file_size)) {
        delete[] pRadv2->tune;
        delete pRadv2->adlib;
        delete pRadv2->player;
        pRadv2->tune = nullptr;
        pRadv2->adlib = nullptr;
        pRadv2->player = nullptr;
        return MA_INVALID_FILE;
    }

    // Initialize the player
    // We'll use a lambda here and pass the pRadv2 pointer using 'arg'
    pRadv2->player->Init(
        pRadv2->tune, [](void *arg, uint16_t reg, uint8_t data) { ((ma_radv2 *)arg)->adlib->Port(reg, data); }, pRadv2);

    // Get the playback rate
    if (pRadv2->player->GetHertz() < 0) {
        delete[] pRadv2->tune;
        delete pRadv2->adlib;
        delete pRadv2->player;
        pRadv2->tune = nullptr;
        pRadv2->adlib = nullptr;
        pRadv2->player = nullptr;
        return MA_INVALID_FILE;
    }

    // Get the total playback time
    pRadv2->totalTime = pRadv2->player->ComputeTotalTime();
    // Setup some stuff
    pRadv2->sampleCount = 0;
    pRadv2->sampleUpdate = MA_DEFAULT_SAMPLE_RATE / pRadv2->player->GetHertz();

    return MA_SUCCESS;
}

static ma_result ma_radv2_init_file(const char *pFilePath, const ma_decoding_backend_config *pConfig, const ma_allocation_callbacks *pAllocationCallbacks,
                                    ma_radv2 *pRadv2) {
    ma_result result;

    (void)pAllocationCallbacks; /* Can't seem to find a way to configure memory allocations in libopus. */

    result = ma_radv2_init_internal(pConfig, pRadv2);
    if (result != MA_SUCCESS) {
        return result;
    }

    // Check the file extension
    if (!ma_path_extension_equal(pFilePath, "rad")) {
        return MA_INVALID_FILE;
    }

    // Open the file for reading
    FILE *fd = nullptr;
    if (fopen_s(&fd, pFilePath, "rb") != 0 || !fd) {
        return MA_INVALID_FILE;
    }

    // Create the RADv2 Player objects
    pRadv2->player = new RADPlayer();
    if (!pRadv2->player) {
        fclose(fd);
        return MA_OUT_OF_MEMORY;
    }

    // Create the RADv2 Opal object
    pRadv2->adlib = new Opal(MA_DEFAULT_SAMPLE_RATE);
    if (!pRadv2->adlib) {
        delete pRadv2->player;
        fclose(fd);
        pRadv2->player = nullptr;
        return MA_OUT_OF_MEMORY;
    }

    // Find the size of the file
    if (fseek(fd, 0, SEEK_END) != 0) {
        delete pRadv2->adlib;
        delete pRadv2->player;
        fclose(fd);
        pRadv2->adlib = nullptr;
        pRadv2->player = nullptr;
        return MA_BAD_SEEK;
    }

    // Calculate the length
    ma_int64 file_size = ftell(fd);
    if (file_size < 1) {
        delete pRadv2->adlib;
        delete pRadv2->player;
        fclose(fd);
        pRadv2->adlib = nullptr;
        pRadv2->player = nullptr;
        return MA_INVALID_FILE;
    }

    // Allocate some memory for the tune
    pRadv2->tune = new uint8_t[file_size];
    if (!pRadv2->tune) {
        delete pRadv2->adlib;
        delete pRadv2->player;
        fclose(fd);
        pRadv2->adlib = nullptr;
        pRadv2->player = nullptr;
        return MA_OUT_OF_MEMORY;
    }

    // Seek to the beginning of the file
    if (fseek(fd, 0, SEEK_SET) != 0) {
        delete[] pRadv2->tune;
        delete pRadv2->adlib;
        delete pRadv2->player;
        fclose(fd);
        pRadv2->tune = nullptr;
        pRadv2->adlib = nullptr;
        pRadv2->player = nullptr;
        return MA_BAD_SEEK;
    }

    // Read the file
    if (fread(pRadv2->tune, file_size, sizeof(uint8_t), fd) < 1) {
        delete[] pRadv2->tune;
        delete pRadv2->adlib;
        delete pRadv2->player;
        fclose(fd);
        pRadv2->tune = nullptr;
        pRadv2->adlib = nullptr;
        pRadv2->player = nullptr;
        return MA_IO_ERROR;
    }

    // Close the file now that we've read it into memory
    fclose(fd);

    // Check if the file is valid
    if (RADValidate(pRadv2->tune, file_size)) {
        delete[] pRadv2->tune;
        delete pRadv2->adlib;
        delete pRadv2->player;
        pRadv2->tune = nullptr;
        pRadv2->adlib = nullptr;
        pRadv2->player = nullptr;
        return MA_INVALID_FILE;
    }

    // Initialize the player
    // We'll use a lambda here and pass the pRadv2 pointer using 'arg'
    pRadv2->player->Init(
        pRadv2->tune, [](void *arg, uint16_t reg, uint8_t data) { ((ma_radv2 *)arg)->adlib->Port(reg, data); }, pRadv2);

    // Get the playback rate
    if (pRadv2->player->GetHertz() < 0) {
        delete[] pRadv2->tune;
        delete pRadv2->adlib;
        delete pRadv2->player;
        pRadv2->tune = nullptr;
        pRadv2->adlib = nullptr;
        pRadv2->player = nullptr;
        return MA_INVALID_FILE;
    }

    // Get the total playback time
    pRadv2->totalTime = pRadv2->player->ComputeTotalTime();
    // Setup some stuff
    pRadv2->sampleCount = 0;
    pRadv2->sampleUpdate = MA_DEFAULT_SAMPLE_RATE / pRadv2->player->GetHertz();

    return MA_SUCCESS;
}

static void ma_radv2_uninit(ma_radv2 *pRadv2, const ma_allocation_callbacks *pAllocationCallbacks) {
    if (!pRadv2) {
        return;
    }

    (void)pAllocationCallbacks;

    // Stop any ongoing playback
    pRadv2->player->Stop();

    delete[] pRadv2->tune;
    delete pRadv2->adlib;
    delete pRadv2->player;
    pRadv2->tune = nullptr;
    pRadv2->adlib = nullptr;
    pRadv2->player = nullptr;

    ma_data_source_uninit(&pRadv2->ds);
}

static ma_result ma_decoding_backend_init__radv2(void *pUserData, ma_read_proc onRead, ma_seek_proc onSeek, ma_tell_proc onTell, void *pReadSeekTellUserData,
                                                 const ma_decoding_backend_config *pConfig, const ma_allocation_callbacks *pAllocationCallbacks,
                                                 ma_data_source **ppBackend) {
    ma_result result;
    ma_radv2 *pRadv2;

    (void)pUserData;

    pRadv2 = (ma_radv2 *)ma_malloc(sizeof(ma_radv2), pAllocationCallbacks);
    if (!pRadv2) {
        return MA_OUT_OF_MEMORY;
    }

    result = ma_radv2_init(onRead, onSeek, onTell, pReadSeekTellUserData, pConfig, pAllocationCallbacks, pRadv2);
    if (result != MA_SUCCESS) {
        ma_free(pRadv2, pAllocationCallbacks);
        return result;
    }

    *ppBackend = pRadv2;

    return MA_SUCCESS;
}

static ma_result ma_decoding_backend_init_file__radv2(void *pUserData, const char *pFilePath, const ma_decoding_backend_config *pConfig,
                                                      const ma_allocation_callbacks *pAllocationCallbacks, ma_data_source **ppBackend) {
    ma_result result;
    ma_radv2 *pRadv2;

    (void)pUserData;

    pRadv2 = (ma_radv2 *)ma_malloc(sizeof(ma_radv2), pAllocationCallbacks);
    if (!pRadv2) {
        return MA_OUT_OF_MEMORY;
    }

    result = ma_radv2_init_file(pFilePath, pConfig, pAllocationCallbacks, pRadv2);
    if (result != MA_SUCCESS) {
        ma_free(pRadv2, pAllocationCallbacks);
        return result;
    }

    *ppBackend = pRadv2;

    return MA_SUCCESS;
}

static void ma_decoding_backend_uninit__radv2(void *pUserData, ma_data_source *pBackend, const ma_allocation_callbacks *pAllocationCallbacks) {
    ma_radv2 *pRadv2 = (ma_radv2 *)pBackend;

    (void)pUserData;

    ma_radv2_uninit(pRadv2, pAllocationCallbacks);
    ma_free(pRadv2, pAllocationCallbacks);
}

static ma_decoding_backend_vtable ma_decoding_backend_vtable_radv2 = {ma_decoding_backend_init__radv2, ma_decoding_backend_init_file__radv2,
                                                                      NULL, /* onInitFileW() */
                                                                      NULL, /* onInitMemory() */
                                                                      ma_decoding_backend_uninit__radv2};
