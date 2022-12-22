//----------------------------------------------------------------------------------------------------
//    ___  ___   __ _ _  ___ ___     _          _ _       ___           _
//   / _ \| _ ) / /| | || _ \ __|   /_\ _  _ __| (_)___  | __|_ _  __ _(_)_ _  ___
//  | (_) | _ \/ _ \_  _|  _/ _|   / _ \ || / _` | / _ \ | _|| ' \/ _` | | ' \/ -_)
//   \__\_\___/\___/ |_||_| |___| /_/ \_\_,_\__,_|_\___/ |___|_||_\__, |_|_||_\___|
//                                                                |___/
//
//  QB64-PE Audio Engine powered by miniaudio (https://miniaud.io/)
//
//  Copyright (c) 2022 Samuel Gomes
//  https://github.com/a740g
//
//-----------------------------------------------------------------------------------------------------

// Enable Ogg Vorbis decoding
#define STB_VORBIS_HEADER_ONLY
#include "extras/stb_vorbis.c"
// The main miniaudio header
#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"
// The stb_vorbis implementation must come after the implementation of miniaudio
#undef STB_VORBIS_HEADER_ONLY
#include "extras/stb_vorbis.c"
#include "extras/vtables.h"

// Add custom backend (format) vtables here
// The order in the array defines the order of priority
// The vtables will be passed in to the resource manager config
static ma_decoding_backend_vtable *maCustomBackendVTables[] = {
    &ma_vtable_radv2,
    &ma_vtable_midi,
    &ma_vtable_modplay,
};

/// @brief This simply attaches the format decode VTables array to ma_resource_manager_config
/// @param maDecoderConfig Pointer to a miniaudio resource manager config object. This cannot be NULL
void AudioEngineAttachCustomBackendVTables(ma_resource_manager_config *maResourceManagerConfig) {
    // Attach the VTable
    maResourceManagerConfig->ppCustomDecodingBackendVTables = maCustomBackendVTables;
    maResourceManagerConfig->customDecodingBackendCount = ma_countof(maCustomBackendVTables);
}

/// @brief This simply attaches the format decode VTables array to ma_decoder_config
/// @param maDecoderConfig Pointer to a miniaudio decoder config object. This cannot be NULL
void AudioEngineAttachCustomBackendVTables(ma_decoder_config *maDecoderConfig) {
    // Attach the VTable
    maDecoderConfig->ppCustomBackendVTables = maCustomBackendVTables;
    maDecoderConfig->customBackendCount = ma_countof(maCustomBackendVTables);
}
