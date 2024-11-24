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

#define _USE_MATH_DEFINES

#include "../../libqb.h"
#include "cmem.h"
#include "datetime.h"
#include "error_handle.h"
#include "filepath.h"
#include "filesystem.h"
#include "framework.h"
#include "libqb-common.h"
#include "mem.h"
#include "miniaudio.h"
#include "mutex.h"
#include "qbs.h"

/// @brief The top-level class that implements the QB64-PE audio engine.
struct AudioEngine {
    /// @brief A class that can manage a list of buffers using unique keys.
    class BufferMap {
      private:
        /// @brief A buffer that is made up of std::vector of bytes and reference count.
        struct ManagedBuffer {
            std::vector<uint8_t> data;
            size_t refCount;

            ManagedBuffer() : refCount(0) {}

            ManagedBuffer(const void *src, size_t size) : data(size), refCount(1) {
                std::memcpy(data.data(), src, size);
            }

            ManagedBuffer(std::vector<uint8_t> &&src) : data(std::move(src)), refCount(1) {}
        };

        std::unordered_map<uint64_t, ManagedBuffer> buffers;

      public:
        // Delete copy constructor and assignment operators
        BufferMap(const BufferMap &) = delete;
        BufferMap &operator=(const BufferMap &) = delete;
        // Delete move constructor and assignment operators
        BufferMap(BufferMap &&) = delete;
        BufferMap &operator=(BufferMap &&) = delete;

        BufferMap() = default;

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
                    buffers.emplace(key, ManagedBuffer(data, size));

                    AUDIO_DEBUG_PRINT("Copied buffer of size %zu to map, key: %" PRIu64, size, key);
                } else {
                    it->second.refCount++;

                    AUDIO_DEBUG_PRINT("Increased reference count to %zu, key: %" PRIu64, it->second.refCount, key);
                }

                return true;
            }

            AUDIO_DEBUG_PRINT("Invalid buffer or size %p, %zu", data, size);

            return false;
        }

        /// @brief Adds a buffer to the map using a unique key only if it was not added before. If the buffer is already present then it increases the reference
        /// count.
        /// @param buffer The buffer data. The data is moved.
        /// @param key The unique key that should be used.
        /// @return True if successful.
        bool AddBuffer(std::vector<uint8_t> &&buffer, uint64_t key) {
            if (!buffer.empty()) {
                auto it = buffers.find(key);

                if (it == buffers.end()) {
                    buffers.emplace(key, ManagedBuffer(std::move(buffer)));

                    AUDIO_DEBUG_PRINT("Moved buffer of size %zu to map, key: %" PRIu64, buffers[key].data.size(), key);
                } else {
                    it->second.refCount++;

                    AUDIO_DEBUG_PRINT("Increased reference count to %zu, key: %" PRIu64, it->second.refCount, key);
                }

                return true;
            }

            AUDIO_DEBUG_PRINT("Invalid buffer size %zu", buffer.size());

            return false;
        }

        /// @brief Decrements the buffer reference count and frees the buffer if the reference count reaches zero.
        /// @param key The unique key for the buffer.
        void ReleaseBuffer(uint64_t key) {
            auto it = buffers.find(key);

            if (it != buffers.end()) {
                it->second.refCount--;

                AUDIO_DEBUG_PRINT("Decreased reference count to %zu, key: %" PRIu64, it->second.refCount, key);

                if (it->second.refCount == 0) {
                    AUDIO_DEBUG_PRINT("Erasing buffer of size %zu from map, key: %" PRIu64, it->second.data.size(), key);

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
                AUDIO_DEBUG_PRINT("Returning buffer of size %zu, key: %" PRIu64, it->second.data.size(), key);

                return {it->second.data.data(), it->second.data.size()};
            }

            AUDIO_DEBUG_PRINT("Buffer not found");

            return {nullptr, 0};
        }
    };

    /// @brief miniaudio virtual file system class
    struct VFS {
        ma_vfs_callbacks cb; // has to be first entry
        ma_allocation_callbacks allocCb;

        uint64_t nextFd;

        struct FileState {
            uint64_t key;        // for memory buffers
            ma_int64 offset;     // for memory buffers
            FILE *realFile;      // for real files
            size_t realFileSize; // for real files

            FileState() : key(0), offset(0), realFile(nullptr), realFileSize(0) {}
        };

        std::unordered_map<uint64_t, FileState> fileMap;

        BufferMap *bufferMap;

        /// @brief Constructs a new VFS object with empty callbacks and no associated buffer map.
        VFS() : nextFd(0), bufferMap(nullptr) {
            cb = {};
            allocCb = {};
        }

        /// @brief Creates a new virtual file system (VFS) object with callbacks and associates it with the given buffer map.
        /// @param bufferMap Pointer to the BufferMap to be used with the VFS.
        /// @return Returns a pointer to the created ma_vfs object.
        static ma_vfs *Create(BufferMap *bufferMap) {
            auto vfs = new VFS();
            vfs->bufferMap = bufferMap;
            vfs->cb.onOpen = OnOpen;
            vfs->cb.onClose = OnClose;
            vfs->cb.onRead = OnRead;
            vfs->cb.onSeek = OnSeek;
            vfs->cb.onTell = OnTell;
            vfs->cb.onInfo = OnInfo;

            AUDIO_DEBUG_PRINT("VFS created");

            return reinterpret_cast<ma_vfs *>(vfs);
        }

        /// @brief Frees the memory of a virtual file system object.
        /// @param vfs Pointer to the virtual file system object.
        static void Destroy(ma_vfs *vfs) {
            delete reinterpret_cast<VFS *>(vfs);

            AUDIO_DEBUG_PRINT("VFS destroyed");
        }

        /// @brief Opens a file in the virtual file system by looking up a buffer using a key.
        /// @param pVFS Pointer to the virtual file system.
        /// @param pFilePath String representing the file path, interpreted as a key for buffer lookup.
        /// @param openMode Mode in which to open the file (unused).
        /// @param pFile Pointer to where the file handle will be stored.
        /// @return Returns MA_SUCCESS upon successful opening, or MA_DOES_NOT_EXIST if the buffer is not found.
        static ma_result OnOpen(ma_vfs *pVFS, const char *pFilePath, ma_uint32 openMode, ma_vfs_file *pFile) {
            (void)openMode;

            auto vfs = reinterpret_cast<VFS *>(pVFS);
            auto key = std::strtoull(pFilePath, NULL, 10); // transform pFilePath, look-up in BufferMap

            if (vfs->bufferMap->GetBuffer(key).first) {
                vfs->nextFd++;
                vfs->fileMap[vfs->nextFd].key = key;
                vfs->fileMap[vfs->nextFd].offset = 0;
                vfs->fileMap[vfs->nextFd].realFile = nullptr;
                vfs->fileMap[vfs->nextFd].realFileSize = 0;

                AUDIO_DEBUG_PRINT("Using memory buffer (%llu) %llu", key, vfs->nextFd);
            } else {
                auto file = std::fopen(pFilePath, "rb");
                if (!file) {
                    AUDIO_DEBUG_PRINT("File / memory buffer not found: %s", pFilePath);

                    return MA_DOES_NOT_EXIST;
                }

                if (std::fseek(file, 0, SEEK_END)) {
                    AUDIO_DEBUG_PRINT("Failed to seek to the end of file %s", pFilePath);

                    std::fclose(file);

                    return MA_BAD_SEEK;
                }

                auto fileSize = std::ftell(file);
                if (fileSize < 0) {
                    AUDIO_DEBUG_PRINT("Failed to get the size of file %s", pFilePath);

                    std::fclose(file);

                    return MA_IO_ERROR;
                }

                std::rewind(file);

                vfs->nextFd++;
                vfs->fileMap[vfs->nextFd].key = 0;
                vfs->fileMap[vfs->nextFd].offset = 0;
                vfs->fileMap[vfs->nextFd].realFile = file;
                vfs->fileMap[vfs->nextFd].realFileSize = fileSize;

                AUDIO_DEBUG_PRINT("Opened file (%s) %llu", pFilePath, vfs->nextFd);
            }

            *pFile = reinterpret_cast<ma_vfs_file>(vfs->nextFd);

            return MA_SUCCESS;
        }

        /// @brief Closes a file in the virtual file system.
        /// @param pVFS Pointer to the virtual file system.
        /// @param file Handle to the file within the virtual file system.
        /// @return Returns MA_SUCCESS upon successful closing of the file.
        static ma_result OnClose(ma_vfs *pVFS, ma_vfs_file file) {
            AUDIO_DEBUG_CHECK(pVFS != nullptr);
            AUDIO_DEBUG_CHECK(file != nullptr);

            auto vfs = reinterpret_cast<VFS *>(pVFS);
            auto fd = uint64_t(file);
            auto state = &vfs->fileMap[fd];

            if (state->realFile) {
                std::fclose(state->realFile);

                AUDIO_DEBUG_PRINT("Closed file %llu", fd);
            }

            vfs->fileMap.erase(fd);

            return MA_SUCCESS;
        }

        /// @brief Reads from a file in the virtual file system.
        /// @param pVFS Pointer to the virtual file system.
        /// @param file Handle to the file within the virtual file system.
        /// @param pDst Pointer to the destination buffer.
        /// @param sizeInBytes Requested size of the read.
        /// @param pBytesRead Pointer to a variable where the actual read size is stored.
        /// @return Returns MA_SUCCESS upon successful read.
        static ma_result OnRead(ma_vfs *pVFS, ma_vfs_file file, void *pDst, size_t sizeInBytes, size_t *pBytesRead) {
            AUDIO_DEBUG_CHECK(pVFS != nullptr);
            AUDIO_DEBUG_CHECK(file != nullptr);
            AUDIO_DEBUG_CHECK(pDst != nullptr);
            AUDIO_DEBUG_CHECK(pBytesRead != nullptr);

            auto vfs = reinterpret_cast<VFS *>(pVFS);
            auto fd = uint64_t(file);
            auto state = &vfs->fileMap[fd];

            if (state->realFile) {
                *pBytesRead = std::fread(pDst, sizeof(uint8_t), sizeInBytes, state->realFile);

                if (std::ferror(state->realFile)) {
                    AUDIO_DEBUG_PRINT("Error reading file, fd: %llu", fd);
                    return MA_IO_ERROR;
                }

                AUDIO_DEBUG_PRINT("Read %zu bytes from file %llu", *pBytesRead, fd);
            } else {
                auto [buffer, bufferSize] = vfs->bufferMap->GetBuffer(state->key);
                auto readSize = std::min<ma_uint64>(sizeInBytes, bufferSize - state->offset);

                std::memcpy(pDst, reinterpret_cast<const uint8_t *>(buffer) + state->offset, readSize);
                *pBytesRead = readSize;
                state->offset += readSize;

                AUDIO_DEBUG_PRINT("Read %zu bytes at offset %lld from memory buffer %llu", readSize, state->offset, fd);
            }

            return MA_SUCCESS;
        }

        /// @brief Seeks to a specified position within a file.
        /// @param pVFS Pointer to the virtual file system.
        /// @param file Handle to the file within the virtual file system.
        /// @param offset Offset from the origin to seek to.
        /// @param origin Origin of the seek, either start, current, or end of file.
        /// @return Returns MA_SUCCESS upon successful seeking.
        static ma_result OnSeek(ma_vfs *pVFS, ma_vfs_file file, ma_int64 offset, ma_seek_origin origin) {
            AUDIO_DEBUG_CHECK(pVFS != nullptr);
            AUDIO_DEBUG_CHECK(file != nullptr);
            AUDIO_DEBUG_CHECK(origin == ma_seek_origin::ma_seek_origin_start || origin == ma_seek_origin::ma_seek_origin_current ||
                              origin == ma_seek_origin::ma_seek_origin_end);

            auto vfs = reinterpret_cast<VFS *>(pVFS);
            auto fd = uint64_t(file);
            auto state = &vfs->fileMap[fd];

            if (state->realFile) {
                int whence;
                switch (origin) {
                case ma_seek_origin::ma_seek_origin_start:
                    whence = SEEK_SET;
                    break;

                case ma_seek_origin::ma_seek_origin_current:
                    whence = SEEK_CUR;
                    break;

                case ma_seek_origin::ma_seek_origin_end:
                    whence = SEEK_END;
                    break;

                default:
                    AUDIO_DEBUG_PRINT("Unknown seek origin: %d", origin);
                    return MA_BAD_SEEK;
                }

                if (std::fseek(state->realFile, offset, whence) == 0) {
                    AUDIO_DEBUG_PRINT("Seek file %llu to offset %lld", fd, offset);

                    return MA_SUCCESS;
                }

                AUDIO_DEBUG_PRINT("Failed to seek file %llu to offset %lld", fd, offset);
                return MA_BAD_SEEK;
            } else {
                auto bufferSize = vfs->bufferMap->GetBuffer(state->key).second;

                switch (origin) {
                case ma_seek_origin::ma_seek_origin_start:
                    state->offset = offset;
                    break;

                case ma_seek_origin::ma_seek_origin_current:
                    state->offset = state->offset + offset;
                    break;

                case ma_seek_origin::ma_seek_origin_end:
                    state->offset = bufferSize + offset;
                    break;

                default:
                    AUDIO_DEBUG_PRINT("Unknown seek origin: %d", origin);
                    return MA_BAD_SEEK;
                }

                state->offset = std::clamp<ma_int64>(state->offset, 0, bufferSize);

                AUDIO_DEBUG_PRINT("Seek memory buffer %llu to offset %lld, kind: %d, result: %lld", fd, offset, origin, state->offset);
            }

            return MA_SUCCESS;
        }

        /// @brief Tells the current cursor position of a file in the virtual file system.
        /// @param pVFS Pointer to the virtual file system.
        /// @param file Handle to the file within the virtual file system.
        /// @param pCursor Pointer to a 64-bit signed integer where the current cursor position is stored.
        /// @return Returns MA_SUCCESS upon successful retrieval of the cursor position.
        static ma_result OnTell(ma_vfs *pVFS, ma_vfs_file file, ma_int64 *pCursor) {
            AUDIO_DEBUG_CHECK(pVFS != nullptr);
            AUDIO_DEBUG_CHECK(file != nullptr);
            AUDIO_DEBUG_CHECK(pCursor != nullptr);

            auto vfs = reinterpret_cast<VFS *>(pVFS);
            auto fd = uint64_t(file);
            auto state = &vfs->fileMap[fd];

            if (state->realFile) {
                *pCursor = std::ftell(state->realFile);

                AUDIO_DEBUG_PRINT("File %llu position: %lld", fd, *pCursor);
            } else {
                *pCursor = state->offset;
                AUDIO_DEBUG_PRINT("Memory buffer %llu position: %lld", fd, state->offset);
            }

            return MA_SUCCESS;
        }

        /// @brief Retrieves information about a file, specifically its size in bytes.
        /// @param pVFS Pointer to the virtual file system.
        /// @param file Handle to the file within the virtual file system.
        /// @param pInfo Pointer to a structure where file information will be stored, particularly the file size.
        /// @return Returns MA_SUCCESS upon successful retrieval of file information.
        static ma_result OnInfo(ma_vfs *pVFS, ma_vfs_file file, ma_file_info *pInfo) {
            AUDIO_DEBUG_CHECK(pVFS != nullptr);
            AUDIO_DEBUG_CHECK(file != nullptr);
            AUDIO_DEBUG_CHECK(pInfo != nullptr);

            auto vfs = reinterpret_cast<VFS *>(pVFS);
            auto fd = uint64_t(file);
            auto state = &vfs->fileMap[fd];

            if (state->realFile) {
                pInfo->sizeInBytes = state->realFileSize;

                AUDIO_DEBUG_PRINT("File %llu size: %zu", fd, state->realFileSize);
            } else {
                auto bufferSize = vfs->bufferMap->GetBuffer(state->key).second;

                pInfo->sizeInBytes = bufferSize;

                AUDIO_DEBUG_PRINT("Memory buffer %llu size: %zu", fd, bufferSize);
            }

            return MA_SUCCESS;
        }
    };

    /// @brief A miniaudio raw audio stream datasource.
    struct RawStream {
        ma_data_source_base maDataSource;         // miniaudio data source (this must be the first member of our struct)
        ma_data_source_config maDataSourceConfig; // config struct for the data source
        ma_engine *maEngine;                      // pointer to a ma_engine object that was passed while creating the data source
        ma_sound *maSound;                        // pointer to a ma_sound object that was passed while creating the data source

        struct Buffer {                    // we'll give this a name that we'll use below
            std::vector<SampleFrame> data; // this holds the actual sample frames
            size_t cursor;                 // the read cursor (in frames) in the stream
        } buffer[2];                       // we need two of these to do a proper ping-pong

        Buffer *consumer;        // this is what the miniaudio thread will use to pull data from
        Buffer *producer;        // this is what the main thread will use to push data to
        libqb_mutex *m;          // we'll use a mutex to give exclusive access to resources used by both threads
        bool stop;               // set this to true to stop supply of samples completely (including silent samples)
        std::atomic_bool pause_; // set this to true to pause the stream (only silence samples will be sent to miniaudio)

        // Delete default, copy and move constructors and assignments.
        RawStream() = delete;
        RawStream(const RawStream &) = delete;
        RawStream &operator=(const RawStream &) = delete;
        RawStream &operator=(RawStream &&) = delete;
        RawStream(RawStream &&) = delete;

        /// @brief Sets up the vectors, mutex and set some defaults.
        RawStream(ma_engine *pmaEngine, ma_sound *pmaSound) {
            maSound = pmaSound;                      // Save the pointer to the ma_sound object (this is basically from a QB64-PE sound handle)
            maEngine = pmaEngine;                    // Save the pointer to the ma_engine object (this should come from the QB64-PE sound engine)
            buffer[0].cursor = buffer[1].cursor = 0; // reset the cursors
            consumer = &buffer[0];                   // set default consumer
            producer = &buffer[1];                   // set default producer
            stop = false;                            // we will send silent samples to keep the playback going by default
            Pause(false);                            // the steam will not be paused by default
            m = libqb_mutex_new();
        }

        /// @brief We use this to destroy the mutex.
        ~RawStream() {
            libqb_mutex_free(m);
        }

        /// @brief Pauses or resumes the stream.
        /// @param state true to pause, false to resume.
        void Pause(bool state) {
            pause_.store(state, std::memory_order_relaxed);
        }

        /// @brief Pushes a sample frame at the end of the queue. This is mutex protected and called by the main thread.
        /// @param l Sample frame left channel data.
        /// @param r Sample frame right channel data.
        void PushSampleFrame(float l, float r) {
            libqb_mutex_guard lock(m); // lock the mutex before accessing the vectors

            producer->data.push_back({l, r}); // push the sample frame to the back of the producer queue
        }

        /// @brief Pushes a whole buffer of stereo sample frames to the queue. This is mutex protected and called by the main thread.
        /// @param buffer The buffer containing the stereo sample frames. This cannot be NULL.
        /// @param frames The total number of frames in the buffer.
        void PushSampleFrames(SampleFrame *buffer, ma_uint64 frames) {
            libqb_mutex_guard lock(m); // lock the mutex before accessing the vectors

            std::copy(buffer, buffer + frames, std::back_inserter(producer->data));
        }

        /// @brief Pushes a whole buffer of mono sample frames to the queue. This is mutex protected and called by the main thread.
        /// @param buffer The buffer containing the sample frames. This cannot be NULL.
        /// @param frames The total number of frames in the buffer.
        /// @param gainLeft Left channel gain value (0.0 to 1.0).
        /// @param gainRight Right channel gain value (0.0 to 1.0).
        void PushSampleFrames(float *buffer, ma_uint64 frames, float gainLeft, float gainRight) {
            libqb_mutex_guard lock(m); // lock the mutex before accessing the vectors

            for (ma_uint64 i = 0; i < frames; i++) {
                producer->data.push_back({buffer[i] * gainLeft, buffer[i] * gainRight});
            }
        }

        /// @brief Pushes a whole buffer of mono sample frames to the queue (no FP panning math). This is mutex protected and called by the main thread.
        /// @param buffer The buffer containing the sample frames. This cannot be NULL.
        /// @param frames The total number of frames in the buffer.
        void PushSampleFrames(float *buffer, ma_uint64 frames) {
            libqb_mutex_guard lock(m); // lock the mutex before accessing the vectors

            for (ma_uint64 i = 0; i < frames; i++) {
                producer->data.push_back({buffer[i], buffer[i]});
            }
        }

        /// @brief Returns the length, in sample frames of sound queued.
        /// @return The length left to play in sample frames.
        ma_uint64 GetSampleFramesRemaining() {
            libqb_mutex_guard lock(m); // lock the mutex before accessing the vectors

            return (consumer->data.size() - consumer->cursor) + (producer->data.size() - producer->cursor); // sum of producer and consumer sample frames
        }

        /// @brief Returns the length, in seconds of sound queued.
        /// @return The length left to play in seconds.
        double GetTimeRemaining() {
            return double(GetSampleFramesRemaining()) / double(ma_engine_get_sample_rate(maEngine));
        }

        /// @brief Callback function used by miniaudio to pull a chunk of raw sample frames to play. The samples being read is removed from the queue.
        /// @param pDataSource Pointer to the raw stream data source (cast to RawStream type).
        /// @param pFramesOut The sample frames sent to miniaudio.
        /// @param frameCount The sample frame count requested by miniaudio.
        /// @param pFramesRead The sample frame count that was actually sent (this must not exceed frameCount).
        /// @return MA_SUCCESS on success or an appropriate MA_FAILED_* value on failure.
        static ma_result OnRead(ma_data_source *pDataSource, void *pFramesOut, ma_uint64 frameCount, ma_uint64 *pFramesRead) {
            if (pFramesRead) {
                *pFramesRead = 0;
            }

            if (frameCount == 0) {
                return MA_INVALID_ARGS;
            }

            if (!pDataSource) {
                return MA_INVALID_ARGS;
            }

            auto pRawStream = (RawStream *)pDataSource; // cast to RawStream instance pointer
            auto result = MA_SUCCESS;                   // must be initialized to MA_SUCCESS
            auto maBuffer = (SampleFrame *)pFramesOut;  // cast to sample frame pointer

            ma_uint64 sampleFramesRead;

            if (pRawStream->pause_.load(std::memory_order_relaxed)) {
                // Just play silence if we are paused
                std::fill(maBuffer, maBuffer + frameCount, SampleFrame{SILENCE_SAMPLE, SILENCE_SAMPLE});
                sampleFramesRead = frameCount;
            } else {
                sampleFramesRead = pRawStream->consumer->data.size() - pRawStream->consumer->cursor; // total amount of samples we need to send to miniaudio

                // Swap buffers if we do not have anything left to play
                if (!sampleFramesRead) {
                    pRawStream->consumer->cursor = 0;                                                    // reset the cursor
                    pRawStream->consumer->data.clear();                                                  // clear the consumer vector
                    std::swap(pRawStream->consumer, pRawStream->producer);                               // quickly swap the Buffer pointers
                    sampleFramesRead = pRawStream->consumer->data.size() - pRawStream->consumer->cursor; // get the total number of samples again
                }

                sampleFramesRead = std::min(sampleFramesRead, frameCount); // we'll always send lower of what miniaudio wants or what we have

                if (sampleFramesRead) {
                    // Now send the samples to miniaudio
                    std::copy(pRawStream->consumer->data.data() + pRawStream->consumer->cursor,
                              pRawStream->consumer->data.data() + pRawStream->consumer->cursor + sampleFramesRead, maBuffer);

                    pRawStream->consumer->cursor += sampleFramesRead; // increment the read cursor
                } else {
                    if (pRawStream->stop) {
                        // End of stream was signalled
                        result = MA_AT_END;
                    } else {
                        // To keep the stream going, play silence if there are no frames to play
                        std::fill(maBuffer, maBuffer + frameCount, SampleFrame{SILENCE_SAMPLE, SILENCE_SAMPLE});
                        sampleFramesRead = frameCount;
                    }
                }
            }

            if (pFramesRead) {
                *pFramesRead = sampleFramesRead;
            }

            return result;
        }

        /// @brief A dummy callback function which just tells miniaudio that it succeeded.
        /// @param pDataSource Pointer to the raw stream data source (cast to RawStream type).
        /// @param frameIndex The frame index to seek to (unused).
        /// @return Always MA_SUCCESS.
        static ma_result OnSeek(ma_data_source *pDataSource, ma_uint64 frameIndex) {
            // NOP. Just pretend to be successful.
            (void)pDataSource;
            (void)frameIndex;

            return MA_SUCCESS;
        }

        /// @brief Returns the audio format to miniaudio.
        /// @param pDataSource Pointer to the raw stream data source (cast to RawStream type).
        /// @param pFormat The ma_format to use (we always return ma_format_f32 because that is what QB64 uses).
        /// @param pChannels The number of audio channels (always 2 - stereo).
        /// @param pSampleRate The sample rate of the stream (we always return the engine sample rate).
        /// @param pChannelMap Sent to ma_channel_map_init_standard.
        /// @param channelMapCap Sent to ma_channel_map_init_standard.
        /// @return Always MA_SUCCESS.
        static ma_result OnGetDataFormat(ma_data_source *pDataSource, ma_format *pFormat, ma_uint32 *pChannels, ma_uint32 *pSampleRate, ma_channel *pChannelMap,
                                         size_t channelMapCap) {
            auto pRawStream = (RawStream *)pDataSource;

            if (pFormat)
                *pFormat = ma_format::ma_format_f32; // QB64 SndRaw API uses FP32 samples

            if (pChannels)
                *pChannels = 2; // stereo

            if (pSampleRate)
                *pSampleRate = ma_engine_get_sample_rate(pRawStream->maEngine); // we'll play at the audio engine sampling rate

            if (pChannelMap)
                ma_channel_map_init_standard(ma_standard_channel_map_default, pChannelMap, channelMapCap, 2); // stereo

            return MA_SUCCESS;
        }

        /// @brief Raw stream data source vtable
        static const ma_data_source_vtable rawStreamDataSourceVtable;

        /// @brief Creates, initializes and sets up a raw stream for playback.
        /// @param pmaEngine A pointer to a QB64-PE sound engine object. This cannot be NULL.
        /// @param pmaSound A pointer to an ma_sound object from a QB64-PE sound handle. This cannot be NULL.
        /// @return Returns a pointer to a data source if successful, NULL otherwise.
        static RawStream *Create(ma_engine *pmaEngine, ma_sound *pmaSound) {
            if (!pmaEngine || !pmaSound) { // these should not be NULL
                AUDIO_DEBUG_PRINT("Invalid arguments");

                return nullptr;
            }

            auto pRawStream = new RawStream(pmaEngine, pmaSound); // create the data source object
            if (!pRawStream) {
                AUDIO_DEBUG_PRINT("Failed to create data source");

                return nullptr;
            }

            pRawStream->maDataSource = {};
            pRawStream->maDataSourceConfig = ma_data_source_config_init();
            pRawStream->maDataSourceConfig.vtable = &rawStreamDataSourceVtable; // attach the vtable to the data source

            auto result = ma_data_source_init(&pRawStream->maDataSourceConfig, &pRawStream->maDataSource);
            if (result != MA_SUCCESS) {
                AUDIO_DEBUG_PRINT("Error %i: failed to initialize data source", result);

                delete pRawStream;

                return nullptr;
            }

            result = ma_sound_init_from_data_source(pmaEngine, &pRawStream->maDataSource, MA_SOUND_FLAG_NO_PITCH | MA_SOUND_FLAG_NO_SPATIALIZATION, NULL,
                                                    pmaSound); // attach data source to the ma_sound
            if (result != MA_SUCCESS) {
                AUDIO_DEBUG_PRINT("Error %i: failed to initialize sound from data source", result);

                delete pRawStream;

                return nullptr;
            }

            result = ma_sound_start(pmaSound); // play the ma_sound
            if (result != MA_SUCCESS) {
                AUDIO_DEBUG_PRINT("Error %i: failed to start sound playback", result);

                ma_sound_uninit(pmaSound); // delete the ma_sound object

                delete pRawStream;

                return nullptr;
            }

            AUDIO_DEBUG_PRINT("Raw sound stream created");

            return pRawStream;
        }

        /// @brief Stops and then frees a raw stream data source previously created with RawStreamCreate().
        /// @param pRawStream Pointer to the data source object.
        static void Destroy(RawStream *pRawStream) {
            if (pRawStream) {
                auto result = ma_sound_stop(pRawStream->maSound); // stop playback
                AUDIO_DEBUG_CHECK(result == MA_SUCCESS);

                ma_sound_uninit(pRawStream->maSound); // delete the ma_sound object

                delete pRawStream; // delete the raw stream object

                AUDIO_DEBUG_PRINT("Raw sound stream destroyed");
            }
        }
    };

    /// @brief A PSG class that handles all kinds of sound generation for SOUND and PLAY.
    class PSG {
      public:
        /// @brief Various types of waveform that can be generated.
        enum class WaveformType { NONE, SQUARE, SAWTOOTH, TRIANGLE, SINE, NOISE_WHITE, NOISE_PINK, NOISE_BROWNIAN, NOISE_LFSR, PULSE, CUSTOM, COUNT };

        static constexpr auto PAN_LEFT = -1.0f;                  // left-most pan position
        static constexpr auto PAN_RIGHT = 1.0f;                  // right-most pan position
        static constexpr auto PAN_CENTER = PAN_LEFT + PAN_RIGHT; // center pan position
        static constexpr auto VOLUME_MIN = 0.0f;                 // minimum volume
        static constexpr auto VOLUME_MAX = 1.0f;                 // maximum volume
        static constexpr auto FREQUENCY_MIN = 20.0f;             // minimum frequency
        static constexpr auto FREQUENCY_MAX = 32767.0f;          // maximum frequency
        static constexpr auto FREQUENCY_LIMIT = 20000.0f;        // anything above this will generate silence
        static constexpr auto PULSE_WAVE_DUTY_CYCLE_MIN = 0.0f;  // minimum pulse wave duty cycle
        static constexpr auto PULSE_WAVE_DUTY_CYCLE_MAX = 1.0f;  // maximum pulse wave duty cycle
        static const auto CUSTOM_WAVEFORM_FRAMES_MIN = 2;        // minimum custom waveform frames

      private:
        // These are some constants that can be tweaked to change the behavior of the PSG and MML parser.
        // These mostly conform to the QBasic and QB64 spec.
        static const auto WAVEFORM_TYPE_DEFAULT = WaveformType::SQUARE;                                // the PC speaker generates a square wave
        static const auto FREQUENCY_DEFAULT = 440;                                                     // the default frequency in Hz
        static const auto MML_VOLUME_MIN = 0;                                                          // minimum volume (percentage)
        static const auto MML_VOLUME_MAX = 100;                                                        // maximum volume (percentage)
        static const auto MML_PAN_LEFT = 0;                                                            // left-most pan position
        static const auto MML_PAN_RIGHT = 100;                                                         // right-most pan position
        static const auto MML_PAN_CENTER = MML_PAN_RIGHT / 2;                                          // center pan position
        static constexpr auto VOLUME_DEFAULT = (float(MML_VOLUME_MAX) / 2.0f) / float(MML_VOLUME_MAX); // default volume (FP32)
        static const auto MML_TEMPO_MIN = 32;                                                          // the minimum MML tempo allowed
        static const auto MML_TEMPO_MAX = 255;                                                         // the maximum MML tempo allowed
        static const auto MML_TEMPO_DEFAULT = 120;                                                     // the default MML tempo
        static const auto MML_OCTAVE_MIN = 0;                                                          // the minimum MML octave allowed
        static const auto MML_OCTAVE_MAX = 7;                                                          // the maximum MML octave allowed
        static const auto MML_OCTAVE_DEFAULT = 4;                                                      // the default MML octave
        static const auto MML_LENGTH_MIN = 1;                                                          // the minimum MML note length allowed
        static const auto MML_LENGTH_MAX = 64;                                                         // the maximum MML note length allowed
        static constexpr auto MML_LENGTH_DEFAULT = 4.0;                                                // the default MML note length
        static constexpr auto MML_PAUSE_DEFAULT = 1.0 / 8.0;                                           // the default MML pause length
        static const auto MML_VOLUME_RAMP_MIN = 0;                                                     // the minimum MML volume ramp (percentage)
        static const auto MML_VOLUME_RAMP_MAX = 100;                                                   // the maximum MML volume ramp (percentage)
        static const auto MML_WAVE_PARAM_MIN = 0;                                                      // the minimum MML wave parameter
        static const auto MML_WAVE_PARAM_MAX = 100;                                                    // the maximum MML wave parameter
        static const auto MML_RELEASE_MIN = 0;                                                         // the minimum MML release (percentage)
        static const auto MML_RELEASE_MAX = 100;                                                       // the maximum MML release (percentage)
        static const auto MML_SUSTAIN_MIN = 0;                                                         // the minimum MML sustain (percentage)
        static const auto MML_SUSTAIN_MAX = 100;                                                       // the maximum MML sustain (percentage)
        static const auto MML_DECAY_MIN = 0;                                                           // the minimum MML decay (percentage)
        static const auto MML_DECAY_MAX = 100;                                                         // the maximum MML decay (percentage)
        static const auto MML_ATTACK_MIN = 0;                                                          // the minimum MML attack (percentage)
        static const auto MML_ATTACK_MAX = 100;                                                        // the maximum MML attack (percentage)
        static constexpr auto PULSE_WAVE_DUTY_CYCLE_DEFAULT = 0.5f;                                    // the default pulse wave duty cycle (square)

        /// @brief A simple ADSR envelope generator.
        class Envelope {
          public:
            Envelope(const Envelope &) = delete;
            Envelope &operator=(const Envelope &) = delete;
            Envelope &operator=(Envelope &&) = delete;
            Envelope(Envelope &&) = delete;

            Envelope() : sampleFrames(0) {
                SetSimpleRamp(0.0);
            }

            void SetAttack(double attack) {
                this->attack = std::clamp(attack, 0.0, 1.0);
                UpdateEnvelope();
            }

            void SetDecay(double decay) {
                this->decay = std::clamp(decay, 0.0, 1.0);
                UpdateEnvelope();
            }

            void SetSustain(double sustain) {
                this->sustain = std::clamp(sustain, 0.0, 1.0);
            }

            void SetRelease(double release) {
                this->release = std::clamp(release, 0.0, 1.0);
                UpdateEnvelope();
            }

            void SetSimpleRamp(double ramp) {
                attack = ramp / 2.0;
                decay = 0.0;
                sustain = 1.0;
                release = ramp / 2.0;
                UpdateEnvelope();
            }

            double GetVolume(ma_uint64 currentFrame) const {
                if (currentFrame < attackFrames) {
                    return double(currentFrame) / double(attackFrames);
                }

                currentFrame -= attackFrames;

                if (currentFrame < decayFrames) {
                    return 1.0 - (1.0 - sustain) * (double(currentFrame) / double(decayFrames));
                }

                currentFrame -= decayFrames;

                if (currentFrame < sustainFrames) {
                    return sustain;
                }

                currentFrame -= sustainFrames;

                if (currentFrame < releaseFrames) {
                    return sustain * (1.0 - (double(currentFrame) / double(releaseFrames)));
                }

                return 0.0; // after the release phase the volume is zero
            }

            void SetSampleFrames(ma_uint64 sampleFrames) {
                this->sampleFrames = sampleFrames;
                UpdateEnvelope();
            }

          private:
            void UpdateEnvelope() {
                // Ensure that the sum of attack, decay, and release is not greater than 1.0
                auto total = attack + decay + release;

                if (total > 1.0) {
                    attack /= total;
                    decay /= total;
                    release /= total;
                }

                attackFrames = attack * sampleFrames;
                decayFrames = decay * sampleFrames;
                releaseFrames = release * sampleFrames;
                sustainFrames = sampleFrames - (attackFrames + decayFrames + releaseFrames);
            }

            ma_uint64 sampleFrames;
            ma_uint64 attackFrames;
            ma_uint64 decayFrames;
            ma_uint64 sustainFrames;
            ma_uint64 releaseFrames;
            double attack;  // time
            double decay;   // time
            double sustain; // volume
            double release; // time
        };

        /// @brief Simple LFSR noise generator class. Inspirations from AY-3-8910 and SN76489.
        class NoiseGenerator {
          private:
            static const auto BASE_SAMPLE_RATE = 48000;
            static const auto DEFAULT_CLOCK_RATE = BASE_SAMPLE_RATE >> 2;

          public:
            NoiseGenerator() = delete;
            NoiseGenerator(const NoiseGenerator &) = delete;
            NoiseGenerator &operator=(const NoiseGenerator &) = delete;
            NoiseGenerator &operator=(NoiseGenerator &&) = delete;
            NoiseGenerator(NoiseGenerator &&) = delete;

            NoiseGenerator(uint32_t systemSampleRate) {
                seed = uint32_t(func_timer(0.001, 1)) | 1u;
                clockRate = DEFAULT_CLOCK_RATE;
                counter = 0;
                frequency = FREQUENCY_DEFAULT;
                currentSample = SILENCE_SAMPLE;
                amplitude = VOLUME_DEFAULT;
                resampleRatio = float(systemSampleRate) / float(BASE_SAMPLE_RATE);
                updateInterval = (float(clockRate) / float(frequency)) * resampleRatio;
            }

            void SetClockRate(uint32_t clockRate) {
                this->clockRate = clockRate;
                updateInterval = (float(clockRate) / float(frequency)) * resampleRatio;
            }

            void SetFrequency(uint32_t frequency) {
                this->frequency = std::max(frequency, 1u);
                updateInterval = (float(clockRate) / float(frequency)) * resampleRatio;
                counter = 0;
            }

            void SetAmplitude(float amplitude) {
                this->amplitude = std::clamp(amplitude, VOLUME_MIN, VOLUME_MAX);
            }

            float GenerateSample() {
                if (counter >= updateInterval) {
                    StepLFSR();
                    counter = 0;
                } else {
                    counter++;
                }

                return amplitude * currentSample;
            }

          private:
            uint32_t seed;
            uint32_t clockRate;
            uint32_t updateInterval;
            uint32_t counter;
            uint32_t frequency;
            float currentSample;
            float amplitude;
            float resampleRatio;

            /// @brief See https://en.wikipedia.org/wiki/Linear-feedback_shift_register.
            void StepLFSR() {
                auto feedback = ((seed >> 0) ^ (seed >> 1) ^ (seed >> 21) ^ (seed >> 31)) & 1;
                seed = (seed >> 1) | (feedback << 31);
                currentSample = int32_t(seed) / 2147483648.0f;
            }
        };

        /// @brief Custom waveform generator class using a user-defined waveform shape.
        class CustomWaveform {
          private:
            static const auto WAVEFORM_FRAMES_DEFAULT = 256; // number of samples in the default sine waveform

          public:
            CustomWaveform() = delete;
            CustomWaveform(const CustomWaveform &) = delete;
            CustomWaveform &operator=(const CustomWaveform &) = delete;
            CustomWaveform &operator=(CustomWaveform &&) = delete;
            CustomWaveform(CustomWaveform &&) = delete;

            CustomWaveform(ma_uint32 systemSampleRate)
                : frequency(float(FREQUENCY_DEFAULT)), amplitude(VOLUME_DEFAULT), sampleRate(systemSampleRate), phase(0.0f) {

                SetDefaultWaveform();
            }

            /// @brief Generates a default sine waveform
            void SetDefaultWaveform() {
                waveform.resize(WAVEFORM_FRAMES_DEFAULT);
                for (auto i = 0; i < WAVEFORM_FRAMES_DEFAULT; i++) {
                    waveform[i] = std::sin(2.0f * float(M_PI) * float(i) / float(WAVEFORM_FRAMES_DEFAULT));
                }

                UpdatePhaseIncrement();
            }

            void SetFrequency(float frequency) {
                this->frequency = std::max(frequency, 0.0f);

                UpdatePhaseIncrement();
            }

            void SetAmplitude(float amplitude) {
                this->amplitude = std::clamp(amplitude, VOLUME_MIN, VOLUME_MAX);
            }

            /// @brief Update the internal waveform data with a new user-defined waveform.
            /// @param newWaveform A pointer to the new waveform data.
            /// @param newWaveformSize The number of samples in the new waveform data.
            void SetWaveform(const int8_t *newWaveform, size_t newWaveformSize) {
                // Resize and normalize the new waveform data to internal storage
                waveform.resize(newWaveformSize);

                for (size_t i = 0; i < newWaveformSize; i++) {
                    waveform[i] = newWaveform[i] / 128.0f;
                }

                phase = 0.0f;

                UpdatePhaseIncrement();
            }

            // @brief Generate a single sample using the custom waveform.
            // @return The next sample in the waveform.
            float GenerateSample() {
                auto waveformSize = waveform.size();
                auto index = size_t(phase);
                auto fraction = phase - float(index);
                float outputSample = amplitude * ((1.0f - fraction) * waveform[index] + fraction * waveform[(index + 1) % waveformSize]);

                // Increment phase, wrapping around at the end of the waveform
                phase += phaseIncrement;
                if (phase >= waveformSize) {
                    phase -= waveformSize;
                }

                return outputSample;
            }

          private:
            std::vector<float> waveform; // internally stored normalized waveform
            float frequency;             // frequency of the generated waveform
            float amplitude;             // amplitude of the generated waveform
            ma_uint32 sampleRate;        // system sample rate
            float phaseIncrement;        // phase increment per sample
            float phase;                 // current phase position in waveform

            /// @brief Update the phase increment based on the frequency, sample rate, and waveform size.
            void UpdatePhaseIncrement() {
                phaseIncrement = (frequency * float(waveform.size())) / float(sampleRate);
            }
        };

        /// @brief A struct to used to hold the state of the MML player and also used for the state stack (i.e. when VARPTR$ substrings are used).
        struct State {
            const uint8_t *byte; // pointer to a byte in an MML string
            int32_t length;      // this needs to be signed
        };

        RawStream *rawStream;                  // this is the RawStream where the samples data will be pushed to
        ma_waveform_config maWaveformConfig;   // miniaudio waveform configuration
        ma_waveform maWaveform;                // miniaudio waveform
        ma_noise_config maWhiteNoiseConfig;    // miniaudio white noise configuration
        ma_noise maWhiteNoise;                 // miniaudio white noise
        ma_noise_config maPinkNoiseConfig;     // miniaudio pink noise configuration
        ma_noise maPinkNoise;                  // miniaudio pink noise
        ma_noise_config maBrownianNoiseConfig; // miniaudio brownian noise configuration
        ma_noise maBrownianNoise;              // miniaudio brownian noise
        NoiseGenerator *noise;                 // LFSR noise generator
        CustomWaveform *customWaveform;        // custom waveform generator
        ma_pulsewave_config maPulseWaveConfig; // miniaudio pulse wave configuration
        ma_pulsewave maPulseWave;              // miniaudio pulse wave
        ma_result maResult;                    // result of the last miniaudio operation
        std::vector<float> noteBuffer;         // note frames are rendered here temporarily before it is mixed to waveBuffer
        std::vector<float> waveBuffer;         // this is where the waveform is rendered / mixed before being pushed to RawStream
        std::vector<SampleFrame> pausedBuffer; // this is where the final waveform is pushed to and accumulated when the PSG is paused
        bool isPaused;                         // this is true if the PSG is paused
        ma_uint64 mixCursor;                   // this is the cursor position in waveBuffer where the next mix should happen (this can be < waveBuffer.size())
        WaveformType waveformType;             // the currently selected waveform type (applies to MML and sound)
        Envelope envelope;                     // the ADSR envelope (used for sound and MML)
        bool background;                       // if this is true, then control will be returned back to the caller as soon as the sound / MML is rendered
        float panPosition;                     // stereo pan setting for SOUND (-1.0f - 0.0f - 1.0f)
        float gainLeft;                        // this is calculated from panPosition
        float gainRight;                       // this is calculated from panPosition
        std::stack<State> stateStack;          // this maintains the state stack if we need to process substrings (VARPTR$)
        State currentState;                    // this is the current state. See State struct
        int tempo;                             // the tempo of the MML tune (this impacts all lengths)
        int octave;                            // the current octave that we'll use for MML notes
        double length;                         // the length of each MML note (1 = full, 4 = quarter etc.)
        double pause;                          // the duration of silence after an MML note (this eats away from the note length)
        double duration;                       // the duration of a sound / MML note / silence (in seconds)
        int dots;                              // the dots after a note or a pause that increases the duration
        bool playIt;                           // flag that is set when the buffer can be played

        /// @brief Generates a waveform to waveBuffer starting at the mixCursor sample location. The buffer must be resized before calling this. We could have
        /// resized waveBuffer inside this. However, PLAY supports stuff like staccato etc. that needs some silence after the waveform. So it makes sense for
        /// the calling function to do the resize before calling this.
        /// @param waveDuration The duration of the waveform in seconds.
        /// @param mix Mixes the generated waveform to the buffer instead of overwriting it.
        void GenerateWaveform(double waveDuration, bool mix = false) {
            auto neededFrames = ma_uint64(waveDuration * ma_engine_get_sample_rate(rawStream->maEngine));

            if (!neededFrames || maWaveform.config.frequency >= FREQUENCY_LIMIT || mixCursor + neededFrames > waveBuffer.size()) {
                AUDIO_DEBUG_PRINT("Not generating any waveform. Frames = %llu, frequency = %lf, cursor = %llu", neededFrames, maWaveform.config.frequency,
                                  mixCursor);
                return; // nothing to do
            }

            maResult = MA_SUCCESS;
            ma_uint64 generatedFrames = neededFrames;        // assume we'll generate all the frames we need
            noteBuffer.assign(neededFrames, SILENCE_SAMPLE); // resize the noteBuffer vector to render the waveform and also zero (silence) everything

            // Generate to the temp buffer and then we'll mix later
            switch (waveformType) {
            case WaveformType::NONE:
                // NOP
                break;

            case WaveformType::SQUARE:
            case WaveformType::SAWTOOTH:
            case WaveformType::TRIANGLE:
            case WaveformType::SINE:
                maResult = ma_waveform_read_pcm_frames(&maWaveform, noteBuffer.data(), neededFrames, &generatedFrames);
                break;

            case WaveformType::NOISE_WHITE:
                maResult = ma_noise_read_pcm_frames(&maWhiteNoise, noteBuffer.data(), neededFrames, &generatedFrames);
                break;

            case WaveformType::NOISE_PINK:
                maResult = ma_noise_read_pcm_frames(&maPinkNoise, noteBuffer.data(), neededFrames, &generatedFrames);
                break;

            case WaveformType::NOISE_BROWNIAN:
                maResult = ma_noise_read_pcm_frames(&maBrownianNoise, noteBuffer.data(), neededFrames, &generatedFrames);
                break;

            case WaveformType::NOISE_LFSR:
                for (ma_uint64 i = 0; i < neededFrames; i++) {
                    noteBuffer[i] = noise->GenerateSample();
                }
                break;

            case WaveformType::PULSE:
                maResult = ma_pulsewave_read_pcm_frames(&maPulseWave, noteBuffer.data(), neededFrames, &generatedFrames);
                break;

            case WaveformType::CUSTOM:
                for (ma_uint64 i = 0; i < neededFrames; i++) {
                    noteBuffer[i] = customWaveform->GenerateSample();
                }
                break;

            case WaveformType::COUNT:
            default:
                // NOP
                break;
            }

            if (maResult != MA_SUCCESS) {
                AUDIO_DEBUG_PRINT("maResult = %i", maResult);
                return; // something went wrong
            }

            envelope.SetSampleFrames(generatedFrames);
            auto destination = waveBuffer.data() + mixCursor;

            if (mix) {
                // Mix the samples to the buffer
                for (ma_uint64 i = 0; i < generatedFrames; i++) {
                    destination[i] = std::fmaf(noteBuffer[i], envelope.GetVolume(i), destination[i]);
                }

                AUDIO_DEBUG_PRINT("Waveform = %i, frames requested = %llu, frames mixed = %llu", int(waveformType), neededFrames, generatedFrames);
            } else {
                // Copy the samples to the buffer
                for (ma_uint64 i = 0; i < generatedFrames; i++) {
                    destination[i] = noteBuffer[i] * envelope.GetVolume(i); // apply the envelope volume
                }

                AUDIO_DEBUG_PRINT("Waveform = %i, frames requested = %llu, frames generated = %llu", int(waveformType), neededFrames, generatedFrames);
            }
        }

        /// @brief Sets the frequency of the waveform.
        /// @param frequency The frequency of the waveform.
        void SetFrequency(double frequency) {
            maResult = ma_waveform_set_frequency(&maWaveform, frequency);
            AUDIO_DEBUG_CHECK(maResult == MA_SUCCESS);
            maResult = ma_pulsewave_set_frequency(&maPulseWave, frequency);
            AUDIO_DEBUG_CHECK(maResult == MA_SUCCESS);
            noise->SetFrequency(uint32_t(frequency));
            customWaveform->SetFrequency(frequency);
        }

        /// @brief Sets MML friendly pan position value.
        /// @param value A value from 0 to 100.
        void SetMMLPanPosition(long value) {
            SetPanPosition((float(value) / float(MML_PAN_CENTER)) - PAN_RIGHT);
        }

        /// @brief Gets MML friendly pan position value.
        /// @return A value from 0 to 100.
        long GetMMLPanPosition() {
            return long((panPosition + PAN_RIGHT) * float(MML_PAN_CENTER));
        }

        /// @brief Sets MML friendly amplitude value.
        /// @param amplitude A value from 0 to 100.
        void SetMMLAmplitude(long amplitude) {
            SetAmplitude(double(amplitude) / MML_VOLUME_MAX);
        }

        /// @brief Gets MML friendly amplitude value.
        /// @return A value from 0 to 100.
        long GetMMLAmplitude() {
            return std::lround(maWaveform.config.amplitude * MML_VOLUME_MAX);
        }

        /// @brief Accumulates the samples into the paused buffer until the PSG is unpaused.
        /// @param samples The samples to collect.
        /// @param samplesCount The number of samples to collect.
        /// @param gainLeft Left channel gain value (0.0 to 1.0).
        /// @param gainRight Right channel gain value (0.0 to 1.0).
        void AccumulateSampleFrames(const float *samples, size_t samplesCount, float gainLeft, float gainRight) {
            for (size_t i = 0; i < samplesCount; i++) {
                pausedBuffer.push_back({samples[i] * gainLeft, samples[i] * gainRight});
            }
        }

        /// @brief Sends the buffer for playback.
        void PushBufferForPlayback() {
            if (!waveBuffer.empty()) {
                if (isPaused) {
                    AccumulateSampleFrames(waveBuffer.data(), waveBuffer.size(), gainLeft, gainRight);
                } else {
                    rawStream->PushSampleFrames(waveBuffer.data(), waveBuffer.size(), gainLeft, gainRight);
                }

                AUDIO_DEBUG_PRINT("Sent %llu samples for playback", waveBuffer.size());

                waveBuffer.clear(); // set the buffer size to zero
                mixCursor = 0;      // reset the cursor
            }
        }

        /// @brief Waits for any playback to complete.
        void AwaitPlaybackCompletion() {
            if (background || isPaused) {
                return; // no need to wait
            }

            auto timeSec = rawStream->GetTimeRemaining() * 0.95 - 0.25; // per original QB64 behavior

            if (timeSec > 0) {
                AUDIO_DEBUG_PRINT("Waiting %f seconds for playback to complete", timeSec);

                sub__delay(timeSec); // we are using sub_delay() because ON TIMER and other events may need to be called while we are waiting
            }

            AUDIO_DEBUG_PRINT("Playback complete");
        }

      public:
        // Delete default, copy and move constructors and assignments.
        PSG() = delete;
        PSG(const PSG &) = delete;
        PSG &operator=(const PSG &) = delete;
        PSG &operator=(PSG &&) = delete;
        PSG(PSG &&) = delete;

        /// @brief Initializes and allocates required miniaudio resources.
        /// @param pRawStream A valid RawStream object pointer. This cannot be NULL.
        PSG(RawStream *pRawStream) {
            rawStream = pRawStream; // save the RawStream object pointer
            isPaused = false;
            mixCursor = 0;
            playIt = background = false; // default to foreground playback
            tempo = MML_TEMPO_DEFAULT;
            octave = MML_OCTAVE_DEFAULT;
            length = MML_LENGTH_DEFAULT;
            pause = MML_PAUSE_DEFAULT;
            duration = 0;
            dots = 0;
            currentState = {};
            SetPanPosition(PAN_CENTER);

            maWaveformConfig = ma_waveform_config_init(ma_format::ma_format_f32, 1, ma_engine_get_sample_rate(rawStream->maEngine),
                                                       ma_waveform_type::ma_waveform_type_square, VOLUME_DEFAULT, FREQUENCY_DEFAULT);
            maResult = ma_waveform_init(&maWaveformConfig, &maWaveform);
            AUDIO_DEBUG_CHECK(maResult == MA_SUCCESS);

            maWhiteNoiseConfig = ma_noise_config_init(ma_format::ma_format_f32, 1, ma_noise_type::ma_noise_type_white, 0, VOLUME_DEFAULT);
            maResult = ma_noise_init(&maWhiteNoiseConfig, NULL, &maWhiteNoise);
            AUDIO_DEBUG_CHECK(maResult == MA_SUCCESS);

            maPinkNoiseConfig = ma_noise_config_init(ma_format::ma_format_f32, 1, ma_noise_type::ma_noise_type_pink, 0, VOLUME_DEFAULT);
            maResult = ma_noise_init(&maPinkNoiseConfig, NULL, &maPinkNoise);
            AUDIO_DEBUG_CHECK(maResult == MA_SUCCESS);

            maBrownianNoiseConfig = ma_noise_config_init(ma_format::ma_format_f32, 1, ma_noise_type::ma_noise_type_brownian, 0, VOLUME_DEFAULT);
            maResult = ma_noise_init(&maBrownianNoiseConfig, NULL, &maBrownianNoise);
            AUDIO_DEBUG_CHECK(maResult == MA_SUCCESS);

            maPulseWaveConfig = ma_pulsewave_config_init(ma_format::ma_format_f32, 1, ma_engine_get_sample_rate(rawStream->maEngine),
                                                         PULSE_WAVE_DUTY_CYCLE_DEFAULT, VOLUME_DEFAULT, FREQUENCY_DEFAULT);
            maResult = ma_pulsewave_init(&maPulseWaveConfig, &maPulseWave);
            AUDIO_DEBUG_CHECK(maResult == MA_SUCCESS);

            noise = new NoiseGenerator(ma_engine_get_sample_rate(rawStream->maEngine));
            AUDIO_DEBUG_CHECK(noise != nullptr);
            noise->SetAmplitude(VOLUME_DEFAULT);
            noise->SetFrequency(FREQUENCY_DEFAULT);

            customWaveform = new CustomWaveform(ma_engine_get_sample_rate(rawStream->maEngine));
            AUDIO_DEBUG_CHECK(customWaveform != nullptr);
            customWaveform->SetAmplitude(VOLUME_DEFAULT);
            customWaveform->SetFrequency(FREQUENCY_DEFAULT);

            SetWaveformType(WAVEFORM_TYPE_DEFAULT);

            AUDIO_DEBUG_PRINT("PSG initialized @ %uHz", maWaveform.config.sampleRate);
        }

        /// @brief Frees the waveform buffer and cleans up the waveform resources.
        ~PSG() {
            delete customWaveform;
            delete noise;
            ma_pulsewave_uninit(&maPulseWave);
            ma_noise_uninit(&maBrownianNoise, NULL);
            ma_noise_uninit(&maPinkNoise, NULL);
            ma_noise_uninit(&maWhiteNoise, NULL);
            ma_waveform_uninit(&maWaveform);

            AUDIO_DEBUG_PRINT("PSG destroyed");
        }

        /// @brief Sets the waveform type.
        /// @param type The waveform type. See Waveform::Type enum.
        void SetWaveformType(WaveformType waveType) {
            switch (waveType) {
            case WaveformType::NONE:
                // NOP
                break;

            case WaveformType::SQUARE:
                maResult = ma_waveform_set_type(&maWaveform, ma_waveform_type::ma_waveform_type_square);
                break;

            case WaveformType::SAWTOOTH:
                maResult = ma_waveform_set_type(&maWaveform, ma_waveform_type::ma_waveform_type_sawtooth);
                break;

            case WaveformType::TRIANGLE:
                maResult = ma_waveform_set_type(&maWaveform, ma_waveform_type::ma_waveform_type_triangle);
                break;

            case WaveformType::SINE:
                maResult = ma_waveform_set_type(&maWaveform, ma_waveform_type::ma_waveform_type_sine);
                break;

            case WaveformType::NOISE_WHITE:
            case WaveformType::NOISE_PINK:
            case WaveformType::NOISE_BROWNIAN:
            case WaveformType::NOISE_LFSR:
            case WaveformType::PULSE:
            case WaveformType::CUSTOM:
            case WaveformType::COUNT:
            default:
                // NOP
                break;
            }

            AUDIO_DEBUG_CHECK(maResult == MA_SUCCESS);

            waveformType = waveType;

            AUDIO_DEBUG_PRINT("Waveform type set to %i", int(waveformType));
        }

        /// @brief Sets any special waveform parameter (e.g. duty cycle of a pulse wave).
        /// @param value The parameter value (depending on the waveform type).
        void SetWaveformParameter(float value) {
            switch (waveformType) {
            case WaveformType::NONE:
                // NOP
                break;

            case WaveformType::SQUARE:
                envelope.SetAttack(value);
                break;

            case WaveformType::SAWTOOTH:
                envelope.SetDecay(value);
                break;

            case WaveformType::TRIANGLE:
                envelope.SetSustain(value);
                break;

            case WaveformType::SINE:
                envelope.SetRelease(value);
                break;

            case WaveformType::NOISE_WHITE:
                maResult = ma_noise_set_seed(&maWhiteNoise, ma_int32(value));
                break;

            case WaveformType::NOISE_PINK:
                maResult = ma_noise_set_seed(&maPinkNoise, ma_int32(value));
                break;

            case WaveformType::NOISE_BROWNIAN:
                maResult = ma_noise_set_seed(&maBrownianNoise, ma_int32(value));
                break;

            case WaveformType::NOISE_LFSR:
                noise->SetClockRate(uint32_t(value));
                break;

            case WaveformType::PULSE:
                maResult = ma_pulsewave_set_duty_cycle(&maPulseWave, value);
                break;

            case WaveformType::CUSTOM:
            case WaveformType::COUNT:
            default:
                // NOP
                break;
            }

            AUDIO_DEBUG_CHECK(maResult == MA_SUCCESS);
        }

        /// @brief Sets the amplitude of the waveform.
        /// @param amplitude The amplitude of the waveform.
        void SetAmplitude(double amplitude) {
            amplitude = std::clamp<double>(amplitude, VOLUME_MIN, VOLUME_MAX);

            maResult = ma_waveform_set_amplitude(&maWaveform, amplitude);
            AUDIO_DEBUG_CHECK(maResult == MA_SUCCESS);
            maResult = ma_noise_set_amplitude(&maWhiteNoise, amplitude);
            AUDIO_DEBUG_CHECK(maResult == MA_SUCCESS);
            maResult = ma_noise_set_amplitude(&maPinkNoise, amplitude);
            AUDIO_DEBUG_CHECK(maResult == MA_SUCCESS);
            maResult = ma_noise_set_amplitude(&maBrownianNoise, amplitude);
            AUDIO_DEBUG_CHECK(maResult == MA_SUCCESS);
            maResult = ma_pulsewave_set_amplitude(&maPulseWave, amplitude);
            AUDIO_DEBUG_CHECK(maResult == MA_SUCCESS);
            noise->SetAmplitude(float(amplitude));
            customWaveform->SetAmplitude(float(amplitude));

            AUDIO_DEBUG_PRINT("Amplitude set to %lf", amplitude);
        }

        /// @brief Set the PSG pan position.
        /// @param value A number between -1.0 to 1.0. Where 0.0 is center.
        void SetPanPosition(float value) {
            static constexpr auto QUARTER_PI = float(M_PI) / 4.0f;

            panPosition = std::clamp(value, PAN_LEFT, PAN_RIGHT); // clamp the value;

            // Calculate the left and right channel gain values using pan law (-3.0dB pan depth)
            auto panMapped = (panPosition + 1.0f) * QUARTER_PI;
            gainLeft = std::cos(panMapped);
            gainRight = std::sin(panMapped);

            AUDIO_DEBUG_PRINT("Pan position = %f, Left gain = %f, Right gain = %f", panPosition, gainLeft, gainRight);
        }

        /// @brief Pauses or resumes PSG playback. Once paused, the samples for the commands processed are accumulated in pausedBuffer.
        /// pausedBuffer is pushed and flushed as soon as the state is set to false.
        /// @param state true to pause, false to resume.
        void Pause(bool state) {
            isPaused = state; // save the state

            if (!isPaused) {
                // We'll need to push the samples in the paused buffer to RawStream
                rawStream->PushSampleFrames(pausedBuffer.data(), pausedBuffer.size());
                pausedBuffer.clear();
            }
        }

        /// @brief Sets a custom mono 8-bit waveform for the PSG.
        /// @param data A pointer to the waveform data.
        /// @param length The length of the waveform data in samples.
        void SetCustomWaveform(int8_t *data, size_t length) {
            customWaveform->SetWaveform(data, length);
        }

        /// @brief Emulates a PC speaker sound. The volume, waveform and background mode can be changed using PLAY.
        void Sound(double frequency, double lengthInClockTicks) {
            SetFrequency(frequency);
            auto soundDuration = lengthInClockTicks / 18.2;
            waveBuffer.assign(size_t(soundDuration * ma_engine_get_sample_rate(rawStream->maEngine)), SILENCE_SAMPLE);
            GenerateWaveform(soundDuration);
            PushBufferForPlayback();
            AwaitPlaybackCompletion(); // await playback to complete if we are in MF mode
        }

        /// @brief An MML parser that implements the QB64 MML spec and more.
        /// https://qb64phoenix.com/qb64wiki/index.php/PLAY
        /// http://vgmpf.com/Wiki/index.php?title=Music_Macro_Language
        /// https://en.wikipedia.org/wiki/Music_Macro_Language
        /// https://sneslab.net/wiki/Music_Macro_Language
        /// http://www.mirbsd.org/htman/i386/man4/speaker.htm
        /// https://www.qbasic.net/en/reference/qb11/Statement/PLAY-006.htm
        /// https://woolyss.com/chipmusic-mml.php
        /// frequency = 440.0 * pow(2.0, (note + (octave - 2.0) * 12.0 - 9.0) / 12.0)
        // const float FREQUENCY_TABLE[] = {
        //	0,
        //	//1       2         3         4         5         6         7         8         9         10        11        12
        //	//C       C#        D         D#        E         F         F#        G         G#        A         A#        B
        //	16.35f,   17.32f,   18.35f,   19.45f,   20.60f,   21.83f,   23.12f,   24.50f,   25.96f,   27.50f,   29.14f,   30.87f,   // Octave 0
        //	32.70f,   34.65f,   36.71f,   38.89f,   41.20f,   43.65f,   46.25f,   49.00f,   51.91f,   55.00f,   58.27f,   61.74f,   // Octave 1
        //	65.41f,   69.30f,   73.42f,   77.78f,   82.41f,   87.31f,   92.50f,   98.00f,   103.83f,  110.00f,  116.54f,  123.47f,  // Octave 2
        //	130.81f,  138.59f,  146.83f,  155.56f,  164.81f,  174.62f,  185.00f,  196.00f,  207.65f,  220.00f,  233.08f,  246.94f,  // Octave 3
        //	261.63f,  277.18f,  293.67f,  311.13f,  329.63f,  349.23f,  370.00f,  392.00f,  415.31f,  440.00f,  466.17f,  493.89f,  // Octave 4
        //	523.25f,  554.37f,  587.33f,  622.26f,  659.26f,  698.46f,  739.99f,  783.99f,  830.61f,  880.00f,  932.33f,  987.77f,  // Octave 5
        //	1046.51f, 1108.74f, 1174.67f, 1244.51f, 1318.52f, 1396.92f, 1479.99f, 1567.99f, 1661.23f, 1760.01f, 1864.66f, 1975.54f, // Octave 6
        //	2093.02f, 2217.47f, 2349.33f, 2489.03f, 2637.03f, 2793.84f, 2959.97f, 3135.98f, 3322.45f, 3520.02f, 3729.33f, 3951.09f, // Octave 7
        // };
        /// @param mml A string containing the MML tune
        void Play(const qbs *mml) {
            if (!mml || !mml->len) // exit if string is empty
                return;

            auto currentChar = 0;
            auto processedChar = 0;
            auto numberEntered = 0;
            int64_t number = 0;
            bool noteShifted = false;
            auto noteOffset = 0;
            auto followUp = 0;
            auto noDotDuration = 1.0 / (tempo / 60.0) * (4.0 / length);

            playIt = false;

            stateStack.push({mml->chr, mml->len}); // push the string to the state stack

            // Process until our state stack is empty
            while (!stateStack.empty()) {
                // Pop and use the top item in the state stack
                currentState = stateStack.top();
                stateStack.pop();

                while ((currentState.length--) || followUp) {
                    if (currentState.length < 0) {
                        currentChar = ' ';
                        goto follow_up;
                    }

                    currentChar = *currentState.byte++;
                    if (isspace(currentChar) || ';' == currentChar) // #554: skip whitespace and semicolon separator
                        continue;

                    processedChar = toupper(currentChar);

                    if (processedChar == 'X') { // "X" + VARPTR$()
                        // A minimum of 3 bytes is need to read the address
                        if (currentState.length < 3) {
                            error(QB_ERROR_ILLEGAL_FUNCTION_CALL);
                            return;
                        }

                        // Read type byte
                        currentChar = *currentState.byte++;
                        currentState.length--;

                        // Read offset within DBLOCK
                        auto offset = *(uint16_t *)currentState.byte;
                        currentState.byte += 2;
                        currentState.length -= 2;

                        stateStack.push(currentState); // push the current state to the stack

                        // Set new state
                        currentState.byte = &cmem[1280] + (cmem[1280 + offset + 3] * 256 + cmem[1280 + offset + 2]);
                        currentState.length = cmem[1280 + offset + 1] * 256 + cmem[1280 + offset + 0];

                        continue;
                    } else if (currentChar == '=') { // "=" + VARPTR$()
                        if (dots) {
                            error(QB_ERROR_ILLEGAL_FUNCTION_CALL);
                            return;
                        }

                        if (numberEntered) {
                            error(QB_ERROR_ILLEGAL_FUNCTION_CALL);
                            return;
                        }

                        numberEntered = 2;

                        // VARPTR$ reference
                        /*
                           'BYTE=1
                           'INTEGER=2
                           'STRING=3 SUB-STRINGS must use "X"+VARPTR$(string$)
                           'SINGLE=4
                           'INT64=5
                           'FLOAT=6
                           'DOUBLE=8
                           'LONG=20
                           'BIT=64+n
                         */

                        if (currentState.length < 3) {
                            error(QB_ERROR_ILLEGAL_FUNCTION_CALL);
                            return;
                        }

                        currentChar = *currentState.byte++; // read type byte
                        currentState.length--;

                        auto x = *(uint16_t *)currentState.byte; // read offset within DBLOCK
                        currentState.byte += 2;
                        currentState.length -= 2;

                        // note: allowable _BIT type variables in VARPTR$ are all at a byte offset and are all
                        //      padded until the next byte
                        int64_t d = 0;

                        switch (currentChar) {
                        case 1:
                            d = *(char *)(dblock + x);
                            break;
                        case (1 + 128):
                            d = *(uint8_t *)(dblock + x);
                            break;
                        case 2:
                            d = *(int16_t *)(dblock + x);
                            break;
                        case (2 + 128):
                            d = *(uint16_t *)(dblock + x);
                            break;
                        case 4:
                            d = *(float *)(dblock + x);
                            break;
                        case 5:
                            d = *(int64_t *)(dblock + x);
                            break;
                        case (5 + 128):
                            d = *(int64_t *)(dblock + x); // unsigned conversion is unsupported!
                            break;
                        case 6:
                            d = *(long double *)(dblock + x);
                            break;
                        case 8:
                            d = *(double *)(dblock + x);
                            break;
                        case 20:
                            d = *(int32_t *)(dblock + x);
                            break;
                        case (20 + 128):
                            d = *(uint32_t *)(dblock + x);
                            break;
                        default:
                            // bit type?
                            if ((currentChar & 64) == 0) {
                                error(QB_ERROR_ILLEGAL_FUNCTION_CALL);
                                return;
                            }

                            auto x2 = currentChar & 63;

                            if (x2 > 56) {
                                error(QB_ERROR_ILLEGAL_FUNCTION_CALL);
                                return;
                            } // valid number of bits?

                            // create a mask
                            auto mask = (((int64_t)1) << x2) - 1;
                            auto i64num = (*(int64_t *)(dblock + x)) & mask;

                            // signed?
                            if (currentChar & 128) {
                                mask = ((int64_t)1) << (x2 - 1);
                                if (i64num & mask) { // top bit on?
                                    mask = -1;
                                    mask <<= x2;
                                    i64num += mask;
                                }
                            } // signed

                            d = i64num;
                        }

                        if (d > 2147483647ll || d < -2147483648ll) {
                            error(QB_ERROR_ILLEGAL_FUNCTION_CALL); // out of range value!
                            return;
                        }

                        number = d;

                        continue;
                    } else if (currentChar >= '0' && currentChar <= '9') {
                        if (dots || numberEntered == 2) {
                            error(QB_ERROR_ILLEGAL_FUNCTION_CALL);
                            return;
                        }

                        if (!numberEntered) {
                            number = 0;
                            numberEntered = 1;
                        }

                        number = number * 10 + currentChar - 48;

                        continue;
                    } else if (currentChar == '.') {
                        if (followUp != 7 && followUp != 1 && followUp != 4) {
                            error(QB_ERROR_ILLEGAL_FUNCTION_CALL);
                            return;
                        }

                        dots++;

                        continue;
                    }

                follow_up:
                    if (followUp == 16) { // S...
                        if (currentChar == '-') {
                            SetMMLPanPosition(GetMMLPanPosition() - 1);

                            followUp = 0;

                            continue;
                        } else if (currentChar == '+') {
                            SetMMLPanPosition(GetMMLPanPosition() + 1);

                            followUp = 0;

                            continue;
                        }

                        if (!numberEntered) {
                            error(QB_ERROR_ILLEGAL_FUNCTION_CALL);
                            return;
                        }

                        numberEntered = 0;

                        if (number < MML_PAN_LEFT || number > MML_PAN_RIGHT) {
                            error(QB_ERROR_ILLEGAL_FUNCTION_CALL);
                            return;
                        }

                        SetMMLPanPosition(number);

                        followUp = 0;

                        if (currentState.length < 0) {
                            break;
                        }
                    } else if (followUp == 15) { // Y...
                        if (!numberEntered) {
                            error(QB_ERROR_ILLEGAL_FUNCTION_CALL);
                            return;
                        }

                        numberEntered = 0;

                        if (number < MML_WAVE_PARAM_MIN || number > MML_WAVE_PARAM_MAX) {
                            error(QB_ERROR_ILLEGAL_FUNCTION_CALL);
                            return;
                        }

                        SetWaveformParameter(float(number) / float(MML_WAVE_PARAM_MAX));

                        followUp = 0;

                        if (currentState.length < 0) {
                            break;
                        }
                    } else if (followUp == 14) { // _...
                        if (!numberEntered) {
                            error(QB_ERROR_ILLEGAL_FUNCTION_CALL);
                            return;
                        }

                        numberEntered = 0;

                        if (number < MML_RELEASE_MIN || number > MML_RELEASE_MAX) {
                            error(QB_ERROR_ILLEGAL_FUNCTION_CALL);
                            return;
                        }

                        envelope.SetRelease(double(number) / double(MML_RELEASE_MAX));

                        followUp = 0;

                        if (currentState.length < 0) {
                            break;
                        }
                    } else if (followUp == 13) { // ^...
                        if (!numberEntered) {
                            error(QB_ERROR_ILLEGAL_FUNCTION_CALL);
                            return;
                        }

                        numberEntered = 0;

                        if (number < MML_SUSTAIN_MIN || number > MML_SUSTAIN_MAX) {
                            error(QB_ERROR_ILLEGAL_FUNCTION_CALL);
                            return;
                        }

                        envelope.SetSustain(double(number) / double(MML_SUSTAIN_MAX));

                        followUp = 0;

                        if (currentState.length < 0) {
                            break;
                        }
                    } else if (followUp == 12) { // \...
                        if (!numberEntered) {
                            error(QB_ERROR_ILLEGAL_FUNCTION_CALL);
                            return;
                        }

                        numberEntered = 0;

                        if (number < MML_DECAY_MIN || number > MML_DECAY_MAX) {
                            error(QB_ERROR_ILLEGAL_FUNCTION_CALL);
                            return;
                        }

                        envelope.SetDecay(double(number) / double(MML_DECAY_MAX));

                        followUp = 0;

                        if (currentState.length < 0) {
                            break;
                        }
                    } else if (followUp == 11) { // /...
                        if (!numberEntered) {
                            error(QB_ERROR_ILLEGAL_FUNCTION_CALL);
                            return;
                        }

                        numberEntered = 0;

                        if (number < MML_ATTACK_MIN || number > MML_ATTACK_MAX) {
                            error(QB_ERROR_ILLEGAL_FUNCTION_CALL);
                            return;
                        }

                        envelope.SetAttack(double(number) / double(MML_ATTACK_MAX));

                        followUp = 0;

                        if (currentState.length < 0) {
                            break;
                        }
                    } else if (followUp == 10) { // Q...
                        if (!numberEntered) {
                            error(QB_ERROR_ILLEGAL_FUNCTION_CALL);
                            return;
                        }

                        numberEntered = 0;

                        if (number < MML_VOLUME_RAMP_MIN || number > MML_VOLUME_RAMP_MAX) {
                            error(QB_ERROR_ILLEGAL_FUNCTION_CALL);
                            return;
                        }

                        envelope.SetSimpleRamp(double(number) / double(MML_VOLUME_RAMP_MAX));

                        followUp = 0;

                        if (currentState.length < 0)
                            break;
                    } else if (followUp == 9) { // @...
                        if (!numberEntered) {
                            error(QB_ERROR_ILLEGAL_FUNCTION_CALL);
                            return;
                        }

                        numberEntered = 0;

                        if ((WaveformType)number <= WaveformType::NONE || (WaveformType)number >= WaveformType::COUNT) {
                            error(QB_ERROR_ILLEGAL_FUNCTION_CALL);
                            return;
                        }

                        SetWaveformType((WaveformType)number);

                        followUp = 0;

                        if (currentState.length < 0)
                            break;
                    } else if (followUp == 8) { // V...
                        if (currentChar == '-') {
                            SetMMLAmplitude(GetMMLAmplitude() - 1);

                            followUp = 0;

                            continue;
                        } else if (currentChar == '+') {
                            SetMMLAmplitude(GetMMLAmplitude() + 1);

                            followUp = 0;

                            continue;
                        }

                        if (!numberEntered) {
                            error(QB_ERROR_ILLEGAL_FUNCTION_CALL);
                            return;
                        }

                        numberEntered = 0;

                        if (number < MML_VOLUME_MIN || number > MML_VOLUME_MAX) {
                            error(QB_ERROR_ILLEGAL_FUNCTION_CALL);
                            return;
                        }

                        SetAmplitude(double(number) / double(MML_VOLUME_MAX));

                        followUp = 0;

                        if (currentState.length < 0)
                            break;
                    } else if (followUp == 7) { // P...
                        if (numberEntered) {
                            numberEntered = 0;
                            if (number < 1 || number > 64) {
                                error(QB_ERROR_ILLEGAL_FUNCTION_CALL);
                                return;
                            }
                            duration = 1.0 / (tempo / 60.0) * (4.0 / ((double)number));
                        } else {
                            duration = noDotDuration;
                        }

                        auto dotDuration = duration;

                        for (auto i = 0; i < dots; i++) {
                            dotDuration /= 2.0;
                            duration += dotDuration;
                        }

                        dots = 0;

                        auto noteFrames = ma_uint64(duration * ma_engine_get_sample_rate(rawStream->maEngine));

                        if ((mixCursor + noteFrames) > waveBuffer.size()) {
                            waveBuffer.resize(mixCursor + noteFrames, SILENCE_SAMPLE);
                        }

                        if (currentChar != ',') {
                            mixCursor += noteFrames;
                        }

                        playIt = true;
                        followUp = 0;

                        if (currentChar == ',')
                            continue;

                        if (currentState.length < 0)
                            break;
                    } else if (followUp == 6) { // T...
                        if (!numberEntered) {
                            error(QB_ERROR_ILLEGAL_FUNCTION_CALL);
                            return;
                        }

                        numberEntered = 0;

                        if (number < MML_TEMPO_MIN || number > MML_TEMPO_MAX) {
                            number = MML_TEMPO_DEFAULT;
                        }

                        tempo = number;
                        noDotDuration = 1.0 / (tempo / 60.0) * (4.0 / length);
                        followUp = 0;

                        if (currentState.length < 0)
                            break;
                    } else if (followUp == 5) { // M...
                        if (numberEntered) {
                            error(QB_ERROR_ILLEGAL_FUNCTION_CALL);
                            return;
                        }

                        switch (processedChar) {
                        case 'L': // legato
                            pause = 0.0;
                            break;
                        case 'N': // normal
                            pause = 1.0 / 8.0;
                            break;
                        case 'S': // staccato
                            pause = 1.0 / 4.0;
                            break;
                        case 'B': // background
                            if (!background) {
                                if (playIt) {
                                    // Play pending buffer in foreground before we switch to background
                                    playIt = false;
                                    PushBufferForPlayback();
                                    AwaitPlaybackCompletion();
                                }
                                background = true;
                            }
                            break;
                        case 'F': // foreground
                            background = false;
                            break;
                        default:
                            error(QB_ERROR_ILLEGAL_FUNCTION_CALL);
                            return;
                        }

                        followUp = 0;

                        continue;
                    } else if (followUp == 4) { // N...
                        if (!numberEntered) {
                            error(QB_ERROR_ILLEGAL_FUNCTION_CALL);
                            return;
                        }

                        numberEntered = 0;

                        if (number > 84) {
                            error(QB_ERROR_ILLEGAL_FUNCTION_CALL);
                            return;
                        }

                        noteOffset = -45 + number;

                        goto follow_up_1;
                    } else if (followUp == 3) { // O...
                        if (!numberEntered) {
                            error(QB_ERROR_ILLEGAL_FUNCTION_CALL);
                            return;
                        }

                        numberEntered = 0;

                        if (number < MML_OCTAVE_MIN || number > MML_OCTAVE_MAX) {
                            error(QB_ERROR_ILLEGAL_FUNCTION_CALL);
                            return;
                        }

                        octave = number;

                        followUp = 0;

                        if (currentState.length < 0)
                            break;
                    } else if (followUp == 2) { // L...
                        if (!numberEntered) {
                            error(QB_ERROR_ILLEGAL_FUNCTION_CALL);
                            return;
                        }

                        numberEntered = 0;

                        if (number < MML_LENGTH_MIN || number > MML_LENGTH_MAX) {
                            error(QB_ERROR_ILLEGAL_FUNCTION_CALL);
                            return;
                        }

                        length = number;
                        noDotDuration = 1.0 / (tempo / 60.0) * (4.0 / length);
                        followUp = 0;

                        if (currentState.length < 0)
                            break;
                    } else if (followUp == 1) { // A-G...
                        if (currentChar == '-') {
                            if (noteShifted || numberEntered) {
                                error(QB_ERROR_ILLEGAL_FUNCTION_CALL);
                                return;
                            }

                            noteShifted = true;
                            noteOffset--;

                            continue;
                        }
                        if (currentChar == '+' || currentChar == '#') {
                            if (noteShifted || numberEntered) {
                                error(QB_ERROR_ILLEGAL_FUNCTION_CALL);
                                return;
                            }

                            noteShifted = true;
                            noteOffset++;

                            continue;
                        }

                    follow_up_1:
                        if (numberEntered) {
                            numberEntered = 0;

                            if (number < 0 || number > 64) {
                                error(QB_ERROR_ILLEGAL_FUNCTION_CALL);
                                return;
                            }

                            if (!number)
                                duration = noDotDuration;
                            else
                                duration = 1.0 / (tempo / 60.0) * (4.0 / ((double)number));
                        } else {
                            duration = noDotDuration;
                        }

                        auto dotDuration = duration;

                        for (auto i = 0; i < dots; i++) {
                            dotDuration /= 2.0;
                            duration += dotDuration;
                        }

                        dots = 0;

                        SetFrequency(pow(2.0, ((double)noteOffset) / 12.0) * 440.0);

                        auto noteFrames = ma_uint64(duration * ma_engine_get_sample_rate(rawStream->maEngine));

                        if (mixCursor + noteFrames > waveBuffer.size()) {
                            waveBuffer.resize(mixCursor + noteFrames, SILENCE_SAMPLE);
                        }

                        if (noteOffset > -45) // this ensures that we correctly handle N0 as rest
                            GenerateWaveform(duration * (1.0 - pause), mixCursor != waveBuffer.size());

                        if (currentChar != ',') {
                            mixCursor += noteFrames;
                        }

                        playIt = true;
                        noteShifted = false;
                        followUp = 0;

                        if (currentChar == ',')
                            continue;

                        if (currentState.length < 0)
                            break;
                    }

                    if (processedChar >= 'A' && processedChar <= 'G') {
                        switch (processedChar) {
                        case 'A':
                            noteOffset = 9;
                            break;
                        case 'B':
                            noteOffset = 11;
                            break;
                        case 'C':
                            noteOffset = 0;
                            break;
                        case 'D':
                            noteOffset = 2;
                            break;
                        case 'E':
                            noteOffset = 4;
                            break;
                        case 'F':
                            noteOffset = 5;
                            break;
                        case 'G':
                            noteOffset = 7;
                            break;
                        }
                        noteOffset = noteOffset + (octave - 2) * 12 - 9;
                        followUp = 1;
                        continue;
                    } else if (processedChar == 'L') { // length
                        followUp = 2;
                        continue;
                    } else if (processedChar == 'M') { // timing
                        followUp = 5;
                        continue;
                    } else if (processedChar == 'N') { // note 'n'
                        followUp = 4;
                        continue;
                    } else if (processedChar == 'O') { // octave
                        followUp = 3;
                        continue;
                    } else if (processedChar == 'T') { // tempo
                        followUp = 6;
                        continue;
                    } else if (processedChar == '<') { // octave --
                        --octave;
                        if (octave < 0)
                            octave = 0;
                        continue;
                    } else if (processedChar == '>') { // octave ++
                        ++octave;
                        if (octave > 6)
                            octave = 6;
                        continue;
                    } else if (processedChar == 'P' || processedChar == 'R') { // rest
                        followUp = 7;
                        continue;
                    } else if (processedChar == 'V') { // volume
                        followUp = 8;
                        continue;
                    } else if (processedChar == '@' || processedChar == 'W') { // waveform
                        followUp = 9;
                        continue;
                    } else if (processedChar == 'Q') { // vol-ramp
                        followUp = 10;
                        continue;
                    } else if (processedChar == '/') { // attack
                        followUp = 11;
                        continue;
                    } else if (processedChar == '\\') { // decay
                        followUp = 12;
                        continue;
                    } else if (processedChar == '^') { // sustain
                        followUp = 13;
                        continue;
                    } else if (processedChar == '_') { // release
                        followUp = 14;
                        continue;
                    } else if (processedChar == 'Y') { // waveform parameter
                        followUp = 15;
                        continue;
                    } else if (processedChar == 'S') { // pan position
                        followUp = 16;
                        continue;
                    }

                    error(QB_ERROR_ILLEGAL_FUNCTION_CALL);
                    return;
                }

                if (numberEntered || followUp) {
                    error(QB_ERROR_ILLEGAL_FUNCTION_CALL); // unhandled data
                    return;
                }

                if (playIt) {
                    PushBufferForPlayback();
                    AwaitPlaybackCompletion();
                }
            }
        }
    };

    /// @brief A QB64-PE audio engine sound handle internal struct. Describes every sound the system will ever play (including raw streams).
    struct SoundHandle {
        /// @brief Type of sound.
        /// NONE: No sound or internal sound whose buffer is managed by the QB64-PE audio engine.
        /// STATIC: Static sounds that are completely managed by miniaudio.
        /// RAW: Raw sound stream that is managed by the QB64-PE audio engine.
        enum class Type { NONE, STATIC, RAW };

        bool isUsed;                                // Is this handle in active use?
        Type type;                                  // Type of sound (see Type enum above)
        bool autoKill;                              // Do we need to auto-clean this sample / stream after playback is done?
        ma_sound maSound;                           // miniaudio sound
        ma_uint32 maFlags;                          // miniaudio flags that were used when initializing the sound
        uint64_t bufferKey;                         // a key that will uniquely identify the data the sound will use
        ma_audio_buffer_config maAudioBufferConfig; // miniaudio buffer configuration
        ma_audio_buffer *maAudioBuffer;             // this is used for user created audio buffers (memory is managed by miniaudio)
        RawStream *rawStream;                       // Raw sample frame queue
        PSG *psg;                                   // PSG object (if any) linked to this sound
        void *memLockOffset;                        // This is a pointer from new_mem_lock()
        uint64_t memLockId;                         // This is mem_lock_id created by new_mem_lock()

        // Delete copy and move constructors and assignments
        SoundHandle(const SoundHandle &) = delete;
        SoundHandle &operator=(const SoundHandle &) = delete;
        SoundHandle(SoundHandle &&) = delete;
        SoundHandle &operator=(SoundHandle &&) = delete;

        /// @brief Just initializes some important members. 'inUse' will be set to true by CreateHandle(). This is done here, as well as slightly differently in
        /// CreateHandle() for safety.
        SoundHandle() {
            isUsed = false;
            type = Type::NONE;
            autoKill = false;
            maSound = {};
            maFlags = MA_SOUND_FLAG_NO_PITCH | MA_SOUND_FLAG_NO_SPATIALIZATION | MA_SOUND_FLAG_WAIT_INIT;
            bufferKey = 0;
            maAudioBufferConfig = {};
            maAudioBuffer = nullptr;
            rawStream = nullptr;
            psg = nullptr;
            memLockOffset = nullptr;
            memLockId = INVALID_MEM_LOCK;
        }
    };

    static const auto INVALID_SOUND_HANDLE_INTERNAL = -1; // this is what is returned to the caller by CreateHandle() if handle allocation fails
    static const auto INVALID_SOUND_HANDLE = 0; // this should be returned to the caller by top-level sound APIs if a handle allocation fails with a -1
    static const auto PSG_VOICES = 4;           // this is the number of PSG objects that we will use

    bool isInitialized;                                 // this is set to true if we were able to initialize miniaudio and allocated all required resources
    bool initializationFailed;                          // this is set to true if a past initialization attempt failed
    ma_resource_manager_config maResourceManagerConfig; // miniaudio resource manager configuration
    ma_resource_manager maResourceManager;              // miniaudio resource manager
    ma_engine_config maEngineConfig;                    // miniaudio engine configuration (will be used to pass in the resource manager)
    ma_engine maEngine;                                 // this is the primary miniaudio engine 'context'. Everything happens using this!
    ma_result maResult;                                 // this is the result of the last miniaudio operation (used for trapping errors)
    std::array<int32_t, PSG_VOICES> psgVoices;          // internal sound handles that we will use for Play() and Sound()
    int32_t internalSndRaw;                             // internal sound handle that we will use for the QB64 'handle-less' raw stream
    std::vector<SoundHandle *> soundHandles;            // this is the audio handle list used by the engine and by everything else
    int32_t lowestFreeHandle;                           // this is the lowest handle then was recently freed. We'll start checking for free handles from here
    BufferMap bufferMap;                                // this is used to keep track of and manage memory used by 'in-memory' sound files
    ma_vfs *vfs;                                        // this is an ma_vfs backed by the BufferMap

    // Delete copy and move constructors and assignments
    AudioEngine(const AudioEngine &) = delete;
    AudioEngine &operator=(const AudioEngine &) = delete;
    AudioEngine &operator=(AudioEngine &&) = delete;
    AudioEngine(AudioEngine &&) = delete;

    /// @brief Initializes some important members.
    AudioEngine() {
        isInitialized = initializationFailed = false;
        maResourceManagerConfig = {};
        maResourceManager = {};
        maEngineConfig = {};
        maEngine = {};
        maResult = ma_result::MA_SUCCESS;
        psgVoices.fill(INVALID_SOUND_HANDLE_INTERNAL);  // should not use INVALID_SOUND_HANDLE here
        internalSndRaw = INVALID_SOUND_HANDLE_INTERNAL; // should not use INVALID_SOUND_HANDLE here
        lowestFreeHandle = 0;
        vfs = nullptr;
    }

    /// @brief Allocates a sound handle. It will return -1 on error. Handle 0 is used internally for Sound and Play and thus cannot be used by the user.
    /// Basically, we go through the vector and find an object pointer were 'isUsed' is set as false and return the index. If such an object pointer is not
    /// found, then we add a pointer to a new object at the end of the vector and return the index. We are using pointers because miniaudio keeps using stuff
    /// from ma_sound and these cannot move in memory when the vector is resized. The handle is put-up for recycling simply by setting the 'isUsed' member to
    /// false. Note that this means the vector will keep growing until the largest handle (index) and never shrink. The vector will be pruned only when
    /// snd_un_init gets called. This also increments 'lowestFreeHandle' to allocated handle + 1.
    /// @return Returns a non-negative handle if successful.
    int32_t CreateHandle() {
        if (!isInitialized) {
            return INVALID_SOUND_HANDLE_INTERNAL; // We cannot return 0 here. Since 0 is a valid internal handle
        }

        size_t h, vectorSize = soundHandles.size(); // Save the vector size

        // Scan the vector starting from lowestFreeHandle
        // This will help us quickly allocate a free handle and should be a decent optimization for SndPlayCopy()
        for (h = lowestFreeHandle; h < vectorSize; h++) {
            if (!soundHandles[h]->isUsed) {
                AUDIO_DEBUG_PRINT("Recent sound handle %zu recycled", h);
                break;
            }
        }

        if (h >= vectorSize) {
            // Scan through the entire vector and return a slot that is not being used
            // Ideally this should execute in extremely few (if at all) scenarios
            // Also, this loop should not execute if size is 0
            for (h = 0; h < vectorSize; h++) {
                if (!soundHandles[h]->isUsed) {
                    AUDIO_DEBUG_PRINT("Sound handle %zu recycled", h);
                    break;
                }
            }
        }

        if (h >= vectorSize) {
            // If we have reached here then either the vector is empty or there are no empty slots
            // Simply create a new SoundHandle at the back of the vector
            auto newHandle = new SoundHandle;

            if (!newHandle)
                return INVALID_SOUND_HANDLE_INTERNAL; // We cannot return 0 here. Since 0 is a valid internal handle

            soundHandles.push_back(newHandle);
            auto newVectorSize = soundHandles.size();

            // If newVectorSize == vectorSize then push_back() failed
            if (newVectorSize <= vectorSize) {
                delete newHandle;
                return INVALID_SOUND_HANDLE_INTERNAL; // We cannot return 0 here. Since 0 is a valid internal handle
            }

            h = newVectorSize - 1; // The handle is simply newVectorSize - 1

            AUDIO_DEBUG_PRINT("Sound handle %zu created", h);
        }

        AUDIO_DEBUG_CHECK(soundHandles[h]->isUsed == false);

        // Initializes a sound handle that was just allocated.
        // This will set it to 'in use' after applying some defaults.
        soundHandles[h]->type = SoundHandle::Type::NONE;
        soundHandles[h]->autoKill = false;
        soundHandles[h]->maSound = {};
        // We do not use pitch shifting, so this will give a little performance boost
        // Spatialization is disabled by default but will be enabled on the fly if required
        soundHandles[h]->maFlags = MA_SOUND_FLAG_NO_PITCH | MA_SOUND_FLAG_NO_SPATIALIZATION | MA_SOUND_FLAG_WAIT_INIT;
        soundHandles[h]->bufferKey = 0;
        soundHandles[h]->maAudioBuffer = nullptr;
        soundHandles[h]->rawStream = nullptr;
        soundHandles[h]->psg = nullptr;
        soundHandles[h]->memLockId = INVALID_MEM_LOCK;
        soundHandles[h]->memLockOffset = nullptr;
        soundHandles[h]->isUsed = true;

        AUDIO_DEBUG_PRINT("Sound handle %zu returned", h);

        lowestFreeHandle = h + 1; // Set lowestFreeHandle to allocated handle + 1

        return int32_t(h);
    }

    /// @brief Frees and unloads an open sound. If the sound is playing or looping, it will be stopped. If the sound is a stream of raw samples then it is
    /// stopped and freed. Finally the handle is invalidated and put-up for recycling. If the handle being freed is lower than 'lowestFreeHandle' then this
    /// saves the handle to 'lowestFreeHandle'.
    /// @param handle A sound handle.
    void ReleaseHandle(int32_t handle) {
        if (isInitialized && handle >= 0 && handle < soundHandles.size() && soundHandles[handle]->isUsed) {
            // Free any initialized miniaudio sound
            if (soundHandles[handle]->type == SoundHandle::Type::STATIC) {
                ma_sound_uninit(&soundHandles[handle]->maSound);

                AUDIO_DEBUG_PRINT("Sound uninitialized");
            }

            // Free any initialized raw stream
            RawStream::Destroy(soundHandles[handle]->rawStream);
            soundHandles[handle]->rawStream = nullptr;

            // Free any initialized PSG
            delete soundHandles[handle]->psg;
            soundHandles[handle]->psg = nullptr;

            // Free any initialized audio buffer
            if (soundHandles[handle]->maAudioBuffer) {
                ma_audio_buffer_uninit_and_free(soundHandles[handle]->maAudioBuffer);
                soundHandles[handle]->maAudioBuffer = nullptr;

                AUDIO_DEBUG_PRINT("Audio buffer uninitialized & freed");
            }

            // Invalidate any memsound stuff
            if (soundHandles[handle]->memLockOffset) {
                free_mem_lock((mem_lock *)soundHandles[handle]->memLockOffset);
                soundHandles[handle]->memLockId = INVALID_MEM_LOCK;
                soundHandles[handle]->memLockOffset = nullptr;

                AUDIO_DEBUG_PRINT("MemSound stuff invalidated");
            }

            // Release buffer added by _SNDOPEN
            if (soundHandles[handle]->bufferKey) {
                AUDIO_DEBUG_PRINT("Releasing buffer %llu", soundHandles[handle]->bufferKey);

                bufferMap.ReleaseBuffer(soundHandles[handle]->bufferKey);
                soundHandles[handle]->bufferKey = 0;
            }

            // Now simply set the 'isUsed' member to false so that the handle can be recycled
            soundHandles[handle]->isUsed = false;
            soundHandles[handle]->type = SoundHandle::Type::NONE;

            // Save the free handle to lowestFreeHandle if it is lower than lowestFreeHandle
            if (handle < lowestFreeHandle) {
                lowestFreeHandle = handle;
            }

            AUDIO_DEBUG_PRINT("Sound handle %i marked as free", handle);
        }
    }

    /// @brief Checks if a user sound handle is valid.
    /// @param handle A sound handle.
    /// @return Returns true if the handle is valid.
    bool IsHandleValid(int32_t handle) {
        return handle > 0 && handle < int32_t(soundHandles.size()) && soundHandles[handle]->isUsed && !soundHandles[handle]->autoKill;
    }

    /// @brief Initializes the first PSG object and it's RawStream object. This only happens once. Subsequent calls to this will return true.
    /// @return Returns true if both objects were successfully created.
    bool InitializePSG(size_t voice) {
        if (!isInitialized || voice >= PSG_VOICES) {
            return false;
        }

        if (voice) {
            // Create and reserve resources for any non-primary voice
            if (!IsHandleValid(psgVoices[voice])) {
                AUDIO_DEBUG_PRINT("Setting up PSG for voice %zu", voice);

                psgVoices[voice] = func__sndopenraw();

                if (!IsHandleValid(psgVoices[voice])) {
                    AUDIO_DEBUG_PRINT("Failed to create raw sound stream for voice %zu", voice);

                    psgVoices[voice] = INVALID_SOUND_HANDLE_INTERNAL; // we cannot use INVALID_SOUND_HANDLE here

                    return false;
                }

                if (!soundHandles[psgVoices[voice]]->psg) {
                    AUDIO_DEBUG_PRINT("Creating PSG object for voice %zu", voice);

                    soundHandles[psgVoices[voice]]->psg = new PSG(soundHandles[psgVoices[voice]]->rawStream);

                    if (!soundHandles[psgVoices[voice]]->psg) {
                        AUDIO_DEBUG_PRINT("Failed to create PSG object for voice %zu", voice);

                        // Cleanup
                        sub__sndclose(psgVoices[voice]);
                        ReleaseHandle(psgVoices[voice]);                  // we'll clean this up ourself
                        psgVoices[voice] = INVALID_SOUND_HANDLE_INTERNAL; // we cannot use INVALID_SOUND_HANDLE here

                        return false;
                    }
                }
            }
        } else {
            // Special case handle 0: Create and reserve resources for the primary PSG
            if (psgVoices[0] != 0) {
                AUDIO_DEBUG_PRINT("Primary PSG sound handle not reserved");

                return false;
            }

            if (!soundHandles[psgVoices[0]]->rawStream) {
                // Special case: create and kickstart the primary raw stream and PSG if it is not already
                AUDIO_DEBUG_PRINT("Creating rawStream object for primary PSG");

                soundHandles[psgVoices[0]]->rawStream = RawStream::Create(&maEngine, &soundHandles[psgVoices[0]]->maSound);

                if (!soundHandles[psgVoices[0]]->rawStream) {
                    AUDIO_DEBUG_PRINT("Failed to create rawStream object for primary PSG");

                    return false;
                }

                soundHandles[psgVoices[0]]->type = SoundHandle::Type::RAW;

                if (!soundHandles[psgVoices[0]]->psg) {
                    AUDIO_DEBUG_PRINT("Creating primary PSG object");

                    soundHandles[psgVoices[0]]->psg = new PSG(soundHandles[psgVoices[0]]->rawStream);

                    if (!soundHandles[psgVoices[0]]->psg) {
                        AUDIO_DEBUG_PRINT("Failed to create primary PSG object");

                        // Cleanup
                        RawStream::Destroy(soundHandles[psgVoices[0]]->rawStream);
                        soundHandles[psgVoices[0]]->rawStream = nullptr;
                        soundHandles[psgVoices[0]]->type = SoundHandle::Type::NONE;

                        return false;
                    }
                }
            }
        }

        return (soundHandles[psgVoices[voice]]->rawStream && soundHandles[psgVoices[voice]]->psg);
    }

    /// @brief Shuts down and frees resources allocated by any running PSG.
    void ShutDownPSGs() {
        // Special case for primary PSG
        if (soundHandles[psgVoices[0]]->psg) {
            soundHandles[psgVoices[0]]->psg->Pause(false);       // unpause the PSG
            soundHandles[psgVoices[0]]->rawStream->Pause(false); // unpause the RawStream
            soundHandles[psgVoices[0]]->rawStream->stop = true;  // signal miniaudio thread that we are going to end playback
        }

        // Release the sound handles
        for (auto snd : psgVoices) {
            sub__sndclose(snd);
        }
    }

    /// @brief Pauses all running PSGs and also their RawStreams (if nothing is playing).
    void PausePSGs() {
        AUDIO_DEBUG_PRINT("Pausing all PSGs");

        bool psgsIdle = true;

        for (auto i = 0; i < AudioEngine::PSG_VOICES; i++) {
            if (InitializePSG(i)) {
                // Only proceed if the underlying PSG is initialized
                AUDIO_DEBUG_PRINT("Pausing PSG %d (handle %d)", i, psgVoices[i]);
                soundHandles[psgVoices[i]]->psg->Pause(true);

                if (soundHandles[psgVoices[i]]->rawStream->GetSampleFramesRemaining()) {
                    psgsIdle = false;
                }
            }
        }

        if (psgsIdle) {
            // This enables better voice syncronization when all voices are activated and nothing is playing
            AUDIO_DEBUG_PRINT("Pausing all PSG RawStreams");

            for (auto i = 0; i < AudioEngine::PSG_VOICES; i++) {
                if (InitializePSG(i)) {
                    // Only proceed if the underlying PSG is initialized
                    AUDIO_DEBUG_PRINT("Pausing RawStream %d (handle %d)", i, psgVoices[i]);
                    soundHandles[psgVoices[i]]->rawStream->Pause(true);
                }
            }
        }
    }

    /// @brief Resumes all paused PSGs and also their RawStreams.
    void ResumePSGs() {
        AUDIO_DEBUG_PRINT("Resuming all PSGs");

        for (auto i = 0; i < AudioEngine::PSG_VOICES; i++) {
            if (InitializePSG(i)) {
                // Only proceed if the underlying PSG is initialized
                AUDIO_DEBUG_PRINT("Resuming PSG %d (handle %d)", i, psgVoices[i]);
                soundHandles[psgVoices[i]]->psg->Pause(false); // this can take time due to the underlying buffer copying
            }
        }

        AUDIO_DEBUG_PRINT("Resuming all PSG RawStreams");

        for (auto i = 0; i < AudioEngine::PSG_VOICES; i++) {
            if (InitializePSG(i)) {
                // Only proceed if the underlying PSG is initialized
                AUDIO_DEBUG_PRINT("Resuming RawStream %d (handle %d)", i, psgVoices[i]);
                soundHandles[psgVoices[i]]->rawStream->Pause(false);
            }
        }
    }

    /// @brief Initializes the audio subsystem. We simply attempt to initialize and then set some globals with the results.
    void Initialize() {
        // Exit if engine is initialize or already initialization was attempted but failed
        if (isInitialized || initializationFailed)
            return;

        // Create VFS
        vfs = VFS::Create(&bufferMap);

        // Initialize the miniaudio resource manager
        maResourceManagerConfig = ma_resource_manager_config_init();
        AudioEngine_AttachCustomBackendVTables(&maResourceManagerConfig);
        maResourceManagerConfig.pCustomDecodingBackendUserData = NULL; // <- pUserData parameter of each function in the decoding backend vtables
        maResourceManagerConfig.pVFS = vfs;

        maResult = ma_resource_manager_init(&maResourceManagerConfig, &maResourceManager);
        if (maResult != MA_SUCCESS) {
            initializationFailed = true;
            AUDIO_DEBUG_PRINT("Failed to initialize miniaudio resource manager");
            return;
        }

        // Once we have a resource manager we can create the engine
        maEngineConfig = ma_engine_config_init();
        maEngineConfig.pResourceManager = &maResourceManager;

        // Attempt to initialize with miniaudio defaults
        maResult = ma_engine_init(&maEngineConfig, &maEngine);
        // If failed, then set the global flag so that we don't attempt to initialize again
        if (maResult != MA_SUCCESS) {
            ma_resource_manager_uninit(&maResourceManager);
            initializationFailed = true;
            AUDIO_DEBUG_PRINT("miniaudio initialization failed");
            return;
        }

        // Set the resource manager decoder sample rate to the device sample rate (miniaudio engine bug?)
        maResourceManager.config.decodedSampleRate = ma_engine_get_sample_rate(&maEngine);

        // Set the initialized flag as true
        isInitialized = true;

        AUDIO_DEBUG_PRINT("Audio engine initialized @ %uHz", ma_engine_get_sample_rate(&maEngine));

        // Reserve sound handle 0 so that nothing else can use it
        // We'll kickstart it later when we need it
        // We will use this handle internally for Play() and Sound() etc.
        psgVoices[0] = CreateHandle();
        AUDIO_DEBUG_CHECK(psgVoices[0] == 0); // The first handle must return 0 and this is what is used by Play and Sound
    }

    /// @brief Shuts down the audio engine and frees any resources used.
    void ShutDown() {
        if (isInitialized) {
            // Shut down all PSGs
            ShutDownPSGs();

            // Free all sound handles here
            for (size_t handle = 0; handle < soundHandles.size(); handle++) {
                ReleaseHandle(handle);       // let ReleaseHandle do its thing
                delete soundHandles[handle]; // now free the object created by CreateHandle()
            }

            // Now that all sounds are closed and SoundHandle objects are freed, clear the vector
            soundHandles.clear();

            // Invalidate internal handles
            psgVoices.fill(INVALID_SOUND_HANDLE_INTERNAL);
            internalSndRaw = INVALID_SOUND_HANDLE_INTERNAL;

            // Shutdown miniaudio
            ma_engine_uninit(&maEngine);

            // Shutdown the miniaudio resource manager
            ma_resource_manager_uninit(&maResourceManager);

            AUDIO_DEBUG_PRINT("Audio engine shutdown");

            // Destroy VFS
            VFS::Destroy(vfs);

            // Set engine initialized flag as false
            isInitialized = false;
        }
    }

    /// @brief Used for housekeeping and other stuff. Called by the QB64-PE internally at ~60Hz.
    void Update() {
        if (isInitialized) {
            // Scan through the whole handle vector to find anything we need to update or close
            for (size_t handle = 0; handle < soundHandles.size(); handle++) {
                // Only process handles that are in use
                if (soundHandles[handle]->isUsed) {
                    // Look for stuff that is set to auto-destruct
                    if (soundHandles[handle]->autoKill) {
                        switch (soundHandles[handle]->type) {
                        case SoundHandle::Type::STATIC:
                        case SoundHandle::Type::RAW:
                            // Dispose the sound if it has finished playing
                            // Note that this means that temporary looping sounds will never close
                            // Well that's on the programmer. Probably they want it that way
                            if (!ma_sound_is_playing(&soundHandles[handle]->maSound))
                                ReleaseHandle(handle);

                            break;

                        case SoundHandle::Type::NONE:
                            if (handle != 0)
                                AUDIO_DEBUG_PRINT("Sound type is 'None' when handle value is not 0");

                            break;

                        default:
                            AUDIO_DEBUG_PRINT("Condition not handled"); // It should not come here
                        }
                    }
                }
            }
        }
    }
};

/// @brief Raw stream data source vtable.
const ma_data_source_vtable AudioEngine::RawStream::rawStreamDataSourceVtable = {
    AudioEngine::RawStream::OnRead,          // Returns a bunch of samples from a raw sample stream queue. The samples being returned is removed from the queue
    AudioEngine::RawStream::OnSeek,          // NOP for raw sample stream
    AudioEngine::RawStream::OnGetDataFormat, // Returns the audio format to miniaudio
    NULL,                                    // No notion of a cursor for raw sample stream
    NULL,                                    // No notion of a length for raw sample stream
    NULL,                                    // Cannot loop raw sample stream
    0                                        // flags
};

// Global audio engine object.
static AudioEngine audioEngine;

/// @brief Returns the time left to play when Play() and Sound() are used.
/// @param voice The voice to get the information for (0 to 3).
/// @param passed Optional parameter flags.
/// @return Time (in seconds).
double func_play(uint32_t voice, int32_t passed) {
    if (is_error_pending()) {
        return 0.0;
    }

    // Default to voice 0 if voice is out of range
    voice = (passed && voice < AudioEngine::PSG_VOICES) ? voice : 0;

    if (audioEngine.InitializePSG(voice)) {
        // Only proceed if the underlying PSG is initialized
        return audioEngine.soundHandles[audioEngine.psgVoices[voice]]->rawStream->GetTimeRemaining();
    }

    return 0.0;
}

/// @brief Processes and plays the MML specified in the strings.
/// @param str1 MML string for voice 0.
/// @param str2 MML string for voice 1.
/// @param str3 MML string for voice 2.
/// @param str4 MML string for voice 3.
/// @param passed Optional parameter flags.
void sub_play(const qbs *str1, const qbs *str2, const qbs *str3, const qbs *str4, int32_t passed) {
    if (is_error_pending()) {
        return;
    }

    if (passed) {
        // Multi-voice mode
        AUDIO_DEBUG_PRINT("Starting multi-voice MML playback");

        audioEngine.PausePSGs();

        if ((passed & 4) && audioEngine.InitializePSG(3)) {
            AUDIO_DEBUG_PRINT("Mixing MML voice 4");

            audioEngine.soundHandles[audioEngine.psgVoices[3]]->psg->Play(str4);
        }

        if ((passed & 2) && audioEngine.InitializePSG(2)) {
            AUDIO_DEBUG_PRINT("Mixing MML voice 3");

            audioEngine.soundHandles[audioEngine.psgVoices[2]]->psg->Play(str3);
        }

        if ((passed & 1) && audioEngine.InitializePSG(1)) {
            AUDIO_DEBUG_PRINT("Mixing MML voice 2");

            audioEngine.soundHandles[audioEngine.psgVoices[1]]->psg->Play(str2);
        }

        if (audioEngine.InitializePSG(0)) {
            AUDIO_DEBUG_PRINT("Mixing MML voice 1");

            audioEngine.soundHandles[audioEngine.psgVoices[0]]->psg->Play(str1);
        }

        audioEngine.ResumePSGs();
    } else {
        // Legacy single-voice mode
        if (audioEngine.InitializePSG(0)) {
            audioEngine.soundHandles[audioEngine.psgVoices[0]]->psg->Play(str1);
        }
    }
}

/// @brief Generates a sound at the specified frequency for the specified amount of time.
/// @param frequency Sound frequency.
/// @param lengthInClockTicks Duration in clock ticks. There are 18.2 clock ticks per second.
/// @param volume Volume (0.0 to 1.0).
/// @param panPosition Pan position (-1.0 to 1.0).
/// @param waveform Waveform (1=Square, 2=Saw, 3=Triangle, 4=Sine, 5=White Noise, 6=Pink Noise, 7=Brown Noise, 8=LFSR Noise, 9=Pulse).
/// @param waveformParam Waveform parameter (if applicable).
/// @param voice The voice to use (0 to 3).
/// @param option WAIT=1 or RESUME=2
/// @param passed Optional parameter flags.
void sub_sound(float frequency, float lengthInClockTicks, float volume, float panPosition, int32_t waveform, float waveformParam, uint32_t voice,
               int32_t option, int32_t passed) {
    if (is_error_pending()) {
        return;
    }

    AUDIO_DEBUG_PRINT("option = %d, passed = %d", option, passed);

    if (!option && !passed) {
        error(QB_ERROR_ILLEGAL_FUNCTION_CALL);

        return;
    }

    // Handle SOUND WAIT first if present
    if (option == 1) {
        audioEngine.PausePSGs();
    }

    // Handle regular sound arguments if they are present
    if (passed & 1) {
        if (frequency == 0.0f) {
            frequency = AudioEngine::PSG::FREQUENCY_LIMIT; // this forces a frequency of 0.0 to be treated as a silent sound
        }

        // Validate mandatory parameters
        if (frequency < AudioEngine::PSG::FREQUENCY_MIN || frequency > AudioEngine::PSG::FREQUENCY_MAX || lengthInClockTicks < 0.0f ||
            lengthInClockTicks > 65535.0f) {
            error(QB_ERROR_ILLEGAL_FUNCTION_CALL);

            return;
        }

        if (passed & 32) {
            AUDIO_DEBUG_PRINT("Voice specified (%u)", voice);

            if (voice >= AudioEngine::PSG_VOICES) {
                error(QB_ERROR_ILLEGAL_FUNCTION_CALL);

                return;
            }
        } else {
            voice = 0;
        }

        AUDIO_DEBUG_PRINT("Using voice %u", voice);

        if (!audioEngine.InitializePSG(voice)) {
            AUDIO_DEBUG_PRINT("Failed to initialize PSG for voice %u", voice);

            return;
        }

        if (passed & 2) {
            AUDIO_DEBUG_PRINT("Volume specified (%f)", volume);

            if (volume < AudioEngine::PSG::VOLUME_MIN || volume > AudioEngine::PSG::VOLUME_MAX) {
                error(QB_ERROR_ILLEGAL_FUNCTION_CALL);

                return;
            }
            audioEngine.soundHandles[audioEngine.psgVoices[voice]]->psg->SetAmplitude(volume);
        }

        if (passed & 4) {
            AUDIO_DEBUG_PRINT("Pan position specified (%f)", panPosition);

            if (panPosition < AudioEngine::PSG::PAN_LEFT || panPosition > AudioEngine::PSG::PAN_RIGHT) {
                error(QB_ERROR_ILLEGAL_FUNCTION_CALL);

                return;
            }
            audioEngine.soundHandles[audioEngine.psgVoices[voice]]->psg->SetPanPosition(panPosition);
        }

        if (passed & 8) {
            AUDIO_DEBUG_PRINT("Waveform specified (%i)", waveform);

            if ((AudioEngine::PSG::WaveformType)waveform <= AudioEngine::PSG::WaveformType::NONE ||
                (AudioEngine::PSG::WaveformType)waveform >= AudioEngine::PSG::WaveformType::COUNT) {
                error(QB_ERROR_ILLEGAL_FUNCTION_CALL);

                return;
            }
            audioEngine.soundHandles[audioEngine.psgVoices[voice]]->psg->SetWaveformType((AudioEngine::PSG::WaveformType)waveform);
        }

        if (passed & 16) {
            AUDIO_DEBUG_PRINT("Waveform parameter specified (%f)", waveformParam);

            if (((AudioEngine::PSG::WaveformType)waveform == AudioEngine::PSG::WaveformType::PULSE) &&
                (waveformParam < AudioEngine::PSG::PULSE_WAVE_DUTY_CYCLE_MIN || waveformParam > AudioEngine::PSG::PULSE_WAVE_DUTY_CYCLE_MAX)) {
                error(QB_ERROR_ILLEGAL_FUNCTION_CALL);

                return;
            }
            audioEngine.soundHandles[audioEngine.psgVoices[voice]]->psg->SetWaveformParameter(waveformParam);
        }

        if (lengthInClockTicks > 0.0f) {
            AUDIO_DEBUG_PRINT("Frequency = %f, duration = %f", frequency, lengthInClockTicks);

            // Generate the sound only if we have a positive non-zero duration
            audioEngine.soundHandles[audioEngine.psgVoices[voice]]->psg->Sound(frequency, lengthInClockTicks);
        }
    }

    // Handle SOUND RESUME at the end once everything else is processed and we are ready to go
    if (option == 2) {
        audioEngine.ResumePSGs();
    }
}

/// @brief Defines the shape of a custom sound wave for a specified SOUND/PLAY voice.
/// @param voice The voice number (0 to 4).
/// @param waveDefinition A pointer to a QB64 _BYTE array.
/// @param frameCount The number of frames to use from the array.
/// @param passed Optional parameter flags.
void sub__wave(uint32_t voice, void *waveDefinition, uint32_t frameCount, int32_t passed) {
    if (is_error_pending()) {
        return;
    }

    if (voice >= AudioEngine::PSG_VOICES) {
        AUDIO_DEBUG_PRINT("Invalid voice = %u", voice);

        error(QB_ERROR_ILLEGAL_FUNCTION_CALL);

        return;
    }

    auto audioBufferFrames = uint32_t((reinterpret_cast<byte_element_struct *>(waveDefinition))->length);

    if (passed) {
        if (frameCount > audioBufferFrames) {
            AUDIO_DEBUG_PRINT("Adjusting frame count from %u to %u\n", frameCount, audioBufferFrames);

            frameCount = audioBufferFrames;
        }
    } else {
        frameCount = audioBufferFrames;
    }

    if (audioBufferFrames < AudioEngine::PSG::CUSTOM_WAVEFORM_FRAMES_MIN || frameCount < AudioEngine::PSG::CUSTOM_WAVEFORM_FRAMES_MIN) {
        AUDIO_DEBUG_PRINT("Audio buffer too small. audioBufferFrames = %u, frameCount = %u", audioBufferFrames, frameCount);

        error(QB_ERROR_ILLEGAL_FUNCTION_CALL);

        return;
    }

    auto waveBuffer = reinterpret_cast<int8_t *>((reinterpret_cast<byte_element_struct *>(waveDefinition))->offset);

    if (audioEngine.InitializePSG(voice)) {
        // Only proceed if the underlying PSG is initialized
        AUDIO_DEBUG_PRINT("Storing waveform in voice %u (handle %d)", voice, audioEngine.psgVoices[voice]);
        audioEngine.soundHandles[audioEngine.psgVoices[voice]]->psg->SetCustomWaveform(waveBuffer, frameCount);
    }
}

/// @brief Returns the device sample rate if the audio engine is initialized.
/// @return The device sample rate.
int32_t func__sndrate() {
    return ma_engine_get_sample_rate(&audioEngine.maEngine);
}

/// @brief Loads a sound file into memory and returns a LONG handle value above 0.
/// @param qbsFileName The is the pathname for the sound file. This can be any format that miniaudio or a miniaudio plugin supports.
/// @param qbsRequirements This is leftover from the old QB64-SDL days. But we use this to pass some parameters like 'stream'.
/// @param passed Optional parameter flags.
/// @return Returns a valid sound handle (> 0) if successful or 0 if it fails.
int32_t func__sndopen(qbs *qbsFileName, qbs *qbsRequirements, int32_t passed) {
    if (!audioEngine.isInitialized || !qbsFileName->len)
        return AudioEngine::INVALID_SOUND_HANDLE;

    // Allocate a sound handle
    auto handle = audioEngine.CreateHandle();
    if (handle < 1)
        return AudioEngine::INVALID_SOUND_HANDLE;

    // Set some sound default properties
    audioEngine.soundHandles[handle]->type = AudioEngine::SoundHandle::Type::STATIC; // set the sound type to static by default
    audioEngine.soundHandles[handle]->maFlags |= MA_SOUND_FLAG_DECODE;               // set the sound to decode completely first before playing (QB64 default)
    audioEngine.soundHandles[handle]->maFlags |= MA_SOUND_FLAG_ASYNC;                // set the sound to decode asynchronously by default
    auto fromMemory = false;                                                         // we'll assume we are loading the sound from disk

    AUDIO_DEBUG_PRINT("Sound set to fully decode asynchronously");

    if (passed && qbsRequirements->len) {
        // Parse all flags in one go
        std::string requirements(reinterpret_cast<char const *>(qbsRequirements->chr), qbsRequirements->len);
        std::transform(requirements.begin(), requirements.end(), requirements.begin(), ::tolower);

        // Check if user wants to set the stream flag
        if (requirements.find("stream") != std::string::npos) {
            audioEngine.soundHandles[handle]->maFlags |= MA_SOUND_FLAG_STREAM;
            AUDIO_DEBUG_PRINT("Sound will be streamed");
        }

        // Check if the user wants to unset the decode flag
        if (requirements.find("nodecode") != std::string::npos) {
            audioEngine.soundHandles[handle]->maFlags &= ~MA_SOUND_FLAG_DECODE;
            AUDIO_DEBUG_PRINT("Sound will not be decoded on load");
        }

        // Check if the user wants to unset the async flag
        if (requirements.find("noasync") != std::string::npos) {
            audioEngine.soundHandles[handle]->maFlags &= ~MA_SOUND_FLAG_ASYNC;
            AUDIO_DEBUG_PRINT("Sound will not be decoded asynchronously");
        }

        // Check for memory flag
        if (requirements.find("memory") != std::string::npos) {
            fromMemory = true;
            AUDIO_DEBUG_PRINT("Sound will be loaded from memory");
        }
    }

    // Load the file from file or memory based on the requirements string
    if (fromMemory) {
        // Make a unique key and save it
        audioEngine.soundHandles[handle]->bufferKey =
            std::hash<std::string_view>{}(std::string_view(reinterpret_cast<const char *>(qbsFileName->chr), qbsFileName->len));

        // Make a copy of the buffer
        if (!audioEngine.bufferMap.AddBuffer(qbsFileName->chr, qbsFileName->len, audioEngine.soundHandles[handle]->bufferKey)) {
            AUDIO_DEBUG_PRINT("Failed to add buffer to buffer map");

            goto handle_cleanup;
        }

        // Convert the buffer key to a string
        auto fname = std::to_string(audioEngine.soundHandles[handle]->bufferKey);

        // Create the ma_sound
        audioEngine.maResult = ma_sound_init_from_file(&audioEngine.maEngine, fname.c_str(), audioEngine.soundHandles[handle]->maFlags, NULL, NULL,
                                                       &audioEngine.soundHandles[handle]->maSound);
    } else {
        std::string fileName(reinterpret_cast<char const *>(qbsFileName->chr), qbsFileName->len);

        if (audioEngine.soundHandles[handle]->maFlags & MA_SOUND_FLAG_STREAM) {
            AUDIO_DEBUG_PRINT("Streaming sound from file '%s'", fileName.c_str());

            audioEngine.maResult = ma_sound_init_from_file(&audioEngine.maEngine, fileName.c_str(), audioEngine.soundHandles[handle]->maFlags, NULL, NULL,
                                                           &audioEngine.soundHandles[handle]->maSound); // create the ma_sound
        } else {
            AUDIO_DEBUG_PRINT("Loading sound from file '%s'", fileName.c_str());

            auto contents = AudioEngine_LoadFile<std::vector<uint8_t>>(fileName.c_str());

            if (contents.empty()) {
                AUDIO_DEBUG_PRINT("Failed to open sound file '%s'", fileName.c_str());

                goto handle_cleanup;
            }

            AUDIO_DEBUG_PRINT("Sound length: %zu", contents.size());

            // Make a unique key and save it
            audioEngine.soundHandles[handle]->bufferKey =
                std::hash<std::string_view>{}(std::string_view(reinterpret_cast<const char *>(contents.data()), contents.size()));

            // Make a copy of the buffer
            if (!audioEngine.bufferMap.AddBuffer(std::move(contents), audioEngine.soundHandles[handle]->bufferKey)) {
                AUDIO_DEBUG_PRINT("Failed to add buffer to buffer map");

                goto handle_cleanup;
            }

            // Convert the buffer key to a string
            auto fname = std::to_string(audioEngine.soundHandles[handle]->bufferKey);

            // Create the ma_sound
            audioEngine.maResult = ma_sound_init_from_file(&audioEngine.maEngine, fname.c_str(), audioEngine.soundHandles[handle]->maFlags, NULL, NULL,
                                                           &audioEngine.soundHandles[handle]->maSound);
        }
    }

    // If the sound failed to initialize, then free the handle and return INVALID_SOUND_HANDLE
    if (audioEngine.maResult != MA_SUCCESS) {
        AUDIO_DEBUG_PRINT("Error %i: failed to open sound", audioEngine.maResult);

        goto handle_cleanup;
    }

    AUDIO_DEBUG_PRINT("Sound successfully loaded");

    return handle;

handle_cleanup:
    audioEngine.soundHandles[handle]->isUsed = false;

    return AudioEngine::INVALID_SOUND_HANDLE;
}

/// @brief Frees and unloads an open sound.
/// If the sound is playing, it'll let it finish. Looping sounds will loop until the program is closed.
/// If the sound is a stream of raw samples then any remaining samples pending for playback will be sent to miniaudio and then the handle will be freed.
/// @param handle A valid sound handle.
void sub__sndclose(int32_t handle) {
    if (audioEngine.isInitialized && audioEngine.IsHandleValid(handle)) {
        if (audioEngine.soundHandles[handle]->rawStream) {
            audioEngine.soundHandles[handle]->rawStream->Pause(false); // unpause the stream
            audioEngine.soundHandles[handle]->rawStream->stop = true;  // signal miniaudio thread that we are going to end playback
        }

        // Simply set the autokill flag to true and let the sound loop handle disposing the sound
        audioEngine.soundHandles[handle]->autoKill = true;
    }
}

/// @brief Copies a sound to a new handle so that two or more of the same sound can be played at once.
/// @param src_handle A source sound handle.
/// @return A new sound handle if successful or 0 on failure.
int32_t func__sndcopy(int32_t src_handle) {
    // Check for all invalid cases
    if (!audioEngine.isInitialized || !audioEngine.IsHandleValid(src_handle) ||
        audioEngine.soundHandles[src_handle]->type != AudioEngine::SoundHandle::Type::STATIC)
        return AudioEngine::INVALID_SOUND_HANDLE;

    int32_t dst_handle = AudioEngine::INVALID_SOUND_HANDLE;

    // Miniaudio will not copy sounds attached to ma_audio_buffers so we'll handle the duplication ourselves
    // Sadly, since this involves a buffer copy there may be a delay before the sound can play (especially if the sound is lengthy)
    // The delay may be noticeable when _SNDPLAYCOPY is used multiple times on a _SNDNEW sound
    if (audioEngine.soundHandles[src_handle]->maAudioBuffer) {
        AUDIO_DEBUG_PRINT("Doing custom sound copy for ma_audio_buffer");

        auto frames = audioEngine.soundHandles[src_handle]->maAudioBuffer->ref.sizeInFrames;
        auto channels = audioEngine.soundHandles[src_handle]->maAudioBuffer->ref.channels;
        auto format = audioEngine.soundHandles[src_handle]->maAudioBuffer->ref.format;

        // First create a new _SNDNEW sound with the same properties at the source
        dst_handle = func__sndnew(frames, channels, CHAR_BIT * ma_get_bytes_per_sample(format));
        if (dst_handle < 1)
            return AudioEngine::INVALID_SOUND_HANDLE;

        // Next memcopy the samples from the source to the dest
        memcpy((void *)audioEngine.soundHandles[dst_handle]->maAudioBuffer->ref.pData, audioEngine.soundHandles[src_handle]->maAudioBuffer->ref.pData,
               frames * ma_get_bytes_per_frame(format, channels)); // naughty const void* casting, but should be OK
    } else {
        AUDIO_DEBUG_PRINT("Doing regular miniaudio sound copy");

        dst_handle = audioEngine.CreateHandle(); // allocate a sound handle
        if (dst_handle < 1)
            return AudioEngine::INVALID_SOUND_HANDLE;

        audioEngine.soundHandles[dst_handle]->type = AudioEngine::SoundHandle::Type::STATIC;           // set some handle properties
        audioEngine.soundHandles[dst_handle]->maFlags = audioEngine.soundHandles[src_handle]->maFlags; // copy the flags

        // Initialize a new copy of the sound
        audioEngine.maResult = ma_sound_init_copy(&audioEngine.maEngine, &audioEngine.soundHandles[src_handle]->maSound,
                                                  audioEngine.soundHandles[dst_handle]->maFlags, NULL, &audioEngine.soundHandles[dst_handle]->maSound);

        // If the sound failed to copy, then free the handle and return INVALID_SOUND_HANDLE
        if (audioEngine.maResult != MA_SUCCESS) {
            audioEngine.soundHandles[dst_handle]->isUsed = false;
            AUDIO_DEBUG_PRINT("Error %i: failed to copy sound", audioEngine.maResult);

            return AudioEngine::INVALID_SOUND_HANDLE;
        }

        // Reset any limit
        ma_sound_set_stop_time_in_pcm_frames(&audioEngine.soundHandles[dst_handle]->maSound, ~(ma_uint64)0);
    }

    return dst_handle;
}

/// @brief Plays a sound.
/// @param handle A sound handle.
void sub__sndplay(int32_t handle) {
    if (audioEngine.isInitialized && audioEngine.IsHandleValid(handle) && audioEngine.soundHandles[handle]->type == AudioEngine::SoundHandle::Type::STATIC) {
        // Reset position to zero only if we are playing and (not looping or we've reached the end of the sound)
        // This is based on the old OpenAl-soft code behavior
        if (ma_sound_is_playing(&audioEngine.soundHandles[handle]->maSound) &&
            (!ma_sound_is_looping(&audioEngine.soundHandles[handle]->maSound) || ma_sound_at_end(&audioEngine.soundHandles[handle]->maSound))) {
            audioEngine.maResult = ma_sound_seek_to_pcm_frame(&audioEngine.soundHandles[handle]->maSound, 0);
            AUDIO_DEBUG_CHECK(audioEngine.maResult == MA_SUCCESS);

            // Reset any limit
            ma_sound_set_stop_time_in_pcm_frames(&audioEngine.soundHandles[handle]->maSound, ~(ma_uint64)0);
        }

        // Kickstart playback
        audioEngine.maResult = ma_sound_start(&audioEngine.soundHandles[handle]->maSound);
        AUDIO_DEBUG_CHECK(audioEngine.maResult == MA_SUCCESS);

        // Stop looping the sound if it is
        if (ma_sound_is_looping(&audioEngine.soundHandles[handle]->maSound)) {
            ma_sound_set_looping(&audioEngine.soundHandles[handle]->maSound, MA_FALSE);
        }

        AUDIO_DEBUG_PRINT("Playing sound %i", handle);
    }
}

/// @brief Copies a sound, plays it, and automatically closes the copy.
/// @param src_handle A sound handle to copy.
/// @param volume The volume at which the sound should be played (0.0 - 1.0).
/// @param x x distance values go from left (negative) to right (positive).
/// @param y y distance values go from below (negative) to above (positive).
/// @param z z distance values go from behind (negative) to in front (positive).
/// @param passed Optional parameter flags.
void sub__sndplaycopy(int32_t src_handle, float volume, float x, float y, float z, int32_t passed) {
    // We are simply going to use sndcopy, then setup some stuff like volume and autokill and then use sndplay
    // We are not checking if the audio engine was initialized because if not we'll get an invalid handle anyway
    auto dst_handle = func__sndcopy(src_handle);

    AUDIO_DEBUG_PRINT("Source handle = %i, destination handle = %i", src_handle, dst_handle);

    // Check if we succeeded and then proceed
    if (dst_handle > 0) {
        // Set the volume if requested
        if (passed & 1)
            ma_sound_set_volume(&audioEngine.soundHandles[dst_handle]->maSound, volume);

        if (passed & 4 || passed & 8) {                                                                    // If y or z or both are passed
            ma_sound_set_spatialization_enabled(&audioEngine.soundHandles[dst_handle]->maSound, MA_TRUE);  // Enable 3D spatialization
            ma_sound_set_position(&audioEngine.soundHandles[dst_handle]->maSound, x, y, z);                // Use full 3D positioning
        } else if (passed & 2) {                                                                           // If x is passed
            ma_sound_set_spatialization_enabled(&audioEngine.soundHandles[dst_handle]->maSound, MA_FALSE); // Disable spatialization for better stereo sound
            ma_sound_set_pan_mode(&audioEngine.soundHandles[dst_handle]->maSound, ma_pan_mode_pan);        // Set true stereo panning
            ma_sound_set_pan(&audioEngine.soundHandles[dst_handle]->maSound, x);                           // Just use stereo panning
        }

        sub__sndplay(dst_handle);                              // play the sound
        audioEngine.soundHandles[dst_handle]->autoKill = true; // must be set after sub__sndplay

        AUDIO_DEBUG_PRINT("Playing sound copy %i: volume %f, 3D (%f, %f, %f)", dst_handle, volume, x, y, z);
    }
}

/// @brief A "fire and forget" style of function. The engine will manage the sound handle internally.
/// When the sound finishes playing, the handle will be put up for recycling. Playback starts asynchronously.
/// @param fileName The is the name of the file to be played.
/// @param sync This parameter is ignored.
/// @param volume The sound playback volume - 0 (silent) to 1 (full).
/// @param passed Optional parameter flags.
void sub__sndplayfile(qbs *fileName, int32_t sync, float volume, int32_t passed) {
    (void)sync;

    // We need this to send requirements to SndOpen
    static qbs *reqs = nullptr;

    if (!reqs) {
        // Since this never changes, we can get away by doing this just once
        reqs = qbs_new(0, 0);
        qbs_set(reqs, qbs_new_txt("stream, nodecode")); // stream the sound and decode during playback
    }

    // We will not wrap this in a 'if initialized' block because SndOpen will take care of that
    auto handle = func__sndopen(fileName, reqs, 1);

    if (handle > 0) {
        if (passed & 2)
            ma_sound_set_volume(&audioEngine.soundHandles[handle]->maSound, volume);

        sub__sndplay(handle);                              // play the sound
        audioEngine.soundHandles[handle]->autoKill = true; // must be set after sub__sndplay
    }
}

/// @brief Pauses a sound.
/// @param handle A sound handle.
void sub__sndpause(int32_t handle) {
    if (audioEngine.isInitialized && audioEngine.IsHandleValid(handle) && audioEngine.soundHandles[handle]->type == AudioEngine::SoundHandle::Type::STATIC) {
        // Stop the sound and just leave it at that
        // miniaudio does not reset the play cursor
        audioEngine.maResult = ma_sound_stop(&audioEngine.soundHandles[handle]->maSound);
        AUDIO_DEBUG_CHECK(audioEngine.maResult == MA_SUCCESS);
    }
}

/// @brief Returns whether a sound is being played.
/// @param handle A sound handle.
/// @return Returns true if the sound is playing. False otherwise.
int32_t func__sndplaying(int32_t handle) {
    if (audioEngine.isInitialized && audioEngine.IsHandleValid(handle) && audioEngine.soundHandles[handle]->type == AudioEngine::SoundHandle::Type::STATIC) {
        return ma_sound_is_playing(&audioEngine.soundHandles[handle]->maSound) ? QB_TRUE : QB_FALSE;
    }

    return QB_FALSE;
}

/// @brief Checks if a sound is paused.
/// @param handle A sound handle.
/// @return Returns true if the sound is paused. False otherwise.
int32_t func__sndpaused(int32_t handle) {
    if (audioEngine.isInitialized && audioEngine.IsHandleValid(handle) && audioEngine.soundHandles[handle]->type == AudioEngine::SoundHandle::Type::STATIC) {
        return !ma_sound_is_playing(&audioEngine.soundHandles[handle]->maSound) &&
                       (ma_sound_is_looping(&audioEngine.soundHandles[handle]->maSound) || !ma_sound_at_end(&audioEngine.soundHandles[handle]->maSound))
                   ? QB_TRUE
                   : QB_FALSE;
    }

    return QB_FALSE;
}

/// @brief Sets the volume of a sound loaded in memory using a sound handle. This works for both static and raw sounds.
/// @param handle A sound handle.
/// @param volume A float point value with 0 resulting in silence and anything above 1 resulting in amplification.
void sub__sndvol(int32_t handle, float volume) {
    if (audioEngine.isInitialized && audioEngine.IsHandleValid(handle) &&
        (audioEngine.soundHandles[handle]->type == AudioEngine::SoundHandle::Type::STATIC ||
         audioEngine.soundHandles[handle]->type == AudioEngine::SoundHandle::Type::RAW)) {
        ma_sound_set_volume(&audioEngine.soundHandles[handle]->maSound, volume);
    }
}

/// @brief Like sub__sndplay(), but the sound is looped.
/// @param handle A sound handle.
void sub__sndloop(int32_t handle) {
    if (audioEngine.isInitialized && audioEngine.IsHandleValid(handle) && audioEngine.soundHandles[handle]->type == AudioEngine::SoundHandle::Type::STATIC) {
        // Reset position to zero only if we are playing and (not looping or we've reached the end of the sound)
        // This is based on the old OpenAl-soft code behavior
        if (ma_sound_is_playing(&audioEngine.soundHandles[handle]->maSound) &&
            (!ma_sound_is_looping(&audioEngine.soundHandles[handle]->maSound) || ma_sound_at_end(&audioEngine.soundHandles[handle]->maSound))) {
            audioEngine.maResult = ma_sound_seek_to_pcm_frame(&audioEngine.soundHandles[handle]->maSound, 0);
            AUDIO_DEBUG_CHECK(audioEngine.maResult == MA_SUCCESS);

            // Reset any limit
            ma_sound_set_stop_time_in_pcm_frames(&audioEngine.soundHandles[handle]->maSound, ~(ma_uint64)0);
        }

        // Kickstart playback
        audioEngine.maResult = ma_sound_start(&audioEngine.soundHandles[handle]->maSound);
        AUDIO_DEBUG_CHECK(audioEngine.maResult == MA_SUCCESS);

        // Start looping the sound if it is not
        if (!ma_sound_is_looping(&audioEngine.soundHandles[handle]->maSound)) {
            ma_sound_set_looping(&audioEngine.soundHandles[handle]->maSound, MA_TRUE);
        }

        AUDIO_DEBUG_PRINT("Looping sound %i", handle);
    }
}

/// @brief Sets the balance or 3D position of a sound. This works for both static and raw sounds.
/// It will do pure stereo panning if y & z are absent.
/// @param handle A sound handle.
/// @param x x distance values go from left (negative) to right (positive).
/// @param y y distance values go from below (negative) to above (positive).
/// @param z z distance values go from behind (negative) to in front (positive).
/// @param channel This has no meaning for miniaudio and is ignored.
/// @param passed Optional parameter flags.
void sub__sndbal(int32_t handle, float x, float y, float z, int32_t channel, int32_t passed) {
    (void)channel;

    if (audioEngine.isInitialized && audioEngine.IsHandleValid(handle) &&
        (audioEngine.soundHandles[handle]->type == AudioEngine::SoundHandle::Type::STATIC ||
         audioEngine.soundHandles[handle]->type == AudioEngine::SoundHandle::Type::RAW)) {
        if (passed & 2 || passed & 4) {                                                               // If y or z or both are passed
            ma_sound_set_spatialization_enabled(&audioEngine.soundHandles[handle]->maSound, MA_TRUE); // Enable 3D spatialization

            auto v = ma_sound_get_position(&audioEngine.soundHandles[handle]->maSound); // Get the current position in 3D space

            // Set the previous values of x, y, z if these were not passed
            if (!(passed & 1))
                x = v.x;
            if (!(passed & 2))
                y = v.y;
            if (!(passed & 4))
                z = v.z;

            ma_sound_set_position(&audioEngine.soundHandles[handle]->maSound, x, y, z);                // Use full 3D positioning
        } else if (passed & 1) {                                                                       // Only bother if x is passed
            ma_sound_set_spatialization_enabled(&audioEngine.soundHandles[handle]->maSound, MA_FALSE); // Disable spatialization for better stereo sound
            ma_sound_set_pan_mode(&audioEngine.soundHandles[handle]->maSound, ma_pan_mode_pan);        // Set true panning
            ma_sound_set_pan(&audioEngine.soundHandles[handle]->maSound, x);                           // Just use stereo panning
        }
    }
}

/// @brief Returns the length in seconds of a loaded sound using a sound handle.
/// @param handle A sound handle.
/// @return Returns the length of a sound in seconds.
double func__sndlen(int32_t handle) {
    if (audioEngine.isInitialized && audioEngine.IsHandleValid(handle) && audioEngine.soundHandles[handle]->type == AudioEngine::SoundHandle::Type::STATIC) {
        float lengthSeconds = 0;
        audioEngine.maResult = ma_sound_get_length_in_seconds(&audioEngine.soundHandles[handle]->maSound, &lengthSeconds);
        AUDIO_DEBUG_CHECK(audioEngine.maResult == MA_SUCCESS);
        return lengthSeconds;
    }

    return 0.0;
}

/// @brief Returns the current playing position in seconds using a sound handle.
/// @param handle A sound handle.
/// @return Returns the current playing position in seconds for a loaded sound.
double func__sndgetpos(int32_t handle) {
    if (audioEngine.isInitialized && audioEngine.IsHandleValid(handle) && audioEngine.soundHandles[handle]->type == AudioEngine::SoundHandle::Type::STATIC) {
        float playCursorSeconds = 0;
        audioEngine.maResult = ma_sound_get_cursor_in_seconds(&audioEngine.soundHandles[handle]->maSound, &playCursorSeconds);
        AUDIO_DEBUG_CHECK(audioEngine.maResult == MA_SUCCESS);
        return playCursorSeconds;
    }

    return 0.0;
}

/// @brief Changes the current/starting playing position in seconds of a sound.
/// @param handle A sound handle.
/// @param seconds The position to set in seconds.
void sub__sndsetpos(int32_t handle, double seconds) {
    if (audioEngine.isInitialized && audioEngine.IsHandleValid(handle) && audioEngine.soundHandles[handle]->type == AudioEngine::SoundHandle::Type::STATIC &&
        seconds >= 0.0) {
        // Get the sound sample rate
        ma_uint32 sampleRate;
        audioEngine.maResult = ma_sound_get_data_format(&audioEngine.soundHandles[handle]->maSound, NULL, NULL, &sampleRate, NULL, 0);
        if (audioEngine.maResult != MA_SUCCESS) {
            AUDIO_DEBUG_PRINT("Failed to get sample rate of sound %i", handle);

            return;
        }

        // Convert seconds to PCM frames
        ma_uint64 seekToFrame = seconds * sampleRate;

        // Get the length of the sound
        ma_uint64 lengthFrames;
        audioEngine.maResult = ma_sound_get_length_in_pcm_frames(&audioEngine.soundHandles[handle]->maSound, &lengthFrames);
        if (audioEngine.maResult != MA_SUCCESS) {
            AUDIO_DEBUG_PRINT("Failed to get length of sound %i", handle);

            return;
        }

        // If position is beyond length then simply stop playback and exit
        if (seekToFrame >= lengthFrames) {
            AUDIO_DEBUG_PRINT("Position is beyond length of sound %zu / %zu", seekToFrame, lengthFrames);

            audioEngine.maResult = ma_sound_stop(&audioEngine.soundHandles[handle]->maSound);
            AUDIO_DEBUG_CHECK(audioEngine.maResult == MA_SUCCESS);

            return;
        }

        // Reset the limit
        ma_sound_set_stop_time_in_pcm_frames(&audioEngine.soundHandles[handle]->maSound, ~(ma_uint64)0);

        // Set the position in PCM frames
        audioEngine.maResult = ma_sound_seek_to_pcm_frame(&audioEngine.soundHandles[handle]->maSound, seekToFrame);
        AUDIO_DEBUG_CHECK(audioEngine.maResult == MA_SUCCESS);
    }
}

/// @brief Stops playing a sound after it has been playing for a set number of seconds.
/// @param handle A sound handle.
/// @param limit The number of seconds that the sound will play.
void sub__sndlimit(int32_t handle, double limit) {
    if (audioEngine.isInitialized && audioEngine.IsHandleValid(handle) && audioEngine.soundHandles[handle]->type == AudioEngine::SoundHandle::Type::STATIC &&
        limit >= 0.0) {
        float lengthSeconds;
        audioEngine.maResult = ma_sound_get_length_in_seconds(&audioEngine.soundHandles[handle]->maSound, &lengthSeconds);
        if (audioEngine.maResult != MA_SUCCESS) {
            AUDIO_DEBUG_PRINT("Failed to get length of sound %i", handle);

            return;
        }

        if (limit >= lengthSeconds) {
            AUDIO_DEBUG_PRINT("Limit is beyond length of sound: %f / %f", limit, lengthSeconds);

            return;
        }

        ma_sound_set_stop_time_in_milliseconds(&audioEngine.soundHandles[handle]->maSound, limit * 1000.0);
    }
}

/// @brief Stops a playing or paused sound using a sound handle.
/// @param handle A sound handle.
void sub__sndstop(int32_t handle) {
    if (audioEngine.isInitialized && audioEngine.IsHandleValid(handle) && audioEngine.soundHandles[handle]->type == AudioEngine::SoundHandle::Type::STATIC) {
        // Stop the sound first
        audioEngine.maResult = ma_sound_stop(&audioEngine.soundHandles[handle]->maSound);
        AUDIO_DEBUG_CHECK(audioEngine.maResult == MA_SUCCESS);

        // Also reset the playback cursor to zero
        audioEngine.maResult = ma_sound_seek_to_pcm_frame(&audioEngine.soundHandles[handle]->maSound, 0);
        AUDIO_DEBUG_CHECK(audioEngine.maResult == MA_SUCCESS);

        // Reset the limit
        ma_sound_set_stop_time_in_pcm_frames(&audioEngine.soundHandles[handle]->maSound, ~(ma_uint64)0);
    }
}

/// @brief Creates a new raw sound stream and returns a sound handle.
/// @return A new sound handle if successful or 0 on failure.
int32_t func__sndopenraw() {
    // Return invalid handle if audio engine is not initialized
    if (!audioEngine.isInitialized)
        return AudioEngine::INVALID_SOUND_HANDLE;

    // Allocate a sound handle
    auto handle = audioEngine.CreateHandle();
    if (handle < 1)
        return AudioEngine::INVALID_SOUND_HANDLE;

    // Set some handle properties
    audioEngine.soundHandles[handle]->type = AudioEngine::SoundHandle::Type::RAW;

    // Create the raw sound object
    audioEngine.soundHandles[handle]->rawStream = AudioEngine::RawStream::Create(&audioEngine.maEngine, &audioEngine.soundHandles[handle]->maSound);
    if (!audioEngine.soundHandles[handle]->rawStream)
        return AudioEngine::INVALID_SOUND_HANDLE;

    return handle;
}

/// @brief Plays sound wave sample frequencies created by a program.
/// @param left Left channel sample.
/// @param right Right channel sample.
/// @param handle A sound handle.
/// @param passed Optional parameter flags.
void sub__sndraw(float left, float right, int32_t handle, int32_t passed) {
    // Use the default raw handle if handle was not passed
    if (!(passed & 2)) {
        // Check if the default handle was created
        if (audioEngine.internalSndRaw < 1) {
            audioEngine.internalSndRaw = func__sndopenraw();
        }

        handle = audioEngine.internalSndRaw;
    }

    if (audioEngine.isInitialized && audioEngine.IsHandleValid(handle) && audioEngine.soundHandles[handle]->type == AudioEngine::SoundHandle::Type::RAW) {
        if (!(passed & 1)) {
            right = left;
        }

        audioEngine.soundHandles[handle]->rawStream->PushSampleFrame(left, right);
    }
}

/// @brief Plays sound wave sample frequencies created by a program.
/// @param sampleFrameArray A QB64 array of sample frames.
/// @param channels The number of channels (1 or 2).
/// @param handle A sound handle.
/// @param frameCount The number of frames to play.
/// @param passed Optional parameter flags.
void sub__sndrawbatch(void *sampleFrameArray, int32_t channels, int32_t handle, uint32_t frameCount, int32_t passed) {
    // Use the default raw handle if handle was not passed
    if (!(passed & 2)) {
        // Check if the default handle was created
        if (audioEngine.internalSndRaw < 1) {
            audioEngine.internalSndRaw = func__sndopenraw();
        }

        handle = audioEngine.internalSndRaw;
    }

    if (!audioEngine.isInitialized || !audioEngine.IsHandleValid(handle) || audioEngine.soundHandles[handle]->type != AudioEngine::SoundHandle::Type::RAW) {
        return;
    }

    if (passed & 1) {
        if (channels != 1 && channels != 2) {
            AUDIO_DEBUG_PRINT("Invalid number of channels: %i", channels);

            return;
        }
    } else {
        channels = 1; // assume mono
    }

    if (channels == 2) {
        auto audioBuffer = reinterpret_cast<SampleFrame *>((reinterpret_cast<byte_element_struct *>(sampleFrameArray))->offset);
        auto audioBufferFrames = size_t((reinterpret_cast<byte_element_struct *>(sampleFrameArray))->length) / sizeof(SampleFrame);

        AUDIO_DEBUG_PRINT("Audio buffer frames: %zu", audioBufferFrames);

        if (audioBufferFrames) {
            if (passed & 4) {
                // Check if the frame count is more than what we have
                if (frameCount > audioBufferFrames) {
                    AUDIO_DEBUG_PRINT("Adjusting frame count: %u", frameCount);

                    frameCount = audioBufferFrames;
                }
            } else {
                frameCount = audioBufferFrames;
            }

            audioEngine.soundHandles[handle]->rawStream->PushSampleFrames(audioBuffer, frameCount);
        } else {
            AUDIO_DEBUG_PRINT("Audio buffer empty");
        }
    } else {
        auto audioBuffer = reinterpret_cast<float *>((reinterpret_cast<byte_element_struct *>(sampleFrameArray))->offset);
        auto audioBufferFrames = size_t((reinterpret_cast<byte_element_struct *>(sampleFrameArray))->length) / sizeof(float);

        AUDIO_DEBUG_PRINT("Audio buffer frames: %zu", audioBufferFrames);

        if (audioBufferFrames) {
            if (passed & 4) {
                // Check if the frame count is more than what we have
                if (frameCount > audioBufferFrames) {
                    AUDIO_DEBUG_PRINT("Adjusting frame count: %u", frameCount);

                    frameCount = audioBufferFrames;
                }
            } else {
                frameCount = audioBufferFrames;
            }

            audioEngine.soundHandles[handle]->rawStream->PushSampleFrames(audioBuffer, frameCount);
        } else {
            AUDIO_DEBUG_PRINT("Audio buffer empty");
        }
    }
}

/// @brief Returns the length of a raw sound left to be played in seconds.
/// @param handle A sound handle.
/// @param passed Optional parameter flags.
/// @return The length of the sound left to be played in seconds.
double func__sndrawlen(int32_t handle, int32_t passed) {
    // Use the default raw handle if handle was not passed
    if (!passed)
        handle = audioEngine.internalSndRaw;

    if (audioEngine.isInitialized && audioEngine.IsHandleValid(handle) && audioEngine.soundHandles[handle]->type == AudioEngine::SoundHandle::Type::RAW) {
        return audioEngine.soundHandles[handle]->rawStream->GetTimeRemaining();
    }

    return 0.0;
}

/// @brief Returns a sound handle to a newly created sound's raw data in memory with the given specification. The user can then fill the buffer with whatever
/// they want (using _MEMSOUND) and play it. This is basically the sound equivalent of _NEWIMAGE.
/// @param frames The number of sample frames required.
/// @param channels The number of sound channels. This can be 1 (mono) or 2 (stereo).
/// @param bits The bit depth of the sound. This can be 8 (unsigned 8-bit), 16 (signed 16-bit) or 32 (FP32).
/// @return A new sound handle if successful or 0 on failure.
int32_t func__sndnew(uint32_t frames, int32_t channels, int32_t bits) {
    // Validate all parameters
    if (!audioEngine.isInitialized || frames == 0 || (channels != 1 && channels != 2) || (bits != 16 && bits != 32 && bits != 8)) {
        AUDIO_DEBUG_PRINT("Invalid parameters: frames = %u, channels = %u, bits = %u", frames, channels, bits);
        return AudioEngine::INVALID_SOUND_HANDLE;
    }

    // Allocate a sound handle
    auto handle = audioEngine.CreateHandle();
    if (handle < 1)
        return AudioEngine::INVALID_SOUND_HANDLE;

    // Set some handle properties
    audioEngine.soundHandles[handle]->type = AudioEngine::SoundHandle::Type::STATIC;

    // Setup the ma_audio_buffer
    audioEngine.soundHandles[handle]->maAudioBufferConfig = ma_audio_buffer_config_init(
        (bits == 32 ? ma_format::ma_format_f32 : (bits == 16 ? ma_format::ma_format_s16 : ma_format::ma_format_u8)), channels, frames, NULL, NULL);

    // This currently has no effect. Sample rate always defaults to engine sample rate
    // Sample rate support for audio buffer is coming in miniaudio version 0.12
    // audioEngine.soundHandles[handle]->maAudioBufferConfig.sampleRate = sampleRate;

    // Allocate and initialize ma_audio_buffer
    audioEngine.maResult =
        ma_audio_buffer_alloc_and_init(&audioEngine.soundHandles[handle]->maAudioBufferConfig, &audioEngine.soundHandles[handle]->maAudioBuffer);
    if (audioEngine.maResult != MA_SUCCESS) {
        AUDIO_DEBUG_PRINT("Error %i: failed to initialize audio buffer", audioEngine.maResult);
        audioEngine.soundHandles[handle]->isUsed = false;
        return AudioEngine::INVALID_SOUND_HANDLE;
    }

    // Create a ma_sound from the ma_audio_buffer
    audioEngine.maResult = ma_sound_init_from_data_source(&audioEngine.maEngine, audioEngine.soundHandles[handle]->maAudioBuffer,
                                                          audioEngine.soundHandles[handle]->maFlags, NULL, &audioEngine.soundHandles[handle]->maSound);
    if (audioEngine.maResult != MA_SUCCESS) {
        AUDIO_DEBUG_PRINT("Error %i: failed to initialize data source", audioEngine.maResult);
        ma_audio_buffer_uninit_and_free(audioEngine.soundHandles[handle]->maAudioBuffer);
        audioEngine.soundHandles[handle]->maAudioBuffer = nullptr;
        audioEngine.soundHandles[handle]->isUsed = false;
        return AudioEngine::INVALID_SOUND_HANDLE;
    }

    AUDIO_DEBUG_PRINT("Frames = %llu, channels = %i, bits = %i, ma_format = %i, pointer = %p",
                      audioEngine.soundHandles[handle]->maAudioBuffer->ref.sizeInFrames, audioEngine.soundHandles[handle]->maAudioBuffer->ref.channels, bits,
                      audioEngine.soundHandles[handle]->maAudioBuffer->ref.format, audioEngine.soundHandles[handle]->maAudioBuffer->ref.pData);

    return handle;
}

/// @brief Returns a _MEM value referring to a sound's raw data in memory using a designated sound handle created by the _SNDOPEN function. miniaudio supports a
/// variety of sample and channel formats. Translating all of that to basic 2 channel 16-bit format that MemSound was originally supporting would require
/// significant overhead both in terms of system resources and code. For now we are just exposing the underlying PCM data directly from miniaudio. This fits
/// rather well using the existing mem structure. Mono sounds should continue to work just as it was before. Stereo and multi-channel sounds however will be
/// required to be handled correctly by the user by checking the 'elementsize' (for frame size in bytes) and 'type' (for data type) members.
/// @param handle A sound handle.
/// @param targetChannel This should be 0 (for interleaved) or 1 (for mono). Anything else will result in failure.
/// @param passed Optional parameter flags.
/// @return A _MEM value that can be used to access the sound data.
mem_block func__memsound(int32_t handle, int32_t targetChannel, int32_t passed) {
    auto maFormat = ma_format::ma_format_unknown;
    ma_uint32 channels = 0;
    ma_uint64 sampleFrames = 0;
    intptr_t data = NULL;

    // Setup mem_block (assuming failure)
    mem_block mb = {};
    mb.lock_offset = (intptr_t)mem_lock_base;
    mb.lock_id = INVALID_MEM_LOCK;

    // Return invalid mem_block if audio is not initialized, handle is invalid or sound type is not static
    if (!audioEngine.isInitialized || !audioEngine.IsHandleValid(handle) || audioEngine.soundHandles[handle]->type != AudioEngine::SoundHandle::Type::STATIC) {
        AUDIO_DEBUG_PRINT("Invalid handle (%i) or sound type (%i)", handle, int(audioEngine.soundHandles[handle]->type));
        return mb;
    }

    // Simply return an "empty" mem_block if targetChannel is not 0 or 1
    if (passed && targetChannel != 0 && targetChannel != 1) {
        AUDIO_DEBUG_PRINT("Invalid channel (%i)", targetChannel);
        return mb;
    }

    // Check what kind of sound we are dealing with and take appropriate path
    if (audioEngine.soundHandles[handle]->maAudioBuffer) { // we are dealing with a user created audio buffer
        AUDIO_DEBUG_PRINT("Entering ma_audio_buffer path");
        maFormat = audioEngine.soundHandles[handle]->maAudioBuffer->ref.format;
        channels = audioEngine.soundHandles[handle]->maAudioBuffer->ref.channels;
        sampleFrames = audioEngine.soundHandles[handle]->maAudioBuffer->ref.sizeInFrames;
        data = (intptr_t)audioEngine.soundHandles[handle]->maAudioBuffer->ref.pData;
    } else { // we are dealing with a sound loaded from file or memory
        AUDIO_DEBUG_PRINT("Entering ma_resource_manager_data_buffer path");

        // The sound cannot be steaming and must be completely decoded in memory
        if (audioEngine.soundHandles[handle]->maFlags & MA_SOUND_FLAG_STREAM || !(audioEngine.soundHandles[handle]->maFlags & MA_SOUND_FLAG_DECODE)) {
            AUDIO_DEBUG_PRINT("Sound data is not completely decoded");
            return mb;
        }

        // Get the pointer to the data source
        auto ds = (ma_resource_manager_data_buffer *)ma_sound_get_data_source(&audioEngine.soundHandles[handle]->maSound);
        if (!ds || !ds->pNode) {
            AUDIO_DEBUG_PRINT("Data source pointer OR data source node pointer is NULL");
            return mb;
        }

        // Check if the data is one contiguous buffer or a link list of decoded pages
        // We cannot have a mem object for a link list of decoded pages for obvious reasons
        if (ds->pNode->data.type != ma_resource_manager_data_supply_type::ma_resource_manager_data_supply_type_decoded) {
            AUDIO_DEBUG_PRINT("Data is not a contiguous buffer. Type = %u", ds->pNode->data.type);
            return mb;
        }

        // Check the data pointer
        if (!ds->pNode->data.backend.decoded.pData) {
            AUDIO_DEBUG_PRINT("Data source data pointer is NULL");
            return mb;
        }

        // Query the data format
        if (ma_sound_get_data_format(&audioEngine.soundHandles[handle]->maSound, &maFormat, &channels, NULL, NULL, 0) != MA_SUCCESS) {
            AUDIO_DEBUG_PRINT("Data format query failed");
            return mb;
        }

        // Get the length in sample frames
        if (ma_sound_get_length_in_pcm_frames(&audioEngine.soundHandles[handle]->maSound, &sampleFrames) != MA_SUCCESS) {
            AUDIO_DEBUG_PRINT("PCM frames query failed");
            return mb;
        }

        data = (intptr_t)ds->pNode->data.backend.decoded.pData;
    }

    AUDIO_DEBUG_PRINT("Format = %u, channels = %u, frames = %llu", maFormat, channels, sampleFrames);

    // Setup type: This was not done in the old code
    // But we are doing it here. By examining the type the user can now figure out if they have to use FP32 or integers
    switch (maFormat) {
    case ma_format::ma_format_f32:
        mb.type = 4 + 256; // FP32
        break;

    case ma_format::ma_format_s32:
        mb.type = 4 + 128; // signed int32
        break;

    case ma_format::ma_format_s16:
        mb.type = 2 + 128; // signed int16
        break;

    case ma_format::ma_format_u8:
        mb.type = 1 + 128 + 1024; // unsigned int8
        break;

    default:
        AUDIO_DEBUG_PRINT("Unsupported audio format");
        return mb;
    }

    if (audioEngine.soundHandles[handle]->memLockOffset) {
        AUDIO_DEBUG_PRINT("Returning previously created mem_lock");
        mb.lock_offset = (intptr_t)audioEngine.soundHandles[handle]->memLockOffset;
        mb.lock_id = audioEngine.soundHandles[handle]->memLockId;
    } else {
        AUDIO_DEBUG_PRINT("Returning new mem_lock");
        new_mem_lock();
        mem_lock_tmp->type = MEM_TYPE_SOUND;
        mb.lock_offset = (intptr_t)mem_lock_tmp;
        mb.lock_id = mem_lock_id;
        audioEngine.soundHandles[handle]->memLockOffset = (void *)mem_lock_tmp;
        audioEngine.soundHandles[handle]->memLockId = mem_lock_id;
    }

    mb.elementsize = ma_get_bytes_per_frame(maFormat, channels); // Set the element size. This is the size of each PCM frame in bytes
    mb.offset = data;                                            // Setup offset
    mb.size = sampleFrames * mb.elementsize;                     // Setup size (in bytes)
    mb.sound = handle;                                           // Copy the handle
    mb.image = 0;                                                // Not needed. Set to 0

    AUDIO_DEBUG_PRINT("ElementSize = %lli, size = %lli, type = %lli, pointer = %lld", mb.elementsize, mb.size, mb.type, mb.offset);

    return mb;
}

/// @brief Handles loading different sound bank formats based on the provided filename and requirements.
/// @param qbsFileName The filename or qbs buffer for the sound bank.
/// @param qbsRequirements The requirements for the sound bank (can be "memory" and one of the allowed formats).
/// @param passed Optional parameter flags.
void sub__midisoundbank(qbs *qbsFileName, qbs *qbsRequirements, int32_t passed) {
    enum struct SoundBankFormat { WOPL = 0, OP2, TMB, OPL, SF2, SF3, SFO, AD, UNKNOWN };
    static const char *SoundBankName[] = {"wopl", "op2", "tmb", "opl", "sf2", "sf3", "sfo", "ad", "unknown"};

    if (!audioEngine.isInitialized) {
        AUDIO_DEBUG_PRINT("Audio engine is not initialized");
        return;
    }

    auto fromMemory = false;                // by default we'll assume we are loading from a file on disk
    auto format = SoundBankFormat::UNKNOWN; // set to unknown by default

    if (passed && qbsRequirements->len) {
        // Parse the requirements string
        std::string requirements(reinterpret_cast<const char *>(qbsRequirements->chr), qbsRequirements->len);
        std::transform(requirements.begin(), requirements.end(), requirements.begin(), ::tolower);

        AUDIO_DEBUG_PRINT("Parsing requirements string: %s", requirements.c_str());

        if (requirements.find("memory") != std::string::npos) {
            fromMemory = true;
            AUDIO_DEBUG_PRINT("Sound bank will be loaded from memory");
        }

        for (auto i = 0; i < _countof(SoundBankName); i++) {
            AUDIO_DEBUG_PRINT("Checking for: %s", SoundBankName[i]);
            if (requirements.find(SoundBankName[i]) != std::string::npos) {
                format = SoundBankFormat(i);
                AUDIO_DEBUG_PRINT("Found: %s", SoundBankName[int(format)]);
                break;
            }
        }
    }

    if (fromMemory && qbsFileName->len) {
        // Only bother setting up the format if we are loading from memory
        switch (format) {
        case SoundBankFormat::SF2:
            g_InstrumentBankManager.SetData(qbsFileName->chr, qbsFileName->len, InstrumentBankManager::Type::Primesynth);
            AUDIO_DEBUG_PRINT("Uncompressed SondFont");
            break;

        case SoundBankFormat::SF3:
        case SoundBankFormat::SFO:
            g_InstrumentBankManager.SetData(qbsFileName->chr, qbsFileName->len, InstrumentBankManager::Type::TinySoundFont);
            AUDIO_DEBUG_PRINT("Compressed SondFont");
            break;

        case SoundBankFormat::AD:
        case SoundBankFormat::OP2:
        case SoundBankFormat::OPL:
        case SoundBankFormat::TMB:
        case SoundBankFormat::WOPL:
            g_InstrumentBankManager.SetData(qbsFileName->chr, qbsFileName->len, InstrumentBankManager::Type::Opal);
            AUDIO_DEBUG_PRINT("FM Bank");
            break;

        default:
            AUDIO_DEBUG_PRINT("Unknown format");
            return;
        }
    } else {
        if (qbsFileName->len) {
            std::string fileName(reinterpret_cast<const char *>(qbsFileName->chr), qbsFileName->len);

            if (FS_FileExists(filepath_fix_directory(fileName)))
                g_InstrumentBankManager.SetPath(fileName.c_str());
        } else {
            g_InstrumentBankManager.SetPath(nullptr); // load the default sound bank
        }
    }
}

/// @brief Initializes the audio subsystem. We simply attempt to initialize and then set some globals with the results.
void snd_init() {
    audioEngine.Initialize();
}

/// @brief Shuts down the audio engine and frees any resources used.
void snd_un_init() {
    audioEngine.ShutDown();
}

/// @brief Used for housekeeping and other stuff. This is called by the QB64-PE internally at ~60Hz.
void snd_mainloop() {
    audioEngine.Update();
}
