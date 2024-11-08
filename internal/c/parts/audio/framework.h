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

#define ZERO_VARIABLE(_v_) memset(&(_v_), 0, sizeof(_v_))
#define GET_ARRAY_SIZE(_x_) (sizeof(_x_) / sizeof(_x_[0]))
#define SAMPLE_FRAME_SIZE(_type_, _channels_) (sizeof(_type_) * (_channels_))
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

/// @brief A class that can manage a list of buffers using unique keys.
class BufferMap {
  private:
    /// @brief A buffer that is made up of std::vector of bytes and reference count.
    struct Buffer {
        std::vector<uint8_t> data;
        size_t refCount;

        Buffer(const void *src, size_t size) : data(size), refCount(1) { std::memcpy(data.data(), src, size); }
    };

    std::unordered_map<uint64_t, Buffer> buffers;

  public:
    // Delete assignment operators
    BufferMap &operator=(const BufferMap &) = delete;
    BufferMap &operator=(BufferMap &&) = delete;

    /// @brief Adds a buffer to the map using a unique key only if it was not added before. If the buffer is already present then it increases the reference
    /// count.
    /// @param data The raw data pointer. The data is copied.
    /// @param size The size of the data.
    /// @param key The unique key that should be used.
    /// @return True if successful.
    bool AddBuffer(const void *data, size_t size, uint64_t key) {
        if (data && size) {
            auto it = buffers.find(key);

            if (it == buffers.end()) {
                buffers.emplace(std::make_pair(key, Buffer(data, size)));

                AUDIO_DEBUG_PRINT("Added buffer of size %llu to map", size);
            } else {
                it->second.refCount++;

                AUDIO_DEBUG_PRINT("Increased reference count to %llu", it->second.refCount);
            }

            return true;
        }

        AUDIO_DEBUG_PRINT("Invalid buffer or size %p, %llu", data, size);

        return false;
    }

    /// @brief Increments the buffer reference count.
    /// @param key The unique key for the buffer.
    void AddRef(uint64_t key) {
        auto it = buffers.find(key);

        if (it != buffers.end()) {
            it->second.refCount++;

            AUDIO_DEBUG_PRINT("Increased reference count to %llu", it->second.refCount);
        } else {
            AUDIO_DEBUG_PRINT("Buffer not found");
        }
    }

    /// @brief Decrements the buffer reference count and frees the buffer if the reference count reaches zero.
    /// @param key The unique key for the buffer.
    void Release(uint64_t key) {
        auto it = buffers.find(key);

        if (it != buffers.end()) {
            it->second.refCount--;

            AUDIO_DEBUG_PRINT("Decreased reference count to %llu", it->second.refCount);

            if (it->second.refCount == 0) {
                AUDIO_DEBUG_PRINT("Erasing buffer of size %llu", it->second.data.size());

                buffers.erase(it);
            }
        } else {
            AUDIO_DEBUG_PRINT("Buffer not found");
        }
    }

    /// @brief Gets the raw pointer and size of the buffer with the given key.
    /// @param key The unique key for the buffer.
    /// @return An std::pair of the buffer raw pointer and size.
    std::pair<const void *, size_t> GetBuffer(uint64_t key) const {
        auto it = buffers.find(key);

        if (it != buffers.end()) {
            AUDIO_DEBUG_PRINT("Returning buffer of size %llu", it->second.data.size());

            return {it->second.data.data(), it->second.data.size()};
        }

        AUDIO_DEBUG_PRINT("Buffer not found");

        return {nullptr, 0};
    }
};

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

    void Reset() {
        blocks[0].clear();
        blocks[1].clear();
        index = 0;
        cursor = 0;
    }

    bool IsEmpty() const { return blocks[0].empty() && blocks[1].empty(); }

    float *Put(size_t frames) {
        auto writeIndex = 1 - index;

        if (blocks[writeIndex].empty()) {
            // Only resize and return a pointer if there is no data in the block
            blocks[writeIndex].resize(frames);
            return reinterpret_cast<float *>(blocks[writeIndex].data());
        }

        return nullptr; // block is not empty
    }

    size_t Get(float *data, size_t frames) {
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
