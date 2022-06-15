
#include <stddef.h>
#include <stdio.h>
#include <unistd.h>

#include "test.h"
#include "buffer.h"

// A single read and write of the same size from the buffer
void test_single_rw() {
    struct libqb_buffer buffer;

    libqb_buffer_init(&buffer);
    const char str[] = "FOOBAR";

    test_assert_ints(0, libqb_buffer_length(&buffer));

    libqb_buffer_write(&buffer, str, sizeof(str));

    test_assert_ints(sizeof(str), libqb_buffer_length(&buffer));

    char read_buf[sizeof(str)];
    size_t read_len = libqb_buffer_read(&buffer, read_buf, sizeof(str));

    test_assert_ints(sizeof(str), read_len);
    test_assert_buffers(str, read_buf, sizeof(str));
    test_assert_ints(0, libqb_buffer_length(&buffer));

    libqb_buffer_clear(&buffer);
}

// Multple writes and reads of the same size from the buffer
void test_multiple_rw() {
    int count = 10;
    struct libqb_buffer buffer;

    libqb_buffer_init(&buffer);
    const char str[] = "FOOBAR";
    int length = 0;

    test_assert_ints(0, libqb_buffer_length(&buffer));

    for (int i = 0; i < count; i++) {
        char id[20];
        snprintf(id, sizeof(id), "%d", i);

        libqb_buffer_write(&buffer, str, sizeof(str));

        length += sizeof(str);
        test_assert_ints_with_name(id, length, libqb_buffer_length(&buffer));
    }

    for (int i = 0; i < count; i++) {
        char id[20];
        snprintf(id, sizeof(id), "%d", i);

        char read_buf[sizeof(str)];
        size_t read_len = libqb_buffer_read(&buffer, read_buf, sizeof(str));

        length -= sizeof(str);

        test_assert_ints_with_name(id, sizeof(str), read_len);
        test_assert_buffers_with_name(id, str, read_buf, sizeof(str));
        test_assert_ints_with_name(id, length, libqb_buffer_length(&buffer));
    }

    libqb_buffer_clear(&buffer);
}

// Single write, multiple reads
void test_partial_read() {
    struct libqb_buffer buffer;

    libqb_buffer_init(&buffer);
    const char str[] = "FOOBAR1" "FOOBAR2" "FOOBAR3" "FOOBAR4" "FOOBAR5" "FOOBAR6";
    const int str_len = 7;
    const int length = sizeof(str) - 1;

    test_assert_ints(0, libqb_buffer_length(&buffer));

    libqb_buffer_write(&buffer, str, length);

    int length_left = length;
    for (int i = 0; i < length / str_len; i++) {
        char id[20];
        snprintf(id, sizeof(id), "%d", i);

        char read_buf[str_len];
        size_t read_len = libqb_buffer_read(&buffer, read_buf, sizeof(read_buf));

        length_left -= str_len;

        test_assert_ints_with_name(id, str_len, read_len);

        // Check the prefix, without the number
        test_assert_buffers_with_name(id, "FOOBAR", read_buf, str_len - 1);

        // Check the number at the end
        test_assert_ints_with_name(id, '1' + i, read_buf[6]);

        test_assert_ints_with_name(id, length_left, libqb_buffer_length(&buffer));
    }

    libqb_buffer_clear(&buffer);
}

void test_full_read() {
    int count = 6;
    struct libqb_buffer buffer;

    libqb_buffer_init(&buffer);
    const char str[] = "FOOBAR";
    const int str_len = sizeof(str) - 1;
    int length = 0;

    test_assert_ints(0, libqb_buffer_length(&buffer));

    for (int i = 0; i < count; i++) {
        char id[20];
        snprintf(id, sizeof(id), "%d", i);

        libqb_buffer_write(&buffer, str, str_len);

        length += str_len;
        test_assert_ints_with_name(id, length, libqb_buffer_length(&buffer));
    }

    char full_buf[str_len * count];

    size_t read_len = libqb_buffer_read(&buffer, full_buf, sizeof(full_buf));

    test_assert_ints(sizeof(full_buf), read_len);
    test_assert_buffers("FOOBAR" "FOOBAR" "FOOBAR" "FOOBAR" "FOOBAR" "FOOBAR", full_buf, sizeof(full_buf));

    libqb_buffer_clear(&buffer);
}

void test_read_past_end() {
    struct libqb_buffer buffer;

    libqb_buffer_init(&buffer);
    const char str[] = "FOOBAR1" "FOOBAR2";
    int str_len = sizeof(str);

    test_assert_ints(0, libqb_buffer_length(&buffer));

    libqb_buffer_write(&buffer, str, str_len);

    test_assert_ints(str_len, libqb_buffer_length(&buffer));

    char read_buf[200];
    size_t read_len = libqb_buffer_read(&buffer, read_buf, sizeof(read_buf));

    test_assert_ints(str_len, read_len);
    test_assert_buffers(str, read_buf, str_len);
    test_assert_ints(0, libqb_buffer_length(&buffer));

    read_len = libqb_buffer_read(&buffer, read_buf, sizeof(read_buf));

    test_assert_ints(0, read_len);
    test_assert_ints(0, libqb_buffer_length(&buffer));

    libqb_buffer_clear(&buffer);
}

void test_read_write_multiple_partial() {
    struct libqb_buffer buffer;

    libqb_buffer_init(&buffer);
    const char *strs[] = {
        "FOOBAR1",
        "FOOBAR2",
        "FOOBAR3",
        "FOOBAR4",
        "FOOBAR5",
        "FOOBAR6",
        "FOOBAR7",
        "FOOBAR8",
        "FOOBAR9",
    };
    int str_len = 7;
    int length = 0;

    test_assert_ints(0, libqb_buffer_length(&buffer));

    for (int i = 0; i < sizeof(strs) / sizeof(*strs); i++) {
        libqb_buffer_write(&buffer, strs[i], str_len);

        length += str_len;
        test_assert_ints(length, libqb_buffer_length(&buffer));
    }

    char temp_buf[50];
    size_t read_len;

    read_len = libqb_buffer_read(&buffer, temp_buf, 12);
    length -= 12;

    test_assert_ints(12, read_len);
    test_assert_ints(length, libqb_buffer_length(&buffer));
    test_assert_buffers("FOOBAR1FOOBA", temp_buf, 12);

    read_len = libqb_buffer_read(&buffer, temp_buf, 20);
    length -= 20;

    test_assert_ints(20, read_len);
    test_assert_ints(length, libqb_buffer_length(&buffer));
    test_assert_buffers("R2FOOBAR3FOOBAR4FOOB", temp_buf, 20);

    read_len = libqb_buffer_read(&buffer, temp_buf, 25);
    length -= 25;

    test_assert_ints(25, read_len);
    test_assert_ints(length, libqb_buffer_length(&buffer));
    test_assert_buffers("AR5FOOBAR6FOOBAR7FOOBAR8F", temp_buf, 25);

    // Read past the end, should only read the 6 characters left
    read_len = libqb_buffer_read(&buffer, temp_buf, 20);
    length -= 6;

    test_assert_ints(6, read_len);
    test_assert_ints(length, libqb_buffer_length(&buffer));
    test_assert_buffers("OOBAR9", temp_buf, 6);

    test_assert_ints(0, libqb_buffer_length(&buffer));

    libqb_buffer_clear(&buffer);
}

void test_read_write_interweaved() {
    struct libqb_buffer buffer;

    libqb_buffer_init(&buffer);
    const char *strs[] = {
        "FOOBAR1",
        "FOOBAR2",
        "FOOBAR3",
        "FOOBAR4",
        "FOOBAR5",
        "FOOBAR6",
        "FOOBAR7",
        "FOOBAR8",
        "FOOBAR9",
    };
    int str_len = 7;
    int length = 0;
    char temp_buf[50];
    size_t read_len;

    test_assert_ints(0, libqb_buffer_length(&buffer));

    // WRITE
    libqb_buffer_write(&buffer, "FOOBAR1", 7);
    length += 7;
    test_assert_ints(length, libqb_buffer_length(&buffer));

    // WRITE
    libqb_buffer_write(&buffer, "FOOBAR2", 7);
    length += 7;
    test_assert_ints(length, libqb_buffer_length(&buffer));

    // READ
    read_len = libqb_buffer_read(&buffer, temp_buf, 12);
    length -= 12;

    test_assert_ints(12, read_len);
    test_assert_ints(length, libqb_buffer_length(&buffer));
    test_assert_buffers("FOOBAR1FOOBA", temp_buf, 12);

    // WRITE
    libqb_buffer_write(&buffer, "FOOBAR3", 7);
    length += 7;
    test_assert_ints(length, libqb_buffer_length(&buffer));

    // READ
    read_len = libqb_buffer_read(&buffer, temp_buf, 50);
    length -= 9;

    test_assert_ints(9, read_len);
    test_assert_ints(length, libqb_buffer_length(&buffer));
    test_assert_buffers("R2FOOBAR3", temp_buf, 9);

    // WRITE
    libqb_buffer_write(&buffer, "FOOBAR4", 7);
    length += 7;
    test_assert_ints(length, libqb_buffer_length(&buffer));

    // WRITE
    libqb_buffer_write(&buffer, "FOOBAR5", 7);
    length += 7;
    test_assert_ints(length, libqb_buffer_length(&buffer));

    // READ
    read_len = libqb_buffer_read(&buffer, temp_buf, 50);
    length -= 14;

    test_assert_ints(14, read_len);
    test_assert_ints(length, libqb_buffer_length(&buffer));
    test_assert_buffers("FOOBAR4FOOBAR5", temp_buf, 14);

    test_assert_ints(0, libqb_buffer_length(&buffer));

    libqb_buffer_clear(&buffer);
}

int main() {
    struct unit_test tests[] = {
        { test_single_rw, "test-single-read-write" },
        { test_multiple_rw, "test-multiple-read-write" },
        { test_partial_read, "test-partial-read" },
        { test_full_read, "test-full-read" },
        { test_read_past_end, "test-read-past-end" },
        { test_read_write_multiple_partial, "test-read-write-multiple-partial" },
        { test_read_write_interweaved, "test-read-write-interweaved" },
    };

    return run_tests("buffer", tests, sizeof(tests) / sizeof(*tests));
}
