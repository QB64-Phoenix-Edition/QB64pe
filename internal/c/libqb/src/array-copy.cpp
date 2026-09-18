#include "libqb-common.h"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "array-copy.h"
#include "error_handle.h"
#include "qbs.h"

// Defined by qbx.cpp. Generated array code already uses this sentinel to
// distinguish an allocated descriptor from an erased/unallocated payload.
extern uint64_t *nothingvalue;

namespace {

constexpr uint64_t ARRAY_COPY_U64_MAX = ~(uint64_t)0;
constexpr uint64_t ARRAY_COPY_SIZE_MAX = (uint64_t)(~(size_t)0);

bool array_copy_descriptors_ok(intptr_t *source, intptr_t *destination) {
    if ((!source) || (!destination) || !(source[2] & 1) || !(destination[2] & 1) || (!source[0]) || (!destination[0]) ||
        (source[0] == (intptr_t)nothingvalue) || (destination[0] == (intptr_t)nothingvalue)) {
        error(9);
        return false;
    }

    return true;
}

intptr_t array_copy_dim_arg(int32_t dimensions, int32_t dimension) {
    return (intptr_t)(dimensions - 1 - dimension) * 4 + 4;
}

bool array_copy_mul_u64(uint64_t left, uint64_t right, uint64_t *result) {
    if (right && left > (ARRAY_COPY_U64_MAX / right)) {
        error(257);
        return false;
    }

    *result = left * right;
    return true;
}

bool array_copy_add_u64(uint64_t left, uint64_t right, uint64_t *result) {
    if (left > (ARRAY_COPY_U64_MAX - right)) {
        error(257);
        return false;
    }

    *result = left + right;
    return true;
}

struct ArrayCopy1DSpan {
    uint64_t source_offset;
    uint64_t destination_offset;
    uint64_t count;
};

// Validate a 1D source range and destination start once for both fixed-size and
// variable STRING payload paths. This keeps descriptor/bounds error policy from
// drifting while leaving the actual copy operation payload-specific.
bool array_copy_1d_range_span(intptr_t *source, intptr_t *destination, int64_t source_low, int64_t source_high,
                              int64_t destination_start, ArrayCopy1DSpan *span) {
    if (source_low > source_high) {
        error(9);
        return false;
    }

    const int64_t source_lower = (int64_t)source[4];
    const int64_t destination_lower = (int64_t)destination[4];
    const int64_t source_total = (int64_t)source[5];
    const int64_t destination_total = (int64_t)destination[5];

    if ((source_total <= 0) || (destination_total <= 0) || (source_low < source_lower) ||
        (destination_start < destination_lower)) {
        error(9);
        return false;
    }

    span->count = (uint64_t)source_high - (uint64_t)source_low + 1;
    span->source_offset = (uint64_t)source_low - (uint64_t)source_lower;
    span->destination_offset = (uint64_t)destination_start - (uint64_t)destination_lower;

    if ((!span->count) || (span->source_offset >= (uint64_t)source_total) ||
        (span->destination_offset >= (uint64_t)destination_total) ||
        (span->count > ((uint64_t)source_total - span->source_offset)) ||
        (span->count > ((uint64_t)destination_total - span->destination_offset))) {
        error(9);
        return false;
    }

    return true;
}

// Validate a whole 1D source mapped to one destination BASIC coordinate. The
// preserve-index whole form reuses this by passing source LBOUND as the start.
bool array_copy_1d_whole_span(intptr_t *source, intptr_t *destination, int64_t destination_start,
                              ArrayCopy1DSpan *span) {
    const int64_t destination_lower = (int64_t)destination[4];
    const int64_t source_total = (int64_t)source[5];
    const int64_t destination_total = (int64_t)destination[5];

    if ((source_total <= 0) || (destination_total <= 0) || (destination_start < destination_lower)) {
        error(9);
        return false;
    }

    span->source_offset = 0;
    span->count = (uint64_t)source_total;
    span->destination_offset = (uint64_t)destination_start - (uint64_t)destination_lower;

    if ((span->destination_offset >= (uint64_t)destination_total) ||
        (span->count > ((uint64_t)destination_total - span->destination_offset))) {
        error(9);
        return false;
    }

    return true;
}

// Own the temporary qbs objects used by variable STRING copies. Construction is
// explicit so allocation/error handling stays visible at each call site, while
// destruction guarantees that every successfully created qbs is freed on all
// returns, including qbs_set failures.
class ArrayCopyStringSnapshot {
  public:
    ArrayCopyStringSnapshot() = default;
    ~ArrayCopyStringSnapshot() {
        for (uint64_t i = 0; i < made_; ++i) qbs_free(slots_[i]);
        free(slots_);
    }

    ArrayCopyStringSnapshot(const ArrayCopyStringSnapshot &) = delete;
    ArrayCopyStringSnapshot &operator=(const ArrayCopyStringSnapshot &) = delete;

    bool allocate(uint64_t count) {
        if (count > (ARRAY_COPY_SIZE_MAX / sizeof(qbs *))) {
            error(257);
            return false;
        }

        slots_ = (qbs **)malloc((size_t)(count * sizeof(qbs *)));
        if ((!slots_) && count) {
            error(257);
            return false;
        }
        return true;
    }

    // Create snapshot qbs objects in source-copy order. Keeping qbs_new and
    // qbs_set interleaved preserves the allocation/error ordering of the staged
    // implementation while the destructor centralizes cleanup.
    bool make(uint64_t index) {
        slots_[index] = qbs_new(0, 0);
        if (slots_[index]) ++made_;
        return slots_[index] && !is_error_pending();
    }

    qbs *operator[](uint64_t index) const { return slots_[index]; }

  private:
    qbs **slots_ = nullptr;
    uint64_t made_ = 0;
};

bool array_copy_element_byte(uint64_t element, uint64_t element_bytes, uint64_t *byte_offset) {
    if (!element_bytes || element > (ARRAY_COPY_U64_MAX / element_bytes)) {
        error(257);
        return false;
    }

    *byte_offset = element * element_bytes;
    return true;
}

void array_copy_1d_fixed_bytes(intptr_t *source, intptr_t *destination, uint64_t source_offset, uint64_t destination_offset,
                               uint64_t count, uint64_t element_bytes) {
    if (!element_bytes) {
        error(257);
        return;
    }

    if ((count > (ARRAY_COPY_U64_MAX / element_bytes)) || (source_offset > (ARRAY_COPY_U64_MAX / element_bytes)) ||
        (destination_offset > (ARRAY_COPY_U64_MAX / element_bytes))) {
        error(257);
        return;
    }

    const uint64_t bytes = count * element_bytes;
    const uint64_t source_byte = source_offset * element_bytes;
    const uint64_t destination_byte = destination_offset * element_bytes;

    if ((bytes > ARRAY_COPY_SIZE_MAX) || (source_byte > ARRAY_COPY_SIZE_MAX) ||
        (destination_byte > ARRAY_COPY_SIZE_MAX)) {
        error(257);
        return;
    }

    if (bytes > 0) {
        memmove((void *)(((uint8_t *)destination[0]) + (size_t)destination_byte),
                (void *)(((uint8_t *)source[0]) + (size_t)source_byte), (size_t)bytes);
    }
}

// Deep-copy a validated one-dimensional variable STRING span through a complete
// qbs snapshot. Source and destination may alias: every source string is copied
// before the first destination slot is modified, preserving the same overlap and
// ownership semantics as the original generated _ArrayCopy path. The caller owns
// descriptor/bounds validation and supplies physical slot offsets plus element count.
void array_copy_1d_string_snapshot(intptr_t *source, intptr_t *destination, uint64_t source_offset,
                                   uint64_t destination_offset, uint64_t count) {
    ArrayCopyStringSnapshot snapshot;
    if (!snapshot.allocate(count)) return;

    qbs **source_slots = (qbs **)source[0];
    qbs **destination_slots = (qbs **)destination[0];

    for (uint64_t i = 0; i < count; ++i) {
        if (!snapshot.make(i)) return;
        qbs_set(snapshot[i], source_slots[source_offset + i]);
        if (is_error_pending()) return;
    }

    for (uint64_t i = 0; i < count; ++i) {
        qbs_set(destination_slots[destination_offset + i], snapshot[i]);
        if (is_error_pending()) return;
    }
}

bool array_copy_nd_range_validate(intptr_t *source, intptr_t *destination, int32_t dimensions, const int64_t *source_low,
                                  const int64_t *source_high, const int64_t *destination_start) {
    for (int32_t dimension = 0; dimension < dimensions; ++dimension) {
        const intptr_t argument = array_copy_dim_arg(dimensions, dimension);
        const int64_t source_lower = (int64_t)source[argument];
        const int64_t source_elements = (int64_t)source[argument + 1];
        const int64_t destination_lower = (int64_t)destination[argument];
        const int64_t destination_elements = (int64_t)destination[argument + 1];

        if ((source_elements <= 0) || (destination_elements <= 0) || (source_low[dimension] > source_high[dimension]) ||
            (source_low[dimension] < source_lower) || (destination_start[dimension] < destination_lower)) {
            error(9);
            return false;
        }

        const uint64_t count = (uint64_t)source_high[dimension] - (uint64_t)source_low[dimension] + 1;
        const uint64_t source_offset = (uint64_t)source_low[dimension] - (uint64_t)source_lower;
        const uint64_t destination_offset = (uint64_t)destination_start[dimension] - (uint64_t)destination_lower;

        if ((!count) || (source_offset >= (uint64_t)source_elements) ||
            (count > ((uint64_t)source_elements - source_offset)) ||
            (destination_offset >= (uint64_t)destination_elements) ||
            (count > ((uint64_t)destination_elements - destination_offset))) {
            error(9);
            return false;
        }
    }

    return true;
}

bool array_copy_nd_range_layout(intptr_t *source, intptr_t *destination, int32_t dimensions, const int64_t *source_low,
                                const int64_t *source_high, const int64_t *destination_start, uint64_t *rows,
                                uint64_t *row_elements, uint64_t *source_first_element, uint64_t *source_last_row_element,
                                uint64_t *destination_first_element, uint64_t *destination_last_row_element) {
    *rows = 1;
    *row_elements = (uint64_t)source_high[0] - (uint64_t)source_low[0] + 1;

    uint64_t source_stride = 1;
    uint64_t destination_stride = 1;

    // Preserve the generated path's overflow order: establish all strides and
    // row count before calculating first/last element positions.
    for (int32_t dimension = 1; dimension < dimensions; ++dimension) {
        const intptr_t previous_argument = array_copy_dim_arg(dimensions, dimension - 1);
        const uint64_t source_previous_total = (uint64_t)(int64_t)source[previous_argument + 1];
        const uint64_t destination_previous_total = (uint64_t)(int64_t)destination[previous_argument + 1];
        const uint64_t count = (uint64_t)source_high[dimension] - (uint64_t)source_low[dimension] + 1;

        if ((source_stride > (ARRAY_COPY_U64_MAX / source_previous_total)) ||
            (destination_stride > (ARRAY_COPY_U64_MAX / destination_previous_total)) ||
            (*rows > (ARRAY_COPY_U64_MAX / count))) {
            error(257);
            return false;
        }

        source_stride *= source_previous_total;
        destination_stride *= destination_previous_total;
        *rows *= count;
    }

    *source_first_element = 0;
    *destination_first_element = 0;
    source_stride = 1;
    destination_stride = 1;

    for (int32_t dimension = 0; dimension < dimensions; ++dimension) {
        const intptr_t argument = array_copy_dim_arg(dimensions, dimension);
        const int64_t source_lower = (int64_t)source[argument];
        const int64_t destination_lower = (int64_t)destination[argument];
        const uint64_t source_offset = (uint64_t)source_low[dimension] - (uint64_t)source_lower;
        const uint64_t destination_offset = (uint64_t)destination_start[dimension] - (uint64_t)destination_lower;

        uint64_t source_term = 0;
        uint64_t destination_term = 0;
        if (!array_copy_mul_u64(source_offset, source_stride, &source_term) ||
            !array_copy_mul_u64(destination_offset, destination_stride, &destination_term) ||
            !array_copy_add_u64(*source_first_element, source_term, source_first_element) ||
            !array_copy_add_u64(*destination_first_element, destination_term, destination_first_element)) {
            return false;
        }

        if (dimension + 1 < dimensions) {
            const uint64_t source_total = (uint64_t)(int64_t)source[argument + 1];
            const uint64_t destination_total = (uint64_t)(int64_t)destination[argument + 1];
            source_stride *= source_total;
            destination_stride *= destination_total;
        }
    }

    *source_last_row_element = *source_first_element;
    *destination_last_row_element = *destination_first_element;

    const intptr_t first_argument = array_copy_dim_arg(dimensions, 0);
    source_stride = (uint64_t)(int64_t)source[first_argument + 1];
    destination_stride = (uint64_t)(int64_t)destination[first_argument + 1];

    for (int32_t dimension = 1; dimension < dimensions; ++dimension) {
        const uint64_t count = (uint64_t)source_high[dimension] - (uint64_t)source_low[dimension] + 1;
        const uint64_t extra = count - 1;
        uint64_t source_term = 0;
        uint64_t destination_term = 0;

        if (!array_copy_mul_u64(extra, source_stride, &source_term) ||
            !array_copy_mul_u64(extra, destination_stride, &destination_term) ||
            !array_copy_add_u64(*source_last_row_element, source_term, source_last_row_element) ||
            !array_copy_add_u64(*destination_last_row_element, destination_term, destination_last_row_element)) {
            return false;
        }

        if (dimension + 1 < dimensions) {
            const intptr_t argument = array_copy_dim_arg(dimensions, dimension);
            source_stride *= (uint64_t)(int64_t)source[argument + 1];
            destination_stride *= (uint64_t)(int64_t)destination[argument + 1];
        }
    }

    return true;
}

bool array_copy_nd_range_bytes(intptr_t *source, intptr_t *destination, uint64_t row_elements,
                               uint64_t source_first_element, uint64_t source_last_row_element,
                               uint64_t destination_first_element, uint64_t destination_last_row_element,
                               uint64_t element_bytes, uint64_t *row_bytes, bool *backwards) {
    uint64_t source_first_byte = 0;
    uint64_t destination_first_byte = 0;
    uint64_t source_last_start = 0;
    uint64_t destination_last_start = 0;
    uint64_t source_last_end_byte = 0;
    uint64_t destination_last_end_byte = 0;

    if (!array_copy_element_byte(row_elements, element_bytes, row_bytes) ||
        !array_copy_element_byte(source_first_element, element_bytes, &source_first_byte) ||
        !array_copy_element_byte(destination_first_element, element_bytes, &destination_first_byte) ||
        !array_copy_element_byte(source_last_row_element, element_bytes, &source_last_start) ||
        !array_copy_add_u64(source_last_start, *row_bytes, &source_last_end_byte) ||
        !array_copy_element_byte(destination_last_row_element, element_bytes, &destination_last_start) ||
        !array_copy_add_u64(destination_last_start, *row_bytes, &destination_last_end_byte)) {
        return false;
    }

    if ((*row_bytes > ARRAY_COPY_SIZE_MAX) || (source_first_byte > ARRAY_COPY_SIZE_MAX) ||
        (destination_first_byte > ARRAY_COPY_SIZE_MAX) || (source_last_end_byte > ARRAY_COPY_SIZE_MAX) ||
        (destination_last_end_byte > ARRAY_COPY_SIZE_MAX)) {
        error(257);
        return false;
    }

    const uint64_t source_address = (uint64_t)source[0];
    const uint64_t destination_address = (uint64_t)destination[0];
    if ((source_address > (ARRAY_COPY_U64_MAX - source_last_end_byte)) ||
        (destination_address > (ARRAY_COPY_U64_MAX - destination_last_end_byte))) {
        error(257);
        return false;
    }

    const uint64_t source_first_address = source_address + source_first_byte;
    const uint64_t source_end = source_address + source_last_end_byte;
    const uint64_t destination_first_address = destination_address + destination_first_byte;
    const uint64_t destination_end = destination_address + destination_last_end_byte;

    *backwards = (source_first_address < destination_end) && (destination_first_address < source_end) &&
                 (destination_first_address > source_first_address);
    return true;
}


bool array_copy_nd_storage_total(intptr_t *descriptor, int32_t dimensions, uint64_t *total) {
    *total = 1;
    for (int32_t dimension = 0; dimension < dimensions; ++dimension) {
        const intptr_t argument = array_copy_dim_arg(dimensions, dimension);
        const int64_t count = (int64_t)descriptor[argument + 1];
        if (count <= 0) {
            error(9);
            return false;
        }

        if ((uint64_t)count && (*total > (ARRAY_COPY_U64_MAX / (uint64_t)count))) {
            error(257);
            return false;
        }
        *total *= (uint64_t)count;
    }
    return true;
}

// Whole ND source/destination copies require exact runtime bounds. STRING paths
// require a positive element count in every dimension; fixed-size paths retain
// the historical zero-count behavior. The payload layer receives only the final
// validated element count.
bool array_copy_nd_exact_total(intptr_t *source, intptr_t *destination, int32_t dimensions, bool require_positive,
                               uint64_t *total) {
    *total = 1;
    for (int32_t dimension = 0; dimension < dimensions; ++dimension) {
        const intptr_t argument = array_copy_dim_arg(dimensions, dimension);
        if ((destination[argument] != source[argument]) || (destination[argument + 1] != source[argument + 1])) {
            error(9);
            return false;
        }

        const int64_t count = (int64_t)source[argument + 1];
        if (require_positive ? (count <= 0) : (count < 0)) {
            error(9);
            return false;
        }

        if ((uint64_t)count && (*total > (ARRAY_COPY_U64_MAX / (uint64_t)count))) {
            error(257);
            return false;
        }
        *total *= (uint64_t)count;
    }
    return true;
}

// Convert one logical linear index inside a validated ND range to a physical
// descriptor slot. range_low/range_high define the logical shape; mapped_start
// is the BASIC coordinate corresponding to logical offset zero for this operand.
uint64_t array_copy_nd_mapped_element(intptr_t *descriptor, int32_t dimensions, const int64_t *range_low,
                                      const int64_t *range_high, const int64_t *mapped_start, uint64_t linear_index) {
    uint64_t quotient = linear_index;
    uint64_t element = 0;
    uint64_t stride = 1;

    for (int32_t dimension = 0; dimension < dimensions; ++dimension) {
        const intptr_t argument = array_copy_dim_arg(dimensions, dimension);
        const uint64_t count = (uint64_t)range_high[dimension] - (uint64_t)range_low[dimension] + 1;
        const uint64_t coordinate = quotient % count;
        quotient /= count;
        const uint64_t descriptor_lower = (uint64_t)(int64_t)descriptor[argument];
        const uint64_t start_offset = (uint64_t)mapped_start[dimension] - descriptor_lower;
        element += (start_offset + coordinate) * stride;
        if (dimension + 1 < dimensions) stride *= (uint64_t)(int64_t)descriptor[argument + 1];
    }

    return element;
}

struct ArrayCopyNDWholeAtLayout {
    uint64_t total;
    uint64_t row_elements;
    uint64_t rows;
    uint64_t destination_storage_total;
    uint64_t destination_first_element;
    uint64_t destination_last_row_element;
    uint64_t destination_last_element;
};

// Validate Source() -> Destination(starts...) once for both fixed-size and
// variable STRING payloads. The layout captures the contiguous source shape and
// all destination positions needed by the row-copy or mapped STRING write paths.
bool array_copy_nd_whole_at_layout(intptr_t *source, intptr_t *destination, int32_t dimensions,
                                   const int64_t *destination_start, bool validate_destination_storage,
                                   ArrayCopyNDWholeAtLayout *layout) {
    layout->total = 1;
    layout->destination_storage_total = 1;

    for (int32_t dimension = 0; dimension < dimensions; ++dimension) {
        const intptr_t argument = array_copy_dim_arg(dimensions, dimension);
        const int64_t source_elements = (int64_t)source[argument + 1];
        const int64_t destination_lower = (int64_t)destination[argument];
        const int64_t destination_elements = (int64_t)destination[argument + 1];

        if ((source_elements <= 0) || (destination_elements <= 0) ||
            (destination_start[dimension] < destination_lower)) {
            error(9);
            return false;
        }

        const uint64_t source_count = (uint64_t)source_elements;
        const uint64_t destination_count = (uint64_t)destination_elements;
        const uint64_t destination_offset =
            (uint64_t)destination_start[dimension] - (uint64_t)destination_lower;

        if ((destination_offset >= destination_count) || (source_count > (destination_count - destination_offset))) {
            error(9);
            return false;
        }

        if (layout->total > (ARRAY_COPY_U64_MAX / source_count)) {
            error(257);
            return false;
        }
        if (validate_destination_storage &&
            (layout->destination_storage_total > (ARRAY_COPY_U64_MAX / destination_count))) {
            error(257);
            return false;
        }

        layout->total *= source_count;
        if (validate_destination_storage) layout->destination_storage_total *= destination_count;
    }

    const intptr_t first_argument = array_copy_dim_arg(dimensions, 0);
    layout->row_elements = (uint64_t)(int64_t)source[first_argument + 1];
    layout->rows = layout->total / layout->row_elements;
    layout->destination_first_element = 0;

    uint64_t destination_stride = 1;
    for (int32_t dimension = 0; dimension < dimensions; ++dimension) {
        const intptr_t argument = array_copy_dim_arg(dimensions, dimension);
        const uint64_t destination_count = (uint64_t)(int64_t)destination[argument + 1];
        const uint64_t destination_offset =
            (uint64_t)destination_start[dimension] - (uint64_t)(int64_t)destination[argument];
        uint64_t term = 0;
        if (!array_copy_mul_u64(destination_offset, destination_stride, &term) ||
            !array_copy_add_u64(layout->destination_first_element, term, &layout->destination_first_element)) {
            return false;
        }

        if (dimension + 1 < dimensions) {
            if (!array_copy_mul_u64(destination_stride, destination_count, &destination_stride)) return false;
        }
    }

    layout->destination_last_row_element = layout->destination_first_element;
    destination_stride = (uint64_t)(int64_t)destination[first_argument + 1];
    for (int32_t dimension = 1; dimension < dimensions; ++dimension) {
        const intptr_t argument = array_copy_dim_arg(dimensions, dimension);
        const uint64_t source_count = (uint64_t)(int64_t)source[argument + 1];
        const uint64_t destination_count = (uint64_t)(int64_t)destination[argument + 1];
        uint64_t term = 0;
        if (!array_copy_mul_u64(source_count - 1, destination_stride, &term) ||
            !array_copy_add_u64(layout->destination_last_row_element, term, &layout->destination_last_row_element)) {
            return false;
        }

        if (dimension + 1 < dimensions) {
            if (!array_copy_mul_u64(destination_stride, destination_count, &destination_stride)) return false;
        }
    }

    if (!array_copy_add_u64(layout->destination_last_row_element, layout->row_elements - 1,
                            &layout->destination_last_element)) {
        return false;
    }
    if (validate_destination_storage && (layout->destination_last_element >= layout->destination_storage_total)) {
        error(9);
        return false;
    }

    return true;
}

uint64_t array_copy_nd_whole_at_destination_element(intptr_t *source, intptr_t *destination, int32_t dimensions,
                                                     const int64_t *destination_start, uint64_t linear_index) {
    uint64_t quotient = linear_index;
    uint64_t element = 0;
    uint64_t stride = 1;

    for (int32_t dimension = 0; dimension < dimensions; ++dimension) {
        const intptr_t argument = array_copy_dim_arg(dimensions, dimension);
        const uint64_t source_count = (uint64_t)(int64_t)source[argument + 1];
        const uint64_t destination_count = (uint64_t)(int64_t)destination[argument + 1];
        const uint64_t coordinate = quotient % source_count;
        quotient /= source_count;
        const uint64_t destination_offset =
            (uint64_t)destination_start[dimension] - (uint64_t)(int64_t)destination[argument];
        element += (destination_offset + coordinate) * stride;
        if (dimension + 1 < dimensions) stride *= destination_count;
    }

    return element;
}

} // namespace


void qb64_array_copy_1d_range_at_string(intptr_t *source, intptr_t *destination, int64_t source_low, int64_t source_high,
                                         int64_t destination_start) {
    if (!array_copy_descriptors_ok(source, destination)) return;

    ArrayCopy1DSpan span{};
    if (!array_copy_1d_range_span(source, destination, source_low, source_high, destination_start, &span)) return;
    array_copy_1d_string_snapshot(source, destination, span.source_offset, span.destination_offset, span.count);
}

// Copy every variable STRING element from source, in physical array order, to
// destination starting at the requested BASIC coordinate. Whole-source semantics
// use source slot offset zero regardless of its lower bound; destination_start is
// translated through the destination descriptor. The shared snapshot helper keeps
// self-copy and ownership behavior identical to range-to-start.
void qb64_array_copy_1d_whole_at_string(intptr_t *source, intptr_t *destination, int64_t destination_start) {
    if (!array_copy_descriptors_ok(source, destination)) return;

    ArrayCopy1DSpan span{};
    if (!array_copy_1d_whole_span(source, destination, destination_start, &span)) return;
    array_copy_1d_string_snapshot(source, destination, span.source_offset, span.destination_offset, span.count);
}

// Copy a whole one-dimensional variable STRING source while preserving its BASIC
// indexes in the destination. This is the whole-to-whole counterpart of
// qb64_array_copy_1d_whole_at_string: source physical slot zero maps to the
// destination slot whose BASIC coordinate equals source LBOUND.
void qb64_array_copy_1d_whole_string(intptr_t *source, intptr_t *destination) {
    if (!array_copy_descriptors_ok(source, destination)) return;

    ArrayCopy1DSpan span{};
    if (!array_copy_1d_whole_span(source, destination, (int64_t)source[4], &span)) return;
    array_copy_1d_string_snapshot(source, destination, span.source_offset, span.destination_offset, span.count);
}

void qb64_array_copy_1d_range_at_fixed(intptr_t *source, intptr_t *destination, int64_t source_low, int64_t source_high,
                                        int64_t destination_start, uint64_t element_bytes) {
    if (!array_copy_descriptors_ok(source, destination)) return;

    ArrayCopy1DSpan span{};
    if (!array_copy_1d_range_span(source, destination, source_low, source_high, destination_start, &span)) return;
    array_copy_1d_fixed_bytes(source, destination, span.source_offset, span.destination_offset, span.count, element_bytes);
}

void qb64_array_copy_1d_whole_at_fixed(intptr_t *source, intptr_t *destination, int64_t destination_start,
                                        uint64_t element_bytes) {
    if (!array_copy_descriptors_ok(source, destination)) return;

    ArrayCopy1DSpan span{};
    if (!array_copy_1d_whole_span(source, destination, destination_start, &span)) return;
    array_copy_1d_fixed_bytes(source, destination, span.source_offset, span.destination_offset, span.count, element_bytes);
}

void qb64_array_copy_1d_whole_fixed(intptr_t *source, intptr_t *destination, uint64_t element_bytes) {
    if (!array_copy_descriptors_ok(source, destination)) return;

    ArrayCopy1DSpan span{};
    if (!array_copy_1d_whole_span(source, destination, (int64_t)source[4], &span)) return;
    array_copy_1d_fixed_bytes(source, destination, span.source_offset, span.destination_offset, span.count, element_bytes);
}

// Copy a multidimensional variable STRING range into an arbitrary destination
// start coordinate. The compiler has already verified element-type/storage
// compatibility and evaluates all BASIC coordinate expressions before this call.
// Runtime work remains descriptor-driven because bounds can change after REDIM.
//
// The function deliberately avoids a second metadata allocation: range counts,
// offsets, strides, and storage limits are re-derived from descriptors in small
// passes. The only heap allocation is the complete qbs* snapshot required by
// variable STRING ownership and overlap semantics. Every source qbs is deep-copied
// before the first destination slot is modified, exactly like the former generated
// implementation.
void qb64_array_copy_nd_range_at_string(intptr_t *source, intptr_t *destination, int32_t dimensions,
                                         const int64_t *source_low, const int64_t *source_high,
                                         const int64_t *destination_start) {
    if (!array_copy_descriptors_ok(source, destination)) return;
    if (dimensions <= 1 || !source_low || !source_high || !destination_start) {
        error(257);
        return;
    }

    if (!array_copy_nd_range_validate(source, destination, dimensions, source_low, source_high, destination_start)) return;

    uint64_t rows = 0;
    uint64_t row_elements = 0;
    uint64_t source_first_element = 0;
    uint64_t source_last_row_element = 0;
    uint64_t destination_first_element = 0;
    uint64_t destination_last_row_element = 0;
    if (!array_copy_nd_range_layout(source, destination, dimensions, source_low, source_high, destination_start, &rows,
                                    &row_elements, &source_first_element, &source_last_row_element,
                                    &destination_first_element, &destination_last_row_element)) {
        return;
    }

    uint64_t total = 0;
    uint64_t source_storage_total = 0;
    uint64_t destination_storage_total = 0;
    uint64_t source_last = 0;
    uint64_t destination_last = 0;
    if (!array_copy_mul_u64(rows, row_elements, &total) ||
        !array_copy_nd_storage_total(source, dimensions, &source_storage_total) ||
        !array_copy_nd_storage_total(destination, dimensions, &destination_storage_total) ||
        !array_copy_add_u64(source_last_row_element, row_elements - 1, &source_last) ||
        !array_copy_add_u64(destination_last_row_element, row_elements - 1, &destination_last)) {
        return;
    }

    if ((source_last >= source_storage_total) || (destination_last >= destination_storage_total)) {
        error(9);
        return;
    }

    ArrayCopyStringSnapshot snapshot;
    if (!snapshot.allocate(total)) return;

    qbs **source_slots = (qbs **)source[0];
    qbs **destination_slots = (qbs **)destination[0];

    for (uint64_t i = 0; i < total; ++i) {
        const uint64_t source_element =
            array_copy_nd_mapped_element(source, dimensions, source_low, source_high, source_low, i);
        if (!snapshot.make(i)) return;
        qbs_set(snapshot[i], source_slots[source_element]);
        if (is_error_pending()) return;
    }

    for (uint64_t i = 0; i < total; ++i) {
        const uint64_t destination_element =
            array_copy_nd_mapped_element(destination, dimensions, source_low, source_high, destination_start, i);
        qbs_set(destination_slots[destination_element], snapshot[i]);
        if (is_error_pending()) return;
    }
}

// Copy a whole multidimensional variable STRING source into an arbitrary
// destination start coordinate. Source() means every source slot in physical
// array order, while destination_start[] supplies the BASIC coordinate that maps
// to source slot zero. The compiler evaluates those destination expressions once
// before entering this runtime function; descriptor bounds remain runtime state
// because either operand may have been REDIMed since compilation.
//
// A complete qbs snapshot is built before any destination write. This is required
// even though whole-source data are physically contiguous: source and destination
// may alias through the same owner/member storage, and raw qbs* slot copies would
// violate STRING ownership. No auxiliary ND metadata allocation is used; counts,
// offsets, and strides are re-derived from the descriptors in bounded passes.
void qb64_array_copy_nd_whole_at_string(intptr_t *source, intptr_t *destination, int32_t dimensions,
                                         const int64_t *destination_start) {
    if (!array_copy_descriptors_ok(source, destination)) return;
    if (dimensions <= 1 || !destination_start) {
        error(257);
        return;
    }

    ArrayCopyNDWholeAtLayout layout{};
    if (!array_copy_nd_whole_at_layout(source, destination, dimensions, destination_start, true, &layout)) return;

    ArrayCopyStringSnapshot snapshot;
    if (!snapshot.allocate(layout.total)) return;

    qbs **source_slots = (qbs **)source[0];
    qbs **destination_slots = (qbs **)destination[0];

    for (uint64_t i = 0; i < layout.total; ++i) {
        if (!snapshot.make(i)) return;
        qbs_set(snapshot[i], source_slots[i]);
        if (is_error_pending()) return;
    }

    for (uint64_t i = 0; i < layout.total; ++i) {
        const uint64_t destination_element =
            array_copy_nd_whole_at_destination_element(source, destination, dimensions, destination_start, i);
        qbs_set(destination_slots[destination_element], snapshot[i]);
        if (is_error_pending()) return;
    }
}

// Copy a whole multidimensional variable STRING array to another array with
// exactly the same runtime bounds. ND whole-to-whole semantics require both the
// lower bound and element count to match in every dimension; unlike the 1D form,
// a larger containing destination is not accepted. Once validated, physical slot
// order is identical for source and destination, so the shared linear STRING
// snapshot preserves ownership and overlap semantics without any ND metadata.
void qb64_array_copy_nd_whole_string(intptr_t *source, intptr_t *destination, int32_t dimensions) {
    if (!array_copy_descriptors_ok(source, destination)) return;
    if (dimensions <= 1) {
        error(257);
        return;
    }

    uint64_t total = 0;
    if (!array_copy_nd_exact_total(source, destination, dimensions, true, &total)) return;

    // Exact self-copy is already complete and requires no ownership work.
    if (destination[0] == source[0]) return;
    array_copy_1d_string_snapshot(source, destination, 0, 0, total);
}

void qb64_array_copy_nd_range_at_fixed(intptr_t *source, intptr_t *destination, int32_t dimensions,
                                        const int64_t *source_low, const int64_t *source_high,
                                        const int64_t *destination_start, uint64_t element_bytes) {
    if (!array_copy_descriptors_ok(source, destination)) return;
    if (dimensions <= 1 || !source_low || !source_high || !destination_start || !element_bytes) {
        error(257);
        return;
    }

    if (!array_copy_nd_range_validate(source, destination, dimensions, source_low, source_high, destination_start)) return;

    uint64_t rows = 0;
    uint64_t row_elements = 0;
    uint64_t source_first_element = 0;
    uint64_t source_last_row_element = 0;
    uint64_t destination_first_element = 0;
    uint64_t destination_last_row_element = 0;

    if (!array_copy_nd_range_layout(source, destination, dimensions, source_low, source_high, destination_start, &rows,
                                    &row_elements, &source_first_element, &source_last_row_element,
                                    &destination_first_element, &destination_last_row_element)) {
        return;
    }

    uint64_t row_bytes = 0;
    bool backwards = false;
    if (!array_copy_nd_range_bytes(source, destination, row_elements, source_first_element, source_last_row_element,
                                   destination_first_element, destination_last_row_element, element_bytes, &row_bytes,
                                   &backwards)) {
        return;
    }

    const intptr_t first_argument = array_copy_dim_arg(dimensions, 0);
    const uint64_t source_total0 = (uint64_t)(int64_t)source[first_argument + 1];
    const uint64_t destination_total0 = (uint64_t)(int64_t)destination[first_argument + 1];

    for (uint64_t step = 0; step < rows; ++step) {
        const uint64_t row = backwards ? (rows - 1 - step) : step;
        uint64_t quotient = row;
        uint64_t source_element = source_first_element;
        uint64_t destination_element = destination_first_element;
        uint64_t source_stride = source_total0;
        uint64_t destination_stride = destination_total0;

        for (int32_t dimension = 1; dimension < dimensions; ++dimension) {
            const uint64_t count = (uint64_t)source_high[dimension] - (uint64_t)source_low[dimension] + 1;
            const uint64_t coordinate = quotient % count;
            quotient /= count;
            source_element += coordinate * source_stride;
            destination_element += coordinate * destination_stride;

            if (dimension + 1 < dimensions) {
                const intptr_t argument = array_copy_dim_arg(dimensions, dimension);
                source_stride *= (uint64_t)(int64_t)source[argument + 1];
                destination_stride *= (uint64_t)(int64_t)destination[argument + 1];
            }
        }

        const uint64_t source_byte = source_element * element_bytes;
        const uint64_t destination_byte = destination_element * element_bytes;
        memmove((void *)(((uint8_t *)destination[0]) + (size_t)destination_byte),
                (void *)(((uint8_t *)source[0]) + (size_t)source_byte), (size_t)row_bytes);
    }
}

void qb64_array_copy_nd_whole_at_fixed(intptr_t *source, intptr_t *destination, int32_t dimensions,
                                        const int64_t *destination_start, uint64_t element_bytes) {
    if (!array_copy_descriptors_ok(source, destination)) return;
    if (dimensions <= 1 || !destination_start || !element_bytes) {
        error(257);
        return;
    }

    ArrayCopyNDWholeAtLayout layout{};
    if (!array_copy_nd_whole_at_layout(source, destination, dimensions, destination_start, false, &layout)) return;

    uint64_t row_bytes = 0;
    uint64_t source_bytes = 0;
    uint64_t destination_first_byte = 0;
    uint64_t destination_last_start = 0;
    uint64_t destination_last_end_byte = 0;
    if (!array_copy_element_byte(layout.row_elements, element_bytes, &row_bytes) ||
        !array_copy_mul_u64(layout.rows, row_bytes, &source_bytes) ||
        !array_copy_element_byte(layout.destination_first_element, element_bytes, &destination_first_byte) ||
        !array_copy_element_byte(layout.destination_last_row_element, element_bytes, &destination_last_start) ||
        !array_copy_add_u64(destination_last_start, row_bytes, &destination_last_end_byte)) {
        return;
    }

    if ((row_bytes > ARRAY_COPY_SIZE_MAX) || (source_bytes > ARRAY_COPY_SIZE_MAX) ||
        (destination_first_byte > ARRAY_COPY_SIZE_MAX) || (destination_last_end_byte > ARRAY_COPY_SIZE_MAX)) {
        error(257);
        return;
    }

    const uint64_t source_address = (uint64_t)source[0];
    const uint64_t destination_address = (uint64_t)destination[0];
    if ((source_address > (ARRAY_COPY_U64_MAX - source_bytes)) ||
        (destination_address > (ARRAY_COPY_U64_MAX - destination_last_end_byte))) {
        error(257);
        return;
    }

    const uint64_t source_end = source_address + source_bytes;
    const uint64_t destination_first_address = destination_address + destination_first_byte;
    const uint64_t destination_end = destination_address + destination_last_end_byte;
    const bool backwards = (source_address < destination_end) && (destination_first_address < source_end) &&
                           (destination_first_address > source_address);

    const intptr_t first_argument = array_copy_dim_arg(dimensions, 0);
    const uint64_t destination_total0 = (uint64_t)(int64_t)destination[first_argument + 1];
    for (uint64_t step = 0; step < layout.rows; ++step) {
        const uint64_t row = backwards ? (layout.rows - 1 - step) : step;
        uint64_t quotient = row;
        uint64_t destination_element =
            (uint64_t)destination_start[0] - (uint64_t)(int64_t)destination[first_argument];
        uint64_t stride = destination_total0;

        for (int32_t dimension = 1; dimension < dimensions; ++dimension) {
            const intptr_t argument = array_copy_dim_arg(dimensions, dimension);
            const uint64_t source_count = (uint64_t)(int64_t)source[argument + 1];
            const uint64_t coordinate = quotient % source_count;
            quotient /= source_count;
            const uint64_t destination_offset =
                (uint64_t)destination_start[dimension] - (uint64_t)(int64_t)destination[argument];
            destination_element += (destination_offset + coordinate) * stride;
            if (dimension + 1 < dimensions) stride *= (uint64_t)(int64_t)destination[argument + 1];
        }

        const uint64_t source_byte = row * row_bytes;
        const uint64_t destination_byte = destination_element * element_bytes;
        memmove((void *)(((uint8_t *)destination[0]) + (size_t)destination_byte),
                (void *)(((uint8_t *)source[0]) + (size_t)source_byte), (size_t)row_bytes);
    }
}

void qb64_array_copy_nd_whole_fixed(intptr_t *source, intptr_t *destination, int32_t dimensions,
                                     uint64_t element_bytes) {
    if (!array_copy_descriptors_ok(source, destination)) return;
    if (dimensions <= 1 || !element_bytes) {
        error(257);
        return;
    }

    uint64_t total = 0;
    if (!array_copy_nd_exact_total(source, destination, dimensions, false, &total)) return;
    if (destination[0] == source[0]) return;

    uint64_t bytes = 0;
    if (!array_copy_mul_u64(total, element_bytes, &bytes)) return;
    if (bytes > ARRAY_COPY_SIZE_MAX) {
        error(257);
        return;
    }

    memmove((void *)destination[0], (void *)source[0], (size_t)bytes);
}
