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

// Enable Ogg Vorbis decoding
#define STB_VORBIS_HEADER_ONLY
#include "extras/stb_vorbis.c"
// Due to the way miniaudio links to macOS frameworks at runtime, the application may not pass Apple's notarization process :(
// So, we will avoid runtime linking on macOS. See this discussion for more info: https://github.com/mackron/miniaudio/issues/203
#ifdef __APPLE__
#    define MA_NO_RUNTIME_LINKING
#endif
// The main miniaudio header
#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"
// The stb_vorbis implementation must come after the implementation of miniaudio
#undef STB_VORBIS_HEADER_ONLY
#include "extras/stb_vorbis.c"
#include "extras/vtables.h"

// Add custom backend (format) vtables here.
// The order in the array defines the order of priority.
// The vtables will be passed in to the resource manager config.
// ma_vtable_modplay should be the last one because libxmp supports 15-channel MODs
// which does not have any signatures and can lead to incorrect detection.
// clang-format off
static ma_decoding_backend_vtable *maCustomBackendVTables[] = {
    &ma_vtable_radv2,
    &ma_vtable_hively,
    &ma_vtable_midi,
    &ma_vtable_qoa,
    &ma_vtable_modplay,
};
// clang-format on

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
