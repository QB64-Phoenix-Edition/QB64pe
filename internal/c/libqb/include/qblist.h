#pragma once

#include <stdint.h>
#include "mutex.h"

// List Interface
// Purpose: Unify and optimize the way QB64 references lists of objects (such as handles)
// Notes: Does not use index 0
struct list {
    intptr_t user_structure_size;
    intptr_t internal_structure_size;
    uint8_t *structure; // block of structures of user-specified size
    intptr_t structures;
    intptr_t structures_last;
    intptr_t *structure_freed;         // quickly re-reference available structures after they have been removed
    intptr_t *structure_freed_cleanup; // the previous *structure_freed memory block
    intptr_t structures_freed;
    intptr_t structures_freed_last;
    intptr_t structure_base[64]; // every time the 'structure' block is full a new and larger block is allocated
    // because the list doubles each time, 64 entries will never be exceeded
    intptr_t structure_bases;
    intptr_t *index; // pointers to the structures referred to by each index value
    intptr_t *index_cleanup;
    intptr_t indexes;
    intptr_t indexes_last;
    struct libqb_mutex *lock_add;
    struct libqb_mutex *lock_remove;
};

list *list_new(intptr_t structure_size);
list *list_new_threadsafe(intptr_t structure_size);
void list_destroy(list *L);

intptr_t list_add(list *L);
intptr_t list_remove(list *L, intptr_t i); // returns -1 on success, 0 on failure
                                           //
void *list_get(list *L, intptr_t i); // Returns a pointer to an index's structure
                                     //
intptr_t list_get_index(list *L, void *structure); // Retrieves the index value of a structure
