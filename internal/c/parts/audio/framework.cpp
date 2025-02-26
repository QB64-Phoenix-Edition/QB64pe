//----------------------------------------------------------------------------------------------------------------------
//    ___  ___   __ _ _  ___ ___     _          _ _       ___           _
//   / _ \| _ ) / /| | || _ \ __|   /_\ _  _ __| (_)___  | __|_ _  __ _(_)_ _  ___
//  | (_) | _ \/ _ \_  _|  _/ _|   / _ \ || / _` | / _ \ | _|| ' \/ _` | | ' \/ -_)
//   \__\_\___/\___/ |_||_| |___| /_/ \_\_,_\__,_|_\___/ |___|_||_\__, |_|_||_\___|
//                                                                |___/
//
//  QB64-PE Audio Engine powered by miniaudio (https://miniaud.io/)
//
//----------------------------------------------------------------------------------------------------------------------

#include "framework.h"

/// @brief The list of custom decoding backends. Add custom backend (format) vtables here. The order in the array defines the order of priority. The vtables
/// will be passed in to the resource manager config. ma_vtable_modplay should be the last one because libxmp supports 15-channel MODs which does not have any
/// signatures and can lead to incorrect detection.
// clang-format off
static ma_decoding_backend_vtable *maCustomBackendVTables[] = {
    &ma_vtable_radv2,
    &ma_vtable_hively,
    &ma_vtable_midi,
    &ma_vtable_qoa,
    &ma_vtable_modplay
};
// clang-format on

/// @brief This attaches the format decode VTables array to ma_resource_manager_config.
/// @param maDecoderConfig Pointer to a miniaudio resource manager config object. This cannot be NULL.
void AudioEngine_AttachCustomBackendVTables(ma_resource_manager_config *maResourceManagerConfig) {
    // Attach the VTable
    maResourceManagerConfig->ppCustomDecodingBackendVTables = maCustomBackendVTables;
    maResourceManagerConfig->customDecodingBackendCount = _countof(maCustomBackendVTables);
}
