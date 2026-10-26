/* the texture cache's allocator: a ring of blocks in the buffer
 * njInitCacheTextureBuffer gives it */
#include <ninja.h>
#include <sg_xpt.h>

/* the mark every block's header starts with */
#define BLOCK_MARK 0xa5

/* the bytes ahead of the address a block hands out */
#define BLOCK_HEADER 32

/* the most bytes a taken block keeps past its need: the size asked for plus
 * BLOCK_HEADER, rounded up to a multiple of 32. a block with more is split,
 * and the rest stays free. with the rounding, a taken block can hold up to 95
 * bytes past its header and the size asked for */
#define SLACK_MAX 64

struct block {
    Sint16 mark;
    Sint16 taken;
    Int size; /* including the header */
    struct block *next;
    struct block *prev;
    Uint8 unk_10[20];
};

/* the ring's sentinel, outside the buffer and always taken, so that no block
 * merges with it */
static struct block ring;
/* the buffer's first block, which nothing reads */
static struct block *first;

void njInitCacheTextureBuffer(Sint8 *addr, Uint32 size)
{
    struct block *block = (struct block *)addr;

    ring.size = sizeof(ring);
    ring.mark = BLOCK_MARK;
    ring.taken = TRUE;
    ring.next = ring.prev = block;
    block->size = size;
    block->mark = BLOCK_MARK;
    block->taken = FALSE;
    block->next = block->prev = &ring;
    first = ring.next;
}

/* take the first free block of at least size + BLOCK_HEADER bytes, rounded up
 * to a multiple of 32, and return the address past its header. returns NULL
 * when no free block is that large */
void *njCacheTextureMalloc(Uint32 size)
{
    struct block *block;
    struct block *rest;
    Int need = (size + BLOCK_HEADER + 31) & ~31;

    for (block = ring.next; block != &ring; block = block->next) {
        if (!block->taken && block->size >= need) {
            if (block->size - need > SLACK_MAX) {
                rest = (struct block *)((Uint8 *)block + need);
                rest->size = block->size - need;
                rest->mark = BLOCK_MARK;
                rest->taken = FALSE;
                block->next->prev = rest;
                rest->next = block->next;
                block->next = rest;
                rest->prev = block;
                block->size = need;
            }
            block->taken = TRUE;
            return (Uint8 *)block + BLOCK_HEADER;
        }
    }
    return NULL;
}

/* free the block whose header lies just before addr, and merge it with each
 * free neighbor. an addr whose header lacks BLOCK_MARK is ignored */
void njCacheTextureFree(void *addr)
{
    struct block *block = (struct block *)((Uint8 *)addr - BLOCK_HEADER);

    if (block->mark == BLOCK_MARK) {
        if (!block->prev->taken) {
            /* @hack redundant: join, a label nothing jumps to, which gives the
             * merge with the block before R2 and R1 (0x8c67514c to 0x8c67515e);
             * without it, R1 and R0, and 8 of 40 words differ */
        join: /* NOLINT(clang-diagnostic-unused-label) */
            block->next->prev = block->prev;
            block->prev->next = block->next;
            block->prev->size += block->size;
            block = block->prev;
        }
        if (!block->next->taken) {
            block->next->next->prev = block;
            block->size += block->next->size;
            block->next = block->next->next;
        }
        block->taken = FALSE;
    }
}
