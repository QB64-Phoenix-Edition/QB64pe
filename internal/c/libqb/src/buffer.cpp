
#include "libqb-common.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "buffer.h"

void libqb_buffer_init(struct libqb_buffer *buffer) {
    memset(buffer, 0, sizeof(*buffer));

    buffer->tail = &buffer->head;
}

static void libqb_buffer_entry_free(struct libqb_buffer_entry *ent) {
    free(ent->data);
    free(ent);
}

void libqb_buffer_clear(struct libqb_buffer *buffer) {
    struct libqb_buffer_entry *entry = buffer->head;

    while (entry) {
        struct libqb_buffer_entry *nxt = entry->next;

        libqb_buffer_entry_free(entry);

        entry = nxt;
    }

    libqb_buffer_init(buffer);
}

size_t libqb_buffer_length(struct libqb_buffer *buffer) {
    return buffer->total_length;
}

size_t libqb_buffer_read(struct libqb_buffer *buffer, char *out, size_t length) {
    size_t actual_length = 0;

    while (buffer->head && length) {
        struct libqb_buffer_entry *entry = buffer->head;
        size_t offset = buffer->cur_entry_offset;

        size_t len = entry->length - offset;
        if (len > length)
            len = length;

        memcpy(out, entry->data + offset, len);

        out += len;
        length -= len;
        actual_length += len;

        if (len == entry->length - offset) {
            // This buffer is done, drop it
            buffer->head = buffer->head->next;
            buffer->cur_entry_offset = 0;

            libqb_buffer_entry_free(entry);
        } else {
            // We didn't use the whole buffer, length == 0, loop will end
            buffer->cur_entry_offset = offset + len;
        }
    }

    // If the list is now empty, we need to reset the tail pointer
    if (!buffer->head)
        buffer->tail = &buffer->head;

    buffer->total_length -= actual_length;

    return actual_length;
}

void libqb_buffer_write(struct libqb_buffer *buffer, const char *in, size_t length) {
    struct libqb_buffer_entry *new_ent = (struct libqb_buffer_entry *)malloc(sizeof(*new_ent));

    new_ent->length = length;
    new_ent->next = NULL;
    new_ent->data = (char *)malloc(length);

    memcpy(new_ent->data, in, length);

    *buffer->tail = new_ent;
    buffer->tail = &new_ent->next;
    buffer->total_length += length;
}
