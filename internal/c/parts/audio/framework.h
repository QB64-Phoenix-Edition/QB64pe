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
#include <climits>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <stack>
#include <unordered_map>
#include <utility>
#include <vector>

#define ZERO_VARIABLE(_v_) memset(&(_v_), 0, sizeof(_v_))
#define GET_ARRAY_SIZE(_x_) (sizeof(_x_) / sizeof(_x_[0]))
#define SAMPLE_FRAME_SIZE(_type_, _channels_) (sizeof(_type_) * (_channels_))

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

/// @brief A class that can manage a list of buffers using unique keys
class BufferMap {
  private:
    /// @brief A buffer that is made up of a raw pointer, size and reference count
    struct Buffer {
        void *data;
        size_t size;
        size_t refCount;
    };

    std::unordered_map<intptr_t, Buffer> buffers;

  public:
    // Delete assignment operators
    BufferMap &operator=(const BufferMap &) = delete;
    BufferMap &operator=(BufferMap &&) = delete;

    /// @brief This will simply free all buffers that were allocated
    ~BufferMap() {
        for (auto &it : buffers) {
            free(it.second.data);
            AUDIO_DEBUG_PRINT("Buffer freed of size %llu", it.second.size);
        }
    }

    /// @brief Adds a buffer to the map using a unique key only if it was not added before
    /// @param data The raw data pointer. The data is copied
    /// @param size The size of the data
    /// @param key The unique key that should be used
    /// @return True if successful
    bool AddBuffer(const void *data, size_t size, intptr_t key) {
        if (data && size && key && buffers.find(key) == buffers.end()) {
            Buffer buf = {};

            buf.data = malloc(size);
            if (!buf.data)
                return false;

            buf.size = size;
            buf.refCount = 1;
            memcpy(buf.data, data, size);
            buffers.emplace(key, std::move(buf));

            AUDIO_DEBUG_PRINT("Added buffer of size %llu to map", size);
            return true;
        }

        AUDIO_DEBUG_PRINT("Failed to add buffer of size %llu", size);
        return false;
    }

    /// @brief Increments the buffer reference count
    /// @param key The unique key for the buffer
    void AddRef(intptr_t key) {
        const auto it = buffers.find(key);
        if (it != buffers.end()) {
            auto &buf = it->second;
            buf.refCount += 1;
            AUDIO_DEBUG_PRINT("Increased reference count to %llu", buf.refCount);
        } else {
            AUDIO_DEBUG_PRINT("Buffer not found");
        }
    }

    /// @brief Decrements the buffer reference count and frees the buffer if the reference count reaches zero
    /// @param key The unique key for the buffer
    void Release(intptr_t key) {
        const auto it = buffers.find(key);
        if (it != buffers.end()) {
            auto &buf = it->second;
            buf.refCount -= 1;
            AUDIO_DEBUG_PRINT("Decreased reference count to %llu", buf.refCount);

            if (buf.refCount < 1) {
                free(buf.data);
                AUDIO_DEBUG_PRINT("Buffer freed of size %llu", buf.size);
                buffers.erase(key);
            }
        } else {
            AUDIO_DEBUG_PRINT("Buffer not found");
        }
    }

    /// @brief Gets the raw pointer and size of the buffer with the given key
    /// @param key The unique key for the buffer
    /// @return An std::pair of the buffer raw pointer and size
    std::pair<const void *, size_t> GetBuffer(intptr_t key) const {
        const auto it = buffers.find(key);
        if (it == buffers.end()) {
            AUDIO_DEBUG_PRINT("Buffer not found");
            return {nullptr, 0};
        }
        const auto &buf = it->second;
        AUDIO_DEBUG_PRINT("Returning buffer of size %llu", buf.size);
        return {buf.data, buf.size};
    }
};

/// @brief A class that can manage double buffer frame blocks
class DoubleBufferFrameBlock {
    std::vector<SampleFrame> blocks[2];
    size_t index = 0;  // current reading block index
    size_t cursor = 0; // cursor in the active block

  public:
    DoubleBufferFrameBlock(const DoubleBufferFrameBlock &) = delete;
    DoubleBufferFrameBlock(DoubleBufferFrameBlock &&) = delete;
    DoubleBufferFrameBlock &operator=(const DoubleBufferFrameBlock &) = delete;
    DoubleBufferFrameBlock &operator=(DoubleBufferFrameBlock &&) = delete;

    DoubleBufferFrameBlock() { Reset(); }

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
