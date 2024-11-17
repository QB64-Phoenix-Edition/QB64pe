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

// Uncomment this to to print debug messages to stderr
// #define AUDIO_DEBUG 1

#include "audio.h"
#include "extras/foo_midi/InstrumentBankManager.h"
#include "miniaudio.h"
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

#ifndef _countof
#    ifdef __cplusplus
template <typename T, size_t N> static inline constexpr size_t _countof(T const (&)[N]) noexcept { return N; }
#    else
#        define _countof(Array_) (sizeof(Array_) / sizeof(Array_[0]))
#    endif
#endif

#define SILENCE_SAMPLE 0.0f

#ifndef MA_DEFAULT_SAMPLE_RATE
// Since this is used by the extended decoder plugins, it does not matter even if miniaudio changes it the future
#    define MA_DEFAULT_SAMPLE_RATE 48000
#endif

/// @brief A simple FP32 stereo sample frame
struct SampleFrame {
    float l;
    float r;
};

// VTables for our custom decoding backend
extern ma_decoding_backend_vtable ma_vtable_midi;
extern ma_decoding_backend_vtable ma_vtable_modplay;
extern ma_decoding_backend_vtable ma_vtable_radv2;
extern ma_decoding_backend_vtable ma_vtable_hively;
extern ma_decoding_backend_vtable ma_vtable_qoa;

// The global instrument bank manager
extern InstrumentBankManager g_InstrumentBankManager;

// These attaches our customer backend (format decoders) VTables to various miniaudio structs
void AudioEngineAttachCustomBackendVTables(ma_resource_manager_config *maResourceManagerConfig);
void AudioEngineAttachCustomBackendVTables(ma_decoder_config *maDecoderConfig);

/// @brief A class that can manage double buffer frame blocks
class DoubleBufferFrameBlock {
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
    bool IsEmpty() const { return blocks[0].empty() && blocks[1].empty(); }

    /// @brief Gets a float pointer to the write block that can be written to.
    /// @param[in] frames The number of frames to write.
    /// @return A pointer to the write block, or nullptr if the block is not empty.
    float *GetWriteBlock(size_t frames) {
        auto writeIndex = 1 - index;

        if (blocks[writeIndex].empty()) {
            // Only resize and return a pointer if there is no data in the block
            blocks[writeIndex].resize(frames);
            return reinterpret_cast<float *>(blocks[writeIndex].data());
        }

        return nullptr; // block is not empty
    }

    /// @brief Copies up to `frames` number of frames from the current block to `data`. The cursor is advanced by the number of frames copied.
    /// @param[in] data The destination buffer to copy to.
    /// @param[in] frames The number of frames to copy.
    /// @return The number of frames copied.
    size_t ReadFrames(float *data, size_t frames) {
        if (blocks[index].empty()) {
            // Switch to the other block
            index = 1 - index;
            cursor = 0;

            if (blocks[index].empty())
                return 0; // no data available
        }

        auto toCopy = std::min(frames, blocks[index].size() - cursor);                  // clip to whatever is left in the block
        std::memcpy(data, blocks[index].data() + cursor, toCopy * sizeof(SampleFrame)); // copy the data
        cursor += toCopy;                                                               // advance the cursor

        size_t remaining = 0; // we'll set this to zero in case we copy the exact number of frames requested

        if (toCopy < frames) {
            // Switch to the other block since we copied less than requested
            blocks[index].clear();
            index = 1 - index;
            cursor = 0;

            if (blocks[index].empty())
                return toCopy; // return the partial number of frames copied

            remaining = std::min(frames - toCopy, blocks[index].size());                              // clip to block size
            std::memcpy(data + (toCopy << 1), blocks[index].data(), remaining * sizeof(SampleFrame)); // copy the data
            cursor += remaining;                                                                      // advance the cursor
        }

        if (cursor >= blocks[index].size()) {
            // Switch to the other block if we've reached the end
            blocks[index].clear();
            index = 1 - index;
            cursor = 0;
        }

        return toCopy + remaining; // return the number of frames copied
    }
};
