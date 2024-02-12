
#include "libqb-common.h"

#include <stdlib.h>
#include <string.h>

#include "gui.h"
#include "qblist.h"

list *list_new(intptr_t structure_size) {
    list *L;
    L = (list *)calloc(1, sizeof(list));
    L->structure = (uint8_t *)malloc(sizeof(uint8_t *));
    L->structure_base[1] = (intptr_t)L->structure;
    L->structure_bases = 1;
    L->structure_freed = (intptr_t *)malloc(sizeof(intptr_t *));
    L->index = (intptr_t *)malloc(sizeof(intptr_t *));
    L->user_structure_size = structure_size;
    L->internal_structure_size = structure_size + sizeof(intptr_t);
    return L;
}

list *list_new_threadsafe(intptr_t structure_size) {
    list *L = list_new(structure_size);
    L->lock_add = libqb_mutex_new();
    L->lock_remove = libqb_mutex_new();
    return L;
}

intptr_t list_add(list *L) {
    if (L->lock_add)
        libqb_mutex_lock(L->lock_add);

    intptr_t i;
    if (L->structures_freed) { // retrieve index from freed list if possible
        if (L->lock_remove)
            libqb_mutex_lock(L->lock_remove);

        i = L->structure_freed[L->structures_freed--];
        uint8_t *structure;
        structure = (uint8_t *)L->index[i];
        memset(structure, 0, L->user_structure_size);
        *(intptr_t *)(structure + L->user_structure_size) = i;

        if (L->lock_remove)
            libqb_mutex_unlock(L->lock_remove);
    } else {
        // create new buffer?
        if ((L->structures + 1) > L->structures_last) {
            intptr_t new_structures_last;
            new_structures_last = (L->structures_last * 2) + 1;
            // note: L->structure is only modified by list_add
            L->structure = (uint8_t *)calloc(1, L->internal_structure_size * (new_structures_last + 1));
            if (L->structure == NULL) {
                gui_alert("list_add: failed to allocate new buffer, structure size: %lld", (int64_t)L->internal_structure_size);
            }
            L->structures_last = new_structures_last;
            L->structures = 0;
            L->structure_base[++L->structure_bases] = (intptr_t)L->structure;
        }
        i = ++L->indexes;
        *(intptr_t *)(L->structure + (L->internal_structure_size * (++L->structures)) + L->user_structure_size) = i;
        // allocate new index
        if (L->indexes > L->indexes_last) {
            if (L->index_cleanup != NULL)
                free(L->index_cleanup);
            L->index_cleanup = L->index;
            int32_t new_indexes_last = (L->indexes_last * 2) + 1;
            intptr_t *temp = (intptr_t *)malloc(sizeof(intptr_t) * (new_indexes_last + 1));
            memcpy(temp, L->index, sizeof(intptr_t) * (L->indexes_last + 1));
            L->index = temp;
            L->index[i] = (intptr_t)(L->structure + (L->internal_structure_size * L->structures));
            L->indexes_last = new_indexes_last;
        } else {
            L->index[i] = (intptr_t)(L->structure + (L->internal_structure_size * L->structures));
        }
    }

    if (L->lock_add)
        libqb_mutex_unlock(L->lock_add);

    return i;
} // list_add

intptr_t list_remove(list *L, intptr_t i) { // returns -1 on success, 0 on failure
    if (L->lock_remove)
        libqb_mutex_lock(L->lock_remove);

    if ((i < 1) || (i > L->indexes)) {
        if (L->lock_remove)
            libqb_mutex_unlock(L->lock_remove);

        return 0;
    }
    uint8_t *structure;
    structure = (uint8_t *)(L->index[i]);
    if (!*(intptr_t *)(structure + L->user_structure_size)) {
        if (L->lock_remove)
            libqb_mutex_unlock(L->lock_remove);

        return 0;
    }
    // expand buffer?
    if ((L->structures_freed + 1) > L->structures_freed_last) {
        intptr_t new_structures_freed_last;
        new_structures_freed_last = (L->structures_freed_last * 2) + 1;
        intptr_t *temp = (intptr_t *)malloc(sizeof(intptr_t) * (new_structures_freed_last + 1));
        memcpy(temp, L->structure_freed, sizeof(intptr_t) * (L->structures_freed + 1));
        if (L->structure_freed_cleanup != NULL)
            free(L->structure_freed_cleanup);
        L->structure_freed_cleanup = L->structure_freed;
        L->structure_freed = temp;
        L->structures_freed_last = new_structures_freed_last;
    }
    L->structure_freed[L->structures_freed + 1] = i;
    *(intptr_t *)(structure + L->user_structure_size) = 0;
    L->structures_freed++;

    if (L->lock_remove)
        libqb_mutex_unlock(L->lock_remove);

    return -1;
};

void list_destroy(list *L) {
    intptr_t i;
    for (i = 1; i <= L->structure_bases; i++) {
        free((void *)L->structure_base[i]);
    }
    free(L->structure_base);
    free(L->structure_freed);
    free(L);
}

void *list_get(list *L, intptr_t i) { // Returns a pointer to an index's structure
    if ((i < 1) || (i > L->indexes)) {
        return NULL;
    }
    uint8_t *structure;
    structure = (uint8_t *)(L->index[i]);
    if (!*(intptr_t *)(structure + L->user_structure_size))
        return NULL;
    return (void *)structure;
}

intptr_t list_get_index(list *L, void *structure) { // Retrieves the index value of a structure
    intptr_t i = *(intptr_t *)(((uint8_t *)structure) + L->user_structure_size);
    return i;
}
