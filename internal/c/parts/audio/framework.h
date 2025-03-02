//----------------------------------------------------------------------------------------------------------------------
//    ___  ___   __ _ _  ___ ___     _          _ _       ___           _
//   / _ \| _ ) / /| | || _ \ __|   /_\ _  _ __| (_)___  | __|_ _  __ _(_)_ _  ___
//  | (_) | _ \/ _ \_  _|  _/ _|   / _ \ || / _` | / _ \ | _|| ' \/ _` | | ' \/ -_)
//   \__\_\___/\___/ |_||_| |___| /_/ \_\_,_\__,_|_\___/ |___|_||_\__, |_|_||_\___|
//                                                                |___/
//
//  QB64-PE Audio Engine powered by miniaudio (https://miniaud.io/)
//
//  This file has things that is used across multiple parts of the audio library
//
//----------------------------------------------------------------------------------------------------------------------

#pragma once

#include "audio.h"
#include "extras/foo_midi/InstrumentBankManager.h"
#include "extras/stb/stb_vorbis.h"
#include "libqb-common.h"
#include "miniaudio/miniaudio.h"
#include <algorithm>
#include <array>
#include <atomic>
#include <climits>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <functional>
#include <stack>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#define SILENCE_SAMPLE 0.0f

#ifndef MA_DEFAULT_SAMPLE_RATE
// Since this is used by the extended decoder plugins, it does not matter even if miniaudio changes it the future
#    define MA_DEFAULT_SAMPLE_RATE 48000
#endif

// External VTables for our custom decoding backend.
extern ma_decoding_backend_vtable ma_vtable_radv2;
extern ma_decoding_backend_vtable ma_vtable_hively;
extern ma_decoding_backend_vtable ma_vtable_midi;
extern ma_decoding_backend_vtable ma_vtable_qoa;
extern ma_decoding_backend_vtable ma_vtable_modplay;

/// @brief A simple FP32 stereo sample frame
struct SampleFrame {
    float l;
    float r;
};

bool AudioDecoderBackend_Register(ma_decoding_backend_vtable *vtable);
void AudioDecoderBackend_Attach(ma_resource_manager_config *maResourceManagerConfig);

/// @brief Loads a file into memory. If the file cannot be opened or read, an empty container is returned.
/// @tparam Container The type of the container to load the file into.
/// @param fileName The name of the file to load.
/// @return A container of the same type as the template parameter, containing the contents of the file.
template <typename Container> Container AudioFile_Load(const char *fileName) {
    if (!fileName || !fileName[0]) {
        return {};
    }

    auto file = std::fopen(fileName, "rb");
    if (!file) {
        return {};
    }

    if (std::fseek(file, 0, SEEK_END)) {
        std::fclose(file);
        return {};
    }

    auto size = std::ftell(file);
    if (size < 0) {
        std::fclose(file);
        return {};
    }

    auto objectSize = sizeof(typename Container::value_type);
    auto objectCount = size / objectSize;

    Container buffer;
    buffer.resize(objectCount);

    std::rewind(file);

    if (std::fread(buffer.data(), objectSize, objectCount, file) != objectCount || std::ferror(file)) {
        std::fclose(file);
        return {};
    }

    std::fclose(file);

    return std::move(buffer);
}

/// @brief Saves a container's data to a file. If the file cannot be opened or written to, the function returns false.
/// @tparam Container The type of the container holding the data to be saved.
/// @param fileName The name of the file to save the data to.
/// @param data The container with data to be saved to the file.
/// @return True if the data was successfully written to the file, otherwise false.
template <typename Container> bool AudioFile_Save(const char *fileName, const Container &data) {
    if (!fileName || !fileName[0] || data.empty()) {
        return false;
    }

    auto file = std::fopen(fileName, "wb");
    if (!file) {
        return false;
    }

    auto objectSize = sizeof(typename Container::value_type);
    auto objectCount = data.size();

    if (std::fwrite(data.data(), objectSize, objectCount, file) != objectCount || std::ferror(file)) {
        std::fclose(file);
        return false;
    }

    std::fclose(file);
    return true;
}
