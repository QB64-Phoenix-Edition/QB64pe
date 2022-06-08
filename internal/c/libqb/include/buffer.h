#ifndef INCLUDE_LIBQB_BUFFER_H
#define INCLUDE_LIBQB_BUFFER_H

struct libqb_buffer_entry {
    size_t length;
    char *data;
    struct libqb_buffer_entry *next;
};

struct libqb_buffer {
    size_t total_length;
    size_t cur_entry_offset;

    struct libqb_buffer_entry *head;
    struct libqb_buffer_entry **tail;
};

void libqb_buffer_init(struct libqb_buffer *);

// Free's all data used by the buffer.
void libqb_buffer_clear(struct libqb_buffer *);

// Returns the current length of the buffer.
size_t libqb_buffer_length(struct libqb_buffer *);

// Reads data from the buffer. The data read is removed from the buffer
//
// Returns the number of bytes actually read
size_t libqb_buffer_read(struct libqb_buffer *, char *, size_t length);

// Writes data into the buffer
void libqb_buffer_write(struct libqb_buffer *, const char *, size_t length);

#endif
