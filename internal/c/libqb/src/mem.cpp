
#include "libqb-common.h"

#include <stdlib.h>
#include <string.h>

#include "error_handle.h"
#include "mem.h"

// QB64 memory blocks
uint64_t mem_lock_id = 1073741823; // this value should never be 0 or 1
int32_t mem_lock_max = 10000;
int32_t mem_lock_next = 0;
mem_lock *mem_lock_base = (mem_lock *)malloc(sizeof(mem_lock) * mem_lock_max);
mem_lock *mem_lock_tmp;

int32_t mem_lock_freed_max = 1000; // number of allocated entries
int32_t mem_lock_freed_n = 0;      // number of entries
intptr_t *mem_lock_freed = (intptr_t *)malloc(sizeof(intptr_t) * mem_lock_freed_max);

void new_mem_lock() {
    if (mem_lock_freed_n) {
        mem_lock_tmp = (mem_lock *)mem_lock_freed[--mem_lock_freed_n];
    } else {
        if (mem_lock_next == mem_lock_max) {
            mem_lock_base = (mem_lock *)malloc(sizeof(mem_lock) * mem_lock_max);
            mem_lock_next = 0;
        }
        mem_lock_tmp = &mem_lock_base[mem_lock_next++];
    }
    mem_lock_tmp->id = ++mem_lock_id;
}

void free_mem_lock(mem_lock *lock) {
    lock->id = 0; // invalidate lock
    if (lock->type == 1)
        free(lock->offset); // malloc type
    // add to freed list
    if (mem_lock_freed_n == mem_lock_freed_max) {
        mem_lock_freed_max *= 2;
        mem_lock_freed = (intptr_t *)realloc(mem_lock_freed, sizeof(intptr_t) * mem_lock_freed_max);
    }
    mem_lock_freed[mem_lock_freed_n++] = (intptr_t)lock;
}

void sub__memfree(void *mem) {
    // 1:malloc: memory will be freed if it still exists
    // 2:images: will not be freed, no action will be taken
    // exists?
    if (((mem_block *)(mem))->lock_offset == NULL) {
        error(309);
        return;
    }
    if (((mem_lock *)(((mem_block *)(mem))->lock_offset))->id != ((mem_block *)(mem))->lock_id) {
        error(307);
        return;
    }                                                                   // memory has been freed
    if (((mem_lock *)(((mem_block *)(mem))->lock_offset))->type == 0) { // no security
        free_mem_lock((mem_lock *)((mem_block *)(mem))->lock_offset);
    }
    if (((mem_lock *)(((mem_block *)(mem))->lock_offset))->type == 1) { // malloc
        free_mem_lock((mem_lock *)((mem_block *)(mem))->lock_offset);
    }
    // note: type 2(image) is freed when the image is freed
    // invalidate caller's mem structure (avoids misconception that _MEMFREE failed)
    ((mem_block *)(mem))->lock_id = 1073741821;
}

extern mem_block func__mem_at_offset(intptr_t offset, intptr_t size) {
    static mem_block b;
    new_mem_lock();
    mem_lock_tmp->type = 0; // unsecured
    b.lock_offset = (intptr_t)mem_lock_tmp;
    b.lock_id = mem_lock_id;
    b.offset = offset;
    b.size = size;
    b.type = 16384; //_MEMNEW type
    b.elementsize = 1;
    b.image = -1;
    if ((size < 0) || is_error_pending()) {
        b.type = 0;
        b.size = 0;
        b.offset = 0;
        if (size < 0)
            error(301);
    }
    return b;
}

mem_block func__memnew(intptr_t bytes) {
    static mem_block b;
    new_mem_lock();
    b.lock_offset = (intptr_t)mem_lock_tmp;
    b.lock_id = mem_lock_id;
    b.type = 16384; //_MEMNEW type
    b.elementsize = 1;
    b.image = -1;
    if (is_error_pending()) {
        b.type = 0;
        b.offset = 0;
        b.size = 0;
        mem_lock_tmp->type = 0;
        return b;
    }

    if (bytes < 0) {
        // still create a block, but an invalid one and generate an error
        error(5);
        b.offset = 0;
        b.size = 0;
        mem_lock_tmp->type = 0;
    } else {
        if (!bytes) {
            b.offset = 1; // non-zero=success
            b.size = 0;
        } else {
            b.offset = (intptr_t)malloc(bytes);
            if (!b.offset) {
                b.size = 0;
                mem_lock_tmp->type = 0;
            } else {
                b.size = bytes;
                mem_lock_tmp->type = 1;
                mem_lock_tmp->offset = (void *)b.offset;
            }
        }
    }
    return b;
}

int32_t func__memexists(void *void_blk) {
    static mem_block *blk;
    blk = (mem_block *)void_blk;
    if (((mem_block *)(blk))->lock_offset == NULL)
        return 0;
    if (((mem_lock *)(((mem_block *)(blk))->lock_offset))->id == ((mem_block *)(blk))->lock_id)
        return -1;
    return 0;
}

void *func__memget(mem_block *blk, intptr_t off, intptr_t bytes) {
    // checking A
    if (((mem_block *)(blk))->lock_offset == NULL) {
        error(309);
        goto fail;
    }
    // checking B
    if (off < ((mem_block *)(blk))->offset || (off + bytes) > (((mem_block *)(blk))->offset + ((mem_block *)(blk))->size) ||
        ((mem_lock *)(((mem_block *)(blk))->lock_offset))->id != ((mem_block *)(blk))->lock_id) {
        // error reporting
        if (((mem_lock *)(((mem_block *)(blk))->lock_offset))->id != ((mem_block *)(blk))->lock_id) {
            error(308);
            goto fail;
        }
        error(300);
        goto fail;
    }
    return (void *)off;
//------------------------------------------------------------
fail:
    static void *fail_buffer;
    fail_buffer = calloc(bytes, 1);
    if (!fail_buffer)
        error(518); // critical error: out of memory
    return fail_buffer;
}

void sub__memfill_nochecks(intptr_t doff, intptr_t dbytes, intptr_t soff, intptr_t sbytes) {
    if (sbytes == 1) {
        memset((void *)doff, *(uint8_t *)soff, dbytes);
        return;
    }
    static intptr_t si;
    si = 0;
    while (dbytes--) {
        *(int8_t *)(doff++) = *(int8_t *)(soff + si++);
        if (si >= sbytes)
            si = 0;
    }
}

void sub__memfill(mem_block *dblk, intptr_t doff, intptr_t dbytes, intptr_t soff, intptr_t sbytes) {
    if (((mem_block *)(dblk))->lock_offset == NULL) {
        error(309);
        return;
    }
    if (((mem_lock *)(((mem_block *)(dblk))->lock_offset))->id != ((mem_block *)(dblk))->lock_id) {
        error(308);
        return;
    }
    if ((dbytes < 0) || (sbytes == 0)) {
        error(301);
        return;
    }
    if (doff < ((mem_block *)(dblk))->offset || (doff + dbytes) > (((mem_block *)(dblk))->offset + ((mem_block *)(dblk))->size)) {
        error(300);
        return;
    }
    sub__memfill_nochecks(doff, dbytes, soff, sbytes);
}

void sub__memfill_1(mem_block *dblk, intptr_t doff, intptr_t dbytes, int8_t val) { sub__memfill(dblk, doff, dbytes, (intptr_t)&val, 1); }
void sub__memfill_nochecks_1(intptr_t doff, intptr_t dbytes, int8_t val) { sub__memfill_nochecks(doff, dbytes, (intptr_t)&val, 1); }
void sub__memfill_2(mem_block *dblk, intptr_t doff, intptr_t dbytes, int16_t val) { sub__memfill(dblk, doff, dbytes, (intptr_t)&val, 2); }
void sub__memfill_nochecks_2(intptr_t doff, intptr_t dbytes, int16_t val) { sub__memfill_nochecks(doff, dbytes, (intptr_t)&val, 2); }
void sub__memfill_4(mem_block *dblk, intptr_t doff, intptr_t dbytes, int32_t val) { sub__memfill(dblk, doff, dbytes, (intptr_t)&val, 4); }
void sub__memfill_nochecks_4(intptr_t doff, intptr_t dbytes, int32_t val) { sub__memfill_nochecks(doff, dbytes, (intptr_t)&val, 4); }
void sub__memfill_8(mem_block *dblk, intptr_t doff, intptr_t dbytes, int64_t val) { sub__memfill(dblk, doff, dbytes, (intptr_t)&val, 8); }
void sub__memfill_nochecks_8(intptr_t doff, intptr_t dbytes, int64_t val) { sub__memfill_nochecks(doff, dbytes, (intptr_t)&val, 8); }
void sub__memfill_SINGLE(mem_block *dblk, intptr_t doff, intptr_t dbytes, float val) { sub__memfill(dblk, doff, dbytes, (intptr_t)&val, 4); }
void sub__memfill_nochecks_SINGLE(intptr_t doff, intptr_t dbytes, float val) { sub__memfill_nochecks(doff, dbytes, (intptr_t)&val, 4); }
void sub__memfill_DOUBLE(mem_block *dblk, intptr_t doff, intptr_t dbytes, double val) { sub__memfill(dblk, doff, dbytes, (intptr_t)&val, 8); }
void sub__memfill_nochecks_DOUBLE(intptr_t doff, intptr_t dbytes, double val) { sub__memfill_nochecks(doff, dbytes, (intptr_t)&val, 8); }

static uint8_t memfill_FLOAT_padding[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}; // 32 null bytes

void sub__memfill_FLOAT(mem_block *dblk, intptr_t doff, intptr_t dbytes, long double val) {
    *(long double *)memfill_FLOAT_padding = val;
    sub__memfill(dblk, doff, dbytes, (intptr_t)memfill_FLOAT_padding, 32);
}

void sub__memfill_nochecks_FLOAT(intptr_t doff, intptr_t dbytes, long double val) {
    *(long double *)memfill_FLOAT_padding = val;
    sub__memfill_nochecks(doff, dbytes, (intptr_t)memfill_FLOAT_padding, 32);
}

void sub__memfill_OFFSET(mem_block *dblk, intptr_t doff, intptr_t dbytes, intptr_t val) { sub__memfill(dblk, doff, dbytes, (intptr_t)&val, sizeof(intptr_t)); }
void sub__memfill_nochecks_OFFSET(intptr_t doff, intptr_t dbytes, intptr_t val) { sub__memfill_nochecks(doff, dbytes, (intptr_t)&val, sizeof(intptr_t)); }

void sub__memcopy(void *sblk, intptr_t soff, intptr_t bytes, void *dblk, intptr_t doff) {
    // checking A
    if (((mem_block *)(sblk))->lock_offset == NULL || ((mem_block *)(dblk))->lock_offset == NULL) {
        // error reporting
        if (((mem_block *)(sblk))->lock_offset == NULL && ((mem_block *)(dblk))->lock_offset == NULL) {
            error(312);
            return;
        }
        if (((mem_block *)(sblk))->lock_offset == NULL) {
            error(310);
            return;
        }
        error(311);
        return;
    }
    // checking B
    if (bytes < 0 || soff < ((mem_block *)(sblk))->offset || (soff + bytes) > (((mem_block *)(sblk))->offset + ((mem_block *)(sblk))->size) ||
        doff < ((mem_block *)(dblk))->offset || (doff + bytes) > (((mem_block *)(dblk))->offset + ((mem_block *)(dblk))->size) ||
        ((mem_lock *)(((mem_block *)(sblk))->lock_offset))->id != ((mem_block *)(sblk))->lock_id ||
        ((mem_lock *)(((mem_block *)(dblk))->lock_offset))->id != ((mem_block *)(dblk))->lock_id) {
        // error reporting
        if (((mem_lock *)(((mem_block *)(sblk))->lock_offset))->id != ((mem_block *)(sblk))->lock_id &&
            ((mem_lock *)(((mem_block *)(dblk))->lock_offset))->id != ((mem_block *)(dblk))->lock_id) {
            error(313);
            return;
        }
        if (((mem_lock *)(((mem_block *)(sblk))->lock_offset))->id != ((mem_block *)(sblk))->lock_id) {
            error(305);
            return;
        }
        if (((mem_lock *)(((mem_block *)(dblk))->lock_offset))->id != ((mem_block *)(dblk))->lock_id) {
            error(306);
            return;
        }
        if (bytes < 0) {
            error(301);
            return;
        }
        if (soff < ((mem_block *)(sblk))->offset || (soff + bytes) > (((mem_block *)(sblk))->offset + ((mem_block *)(sblk))->size)) {
            if (doff < ((mem_block *)(dblk))->offset || (doff + bytes) > (((mem_block *)(dblk))->offset + ((mem_block *)(dblk))->size)) {
                error(304);
                return;
            }
            error(302);
            return;
        }
        error(303);
        return;
    }
    memmove((char *)doff, (char *)soff, bytes);
}

mem_block func__mem(intptr_t offset, intptr_t size, int32_t type, intptr_t elementsize, mem_lock *lock) {
    static mem_block b;
    b.lock_offset = (intptr_t)lock;
    b.lock_id = lock->id;
    b.offset = offset;
    b.size = size;
    b.type = type;
    b.elementsize = elementsize;
    b.image = -1;
    return b;
}
