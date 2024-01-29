//----------------------------------------------------------------------------------------------------
//    ___  ___   __ _ _  ___ ___     _          _ _       ___           _
//   / _ \| _ ) / /| | || _ \ __|   /_\ _  _ __| (_)___  | __|_ _  __ _(_)_ _  ___
//  | (_) | _ \/ _ \_  _|  _/ _|   / _ \ || / _` | / _ \ | _|| ' \/ _` | | ' \/ -_)
//   \__\_\___/\___/ |_||_| |___| /_/ \_\_,_\__,_|_\___/ |___|_||_\__, |_|_||_\___|
//                                                                |___/
//
//  QB64-PE Audio Engine powered by miniaudio (https://miniaud.io/)
//
//  This implements a data source that decodes MIDI files using TinySoundFont + TinyMidiLoader
//  https://github.com/schellingb/TinySoundFont (MIT)
//
//  Soundfont (awe32rom.h) from dos-like
//  https://github.com/mattiasgustavsson/dos-like (MIT)
//
//-----------------------------------------------------------------------------------------------------

#include "../miniaudio.h"
#include "audio.h"
#include "filepath.h"
#include "libqb-common.h"
#include <string.h>

#define STB_VORBIS_HEADER_ONLY
#include "stb_vorbis.c"
#define TSF_IMPLEMENTATION
#include "tinysoundfont/tsf.h"
#define TML_IMPLEMENTATION
#include "tinysoundfont/tml.h"
#undef STB_VORBIS_HEADER_ONLY

#include "vtables.h"

extern "C" {
// These symbols reference a soundfont compiled into the program
//
// We provide a macro to expand to the correct symbol name

#if defined(QB64_WINDOWS) && defined(QB64_32)
// On 32-bit Windows, we use objcopy, and the symbols do not have an
// underscore prefix
extern char binary_soundfont_sf2_start[];
extern char binary_soundfont_sf2_end[];

#    define SOUNDFONT_BIN binary_soundfont_sf2_start
#    define SOUNDFONT_SIZE (binary_soundfont_sf2_end - binary_soundfont_sf2_start)

#elif defined(QB64_WINDOWS) || defined(QB64_LINUX)
// On Linux and 64-bit Windows, we use objcopy, and the symbols do have an
// underscore prefix.
extern char _binary_soundfont_sf2_start[];
extern char _binary_soundfont_sf2_end[];

#    define SOUNDFONT_BIN _binary_soundfont_sf2_start
#    define SOUNDFONT_SIZE (_binary_soundfont_sf2_end - _binary_soundfont_sf2_start)

#else
// On Mac OS we use xxd, which gives an array and size
extern unsigned char soundfont_sf2[];
extern unsigned int soundfont_sf2_len;

#    define SOUNDFONT_BIN soundfont_sf2
#    define SOUNDFONT_SIZE soundfont_sf2_len

#endif
}

struct ma_tsf {
    // This part is for miniaudio
    ma_data_source_base ds; /* The decoder can be used independently as a data source. */
    ma_read_proc onRead;
    ma_seek_proc onSeek;
    ma_tell_proc onTell;
    void *pReadSeekTellUserData;
    ma_format format;

    // This part is format specific
    tsf *tinySoundFont;          // TinySoundFont context
    tml_message *tinyMidiLoader; // TinyMidiLoader context
    ma_uint32 totalTime;         // Total duration of the MIDI song in msec
    double currentTime;          // Current song playback time in msec
    tml_message *midiMessage;    // Next message to be played (this is set to NULL once the song is over)
};

static ma_result ma_tsf_seek_to_pcm_frame(ma_tsf *pTsf, ma_uint64 frameIndex) {
    if (pTsf == NULL) {
        return MA_INVALID_ARGS;
    }

    // We can only reset the player to the beginning
    if (frameIndex == 0) {
        tsf_reset(pTsf->tinySoundFont);           // Stop playing whatever is playing
        pTsf->midiMessage = pTsf->tinyMidiLoader; // Set up the global MidiMessage pointer to the first MIDI message
        pTsf->currentTime = 0;                    // Reset playback time
        return MA_SUCCESS;
    }

    return MA_INVALID_OPERATION; // Anything else is not seekable
}

static ma_result ma_tsf_get_data_format(ma_tsf *pTsf, ma_format *pFormat, ma_uint32 *pChannels, ma_uint32 *pSampleRate, ma_channel *pChannelMap,
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

    if (pTsf == NULL) {
        return MA_INVALID_OPERATION;
    }

    if (pFormat != NULL) {
        *pFormat = pTsf->format;
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

static ma_result ma_tsf_read_pcm_frames(ma_tsf *pTsf, void *pFramesOut, ma_uint64 frameCount, ma_uint64 *pFramesRead) {
    if (pFramesRead != NULL) {
        *pFramesRead = 0;
    }

    if (frameCount == 0) {
        return MA_INVALID_ARGS;
    }

    if (pTsf == NULL) {
        return MA_INVALID_ARGS;
    }

    ma_result result = MA_SUCCESS; // Must be initialized to MA_SUCCESS
    ma_uint64 totalFramesRead = 0;
    ma_uint8 *buffer = (ma_uint8 *)pFramesOut;
    ma_int64 SampleBlock, SampleCount = frameCount; // Number of sample frames to process

    for (SampleBlock = TSF_RENDER_EFFECTSAMPLEBLOCK; SampleCount; SampleCount -= SampleBlock, buffer += (SampleBlock * (sizeof(short) * 2))) {
        // We progress the MIDI playback and then process TSF_RENDER_EFFECTSAMPLEBLOCK samples at once
        if (SampleBlock > SampleCount)
            SampleBlock = SampleCount;

        // Loop through all MIDI messages which need to be played up until the current playback time
        for (pTsf->currentTime += SampleBlock * (1000.0 / MA_DEFAULT_SAMPLE_RATE); pTsf->midiMessage && pTsf->currentTime >= pTsf->midiMessage->time;
             pTsf->midiMessage = pTsf->midiMessage->next) {
            switch (pTsf->midiMessage->type) {
            case TML_PROGRAM_CHANGE: // Channel program (preset) change (special handling for 10th MIDI channel with drums)
                tsf_channel_set_presetnumber(pTsf->tinySoundFont, pTsf->midiMessage->channel, pTsf->midiMessage->program, (pTsf->midiMessage->channel == 9));
                tsf_channel_midi_control(pTsf->tinySoundFont, pTsf->midiMessage->channel, TML_ALL_NOTES_OFF,
                                         0); // https://github.com/schellingb/TinySoundFont/issues/59
                break;
            case TML_NOTE_ON: // Play a note
                tsf_channel_note_on(pTsf->tinySoundFont, pTsf->midiMessage->channel, pTsf->midiMessage->key, pTsf->midiMessage->velocity / 127.0f);
                break;
            case TML_NOTE_OFF: // Stop a note
                tsf_channel_note_off(pTsf->tinySoundFont, pTsf->midiMessage->channel, pTsf->midiMessage->key);
                break;
            case TML_PITCH_BEND: // Pitch wheel modification
                tsf_channel_set_pitchwheel(pTsf->tinySoundFont, pTsf->midiMessage->channel, pTsf->midiMessage->pitch_bend);
                break;
            case TML_CONTROL_CHANGE: // MIDI controller messages
                tsf_channel_midi_control(pTsf->tinySoundFont, pTsf->midiMessage->channel, pTsf->midiMessage->control, pTsf->midiMessage->control_value);
                break;
            }
        }

        // Render the block of audio samples in int16 format
        tsf_render_short(pTsf->tinySoundFont, (short *)buffer, (int)SampleBlock);
        totalFramesRead += SampleBlock;

        // Signal end of stream if we have reached the end
        if (pTsf->midiMessage == NULL) {
            result = MA_AT_END;
            // However, also reset the position to the beginning just in case we want to loop
            pTsf->midiMessage = pTsf->tinyMidiLoader; // Set up the global MidiMessage pointer to the first MIDI message
            pTsf->currentTime = 0;                    // Reset playback time
            break;
        }
    }

    if (pFramesRead != NULL) {
        *pFramesRead = totalFramesRead;
    }

    return result;
}

static ma_result ma_tsf_get_cursor_in_pcm_frames(ma_tsf *pTsf, ma_uint64 *pCursor) {
    if (pCursor == NULL) {
        return MA_INVALID_ARGS;
    }

    *pCursor = 0; /* Safety. */

    if (pTsf == NULL) {
        return MA_INVALID_ARGS;
    }

    ma_int64 offset = ((ma_int64)pTsf->currentTime * MA_DEFAULT_SAMPLE_RATE) / 1000;
    if (offset < 0) {
        return MA_INVALID_FILE;
    }

    *pCursor = (ma_uint64)offset;

    return MA_SUCCESS;
}

static ma_result ma_tsf_get_length_in_pcm_frames(ma_tsf *pTsf, ma_uint64 *pLength) {
    if (pLength == NULL) {
        return MA_INVALID_ARGS;
    }

    *pLength = 0; /* Safety. */

    if (pTsf == NULL) {
        return MA_INVALID_ARGS;
    }

    // Total time in seconds * Opal sample rate
    ma_int64 length = ((ma_int64)pTsf->totalTime * MA_DEFAULT_SAMPLE_RATE) / 1000;
    if (length < 0) {
        return MA_INVALID_FILE;
    }

    *pLength = (ma_uint64)length;

    return MA_SUCCESS;
}

static ma_result ma_tsf_ds_read(ma_data_source *pDataSource, void *pFramesOut, ma_uint64 frameCount, ma_uint64 *pFramesRead) {
    return ma_tsf_read_pcm_frames((ma_tsf *)pDataSource, pFramesOut, frameCount, pFramesRead);
}

static ma_result ma_tsf_ds_seek(ma_data_source *pDataSource, ma_uint64 frameIndex) { return ma_tsf_seek_to_pcm_frame((ma_tsf *)pDataSource, frameIndex); }

static ma_result ma_tsf_ds_get_data_format(ma_data_source *pDataSource, ma_format *pFormat, ma_uint32 *pChannels, ma_uint32 *pSampleRate,
                                           ma_channel *pChannelMap, size_t channelMapCap) {
    return ma_tsf_get_data_format((ma_tsf *)pDataSource, pFormat, pChannels, pSampleRate, pChannelMap, channelMapCap);
}

static ma_result ma_tsf_ds_get_cursor(ma_data_source *pDataSource, ma_uint64 *pCursor) {
    return ma_tsf_get_cursor_in_pcm_frames((ma_tsf *)pDataSource, pCursor);
}

static ma_result ma_tsf_ds_get_length(ma_data_source *pDataSource, ma_uint64 *pLength) {
    return ma_tsf_get_length_in_pcm_frames((ma_tsf *)pDataSource, pLength);
}

// clang-format off
static ma_data_source_vtable ma_data_source_vtable_tsf = {
    ma_tsf_ds_read, ma_tsf_ds_seek,
    ma_tsf_ds_get_data_format,
    ma_tsf_ds_get_cursor,
    ma_tsf_ds_get_length
};
// clang-format on

static int ma_tsf_of_callback__read(void *pUserData, unsigned char *pBufferOut, int bytesToRead) {
    ma_tsf *pTsf = (ma_tsf *)pUserData;
    ma_result result;
    size_t bytesRead;

    result = pTsf->onRead(pTsf->pReadSeekTellUserData, (void *)pBufferOut, bytesToRead, &bytesRead);

    if (result != MA_SUCCESS) {
        return -1;
    }

    return (int)bytesRead;
}

static int ma_tsf_of_callback__seek(void *pUserData, ma_int64 offset, int whence) {
    ma_tsf *pTsf = (ma_tsf *)pUserData;
    ma_result result;
    ma_seek_origin origin;

    if (whence == SEEK_SET) {
        origin = ma_seek_origin_start;
    } else if (whence == SEEK_END) {
        origin = ma_seek_origin_end;
    } else {
        origin = ma_seek_origin_current;
    }

    result = pTsf->onSeek(pTsf->pReadSeekTellUserData, offset, origin);
    if (result != MA_SUCCESS) {
        return -1;
    }

    return 0;
}

static ma_int64 ma_tsf_of_callback__tell(void *pUserData) {
    ma_tsf *pTsf = (ma_tsf *)pUserData;
    ma_result result;
    ma_int64 cursor;

    if (pTsf->onTell == NULL) {
        return -1;
    }

    result = pTsf->onTell(pTsf->pReadSeekTellUserData, &cursor);
    if (result != MA_SUCCESS) {
        return -1;
    }

    return cursor;
}

static ma_result ma_tsf_init_internal(const ma_decoding_backend_config *pConfig, ma_tsf *pTsf) {
    ma_result result;
    ma_data_source_config dataSourceConfig;

    if (pTsf == NULL) {
        return MA_INVALID_ARGS;
    }

    memset(pTsf, 0, sizeof(&pTsf));
    pTsf->format = ma_format::ma_format_s16; // We'll render 16-bit signed samples by default

    if (pConfig != NULL && pConfig->preferredFormat == ma_format::ma_format_s16) {
        pTsf->format = pConfig->preferredFormat;
    } else {
        /* Getting here means something other than s16 was specified. Just leave this unset to use the default format. */
    }

    dataSourceConfig = ma_data_source_config_init();
    dataSourceConfig.vtable = &ma_data_source_vtable_tsf;

    result = ma_data_source_init(&dataSourceConfig, &pTsf->ds);
    if (result != MA_SUCCESS) {
        return result; /* Failed to initialize the base data source. */
    }

    return MA_SUCCESS;
}

ma_result ma_tsf_load_memory(ma_tsf *pTsf) {
    // Attempt to load a SoundFont from memory
    pTsf->tinySoundFont = tsf_load_memory(SOUNDFONT_BIN, SOUNDFONT_SIZE);

    // Return failure if loading from memory also failed. This should not happen though
    return pTsf->tinySoundFont ? MA_SUCCESS : MA_OUT_OF_MEMORY;
}

static ma_result ma_tsf_init(ma_read_proc onRead, ma_seek_proc onSeek, ma_tell_proc onTell, void *pReadSeekTellUserData,
                             const ma_decoding_backend_config *pConfig, const ma_allocation_callbacks *pAllocationCallbacks, ma_tsf *pTsf) {
    ma_result result;

    (void)pAllocationCallbacks;

    result = ma_tsf_init_internal(pConfig, pTsf);
    if (result != MA_SUCCESS) {
        return result;
    }

    if (onRead == NULL || onSeek == NULL) {
        return MA_INVALID_ARGS; /* onRead and onSeek are mandatory. */
    }

    pTsf->onRead = onRead;
    pTsf->onSeek = onSeek;
    pTsf->onTell = onTell;
    pTsf->pReadSeekTellUserData = pReadSeekTellUserData;

    // Seek to the end of the file
    if (ma_tsf_of_callback__seek(pTsf, 0, SEEK_END) != 0) {
        return MA_BAD_SEEK;
    }

    // Calculate the length
    ma_int64 file_size = ma_tsf_of_callback__tell(pTsf);
    if (file_size < 1) {
        return MA_INVALID_FILE;
    }

    // See to the beginning of the file
    if (ma_tsf_of_callback__seek(pTsf, 0, SEEK_SET) != 0) {
        return MA_BAD_SEEK;
    }

    // Allocate some memory for the tune
    ma_uint8 *tune = new ma_uint8[file_size];
    if (tune == nullptr) {
        return MA_OUT_OF_MEMORY;
    }

    // Read the file
    if (ma_tsf_of_callback__read(pTsf, tune, (int)file_size) < 1) {
        delete[] tune;
        return MA_IO_ERROR;
    }

    // Load soundfont
    result = ma_tsf_load_memory(pTsf);
    if (result != MA_SUCCESS) {
        delete[] tune;
        return result;
    }

    // Initialize preset on special 10th MIDI channel to use percussion sound bank (128) if available
    tsf_channel_set_bank_preset(pTsf->tinySoundFont, 9, 128, 0);

    // Set the SoundFont rendering output mode
    tsf_set_output(pTsf->tinySoundFont, TSF_STEREO_INTERLEAVED, MA_DEFAULT_SAMPLE_RATE);

    // Initialize TML
    pTsf->tinyMidiLoader = tml_load_memory(tune, (int)file_size);
    if (!pTsf->tinyMidiLoader) {
        tsf_close(pTsf->tinySoundFont);
        pTsf->tinySoundFont = TSF_NULL;
        delete[] tune;
        return MA_INVALID_FILE;
    }

    // Free the memory now that we don't need it anymore
    delete[] tune;

    // Get the total duration of the song ignoring the rest of the stuff
    tml_get_info(pTsf->tinyMidiLoader, NULL, NULL, NULL, NULL, &pTsf->totalTime);

    // Setup some stuff
    pTsf->midiMessage = pTsf->tinyMidiLoader; // Set up the global MidiMessage pointer to the first MIDI message
    pTsf->currentTime = 0;                    // Reset playback time

    return MA_SUCCESS;
}

static ma_result ma_tsf_init_file(const char *pFilePath, const ma_decoding_backend_config *pConfig, const ma_allocation_callbacks *pAllocationCallbacks,
                                  ma_tsf *pTsf) {
    ma_result result;

    (void)pAllocationCallbacks;

    result = ma_tsf_init_internal(pConfig, pTsf);
    if (result != MA_SUCCESS) {
        return result;
    }

    // Check the file extension
    if (!filepath_has_extension(pFilePath, "mid") && !filepath_has_extension(pFilePath, "midi")) {
        return MA_INVALID_FILE;
    }

    // Load soundfont
    result = ma_tsf_load_memory(pTsf);
    if (result != MA_SUCCESS)
        return result;

    // Initialize preset on special 10th MIDI channel to use percussion sound bank (128) if available
    tsf_channel_set_bank_preset(pTsf->tinySoundFont, 9, 128, 0);

    // Set the SoundFont rendering output mode
    tsf_set_output(pTsf->tinySoundFont, TSF_STEREO_INTERLEAVED, MA_DEFAULT_SAMPLE_RATE);

    // Initialize TML
    pTsf->tinyMidiLoader = tml_load_filename(pFilePath);
    if (!pTsf->tinyMidiLoader) {
        tsf_close(pTsf->tinySoundFont);
        pTsf->tinySoundFont = TSF_NULL;
        return MA_INVALID_FILE;
    }

    // Get the total duration of the song ignoring the rest of the stuff
    tml_get_info(pTsf->tinyMidiLoader, NULL, NULL, NULL, NULL, &pTsf->totalTime);

    // Setup some stuff
    pTsf->midiMessage = pTsf->tinyMidiLoader; // Set up the global MidiMessage pointer to the first MIDI message
    pTsf->currentTime = 0;                    // Reset playback time

    return MA_SUCCESS;
}

static void ma_tsf_uninit(ma_tsf *pTsf, const ma_allocation_callbacks *pAllocationCallbacks) {
    if (pTsf == NULL) {
        return;
    }

    (void)pAllocationCallbacks;

    tsf_reset(pTsf->tinySoundFont); // Stop playing whatever is playing
    tml_free(pTsf->tinyMidiLoader); // Free TML resources
    pTsf->tinyMidiLoader = TML_NULL;
    tsf_close(pTsf->tinySoundFont);
    pTsf->tinySoundFont = TSF_NULL;

    ma_data_source_uninit(&pTsf->ds);
}

static ma_result ma_decoding_backend_init__tsf(void *pUserData, ma_read_proc onRead, ma_seek_proc onSeek, ma_tell_proc onTell, void *pReadSeekTellUserData,
                                               const ma_decoding_backend_config *pConfig, const ma_allocation_callbacks *pAllocationCallbacks,
                                               ma_data_source **ppBackend) {
    ma_result result;
    ma_tsf *pTsf;

    (void)pUserData;

    pTsf = (ma_tsf *)ma_malloc(sizeof(ma_tsf), pAllocationCallbacks);
    if (pTsf == NULL) {
        return MA_OUT_OF_MEMORY;
    }

    result = ma_tsf_init(onRead, onSeek, onTell, pReadSeekTellUserData, pConfig, pAllocationCallbacks, pTsf);
    if (result != MA_SUCCESS) {
        ma_free(pTsf, pAllocationCallbacks);
        return result;
    }

    *ppBackend = pTsf;

    return MA_SUCCESS;
}

static ma_result ma_decoding_backend_init_file__tsf(void *pUserData, const char *pFilePath, const ma_decoding_backend_config *pConfig,
                                                    const ma_allocation_callbacks *pAllocationCallbacks, ma_data_source **ppBackend) {
    ma_result result;
    ma_tsf *pTsf;

    (void)pUserData;

    pTsf = (ma_tsf *)ma_malloc(sizeof(ma_tsf), pAllocationCallbacks);
    if (pTsf == NULL) {
        return MA_OUT_OF_MEMORY;
    }

    result = ma_tsf_init_file(pFilePath, pConfig, pAllocationCallbacks, pTsf);
    if (result != MA_SUCCESS) {
        ma_free(pTsf, pAllocationCallbacks);
        return result;
    }

    *ppBackend = pTsf;

    return MA_SUCCESS;
}

static void ma_decoding_backend_uninit__tsf(void *pUserData, ma_data_source *pBackend, const ma_allocation_callbacks *pAllocationCallbacks) {
    ma_tsf *pTsf = (ma_tsf *)pBackend;

    (void)pUserData;

    ma_tsf_uninit(pTsf, pAllocationCallbacks);
    ma_free(pTsf, pAllocationCallbacks);
}

// clang-format off
ma_decoding_backend_vtable ma_vtable_midi = {
    ma_decoding_backend_init__tsf,
    ma_decoding_backend_init_file__tsf,
    NULL, /* onInitFileW() */
    NULL, /* onInitMemory() */
    ma_decoding_backend_uninit__tsf
};
// clang-format on
