
/** $VER: IFF.h (2024.05.18) **/

#pragma once

#include "framework.h"

typedef uint32_t fourcc_t;

#define MAKE_FOURCC(char1, char2, char3, char4)                                                                                                                \
    (static_cast<uint32_t>(char1) | (static_cast<uint32_t>(char2) << 8) | (static_cast<uint32_t>(char3) << 16) | (static_cast<uint32_t>(char4) << 24))

const fourcc_t FOURCC_FORM = MAKE_FOURCC('F', 'O', 'R', 'M');
const fourcc_t FOURCC_CAT = MAKE_FOURCC('C', 'A', 'T', ' ');
const fourcc_t FOURCC_EVNT = MAKE_FOURCC('E', 'V', 'N', 'T');

const fourcc_t FOURCC_XDIR = MAKE_FOURCC('X', 'D', 'I', 'R');
const fourcc_t FOURCC_XMID = MAKE_FOURCC('X', 'M', 'I', 'D');

struct iff_chunk_t {
    fourcc_t Id;
    fourcc_t Type;
    std::vector<uint8_t> _Data;
    std::vector<iff_chunk_t> _Chunks;

    iff_chunk_t() : Id(), Type() {}

    iff_chunk_t(const iff_chunk_t &other) {
        Id = other.Id;
        Type = other.Type;
        _Data = other._Data;
        _Chunks = other._Chunks;
    }

    iff_chunk_t &operator=(const iff_chunk_t &other) {
        Id = other.Id;
        Type = other.Type;
        _Data = other._Data;
        _Chunks = other._Chunks;

        return *this;
    }

    /// <summary>
    /// Gets the n-th chunk with the specified id.
    /// </summary>
    const iff_chunk_t &FindChunk(fourcc_t id, uint32_t n = 0) const {
        for (const auto &Chunk : _Chunks) {
            if (Chunk.Id == id) {
                if (n != 0)
                    --n;

                if (n == 0)
                    return Chunk;
            }
        }

        return *this; // throw exception_io_data( pfc::string_formatter() << "Missing IFF chunk: " << p_id );
    }

    /// <summary>
    /// Gets the number of chunks with the specified id.
    /// </summary>
    uint32_t GetChunkCount(fourcc_t id) const {
        uint32_t ChunkCount = 0;

        for (const auto &Chunk : _Chunks) {
            if (Chunk.Id == id)
                ++ChunkCount;
        }

        return ChunkCount;
    }
};

struct iff_stream_t {
    std::vector<iff_chunk_t> _Chunks;

    iff_chunk_t fail;

    /// <summary>
    /// Finds the first chunk with the specified id.
    /// </summary>
    const iff_chunk_t &FindChunk(fourcc_t id) const {
        for (const auto &Chunk : _Chunks) {
            if (Chunk.Id == id)
                return Chunk;
        }

        return fail; // throw exception_io_data( pfc::string_formatter() << "Missing IFF chunk: " << p_id );
    }
};
