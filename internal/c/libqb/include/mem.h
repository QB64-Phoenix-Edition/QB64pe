#pragma once

#include <stdint.h>

struct mem_block {
    intptr_t offset;
    intptr_t size;
    int64_t lock_id;       // 64-bit key, must be present at lock's offset or memory region is invalid
    intptr_t lock_offset; // pointer to lock
    intptr_t type;        // https://qb64phoenix.com/qb64wiki/index.php/MEM
    intptr_t elementsize;
    int32_t image;
    int32_t sound;
};

#define INVALID_MEM_LOCK 1073741821

#define MEM_TYPE_NOSECURITY 0
#define MEM_TYPE_MALLOC 1
#define MEM_TYPE_IMAGE 2
#define MEM_TYPE_SUBFUNC 3
#define MEM_TYPE_ARRAY 4
#define MEM_TYPE_SOUND 5

struct mem_lock {
    int64_t id;
    int32_t type; // required to know what action to take (if any) when a request is made to free the block
    // 0=no security (eg. user defined block from _OFFSET)
    // 1=C-malloc'ed block
    // 2=image
    // 3=sub/function scope block
    // 4=array
    // 5=sound
    //---- type specific variables follow ----
    void *offset; // used by malloc'ed blocks to free them
};

extern uint64_t mem_lock_id;
extern mem_lock *mem_lock_tmp;
extern mem_lock *mem_lock_base;

int32_t func__memexists(void *blk);

void sub__memfill(mem_block *dblk, intptr_t doff, intptr_t dbytes, intptr_t soff, intptr_t sbytes);
void sub__memfill_nochecks(intptr_t doff, intptr_t dbytes, intptr_t soff, intptr_t sbytes);
void sub__memfill_1(mem_block *dblk, intptr_t doff, intptr_t dbytes, int8_t val);
void sub__memfill_nochecks_1(intptr_t doff, intptr_t dbytes, int8_t val);
void sub__memfill_2(mem_block *dblk, intptr_t doff, intptr_t dbytes, int16_t val);
void sub__memfill_nochecks_2(intptr_t doff, intptr_t dbytes, int16_t val);
void sub__memfill_4(mem_block *dblk, intptr_t doff, intptr_t dbytes, int32_t val);
void sub__memfill_nochecks_4(intptr_t doff, intptr_t dbytes, int32_t val);
void sub__memfill_8(mem_block *dblk, intptr_t doff, intptr_t dbytes, int64_t val);
void sub__memfill_nochecks_8(intptr_t doff, intptr_t dbytes, int64_t val);
void sub__memfill_SINGLE(mem_block *dblk, intptr_t doff, intptr_t dbytes, float val);
void sub__memfill_nochecks_SINGLE(intptr_t doff, intptr_t dbytes, float val);
void sub__memfill_DOUBLE(mem_block *dblk, intptr_t doff, intptr_t dbytes, double val);
void sub__memfill_nochecks_DOUBLE(intptr_t doff, intptr_t dbytes, double val);
void sub__memfill_FLOAT(mem_block *dblk, intptr_t doff, intptr_t dbytes, long double val);
void sub__memfill_nochecks_FLOAT(intptr_t doff, intptr_t dbytes, long double val);
void sub__memfill_OFFSET(mem_block *dblk, intptr_t doff, intptr_t dbytes, intptr_t val);
void sub__memfill_nochecks_OFFSET(intptr_t doff, intptr_t dbytes, intptr_t val);

void *func__memget(mem_block *blk, intptr_t off, intptr_t bytes);

void new_mem_lock();
void free_mem_lock(mem_lock *lock);

mem_block func__mem(intptr_t offset, intptr_t size, int32_t type, intptr_t elementsize, mem_lock *lock);
mem_block func__mem_at_offset(intptr_t offset, intptr_t size);

mem_block func__memnew(intptr_t);
void sub__memfree(void *);

void sub__memcopy(void *sblk, intptr_t soff, intptr_t bytes, void *dblk, intptr_t doff);
