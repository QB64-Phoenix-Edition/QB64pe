#ifndef INCLUDE_LIBQB_ARRAY_COPY_H
#define INCLUDE_LIBQB_ARRAY_COPY_H

#include <stdint.h>

// Runtime implementation for _ArrayCopy forms. The compiler resolves
// operand types/storage and evaluates BASIC expressions before calling these
// functions.
//
// Fixed-size forms use byte-copy helpers. Variable-length STRING forms use
// ownership-aware qbs snapshots and deep copy.

void qb64_array_copy_1d_range_at_string(intptr_t *source, intptr_t *destination, int64_t source_low, int64_t source_high,
                                         int64_t destination_start);

void qb64_array_copy_1d_whole_at_string(intptr_t *source, intptr_t *destination, int64_t destination_start);

void qb64_array_copy_1d_whole_string(intptr_t *source, intptr_t *destination);

void qb64_array_copy_nd_range_at_string(intptr_t *source, intptr_t *destination, int32_t dimensions,
                                         const int64_t *source_low, const int64_t *source_high,
                                         const int64_t *destination_start);

void qb64_array_copy_nd_whole_at_string(intptr_t *source, intptr_t *destination, int32_t dimensions,
                                         const int64_t *destination_start);

void qb64_array_copy_nd_whole_string(intptr_t *source, intptr_t *destination, int32_t dimensions);

// Fixed-size byte-copy entry points.
void qb64_array_copy_1d_range_at_fixed(intptr_t *source, intptr_t *destination, int64_t source_low, int64_t source_high,
                                        int64_t destination_start, uint64_t element_bytes);

void qb64_array_copy_1d_whole_at_fixed(intptr_t *source, intptr_t *destination, int64_t destination_start,
                                        uint64_t element_bytes);

void qb64_array_copy_1d_whole_fixed(intptr_t *source, intptr_t *destination, uint64_t element_bytes);

void qb64_array_copy_nd_range_at_fixed(intptr_t *source, intptr_t *destination, int32_t dimensions,
                                        const int64_t *source_low, const int64_t *source_high,
                                        const int64_t *destination_start, uint64_t element_bytes);

void qb64_array_copy_nd_whole_at_fixed(intptr_t *source, intptr_t *destination, int32_t dimensions,
                                        const int64_t *destination_start, uint64_t element_bytes);

void qb64_array_copy_nd_whole_fixed(intptr_t *source, intptr_t *destination, int32_t dimensions,
                                     uint64_t element_bytes);

#endif
