/* the Shinobi heap, behind syMalloc and syFree and the C library's malloc,
 * realloc, calloc and free */
#include <machine.h>
#include <sg_xpt.h>
#include <shinobi.h>
#include <stdlib.h>

/* the heap's 32-byte unit. a block is a run of units, the first its header */
union heap_unit {
    struct {
        union heap_unit *next; /* the next free block, while this is free */
        Uint32 units; /* the block's size in units, the header included */
    } header;
    Sint8 bytes[32];
};

/* the free list is circular, in address order, through free_head, which has
 * no units. a search starts after free_last, where the last search or free
 * stopped */
static union heap_unit free_head;
static union heap_unit *free_last;
/* the heap's start, which syMallocInit stores twice, and its size. nothing
 * reads heap_first or heap_size */
static union heap_unit *heap_base;
static Uint32 heap_size;
static union heap_unit *heap_first;

char *sygMallocBuild = "\r\nsyMalloc Ver 1.01 Build:Sep 17 1998 19:49:50\r\n";
/* the major version, then the minor */
const Uint16 sygMallocVer[2] = {1, 1};

void syMallocInit(void *heap, Uint32 size)
{
    union heap_unit *block;

    /* @hack spelling: both statics set in one statement, which stores the
     * address from R4 into each (0x8c61c2ee and 0x8c61c2f0, and 0x8ce09472
     * and 0x8ce09474 in LOADER.BIN); as two statements, SHC copies it to R2
     * first, and 16 of 26 words differ */
    heap_first = heap_base =
        (union heap_unit *)(((Uint32)heap + sizeof(union heap_unit)) &
                            ~(sizeof(union heap_unit) - 1));
    heap_size = size;
    free_head.header.next = free_last = &free_head;
    free_head.header.units = 0;
    /* free the heap onto the list as one block */
    block = heap_base;
    /* @bug memory: the heap starts up to 32 bytes past heap (ADD
     * #32,R4 at 0x8c61c2e2, and at 0x8ce09466 in LOADER.BIN), but its block
     * still counts all of size. so the heap can reach 32 bytes past the end
     * the caller gave */
    block->header.units = size / sizeof(union heap_unit);
    free(block + 1);
}

void syMallocFinish(void)
{
}

void syMallocStat(Uint32 *wholeFreeSize, Uint32 *biggestFreeBlockSize)
{
    Uint32 biggest;
    Uint32 whole;
    union heap_unit *block;
    union heap_unit *before;
    Uint16 mask;

    mask = get_imask();
    set_imask(IMASK_ALL);
    before = free_last;
    whole = 0;
    biggest = 0;
    for (block = before->header.next;;
         before = block, block = block->header.next) {
        if (block->header.units > 0) {
            whole += block->header.units - 1;
            if (biggest < block->header.units - 1) {
                biggest = block->header.units - 1;
            }
        }
        if (block == free_last) {
            break;
        }
    }
    *wholeFreeSize = whole * sizeof(union heap_unit);
    *biggestFreeBlockSize = biggest * sizeof(union heap_unit);
    set_imask(mask);
}

void *syMalloc(Uint32 size)
{
    return malloc(size);
}

/* take the first free block after free_last with room for size bytes and a
 * header. a larger block gives up its end */
void *malloc(size_t size)
{
    union heap_unit *block;
    union heap_unit *before;
    Uint32 units;
    Uint16 mask;

    mask = get_imask();
    set_imask(IMASK_ALL);
    units =
        ((size + sizeof(union heap_unit) - 1) / sizeof(union heap_unit)) + 1;
    before = free_last;
    for (block = before->header.next;;
         before = block, block = block->header.next) {
        if (block->header.units >= units) {
            if (block->header.units == units) {
                before->header.next = block->header.next;
            } else {
                block->header.units -= units;
                block += block->header.units;
                block->header.units = units;
            }
            free_last = before;
            set_imask(mask);
            return block + 1;
        }
        if (block == free_last) {
            set_imask(mask);
            return NULL;
        }
    }
}

void syFree(void *ptr)
{
    free(ptr);
}

/* return the block to the free list, merged with an adjacent free block on
 * either side */
void free(void *ptr)
{
    union heap_unit *block;
    union heap_unit *before;
    Uint16 mask;

    mask = get_imask();
    set_imask(IMASK_ALL);
    if (ptr == NULL) {
        set_imask(mask);
        return;
    }
    block = (union heap_unit *)ptr - 1;
    /* find the free block below it, or the highest free block when the
     * block lies where the list wraps */
    for (before = free_last; !(block > before && block < before->header.next);
         before = before->header.next) {
        if (before >= before->header.next &&
            (block > before || block < before->header.next)) {
            break;
        }
    }
    if (block + block->header.units == before->header.next) {
        block->header.units += before->header.next->header.units;
        block->header.next = before->header.next->header.next;
    } else {
        block->header.next = before->header.next;
    }
    if (before + before->header.units == block) {
        before->header.units += block->header.units;
        before->header.next = block->header.next;
    } else {
        before->header.next = block;
    }
    free_last = before;
    set_imask(mask);
}

/* free ptr's block, then take a block as malloc does. unless it starts
 * where ptr's did, copy one unit more than the smaller block's data into it.
 * a new block below the old one takes the units from ptr up. one above takes
 * them from ptr down, through the old header and the units below it. they
 * land one unit above the new data, whose first unit stays unwritten. ptr
 * must not be NULL. when no block fits, the result is NULL, and ptr's block
 * stays freed */
void *realloc(void *ptr, size_t size)
{
    union heap_unit *to;
    union heap_unit *from;
    union heap_unit *before;
    union heap_unit *block;
    union heap_unit *rest;
    Uint32 units;
    Uint32 kept;
    Uint16 mask;

    mask = get_imask();
    set_imask(IMASK_ALL);
    units =
        ((size + sizeof(union heap_unit) - 1) / sizeof(union heap_unit)) + 1;
    to = (union heap_unit *)ptr - 1;
    if (units <= to->header.units) {
        kept = units;
    } else {
        kept = to->header.units;
    }
    /* @hack redundant: an in-place shrink that never runs, kept as in the
     * image, which keeps ptr in R14 (0x8c61c50a) and units in R13 (0x8c61c524);
     * without it, the function runs 4 bytes short and 157 of 163 words
     * differ */
    if (0) {
        rest = to + units;
        rest->header.units = kept - units;
        to->header.units = units;
        free(rest + 1);
        return ptr;
    }
    from = (union heap_unit *)ptr;
    free(ptr);
    before = free_last;
    for (block = before->header.next;;
         before = block, block = block->header.next) {
        if (block->header.units >= units) {
            if (block->header.units == units) {
                before->header.next = block->header.next;
            } else {
                block->header.units -= units;
                block += block->header.units;
            }
            /* @hack redundant: an empty arm for from == block + 1, which keeps
             * its test (CMP/EQ R14,R12 at 0x8c61c586, and at 0x8ce0970a in
             * LOADER.BIN); without it, the function is 4 bytes shorter and
             * 101 of 163 words differ */
            if (from == block + 1) {
            } else if (from < block + 1) {
                /* @bug memory: the copy writes down from block + 1 +
                 * kept to block + 2 (its start at 0x8c61c58e to 0x8c61c592),
                 * while it reads down from the old data's first unit (the loop
                 * at 0x8c61c598 to 0x8c61c5be). so it reads the old header and
                 * the units below it, and it never writes the new data's first
                 * unit, block + 1. when kept is units, its first two writes
                 * land past the new block. LOADER.BIN holds the same at
                 * 0x8ce09712 to 0x8ce09716 and 0x8ce0971c to 0x8ce09742 */
                for (to = block + 1 + kept; to > block + 1;) {
                    *to-- = *from--;
                }
            } else if (from > block + 1) {
                /* @bug memory: kept counts the header unit, so the copy
                 * up to block + 1 + kept (0x8c61c5ca to 0x8c61c5ce) moves one
                 * unit more than the smaller block's data (0x8c61c5d4 to
                 * 0x8c61c5fa). it reads the unit after the old block when kept
                 * is the old block's units. it writes the unit after the new
                 * block when kept is units. LOADER.BIN holds the same at
                 * 0x8ce0974e to 0x8ce09752 and 0x8ce09758 to 0x8ce0977e */
                for (to = block + 1; to < block + 1 + kept;) {
                    *to++ = *from++;
                }
            }
            block->header.units = units;
            free_last = before;
            set_imask(mask);
            return block + 1;
        }
        if (block == free_last) {
            set_imask(mask);
            return NULL;
        }
    }
}

/* allocate nmemb times size bytes and clear them a word at a time, through
 * the word that holds the last byte. the second loop runs only for a size
 * from 1 to 3, over bytes the first loop has cleared */
void *calloc(size_t nmemb, size_t size)
{
    Uint8 *memory;
    Uint32 offset;
    Uint16 mask;

    mask = get_imask();
    set_imask(IMASK_ALL);
    size *= nmemb;
    /* @hack spelling: memory assigned in the test, which SHC tests in R0 (TST
     * R0,R0 at 0x8c61c67e, and at 0x8ce09802 in LOADER.BIN) and copies to R7
     * in the branch's delay slot; assigned first, it is tested in R7, and 9
     * of 75 words differ */
    /* @hack redundant: an else after the return, which puts set_imask's mask
     * constant in R2 and SR in R3 (0x8c61c6d0 and 0x8c61c6d2, and 0x8ce09854
     * and 0x8ce09856 in LOADER.BIN); without it, the two trade and 4 of 75
     * words differ */
    if ((memory = malloc(size)) != NULL) {
        for (offset = 0; offset < size; offset += 4) {
            *(Uint32 *)(memory + offset) = 0;
        }
        for (offset -= 4; offset < (size & 3); offset++) {
            memory[offset] = 0;
        }
        set_imask(mask);
        return memory;
    } else {
        set_imask(mask);
        return NULL;
    }
}
