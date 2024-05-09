//----------------------------------------------------------------------------------------------------
//    ___  ___   __ _ _  ___ ___     _          _ _       ___           _
//   / _ \| _ ) / /| | || _ \ __|   /_\ _  _ __| (_)___  | __|_ _  __ _(_)_ _  ___
//  | (_) | _ \/ _ \_  _|  _/ _|   / _ \ || / _` | / _ \ | _|| ' \/ _` | | ' \/ -_)
//   \__\_\___/\___/ |_||_| |___| /_/ \_\_,_\__,_|_\___/ |___|_||_\__, |_|_||_\___|
//                                                                |___/
//
//  QB64-PE Audio Engine powered by miniaudio (https://miniaud.io/)
//
//  This implements a data source that decodes Reality Adlib Tracker 2 tunes
//  https://realityproductions.itch.io/rad (Public Domain)
//
//-----------------------------------------------------------------------------------------------------

#include "../miniaudio.h"
#include "audio.h"
#include "filepath.h"
#include "libqb-common.h"
#include <stdio.h>
#include <string.h>

#include "radv2/opal.h"
#define RAD_DETECT_REPEATS 1
#include "radv2/player20.cpp"
#include "radv2/validate20.cpp"

#include "vtables.h"

struct ma_radv2 {
    // This part is for miniaudio
    ma_data_source_base ds; /* The decoder can be used independently as a data source. */
    ma_read_proc onRead;
    ma_seek_proc onSeek;
    ma_tell_proc onTell;
    void *pReadSeekTellUserData;
    ma_format format;

    // This part is format specific
    RADPlayer *player;  // RADv2 player object
    Opal *adlib;        // Opal Adlib emulator object
    ma_uint8 *tune;     // The song data (RADv2 needs this to be alive for rendering samples!)
    uint32_t totalTime; // Total time in seconds
    int sampleCount;    // The number of samples generated in each update
    int sampleUpdate;   // Size of each update in samples after which the player must be updated
};

static ma_result ma_radv2_seek_to_pcm_frame(ma_radv2 *pRadv2, ma_uint64 frameIndex) {
    if (!pRadv2) {
        return MA_INVALID_ARGS;
    }

    // We can only reset the player to the beginning
    if (frameIndex == 0) {
        pRadv2->player->Stop();
        return MA_SUCCESS;
    }

    return MA_INVALID_OPERATION; // Anything else is not seekable
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
        memset(pChannelMap, 0, sizeof(*pChannelMap) * channelMapCap);
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

    ma_result result = MA_SUCCESS; /* Must be initialized to MA_SUCCESS. */
    ma_uint64 totalFramesRead = 0;
    ma_int16 *buffer = (ma_int16 *)pFramesOut;
    bool repeat = false;

    while (totalFramesRead < frameCount) {
        pRadv2->adlib->Sample(&buffer[0], &buffer[1]); // Get the left and right sample
        buffer += 2;                                   // Increment the buffer pointer twice for 2 channels
        ++totalFramesRead;                             // Increment the frame counter

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

    return result;
}

static ma_result ma_radv2_get_cursor_in_pcm_frames(ma_radv2 *pRadv2, ma_uint64 *pCursor) {
    if (!pCursor) {
        return MA_INVALID_ARGS;
    }

    *pCursor = 0; /* Safety. */

    if (!pRadv2) {
        return MA_INVALID_ARGS;
    }

    ma_int64 offset = (ma_int64)pRadv2->player->GetPlayTimeInSeconds() * MA_DEFAULT_SAMPLE_RATE;
    if (offset < 0) {
        return MA_INVALID_FILE;
    }

    *pCursor = (ma_uint64)offset;

    return MA_SUCCESS;
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
    ma_int64 length = (ma_int64)pRadv2->totalTime * MA_DEFAULT_SAMPLE_RATE;
    if (length < 0) {
        return MA_INVALID_FILE;
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

// clang-format off
static ma_data_source_vtable ma_data_source_vtable_radv2 = {
    ma_radv2_ds_read,
    ma_radv2_ds_seek,
    ma_radv2_ds_get_data_format,
    ma_radv2_ds_get_cursor,
    ma_radv2_ds_get_length
};
// clang-format on

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

    memset(pRadv2, 0, sizeof(*pRadv2));
    pRadv2->format = ma_format::ma_format_s16; // RADv2 Opal outputs 16-bit signed samples by default

    if (pConfig != NULL && pConfig->preferredFormat == ma_format::ma_format_s16) {
        pRadv2->format = pConfig->preferredFormat;
    } else {
        /* Getting here means something other than s16 was specified. Just leave this unset to use the default format. */
    }

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

    (void)pAllocationCallbacks;

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

    // Find the size of the file
    if (ma_radv2_of_callback__seek(pRadv2, 0, SEEK_END) != 0) {
        return MA_BAD_SEEK;
    }

    // Calculate the length
    ma_int64 file_size = ma_radv2_of_callback__tell(pRadv2);
    if (file_size < 1) {
        return MA_INVALID_FILE;
    }

    // See to the beginning of the file
    if (ma_radv2_of_callback__seek(pRadv2, 0, SEEK_SET) != 0) {
        return MA_BAD_SEEK;
    }

    // Allocate some memory for the tune
    pRadv2->tune = new uint8_t[file_size];
    if (!pRadv2->tune) {
        return MA_OUT_OF_MEMORY;
    }

    // Read the file
    if (ma_radv2_of_callback__read(pRadv2, pRadv2->tune, (int)file_size) < 1) {
        delete[] pRadv2->tune;
        pRadv2->tune = nullptr;
        return MA_IO_ERROR;
    }

    // Create the RADv2 Player objects
    pRadv2->player = new RADPlayer();
    if (!pRadv2->player) {
        delete[] pRadv2->tune;
        pRadv2->tune = nullptr;
        return MA_OUT_OF_MEMORY;
    }

    // Create the RADv2 Opal object
    pRadv2->adlib = new Opal(MA_DEFAULT_SAMPLE_RATE);
    if (!pRadv2->adlib) {
        delete pRadv2->player;
        pRadv2->player = nullptr;
        delete[] pRadv2->tune;
        pRadv2->tune = nullptr;
        return MA_OUT_OF_MEMORY;
    }

    // Check if the file is valid
    if (RADValidate(pRadv2->tune, file_size)) {
        delete pRadv2->adlib;
        pRadv2->adlib = nullptr;
        delete pRadv2->player;
        pRadv2->player = nullptr;
        delete[] pRadv2->tune;
        pRadv2->tune = nullptr;
        return MA_INVALID_FILE;
    }

    // Initialize the player
    // We'll use a lambda here and pass the pRadv2 pointer using 'arg'
    pRadv2->player->Init(pRadv2->tune, [](void *arg, uint16_t reg, uint8_t data) { ((ma_radv2 *)arg)->adlib->Port(reg, data); }, pRadv2);

    // Get the playback rate
    if (pRadv2->player->GetHertz() < 0) {
        delete pRadv2->adlib;
        pRadv2->adlib = nullptr;
        delete pRadv2->player;
        pRadv2->player = nullptr;
        delete[] pRadv2->tune;
        pRadv2->tune = nullptr;
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

    (void)pAllocationCallbacks;

    result = ma_radv2_init_internal(pConfig, pRadv2);
    if (result != MA_SUCCESS) {
        return result;
    }

    // Check the file extension
    if (!filepath_has_extension(pFilePath, "rad")) {
        return MA_INVALID_FILE;
    }

    // Open the file for reading
    FILE *fd = fopen(pFilePath, "rb");
    if (!fd) {
        return MA_INVALID_FILE;
    }

    // Find the size of the file
    if (fseek(fd, 0, SEEK_END) != 0) {
        fclose(fd);
        return MA_BAD_SEEK;
    }

    // Calculate the length
    ma_int64 file_size = ftell(fd);
    if (file_size < 1) {
        fclose(fd);
        return MA_INVALID_FILE;
    }

    // Seek to the beginning of the file
    if (fseek(fd, 0, SEEK_SET) != 0) {
        fclose(fd);
        return MA_BAD_SEEK;
    }

    // Allocate some memory for the tune
    pRadv2->tune = new uint8_t[file_size];
    if (!pRadv2->tune) {
        fclose(fd);
        return MA_OUT_OF_MEMORY;
    }

    // Read the file
    if (fread(pRadv2->tune, file_size, sizeof(uint8_t), fd) < 1) {
        delete[] pRadv2->tune;
        pRadv2->tune = nullptr;
        fclose(fd);
        return MA_IO_ERROR;
    }

    // Close the file now that we've read it into memory
    fclose(fd);

    // Create the RADv2 Player objects
    pRadv2->player = new RADPlayer();
    if (!pRadv2->player) {
        delete[] pRadv2->tune;
        pRadv2->tune = nullptr;
        return MA_OUT_OF_MEMORY;
    }

    // Create the RADv2 Opal object
    pRadv2->adlib = new Opal(MA_DEFAULT_SAMPLE_RATE);
    if (!pRadv2->adlib) {
        delete pRadv2->player;
        pRadv2->player = nullptr;
        delete[] pRadv2->tune;
        pRadv2->tune = nullptr;
        return MA_OUT_OF_MEMORY;
    }

    // Check if the file is valid
    if (RADValidate(pRadv2->tune, file_size)) {
        delete pRadv2->adlib;
        pRadv2->adlib = nullptr;
        delete pRadv2->player;
        pRadv2->player = nullptr;
        delete[] pRadv2->tune;
        pRadv2->tune = nullptr;
        return MA_INVALID_FILE;
    }

    // Initialize the player
    // We'll use a lambda here and pass the pRadv2 pointer using 'arg'
    pRadv2->player->Init(pRadv2->tune, [](void *arg, uint16_t reg, uint8_t data) { ((ma_radv2 *)arg)->adlib->Port(reg, data); }, pRadv2);

    // Get the playback rate
    if (pRadv2->player->GetHertz() < 0) {
        delete pRadv2->adlib;
        pRadv2->adlib = nullptr;
        delete pRadv2->player;
        pRadv2->player = nullptr;
        delete[] pRadv2->tune;
        pRadv2->tune = nullptr;
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

    delete pRadv2->adlib;
    pRadv2->adlib = nullptr;
    delete pRadv2->player;
    pRadv2->player = nullptr;
    delete[] pRadv2->tune;
    pRadv2->tune = nullptr;

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

// clang-format off
ma_decoding_backend_vtable ma_vtable_radv2 = {
    ma_decoding_backend_init__radv2,
    ma_decoding_backend_init_file__radv2,
    NULL, /* onInitFileW() */
    NULL, /* onInitMemory() */
    ma_decoding_backend_uninit__radv2
};
// clang-format on
