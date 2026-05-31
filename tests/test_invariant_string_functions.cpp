#include <gtest/gtest.h>
#include <string>
#include <cstring>
#include <cstdlib>

// Include the qbs structure and functions from the actual production code
#include "internal/c/libqb/src/qbs.h"

// Declare the production functions under test
extern "C" {
    void sub_lset(qbs *dest, qbs *source);
    void sub_rset(qbs *dest, qbs *source);
}

class BufferOverreadTest : public ::testing::TestWithParam<std::string> {};

TEST_P(BufferOverreadTest, LsetNeverExceedsDeclaredLength) {
    // Invariant: memcpy in sub_lset must never read/write beyond dest->len bytes
    std::string payload = GetParam();

    const int dest_size = 8;
    qbs *dest   = qbs_new(dest_size, 1);
    qbs *source = qbs_new(static_cast<int>(payload.size()), 1);
    memcpy(source->chr, payload.data(), payload.size());

    // Call the real production function
    sub_lset(dest, source);

    // After lset, dest->len must remain unchanged and within allocated bounds
    EXPECT_EQ(dest->len, dest_size)
        << "dest->len was mutated beyond declared size";

    // Verify no bytes beyond dest_size were touched (canary check)
    // dest->chr is allocated to dest_size; reading past it would be UB caught by sanitizers
    for (int i = 0; i < dest_size; i++) {
        // Just access each byte to let ASan/Valgrind catch overflows
        volatile char c = dest->chr[i];
        (void)c;
    }

    qbs_free(dest);
    qbs_free(source);
}

TEST_P(BufferOverreadTest, RsetNeverExceedsDeclaredLength) {
    // Invariant: memcpy in sub_rset must never read/write beyond dest->len bytes
    std::string payload = GetParam();

    const int dest_size = 8;
    qbs *dest   = qbs_new(dest_size, 1);
    qbs *source = qbs_new(static_cast<int>(payload.size()), 1);
    memcpy(source->chr, payload.data(), payload.size());

    sub_rset(dest, source);

    EXPECT_EQ(dest->len, dest_size)
        << "dest->len was mutated beyond declared size";

    for (int i = 0; i < dest_size; i++) {
        volatile char c = dest->chr[i];
        (void)c;
    }

    qbs_free(dest);
    qbs_free(source);
}

INSTANTIATE_TEST_SUITE_P(
    AdversarialInputs,
    BufferOverreadTest,
    ::testing::Values(
        // Exact exploit: source->len == 2x dest buffer (16 vs 8)
        std::string(16, 'A'),
        // Boundary: source->len == dest->len + 1
        std::string(9, 'B'),
        // Valid input: source->len < dest->len
        std::string(4, 'C'),
        // 10x oversized
        std::string(80, 'X')
    )
);

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}