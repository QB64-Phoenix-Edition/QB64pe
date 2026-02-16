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
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <functional>
#include <limits>
#include <stack>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#define SILENCE_SAMPLE_F32 0.0f
#define SILENCE_SAMPLE_I16 0
#define SILENCE_SAMPLE_I8 0x80u

#ifndef MA_DEFAULT_SAMPLE_RATE
// Since this is used by the extended decoder plugins, it does not matter even if miniaudio changes it the future
#    define MA_DEFAULT_SAMPLE_RATE 48000
#endif

/// @brief A simple FP32 stereo sample frame.
struct SampleFrameF32 {
    float l;
    float r;
};

/// @brief A simple 16-bit stereo sample frame.
struct SampleFrameI16 {
    int16_t l;
    int16_t r;
};

/// @brief A simple 8-bit stereo sample frame.
struct SampleFrameI8 {
    uint8_t l;
    uint8_t r;
};

/// @brief A class that can manage double buffer frame blocks for different sample formats. This is used when decoders cannot do variable frame sizes or when
/// the decoder's frame size is greater than miniaudio's frame size.
template <typename SampleFrame> class DoubleBufferFrameBlock {
    std::vector<SampleFrame> blocks[2];
    size_t index;  // current reading block index
    size_t cursor; // cursor in the active block

  public:
    DoubleBufferFrameBlock(const DoubleBufferFrameBlock &) = delete;
    DoubleBufferFrameBlock(DoubleBufferFrameBlock &&) = delete;
    DoubleBufferFrameBlock &operator=(const DoubleBufferFrameBlock &) = delete;
    DoubleBufferFrameBlock &operator=(DoubleBufferFrameBlock &&) = delete;

    DoubleBufferFrameBlock() : index(0), cursor(0) {}

    /// @brief Resets the double buffer frame blocks by clearing both blocks and resetting the index and cursor to their initial states.
    void Reset() {
        blocks[0].clear();
        blocks[1].clear();
        index = 0;
        cursor = 0;
    }

    /// @brief Checks if both blocks are empty.
    /// @returns true if both blocks are empty, false otherwise.
    bool IsEmpty() const {
        return blocks[0].empty() && blocks[1].empty();
    }

    /// @brief Checks if the write block is empty.
    /// @returns true if the write block is empty, false otherwise.
    bool IsWriteBlockEmpty() const {
        return blocks[1 - index].empty();
    }

    /// @brief Gets a pointer to the write block that can be written to.
    /// @param frames The number of frames to allocate in the write block.
    /// @returns A pointer to the write block if it is empty, otherwise nullptr.
    SampleFrame *GetWriteBlock(size_t frames) {
        auto writeIndex = 1 - index;

        if (blocks[writeIndex].empty()) {
            blocks[writeIndex].resize(frames);
            return blocks[writeIndex].data();
        }

        return nullptr;
    }

    /// @brief Copies up to `frames` number of frames from the current block to `data`. The cursor is advanced by the number of frames copied.
    /// @param data The destination buffer to copy to.
    /// @param frames The number of frames to copy.
    /// @return The number of frames copied.
    size_t ReadFrames(SampleFrame *data, size_t frames) {
        if (blocks[index].empty()) {
            index = 1 - index;
            cursor = 0;

            if (blocks[index].empty()) {
                return 0;
            }
        }

        auto toCopy = std::min(frames, blocks[index].size() - cursor);
        std::memcpy(data, blocks[index].data() + cursor, toCopy * sizeof(SampleFrame));
        cursor += toCopy;

        size_t remaining = 0;

        if (toCopy < frames) {
            blocks[index].clear();
            index = 1 - index;
            cursor = 0;

            if (blocks[index].empty()) {
                return toCopy;
            }

            remaining = std::min(frames - toCopy, blocks[index].size());
            std::memcpy(data + toCopy, blocks[index].data(), remaining * sizeof(SampleFrame));
            cursor += remaining;
        }

        if (cursor >= blocks[index].size()) {
            blocks[index].clear();
            index = 1 - index;
            cursor = 0;
        }

        return toCopy + remaining;
    }
};

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
