/* SDK 1.20J Kamui video memory allocation for frame buffers, textures and VQ
 * pages. the memory is two banks, each a run of offsets in the 64-bit view.
 * each bank keeps two lists of block records from a pool: its free blocks and
 * its allocated ones. a pair is two blocks of one size at the same offset into
 * each bank, and each is the other's twin. small VQ textures share VQ pages,
 * each of which takes a block */
#include <kamui.h>
#include <sg_xpt.h>

#include "loader/sdk/fballoc.h"

/* the size of each pool, in block records and in VQ page records */
#define BLOCKS 4096
#define VQ_PAGES 1024

/* ALIGN is the allocation unit: sizes round up to a multiple of it. mipmaps and
 * VQ pages are placed by the boundaries of PAGE-byte pages */
#define ALIGN 32
#define PAGE 0x800

/* a 16-bit mipmap's 32-by-32 level starts this many bytes into it, after the
 * smaller levels and the pad before them */
#define LEVEL_32X32 0x2b0

/* the video chip's texture control register, as an offset into its register
 * block */
#define TEXTURE_CONTROL 0x0e4

/* block flags. an idle block is a pair's allocated half that no surface uses.
 * kmiMIPalloc can take one */
#define BLOCK_TAKEN 0x0001 /* the record is out of the pool */
#define BLOCK_PAIRED 0x0002 /* the block is half of a pair */
#define BLOCK_LIVE 0x0004 /* the block is allocated and not idle */
/* what an allocated block holds, and the call that cut it. fb_alloc_at and
 * fb_alloc_vq_at cut the PLACED blocks at offsets their callers choose */
#define BLOCK_FRAME 0x0008 /* frame buffers, by fb_alloc_frame */
#define BLOCK_TEXTURE0 0x0020 /* a texture in bank 0, by fb_alloc_texture */
#define BLOCK_TEXTURE1 0x0040 /* a texture in bank 1, by fb_alloc_texture */
#define BLOCK_PLACED0 0x0080 /* a texture in bank 0, by fb_alloc_at */
#define BLOCK_PLACED1 0x0100 /* a texture in bank 1, by fb_alloc_at */
#define BLOCK_VQ0 0x0200 /* a VQ page in bank 0, by fb_alloc_vq */
#define BLOCK_VQ1 0x0400 /* a VQ page in bank 1, by fb_alloc_vq */
#define BLOCK_VQ_PLACED0 0x0800 /* a VQ page in bank 0, by fb_alloc_vq_at */
#define BLOCK_VQ_PLACED1 0x1000 /* a VQ page in bank 1, by fb_alloc_vq_at */
#define BLOCK_TA 0x2000 /* tile accelerator buffers, by fb_alloc_frame */

/* VQ page record flags */
#define PAGE_TAKEN 0x0001 /* the record is out of the pool */
#define PAGE_USED 0x0002 /* a texture has had a slot; nothing reads it */
#define PAGE_KIND 0xf000 /* the page's kind, a KIND_ value */

/* the kinds of VQ page, by the width of the textures in it and whether they
 * have mipmaps */
#define KIND_8 0x1000
#define KIND_8_MIPMAPS 0x2000
#define KIND_16 0x3000
#define KIND_16_MIPMAPS 0x4000
#define KIND_32 0x5000
#define KIND_32_MIPMAPS 0x6000
#define KIND_64 0x7000
#define KIND_64_MIPMAPS 0x8000

/* the most textures a VQ page holds */
#define SLOTS 16
/* the codebook bytes a VQ page holds. a shape's slots are as many as this holds
 * codebooks of its size */
#define PAGE_CODEBOOKS 0x800

/* fb_move copies video memory CHUNK bytes at a time through a buffer, and
 * writes the buffer back from the cache by its CACHE_LINE-byte lines */
#define CHUNK 0x800
#define CACHE_LINE 32

/* a block record: a run of video memory in one bank, free or allocated. offset
 * is in the 64-bit view, and size in bytes. owner holds the address of what an
 * allocated block was allocated for: a surface, a VQ page, or kmiTAallocEx's
 * first, which gets the offset */
struct fb_block {
    KMWORD flags;
    struct fb_block *prev;
    struct fb_block *next;
    KMINT32 offset;
    KMINT32 size;
    KMDWORD owner;
};

/* a bank's used list of allocated blocks, live and idle, and its free list,
 * each with its tail. free_size counts the bytes on the free list */
struct fb_bank {
    struct fb_block *used;
    struct fb_block *used_tail;
    struct fb_block *free;
    struct fb_block *free_tail;
    KMDWORD free_size;
};

/* a VQ page: one block that up to SLOTS small VQ textures of one kind share, a
 * slot each. offset is the block's offset, and slot[i] the surface in slot i,
 * or NULL */
struct fb_vq_page {
    KMWORD flags;
    struct fb_vq_page *next;
    KMINT32 offset;
    KMSURFACEDESC *slot[SLOTS];
};

/* how a VQ page of one kind is laid out */
struct fb_vq_shape {
    KMDWORD kind; /* one of the KIND_ values */
    KMDWORD codebook; /* the slot stride: each texture's codebook bytes */
    KMDWORD index; /* each texture's index bytes, which nothing reads */
    KMDWORD size; /* the bytes the page takes */
    KMDWORD slots; /* how many textures the page holds */
};

/* 1 has kmiFBinit make all of texture memory bank 0, and bank 1 empty */
KMINT32 KM_TEXTURE_ONE_BANK = 0;
/* bank 0's size in bytes, as kmiFBinit sets it, and 0x800000 before it runs.
 * fb_find_idle and fb_find_idle_vq skip a block this size or larger */
KMDWORD KM_TEXTURE_BANK_SIZE = 0x800000;
/* each bank's first offset in the 64-bit view, and its size in bytes, as
 * kmiFBinit sets them */
static KMINT32 fb_bank0_base = 0;
static KMINT32 fb_bank0_size = 0x400000;
static KMINT32 fb_bank1_base = 0x400000;
static KMINT32 fb_bank1_size = 0x400000;
static struct fb_bank fb_banks[2] = {{NULL, NULL, NULL, NULL, 0},
                                     {NULL, NULL, NULL, NULL, 0}};
/* the list of VQ pages, newest first */
static struct fb_vq_page *fb_vq_pages = NULL;
/* fb_next_frame and fb_next_size describe the two frame buffers kmiFBalloc
 * allocated last, until a request of their size takes the second. they hold the
 * block's offset in the 64-bit view, and each buffer's size. both are 0 when no
 * second buffer waits */
static KMDWORD fb_next_frame = 0;
static KMINT32 fb_next_size = 0;
static KMDWORD dat_8ce206bc = 0;
static KMDWORD dat_8ce206c0 = 0;
static struct fb_block fb_blocks[BLOCKS];
static struct fb_vq_page fb_vq_pool[VQ_PAGES];

KMSTATUS kmiFBreset(void);
KMSTATUS kmiFBallocEx(KMINT32 size, KMSURFACEDESC *first,
                      KMSURFACEDESC *second);
KMSTATUS kmiCBalloc(KMINT32 size, KMSURFACEDESC *first, KMSURFACEDESC *second);
KMSTATUS kmiMIPalloc(KMINT32 size, KMSURFACEDESC *surface);
KMSTATUS kmiFBgetfreememEx(KMINT32 *bank0, KMINT32 *bank1);
KMINT32 fb_find_pair_space(KMINT32 size);
struct fb_block *fb_find_idle_vq(short bank, KMINT32 size,
                                 struct fb_vq_page *page);
static KMINT32 fb_place_mipmap(struct fb_block *block, KMDWORD size,
                               KMSURFACEDESC *surface);
static KMINT32 fb_find_pair(KMINT32 size, KMSURFACEDESC *surface);
static KMSTATUS fb_check_free(KMINT32 bank, KMINT32 offset, KMINT32 size);
static KMSTATUS fb_alloc_at(short bank, KMINT32 offset, KMINT32 size,
                            KMSURFACEDESC *surface, KMBOOLEAN paired,
                            KMBOOLEAN live);
static KMSTATUS fb_alloc_frame(short bank, KMINT32 size, void *owner,
                               short kind);
static KMSTATUS fb_alloc_texture(short bank, KMINT32 size,
                                 KMSURFACEDESC *surface);
static KMSTATUS fb_alloc_vq(short bank, KMINT32 size, struct fb_vq_page *page);
static struct fb_block *fb_find_idle(short bank, KMINT32 size,
                                     KMSURFACEDESC *surface);
static struct fb_block *fb_find_used(KMINT32 offset, short *bank);
static KMSTATUS fb_release(short bank, struct fb_block *block);
static struct fb_block *fb_block_take(void);
static void fb_block_drop(struct fb_block *block);
static void fb_list_push(struct fb_block **head, struct fb_block **tail,
                         struct fb_block *block);
static void fb_list_insert(struct fb_block **tail, struct fb_block *block,
                           struct fb_block *prev);
static void fb_list_remove(struct fb_block **head, struct fb_block **tail,
                           struct fb_block *block);
static void fb_move(KMDWORD dst, KMDWORD src, KMDWORD size);
static void fb_list_sort(struct fb_block **head, struct fb_block **tail);
static KMSTATUS fb_compact(struct fb_block **free, struct fb_block **free_tail,
                           struct fb_block **used, struct fb_block **used_tail);
static KMSTATUS fb_alloc_vq_at(short bank, KMINT32 offset, KMINT32 size,
                               struct fb_vq_page *page, KMBOOLEAN paired,
                               KMBOOLEAN live);
static KMINT32 fb_vq_shape_for(struct fb_vq_shape *shape,
                               KMSURFACEDESC *surface);
static struct fb_vq_page *fb_vq_take(void);
static void fb_vq_drop(struct fb_vq_page *page);
static void fb_vq_push(struct fb_vq_page **head, struct fb_vq_page *page);
static void fb_vq_remove(struct fb_vq_page **head, struct fb_vq_page *page);
static KMDWORD fb_to_32bit(KMDWORD offset);
static KMDWORD fb_to_64bit(KMDWORD offset);

/* return every block record and VQ page record to its pool, and empty each
 * bank's lists and the list of VQ pages. the result is KMSTATUS_SUCCESS */
KMSTATUS kmiFBreset(void)
{
    struct fb_block *block;
    struct fb_block *next;
    struct fb_vq_page *page;
    struct fb_vq_page *next_page;
    int i;

    block = fb_banks[0].free;
    if (block) {
        do {
            next = block->next;
            fb_block_drop(block);
            block = next;
        } while (block);
    }
    block = fb_banks[0].used;
    if (block) {
        do {
            next = block->next;
            fb_block_drop(block);
            block = next;
        } while (block);
    }
    block = fb_banks[1].free;
    if (block) {
        do {
            next = block->next;
            fb_block_drop(block);
            block = next;
        } while (block);
    }
    block = fb_banks[1].used;
    if (block) {
        do {
            next = block->next;
            fb_block_drop(block);
            block = next;
        } while (block);
    }
    for (i = 0; i < BLOCKS; i++) {
        fb_blocks[i].flags = 0;
    }
    fb_banks[0].used = NULL;
    fb_banks[0].used_tail = NULL;
    fb_banks[0].free = NULL;
    fb_banks[0].free_tail = NULL;
    fb_banks[0].free_size = 0;
    fb_banks[1].used = NULL;
    fb_banks[1].used_tail = NULL;
    fb_banks[1].free = NULL;
    fb_banks[1].free_tail = NULL;
    fb_banks[1].free_size = 0;
    page = fb_vq_pages;
    if (page) {
        do {
            next_page = page->next;
            fb_vq_drop(page);
            page = next_page;
        } while (page);
    }
    fb_vq_pages = NULL;
    return KMSTATUS_SUCCESS;
}

KMSTATUS kmiFBinit(void)
{
    struct fb_block *block;

    if (KM_TEXTURE_ONE_BANK == 1) {
        if (kmiGetProfileInt("HARDWARE", "EnableTexture16MB", 0) == 1) {
            kmiDeviceExtension.texture_memory_size = 0x1000000;
            KM_TEXTURE_BANK_SIZE = 0x1000000;
            fb_bank0_base = 0;
            fb_bank0_size = 0x1000000;
            fb_bank1_base = 0x1000000;
            fb_bank1_size = 0;
        } else {
            kmiDeviceExtension.texture_memory_size = 0x800000;
            KM_TEXTURE_BANK_SIZE = 0x800000;
            fb_bank0_base = 0;
            fb_bank0_size = 0x800000;
            fb_bank1_base = 0x800000;
            fb_bank1_size = 0;
        }
        kmiWriteRegister(TEXTURE_CONTROL, 0);
        kmiFBreset();
        fb_banks[0].free_size = fb_bank0_size;
        fb_banks[1].free_size = fb_bank1_size;
        block = fb_block_take();
        if (!block) {
            return KMSTATUS_NOT_ENOUGH_MEMORY;
        }
        block->offset = fb_bank0_base;
        block->size = fb_bank0_size;
        block->next = fb_banks[0].free;
        fb_banks[0].free = block;
        block->prev = fb_banks[0].free_tail;
        fb_banks[0].free_tail = block;
    } else {
        if (kmiGetProfileInt("HARDWARE", "EnableTexture16MB", 0) == 1) {
            kmiDeviceExtension.texture_memory_size = 0x1000000;
            KM_TEXTURE_BANK_SIZE = 0x800000;
            fb_bank0_base = 0;
            fb_bank0_size = 0x800000;
            fb_bank1_base = 0x800000;
            fb_bank1_size = 0x800000;
            kmiWriteRegister(TEXTURE_CONTROL, 0);
        } else {
            kmiDeviceExtension.texture_memory_size = 0x800000;
            KM_TEXTURE_BANK_SIZE = 0x400000;
            fb_bank0_base = 0;
            fb_bank0_size = 0x400000;
            fb_bank1_base = 0x400000;
            fb_bank1_size = 0x400000;
            kmiWriteRegister(TEXTURE_CONTROL, 0);
        }
        kmiFBreset();
        fb_banks[0].free_size = fb_bank0_size;
        fb_banks[1].free_size = fb_bank1_size;
        block = fb_block_take();
        if (!block) {
            return KMSTATUS_NOT_ENOUGH_MEMORY;
        }
        block->offset = fb_bank0_base;
        block->size = fb_bank0_size;
        block->next = fb_banks[0].free;
        fb_banks[0].free = block;
        block->prev = fb_banks[0].free_tail;
        fb_banks[0].free_tail = block;
        block = fb_block_take();
        if (!block) {
            return KMSTATUS_NOT_ENOUGH_MEMORY;
        }
        block->offset = fb_bank1_base;
        block->size = fb_bank1_size;
        block->next = fb_banks[1].free;
        fb_banks[1].free = block;
        block->prev = fb_banks[1].free_tail;
        fb_banks[1].free_tail = block;
    }
    /* @hack redundant: a never-run store of 0 to fb_bank1_size, which keeps its
     * address in R11 and fb_bank0_base's in R10 (0x8ce181d0, 0x8ce18170), as
     * the image has them; without it, the two trade and 12 of 154 words
     * differ */
    if (0) {
        fb_bank1_size = 0;
    }
    return KMSTATUS_SUCCESS;
}

KMSTATUS kmiFBalloc(KMINT32 size, KMSURFACEDESC *surface)
{
    KMINT32 rest;
    KMSTATUS status;
    KMDWORD offset;

    rest = size % ALIGN;
    if (rest) {
        size += ALIGN - rest;
    }
    if (fb_next_size != size) {
        if (kmiDeviceExtension.capabilities & KMI_CAP_BALANCE_BANKS) {
            if (fb_banks[0].free_size < fb_banks[1].free_size) {
                status = fb_alloc_frame(1, size * 2, surface, BLOCK_FRAME);
            } else {
                status = fb_alloc_frame(0, size * 2, surface, BLOCK_FRAME);
            }
        } else {
            status = fb_alloc_frame(0, size * 2, surface, BLOCK_FRAME);
        }
        if (!status) {
            fb_next_size = size;
            fb_next_frame = (KMDWORD)surface->pSurface;
            offset = (KMDWORD)surface->pSurface;
            surface->uSize.dwFrameBufferSize = size;
            surface->pSurface = (PKMDWORD)fb_to_32bit(offset);
        }
        return status;
    } else {
        surface->uSize.dwFrameBufferSize = size;
        surface->pSurface = (PKMDWORD)fb_to_32bit(fb_next_frame + 4);
        fb_next_size = 0;
        fb_next_frame = 0;
        return KMSTATUS_SUCCESS;
    }
}

/* allocate two frame buffers of size bytes each, rounded up to a multiple of
 * 32, interleaved a long at a time in one block. first's and second's pSurface
 * get their offsets in the 32-bit view, and their dwFrameBufferSize the rounded
 * size. on a chip with KMI_CAP_BALANCE_BANKS, the block comes from the bank
 * with more free bytes, or bank 1 on a tie. other chips take it from bank 0.
 * the result is KMSTATUS_NOT_ENOUGH_MEMORY when no free block is large enough
 * or no block record is left, and else KMSTATUS_SUCCESS */
KMSTATUS kmiFBallocEx(KMINT32 size, KMSURFACEDESC *first, KMSURFACEDESC *second)
{
    KMINT32 rest;
    KMSTATUS status;
    KMDWORD offset;

    rest = size % ALIGN;
    if (rest) {
        size += ALIGN - rest;
    }
    if (kmiDeviceExtension.capabilities & KMI_CAP_BALANCE_BANKS) {
        if (fb_banks[0].free_size <= fb_banks[1].free_size) {
            status = fb_alloc_frame(1, size * 2, first, BLOCK_FRAME);
        } else {
            status = fb_alloc_frame(0, size * 2, first, BLOCK_FRAME);
        }
    } else {
        status = fb_alloc_frame(0, size * 2, first, BLOCK_FRAME);
    }
    if (!status) {
        offset = (KMDWORD)first->pSurface;
        first->uSize.dwFrameBufferSize = size;
        first->pSurface = (PKMDWORD)fb_to_32bit(offset);
        second->uSize.dwFrameBufferSize = size;
        second->pSurface = (PKMDWORD)fb_to_32bit(offset + 4);
    }
    return status;
}

KMSTATUS kmiTAallocEx(KMINT32 size, KMDWORD *first, KMDWORD *second)
{
    KMINT32 rest;
    KMSTATUS status;
    KMDWORD offset;

    rest = size % ALIGN;
    if (rest) {
        size += ALIGN - rest;
    }
    status = fb_alloc_frame(0, size * 2, first, BLOCK_TA);
    if (!status) {
        offset = *first;
        *first = fb_to_32bit(offset);
        *second = fb_to_32bit(offset + 4);
    }
    return status;
}

KMSTATUS kmiTXalloc(KMINT32 size, KMSURFACEDESC *surface)
{
    KMINT32 bank0;
    KMINT32 bank1;
    KMSTATUS status;

    if (kmiDeviceExtension.capabilities & KMI_CAP_BALANCE_BANKS) {
        kmiFBgetfreememEx(&bank0, &bank1);
        if (bank0 < bank1) {
            status = fb_alloc_texture(1, size, surface);
        } else {
            status = fb_alloc_texture(0, size, surface);
        }
    } else {
        status = fb_alloc_texture(1, size, surface);
    }
    return status;
}

/* allocate a pair for two textures of size bytes: first's half in bank 0, and
 * second's its twin in bank 1. fb_find_pair places the pair by first's flags,
 * and each surface's pSurface gets its half's offset. the result is
 * KMSTATUS_NOT_ENOUGH_MEMORY when no pair fits or no block record is left, and
 * else KMSTATUS_SUCCESS. when second's half fails, first's is freed */
KMSTATUS kmiCBalloc(KMINT32 size, KMSURFACEDESC *first, KMSURFACEDESC *second)
{
    KMINT32 offset;
    KMSTATUS status;

    offset = fb_find_pair(size, first);
    if (offset == -1) {
        return KMSTATUS_NOT_ENOUGH_MEMORY;
    }
    status = fb_alloc_at(0, offset, size, first, TRUE, TRUE);
    if (!status) {
        status =
            fb_alloc_at(1, offset + fb_bank0_size, size, second, TRUE, TRUE);
        if (status) {
            kmiFBfree((KMINT32)first->pSurface);
        }
    }
    return status;
}

KMSTATUS kmiABSalloc(KMINT32 size, KMSURFACEDESC *surface)
{
    KMINT32 offset = (KMINT32)surface->pSurface;
    KMSTATUS status;

    if (offset < fb_bank1_base) {
        status = fb_alloc_at(0, offset, size, surface, FALSE, TRUE);
    } else {
        status = fb_alloc_at(1, offset, size, surface, FALSE, TRUE);
    }
    return status;
}

/* allocate size bytes, rounded up to a multiple of 32, as half of a pair for
 * surface, a mipmapped texture. surface takes the smallest idle half of at
 * least that size whose twin's surface has its flags, from bank 0 before
 * bank 1. with none, surface takes bank 0's half of a new pair placed by
 * fb_find_pair, and bank 1's is left idle. pSurface gets the half's offset. the
 * result is KMSTATUS_NOT_ENOUGH_MEMORY when no pair fits or no block record is
 * left, and else KMSTATUS_SUCCESS. when bank 1's half fails, bank 0's stays
 * surface's */
KMSTATUS kmiMIPalloc(KMINT32 size, KMSURFACEDESC *surface)
{
    KMINT32 rest;
    KMINT32 offset;
    KMSTATUS status;
    struct fb_block *block;
    /* the owner of a new pair's idle half. the block keeps twin's address after
     * the return, when twin is gone, and nothing reads it there: fb_compact
     * moves no paired block. fb_find_idle reads an owner only through an idle
     * half's twin, which is always live */
    KMSURFACEDESC twin;

    rest = size % ALIGN;
    if (rest) {
        size += ALIGN - rest;
    }
    block = fb_find_idle(0, size, surface);
    if (!block) {
        block = fb_find_idle(1, size, surface);
    }
    if (block) {
        surface->pSurface = (PKMDWORD)block->offset;
        block->owner = (KMDWORD)surface;
        block->flags |= BLOCK_LIVE;
        return KMSTATUS_SUCCESS;
    }
    offset = fb_find_pair(size, surface);
    if (offset == -1) {
        return KMSTATUS_NOT_ENOUGH_MEMORY;
    }
    status = fb_alloc_at(0, offset, size, surface, TRUE, TRUE);
    if (!status) {
        status =
            fb_alloc_at(1, offset + fb_bank0_size, size, &twin, TRUE, FALSE);
    }
    return status;
}

KMSTATUS kmiVQalloc(KMSURFACEDESC *surface)
{
    struct fb_vq_shape shape;
    struct fb_vq_page *page;
    struct fb_vq_page *found;
    KMDWORD i;
    KMDWORD slot;
    KMINT32 bank0;
    KMINT32 bank1;
    KMSTATUS status;
    KMINT32 passes;
    KMINT32 size;

    /* @bug uninitialized: for a width other than 8, 16, 32 and 64,
     * fb_vq_shape_for returns 0 without storing a shape (BRA at 0x8ce19c8c),
     * and the reads below take shape's unset frame words: kind at 0x8ce18630
     * and 0x8ce1868c, slots at 0x8ce1863e, size at 0x8ce18694 and codebook at
     * 0x8ce186d2 */
#ifdef GCC
    /* repairs the reads the tag names: a width with no shape takes no slot */
    if (!fb_vq_shape_for(&shape, surface)) {
        return KMSTATUS_INVALID_TEXTURE_TYPE;
    }
#else
    fb_vq_shape_for(&shape, surface);
#endif
    found = NULL;
    page = fb_vq_pages;
    if (page) {
        do {
            if ((page->flags & PAGE_KIND) == shape.kind) {
                for (i = 0; i < shape.slots; i++) {
                    if (!page->slot[i]) {
                        found = page;
                        slot = i;
                        i = shape.slots;
                        page = NULL;
                    }
                }
            }
            if (!found) {
                page = page->next;
            }
        } while (page);
    }
    if (!found) {
        found = fb_vq_take();
        if (!found) {
            return KMSTATUS_NOT_ENOUGH_MEMORY;
        }
        /* @bug uninitialized: a new page record keeps the surfaces its
         * slots held on its last page. a loop of SLOTS passes that stores
         * nothing (0x8ce1867a to 0x8ce18680) stands where 1.30J clears the
         * slots (MOV.L R14,@R4 at 0x8c6724fe in 1ST_READ.BIN). a record that
         * kmiFBreset dropped thus comes back with its old page's surfaces in
         * its slots, which kmiVQalloc's search for a free slot and
         * kmiVQFBfree's test for a used page read (MOV.L @R2,R1 at 0x8ce1864a,
         * MOV.L @R3,R1 at 0x8ce18870) */
        passes = SLOTS;
        while (passes) {
            passes--;
        }
        slot = 0;
        fb_vq_push(&fb_vq_pages, found);
        found->flags |= shape.kind;
        size = shape.size;
        if (kmiDeviceExtension.capabilities & KMI_CAP_BALANCE_BANKS) {
            kmiFBgetfreememEx(&bank0, &bank1);
            if (bank0 < bank1) {
                status = fb_alloc_vq(1, size, found);
            } else {
                status = fb_alloc_vq(0, size, found);
            }
        } else {
            status = fb_alloc_vq(1, size, found);
        }
        if (status) {
            /* @bug memory: the record goes back to the pool but stays on
             * fb_vq_pages, with its kind and no block (JSR at 0x8ce186ca). a
             * later kmiVQalloc can give a texture a slot in it, at the offset
             * the record held before. fb_vq_take can also hand the record out
             * again, and pushing it a second time joins fb_vq_pages into a
             * loop */
            fb_vq_drop(found);
            return KMSTATUS_NOT_ENOUGH_MEMORY;
        }
    }
    surface->pSurface =
        (PKMDWORD)(found->offset + (slot * (shape.codebook >> 2) * 4));
    found->slot[slot] = surface;
    found->flags |= PAGE_USED;
    return KMSTATUS_SUCCESS;
}

KMSTATUS kmiFBfree(KMINT32 offset)
{
    short bank;
    short twin_bank;
    struct fb_block *block;
    struct fb_block *twin;
    KMSTATUS status;
    KMINT32 twin_offset;

    block = fb_find_used(offset, &bank);
    if (!block) {
        return KMSTATUS_INVALID_ADDRESS;
    }
    if (block->flags & BLOCK_PAIRED) {
        if (bank == 0) {
            twin_offset = fb_bank0_size + offset;
        } else {
            twin_offset = offset - fb_bank0_size;
        }
        twin = fb_find_used(twin_offset, &twin_bank);
        if (!twin) {
            fb_release(bank, block);
            return KMSTATUS_SUCCESS;
        }
        if (twin->flags & BLOCK_LIVE) {
            block->flags &= ~BLOCK_LIVE;
            status = KMSTATUS_SUCCESS;
        } else {
            fb_release(bank, block);
            status = fb_release(twin_bank, twin);
        }
    } else {
        status = fb_release(bank, block);
    }
    return status;
}

KMSTATUS kmiFBfreeEx(KMDWORD first, KMDWORD second)
{
    KMSTATUS status;
    /* @hack type: volatile on offset, which keeps the store of each
     * 64-bit view offset to offset's stack slot (MOV.L R0,@R15 at 0x8ce187be,
     * 0x8ce187d0); without it, the stores go, and 20 of 24 words differ */
    volatile KMDWORD offset = first;
    /* @hack spelling: second through other, which has SHC spill first, then
     * second (0x8ce187b6, 0x8ce187b8); through second itself, the two spills
     * swap, and 2 of 24 words differ */
    KMDWORD other = second;

    status = kmiFBfree(offset = fb_to_64bit(offset));
    if (status) {
        status = kmiFBfree(offset = fb_to_64bit(other));
    }
    return status;
}

KMSTATUS kmiVQFBfree(KMSURFACEDESC *surface)
{
    struct fb_vq_shape shape;
    struct fb_vq_page *page;
    struct fb_vq_page *found;
    KMDWORD i;
    KMDWORD slot;
    KMBOOLEAN used;
    KMSTATUS status;
    KMDWORD addr;

    /* @bug uninitialized: for a width other than 8, 16, 32 and 64,
     * fb_vq_shape_for returns 0 without storing a shape (BRA at 0x8ce19c8c),
     * and the reads below take shape's unset frame words: slots at 0x8ce18800,
     * kind at 0x8ce18808 and codebook at 0x8ce18824 */
#ifdef GCC
    /* repairs the reads the tag names: a width with no shape holds no slot */
    if (!fb_vq_shape_for(&shape, surface)) {
        return KMSTATUS_INVALID_ADDRESS;
    }
#else
    fb_vq_shape_for(&shape, surface);
#endif
    found = NULL;
    page = fb_vq_pages;
    if (page) {
        do {
            if ((page->flags & PAGE_KIND) == shape.kind) {
                for (i = 0; i < shape.slots; i++) {
                    if (page->slot[i]) {
                        addr = page->offset + (i * (shape.codebook >> 2) * 4);
                        if (addr == (KMDWORD)surface->pSurface) {
                            found = page;
                            slot = i;
                            i = shape.slots;
                            page = NULL;
                        }
                    }
                }
            }
            if (!found) {
                page = page->next;
            }
        } while (page);
    }
    if (found) {
        found->slot[slot] = NULL;
        used = FALSE;
        for (i = 0; i < shape.slots; i++) {
            if (found->slot[i]) {
                used = TRUE;
            }
        }
        if (!used) {
            status = kmiFBfree(found->offset);
            if (status) {
                return status;
            }
            fb_vq_remove(&fb_vq_pages, found);
            fb_vq_drop(found);
        }
        return KMSTATUS_SUCCESS;
    }
    return KMSTATUS_INVALID_ADDRESS;
}

KMSTATUS kmiFBgetfreemem(KMINT32 *bank0, KMINT32 *bank1, KMINT32 *vq,
                         KMINT32 *largest)
{
    KMINT32 free0;
    KMINT32 free1;
    KMINT32 idle0;
    KMINT32 idle1;
    KMINT32 largest0;
    KMINT32 largest1;
    KMINT32 vq_free;
    KMINT32 bytes;
    KMINT32 slots;
    int i;
    struct fb_block *block;
    struct fb_vq_page *page;

    free0 = free1 = idle0 = idle1 = 0;
    largest0 = largest1 = 0;
    vq_free = 0;
    block = fb_banks[0].free;
    if (block) {
        do {
            if (block->size > largest0) {
                largest0 = block->size;
            }
            free0 += block->size;
            block = block->next;
        } while (block);
    }
    block = fb_banks[0].used;
    if (block) {
        do {
            if (!(block->flags & BLOCK_LIVE)) {
                if (block->size > largest0) {
                    largest0 = block->size;
                }
                idle0 += block->size;
            }
            block = block->next;
        } while (block);
    }
    block = fb_banks[1].free;
    if (block) {
        do {
            if (block->size > largest1) {
                largest1 = block->size;
            }
            free1 += block->size;
            block = block->next;
        } while (block);
    }
    block = fb_banks[1].used;
    if (block) {
        do {
            if (!(block->flags & BLOCK_LIVE)) {
                if (block->size > largest1) {
                    largest1 = block->size;
                }
                idle1 += block->size;
            }
            block = block->next;
        } while (block);
    }
    page = fb_vq_pages;
    if (page) {
        do {
            /* each free slot adds the bytes of a texture of the page's kind */
            switch (page->flags & PAGE_KIND) {
            case KIND_8:
                bytes = 0x90;
                slots = 16;
                break;
            case KIND_8_MIPMAPS:
                bytes = 0xa0;
                slots = 16;
                break;
            case KIND_16:
                bytes = 0xc0;
                slots = 16;
                break;
            case KIND_16_MIPMAPS:
                bytes = 0xe0;
                slots = 16;
                break;
            case KIND_32:
                bytes = 0x200;
                slots = 8;
                break;
            case KIND_32_MIPMAPS:
                bytes = 0x360;
                slots = 4;
                break;
            case KIND_64:
                bytes = 0x800;
                slots = 2;
                break;
            case KIND_64_MIPMAPS:
                bytes = 0xd60;
                slots = 1;
                break;
            default:
                bytes = 0;
                slots = 0;
                break;
            }
            for (i = 0; i < slots; i++) {
                if (!page->slot[i]) {
                    vq_free += bytes;
                }
            }
            page = page->next;
        } while (page);
    }
    *bank0 = free0 + idle0;
    *bank1 = free1 + idle1;
    *vq = vq_free;
    *largest = (largest0 < largest1) ? largest1 : largest0;
    return KMSTATUS_SUCCESS;
}

/* store in *bank0 and *bank1 the bytes of the largest free or idle block in
 * each bank, or 0 for a bank with none. the result is KMSTATUS_SUCCESS */
KMSTATUS kmiFBgetfreememEx(KMINT32 *bank0, KMINT32 *bank1)
{
    KMINT32 largest0;
    KMINT32 largest1;
    struct fb_block *block;

    largest1 = 0;
    largest0 = 0;
    block = fb_banks[0].free;
    if (block) {
        do {
            if (block->size > largest0) {
                largest0 = block->size;
            }
            block = block->next;
        } while (block);
    }
    block = fb_banks[0].used;
    if (block) {
        do {
            if (!(block->flags & BLOCK_LIVE) && block->size > largest0) {
                largest0 = block->size;
            }
            block = block->next;
        } while (block);
    }
    block = fb_banks[1].free;
    if (block) {
        do {
            if (block->size > largest1) {
                largest1 = block->size;
            }
            block = block->next;
        } while (block);
    }
    block = fb_banks[1].used;
    if (block) {
        do {
            if (!(block->flags & BLOCK_LIVE) && block->size > largest1) {
                largest1 = block->size;
            }
            block = block->next;
        } while (block);
    }
    *bank0 = largest0;
    *bank1 = largest1;
    return KMSTATUS_SUCCESS;
}

KMSTATUS kmiFBSortQueue(void)
{
    fb_list_sort(&fb_banks[0].used, &fb_banks[0].used_tail);
    fb_list_sort(&fb_banks[1].used, &fb_banks[1].used_tail);
    fb_list_sort(&fb_banks[0].free, &fb_banks[0].free_tail);
    fb_list_sort(&fb_banks[1].free, &fb_banks[1].free_tail);
    return KMSTATUS_SUCCESS;
}

KMSTATUS kmiFBGarbageCollect(void)
{
    KMSTATUS status;

    status = fb_compact(&fb_banks[0].free, &fb_banks[0].free_tail,
                        &fb_banks[0].used, &fb_banks[0].used_tail);
    if (!status) {
        status = fb_compact(&fb_banks[1].free, &fb_banks[1].free_tail,
                            &fb_banks[1].used, &fb_banks[1].used_tail);
    }
    return status;
}

/* the offset in block for a mipmap of size bytes, or -1 when none fits. a
 * mipmap of at most LEVEL_32X32 bytes has no 32-by-32 level, and takes the
 * first offset where it crosses no page boundary. a larger one takes the first
 * offset that puts its 32-by-32 level on a page boundary. the test that it fits
 * there is the @bug below, which can pass a mipmap that runs past the block's
 * end */
static KMINT32 fb_place_mipmap(struct fb_block *block, KMDWORD size,
                               KMSURFACEDESC *surface)
{
    KMDWORD page;
    KMDWORD end;
    KMINT32 offset;

    page = (block->offset + PAGE) & ~(PAGE - 1);
    end = block->offset + block->size;
    if (size <= LEVEL_32X32) {
        if (page >= end || page - block->offset >= size) {
            offset = block->offset;
        } else if (end - page >= size) {
            offset = page;
        } else {
            offset = -1;
        }
    } else {
        /* @bug logic: each fit test compares the length from the block's
         * start to the mipmap's end with end, the block's end offset, rather
         * than with the block's size, so in a block above offset 0 a mipmap
         * that runs past the block's end passes (CMP/HI R7,R0 at 0x8ce18bce and
         * 0x8ce18be6). fb_find_pair tries no offset past the last that fits, so
         * no mipmap lands there */
        if (page - block->offset >= LEVEL_32X32) {
            if (page - block->offset + size - LEVEL_32X32 > end) {
                offset = -1;
            } else {
                offset = page - LEVEL_32X32;
            }
        } else {
            page += PAGE;
            if (page - block->offset + size - LEVEL_32X32 <= end) {
                offset = page - LEVEL_32X32;
            } else {
                offset = -1;
            }
        }
    }
    return offset;
}

/* how fb_find_pair has searched: not yet, in 32-byte steps, or for a mipmap,
 * by fb_place_mipmap's offset and the pages past it first */
enum { SEARCH_NONE, SEARCH_STEPS, SEARCH_MIPMAP };

/* an offset in bank 0 where size bytes are free, and free at the twin offset in
 * bank 1, or -1 when there is none. the search takes bank 0's free blocks in
 * list order, and each from its start in 32-byte steps. for a mipmap that is
 * neither VQ nor palettized, the search first tries fb_place_mipmap's offset in
 * each block, and offsets whole pages past it. only when none of those fits
 * does it take the first offset the 32-byte steps find */
static KMINT32 fb_find_pair(KMINT32 size, KMSURFACEDESC *surface)
{
    struct fb_block *block;
    KMINT32 rest;
    KMINT32 fallback;
    KMINT32 mode;
    KMINT32 last;
    KMINT32 offset;
    KMINT32 placed;
    KMSTATUS status;

    fallback = 0;
    mode = SEARCH_NONE;
    rest = size % ALIGN;
    if (rest) {
        size += ALIGN - rest;
    }
    block = fb_banks[0].free;
    if (block) {
        do {
            if (block->size >= size) {
                last = block->offset + block->size - size;
                if ((surface->fSurfaceFlags & KM_SURFACEFLAGS_MIPMAPED) &&
                    !(surface->fSurfaceFlags & KM_SURFACEFLAGS_VQ) &&
                    !(surface->fSurfaceFlags & KM_SURFACEFLAGS_PALETTIZED)) {
                    placed = fb_place_mipmap(block, size, surface);
                    if (placed >= 0) {
                        for (offset = placed; offset <= last; offset += PAGE) {
                            status =
                                fb_check_free(1, offset + fb_bank0_size, size);
                            if (!status) {
                                return offset;
                            }
                        }
                    }
                    mode = SEARCH_MIPMAP;
                    /* @bug logic: fallback holds 0 for none, so one found
                     * at offset 0 counts as none (TST R9,R9 at 0x8ce18c8e and
                     * 0x8ce18cf6). later blocks are searched for another, and
                     * when nothing fits there, the result is -1 though
                     * offset 0 fits */
                    if (!fallback) {
                        for (offset = block->offset; offset <= last;
                             offset += ALIGN) {
                            status =
                                fb_check_free(1, offset + fb_bank0_size, size);
                            if (!status) {
                                fallback = offset;
                                offset = last;
                            }
                        }
                    }
                } else {
                    mode = SEARCH_STEPS;
                    for (offset = block->offset; offset <= last;
                         offset += ALIGN) {
                        status = fb_check_free(1, offset + fb_bank0_size, size);
                        if (!status) {
                            return offset;
                        }
                    }
                }
            }
            block = block->next;
        } while (block);
    }
    if (mode == SEARCH_MIPMAP && fallback) {
        return fallback;
    }
    return -1;
}

/* KMSTATUS_SUCCESS when one free block in bank holds the size bytes from
 * offset, and else KMSTATUS_NOT_ENOUGH_MEMORY */
static KMSTATUS fb_check_free(KMINT32 bank, KMINT32 offset, KMINT32 size)
{
    KMINT32 end = offset + size;
    struct fb_block *block;
    struct fb_block *found;

    /* @hack redundant: an empty test of size % ALIGN, which calls __modls to no
     * effect (JSR at 0x8ce18d1e); without it, the call goes, the function is
     * 16 bytes shorter, and 45 of 46 words differ */
    if (size % ALIGN) {
    }
    found = NULL;
    block = fb_banks[bank].free;
    if (block) {
        do {
            if (block->offset <= offset &&
                block->offset + block->size > offset && block->offset <= end &&
                block->offset + block->size >= end) {
                found = block;
                block = NULL;
            } else {
                block = block->next;
            }
        } while (block);
    }
    if (!found) {
        return KMSTATUS_NOT_ENOUGH_MEMORY;
    }
    return KMSTATUS_SUCCESS;
}

/* allocate size bytes at offset in bank for surface, cut from the free block
 * that holds them. the cut takes size rounded up to a multiple of 32, though
 * the free block is found by size as given. surface's pSurface gets the block's
 * offset. the block takes BLOCK_PLACED0 or BLOCK_PLACED1 by bank, and
 * BLOCK_PAIRED and BLOCK_LIVE when paired and live are TRUE. the result is
 * KMSTATUS_NOT_ENOUGH_MEMORY when no free block holds the bytes or no block
 * record is left, and else KMSTATUS_SUCCESS */
static KMSTATUS fb_alloc_at(short bank, KMINT32 offset, KMINT32 size,
                            KMSURFACEDESC *surface, KMBOOLEAN paired,
                            KMBOOLEAN live)
{
    KMINT32 end = offset + size;
    KMINT32 rest;
    KMINT32 whole;
    KMINT32 before;
    /* @hack spelling: each cut's remainder, the free block's size less size,
     * computed into left before it is stored, which holds fb_banks' address in
     * R8 and the bank's offset in R9 (0x8ce18da8, 0x8ce18dac); stored
     * straight, the two trade, and 32 of 303 words differ */
    KMINT32 left;
    struct fb_block *block;
    struct fb_block *found;
    struct fb_block *taken;
    struct fb_block *after;

    rest = size % ALIGN;
    if (rest) {
        size += ALIGN - rest;
    }
    found = NULL;
    block = fb_banks[bank].free;
    if (block) {
        do {
            if (block->offset <= offset &&
                block->offset + block->size > offset && block->offset <= end &&
                block->offset + block->size >= end) {
                found = block;
                block = NULL;
            } else {
                block = block->next;
            }
        } while (block);
    }
    if (!found) {
        return KMSTATUS_NOT_ENOUGH_MEMORY;
    }
    if (found->size == size) {
        fb_list_remove(&fb_banks[bank].free, &fb_banks[bank].free_tail, found);
        fb_list_push(&fb_banks[bank].used, &fb_banks[bank].used_tail, found);
        surface->pSurface = (PKMDWORD)found->offset;
        found->owner = (KMDWORD)surface;
        found->flags = BLOCK_TAKEN;
        if (bank == 0) {
            found->flags |= BLOCK_PLACED0;
        } else {
            found->flags |= BLOCK_PLACED1;
        }
        if (paired) {
            found->flags |= BLOCK_PAIRED;
        }
        if (live) {
            found->flags |= BLOCK_LIVE;
        }
        fb_banks[bank].free_size -= found->size;
    } else if (found->offset == offset) {
        taken = fb_block_take();
        if (!taken) {
            return KMSTATUS_NOT_ENOUGH_MEMORY;
        }
        taken->offset = found->offset;
        taken->size = size;
        left = found->size - size;
        found->size = left;
        found->offset += size;
        fb_list_push(&fb_banks[bank].used, &fb_banks[bank].used_tail, taken);
        surface->pSurface = (PKMDWORD)taken->offset;
        taken->owner = (KMDWORD)surface;
        if (bank == 0) {
            taken->flags |= BLOCK_PLACED0;
        } else {
            taken->flags |= BLOCK_PLACED1;
        }
        if (paired) {
            taken->flags |= BLOCK_PAIRED;
        }
        if (live) {
            taken->flags |= BLOCK_LIVE;
        }
        fb_banks[bank].free_size -= taken->size;
    } else if (found->offset + found->size == end) {
        taken = fb_block_take();
        if (!taken) {
            return KMSTATUS_NOT_ENOUGH_MEMORY;
        }
        left = found->size - size;
        found->size = left;
        taken->offset = offset;
        taken->size = size;
        fb_list_push(&fb_banks[bank].used, &fb_banks[bank].used_tail, taken);
        surface->pSurface = (PKMDWORD)taken->offset;
        taken->owner = (KMDWORD)surface;
        if (bank == 0) {
            taken->flags |= BLOCK_PLACED0;
        } else {
            taken->flags |= BLOCK_PLACED1;
        }
        if (paired) {
            taken->flags |= BLOCK_PAIRED;
        }
        if (live) {
            taken->flags |= BLOCK_LIVE;
        }
        fb_banks[bank].free_size -= taken->size;
    } else {
        taken = fb_block_take();
        if (!taken) {
            return KMSTATUS_NOT_ENOUGH_MEMORY;
        }
        after = fb_block_take();
        if (!after) {
            fb_block_drop(taken);
            return KMSTATUS_NOT_ENOUGH_MEMORY;
        }
        whole = found->size;
        before = offset - found->offset;
        found->size = before;
        taken->offset = offset;
        taken->size = size;
        fb_list_push(&fb_banks[bank].used, &fb_banks[bank].used_tail, taken);
        after->offset = offset + size;
        after->size = whole - size - before;
        fb_list_insert(&fb_banks[bank].free_tail, after,
                       fb_banks[bank].free_tail);
        surface->pSurface = (PKMDWORD)taken->offset;
        taken->owner = (KMDWORD)surface;
        if (bank == 0) {
            taken->flags |= BLOCK_PLACED0;
        } else {
            taken->flags |= BLOCK_PLACED1;
        }
        if (paired) {
            taken->flags |= BLOCK_PAIRED;
        }
        if (live) {
            taken->flags |= BLOCK_LIVE;
        }
        fb_banks[bank].free_size -= taken->size;
    }
    return KMSTATUS_SUCCESS;
}

/* allocate size bytes, rounded up to a multiple of 32, from the start of the
 * first free block in bank that holds them. kind must be BLOCK_FRAME or
 * BLOCK_TA. for BLOCK_FRAME, owner points to a surface whose pSurface gets the
 * offset. for BLOCK_TA, it points to a variable that gets it. the block takes
 * kind and BLOCK_LIVE. the result is KMSTATUS_NOT_ENOUGH_MEMORY when no free
 * block holds the bytes or no block record is left, and else
 * KMSTATUS_SUCCESS */
static KMSTATUS fb_alloc_frame(short bank, KMINT32 size, void *owner,
                               short kind)
{
    KMINT32 rest;
    /* @hack spelling: the cut's remainder, the free block's size less size,
     * computed into left before it is stored, which SHC does in R4
     * (0x8ce190c2 to 0x8ce190c6); stored straight, in R3, and 5 of 166 words
     * differ */
    KMINT32 left;
    struct fb_block *block;
    struct fb_block *found;
    struct fb_block *taken;
    KMSURFACEDESC *surface;
    KMDWORD *addr;

    rest = size % ALIGN;
    if (rest) {
        size += ALIGN - rest;
    }
    found = NULL;
    block = fb_banks[bank].free;
    if (block) {
        do {
            if (block->size >= size) {
                found = block;
                block = NULL;
            } else {
                block = block->next;
            }
        } while (block);
    }
    if (!found) {
        return KMSTATUS_NOT_ENOUGH_MEMORY;
    }
    if (found->size == size) {
        fb_list_remove(&fb_banks[bank].free, &fb_banks[bank].free_tail, found);
        fb_list_push(&fb_banks[bank].used, &fb_banks[bank].used_tail, found);
        found->flags = BLOCK_TAKEN;
        switch (kind) {
        case BLOCK_FRAME:
            surface = owner;
            surface->pSurface = (PKMDWORD)found->offset;
            found->owner = (KMDWORD)surface;
            found->flags |= BLOCK_FRAME;
            break;
        case BLOCK_TA:
            addr = owner;
            *addr = found->offset;
            found->owner = (KMDWORD)addr;
            found->flags |= BLOCK_TA;
            break;
        default:
            return KMSTATUS_NOT_ENOUGH_MEMORY;
        }
        found->flags |= BLOCK_LIVE;
        fb_banks[bank].free_size -= found->size;
    } else {
        taken = fb_block_take();
        if (!taken) {
            return KMSTATUS_NOT_ENOUGH_MEMORY;
        }
        taken->offset = found->offset;
        taken->size = size;
        left = found->size - size;
        found->size = left;
        /* @hack spelling: the new offset summed into size, then stored, which
         * sums into R12 (ADD R3,R12 at 0x8ce190ca); as found->offset += size,
         * R3, and 2 of 166 words differ */
        size += found->offset;
        found->offset = size;
        fb_list_push(&fb_banks[bank].used, &fb_banks[bank].used_tail, taken);
        switch (kind) {
        case BLOCK_FRAME:
            surface = owner;
            surface->pSurface = (PKMDWORD)taken->offset;
            taken->owner = (KMDWORD)surface;
            taken->flags |= BLOCK_FRAME;
            break;
        case BLOCK_TA:
            addr = owner;
            *addr = taken->offset;
            taken->owner = (KMDWORD)addr;
            taken->flags |= BLOCK_TA;
            break;
        default:
            return KMSTATUS_NOT_ENOUGH_MEMORY;
        }
        taken->flags |= BLOCK_LIVE;
        fb_banks[bank].free_size -= taken->size;
    }
    return KMSTATUS_SUCCESS;
}

/* allocate size bytes for surface as fb_alloc_frame does with BLOCK_FRAME, but
 * mark the block BLOCK_TEXTURE0 or BLOCK_TEXTURE1 by bank */
static KMSTATUS fb_alloc_texture(short bank, KMINT32 size,
                                 KMSURFACEDESC *surface)
{
    KMINT32 rest;
    /* @hack spelling: the cut's remainder, the free block's size less size,
     * computed into left before it is stored, which holds fb_banks' address in
     * R10 and the bank's offset in R11 (0x8ce19164, 0x8ce19166); stored
     * straight, the two trade, and 22 of 136 words differ */
    KMINT32 left;
    struct fb_block *block;
    struct fb_block *found;
    struct fb_block *taken;

    rest = size % ALIGN;
    if (rest) {
        size += ALIGN - rest;
    }
    found = NULL;
    block = fb_banks[bank].free;
    if (block) {
        do {
            if (block->size >= size) {
                found = block;
                block = NULL;
            } else {
                block = block->next;
            }
        } while (block);
    }
    if (!found) {
        return KMSTATUS_NOT_ENOUGH_MEMORY;
    }
    if (found->size == size) {
        fb_list_remove(&fb_banks[bank].free, &fb_banks[bank].free_tail, found);
        fb_list_push(&fb_banks[bank].used, &fb_banks[bank].used_tail, found);
        surface->pSurface = (PKMDWORD)found->offset;
        found->owner = (KMDWORD)surface;
        found->flags = BLOCK_TAKEN;
        if (bank == 0) {
            found->flags |= BLOCK_TEXTURE0;
        } else {
            found->flags |= BLOCK_TEXTURE1;
        }
        found->flags |= BLOCK_LIVE;
        fb_banks[bank].free_size -= found->size;
    } else {
        taken = fb_block_take();
        if (!taken) {
            return KMSTATUS_NOT_ENOUGH_MEMORY;
        }
        taken->offset = found->offset;
        taken->size = size;
        left = found->size - size;
        found->size = left;
        found->offset += size;
        fb_list_push(&fb_banks[bank].used, &fb_banks[bank].used_tail, taken);
        surface->pSurface = (PKMDWORD)taken->offset;
        taken->owner = (KMDWORD)surface;
        if (bank == 0) {
            taken->flags |= BLOCK_TEXTURE0;
        } else {
            taken->flags |= BLOCK_TEXTURE1;
        }
        taken->flags |= BLOCK_LIVE;
        fb_banks[bank].free_size -= taken->size;
    }
    return KMSTATUS_SUCCESS;
}

/* where fb_alloc_vq puts a VQ page in the free block it takes: nowhere yet, at
 * the block's start, or at its first page boundary */
enum { VQ_NOWHERE, VQ_AT_START, VQ_AT_PAGE };

/* allocate size bytes, rounded up to a multiple of 32, in bank for page, and
 * store their offset in its offset field. they go in the first free block with
 * room to start a page or cross no page boundary. that is at the block's start
 * if that will do, and else at its first page boundary, through fb_alloc_vq_at.
 * with no such block, they start the first free block that holds them. the
 * result is as fb_alloc_frame's */
static KMSTATUS fb_alloc_vq(short bank, KMINT32 size, struct fb_vq_page *page)
{
    KMINT32 rest;
    KMINT32 mode;
    KMINT32 start;
    KMINT32 end;
    /* @hack spelling: the cut's remainder, the free block's size less size,
     * computed into left before it is stored, which SHC does in R4
     * (0x8ce1939c, 0x8ce193a0); stored straight, in R2, and 12 of 213 words
     * differ */
    KMINT32 left;
    struct fb_block *block;
    struct fb_block *found;
    struct fb_block *fallback;
    struct fb_block *taken;

    mode = VQ_NOWHERE;
    rest = size % ALIGN;
    if (rest) {
        size += ALIGN - rest;
    }
    fallback = NULL;
    found = NULL;
    block = fb_banks[bank].free;
    if (block) {
        do {
            if (block->size >= size) {
                if (!fallback) {
                    fallback = block;
                }
                if (block->offset % PAGE) {
                    start = (block->offset + PAGE) & ~(PAGE - 1);
                    end = block->offset + block->size;
                    if (start >= end) {
                        mode = VQ_AT_START;
                    } else if (start - block->offset >= size) {
                        mode = VQ_AT_START;
                    } else if (end - start >= size) {
                        mode = VQ_AT_PAGE;
                    } else {
                        block = block->next;
                        continue;
                    }
                } else {
                    start = block->offset;
                    mode = VQ_AT_START;
                }
                found = block;
                block = NULL;
            } else {
                block = block->next;
            }
        } while (block);
    }
    if (mode == VQ_NOWHERE && fallback) {
        mode = VQ_AT_START;
        found = fallback;
    }
    if (!found) {
        return KMSTATUS_NOT_ENOUGH_MEMORY;
    } else if (mode == VQ_AT_PAGE) {
        return fb_alloc_vq_at(bank, start, size, page, FALSE, TRUE);
    }
    if (found->size == size) {
        fb_list_remove(&fb_banks[bank].free, &fb_banks[bank].free_tail, found);
        fb_list_push(&fb_banks[bank].used, &fb_banks[bank].used_tail, found);
        page->offset = found->offset;
        found->owner = (KMDWORD)page;
        found->flags = BLOCK_TAKEN;
        if (bank == 0) {
            found->flags |= BLOCK_VQ0;
        } else {
            found->flags |= BLOCK_VQ1;
        }
        found->flags |= BLOCK_LIVE;
        fb_banks[bank].free_size -= found->size;
    } else {
        taken = fb_block_take();
        if (!taken) {
            return KMSTATUS_NOT_ENOUGH_MEMORY;
        }
        taken->offset = found->offset;
        taken->size = size;
        left = found->size - size;
        found->size = left;
        found->offset += size;
        fb_list_push(&fb_banks[bank].used, &fb_banks[bank].used_tail, taken);
        page->offset = taken->offset;
        taken->owner = (KMDWORD)page;
        if (bank == 0) {
            taken->flags |= BLOCK_VQ0;
        } else {
            taken->flags |= BLOCK_VQ1;
        }
        taken->flags |= BLOCK_LIVE;
        fb_banks[bank].free_size -= taken->size;
    }
    return KMSTATUS_SUCCESS;
}

/* the smallest idle half of a texture pair in bank that holds size bytes and
 * whose twin's surface has surface's flags. the result is NULL when there is
 * none. of two the same size, the one nearer the used list's head wins */
static struct fb_block *fb_find_idle(short bank, KMINT32 size,
                                     KMSURFACEDESC *surface)
{
    struct fb_block *block;
    struct fb_block *best;
    struct fb_block *twin;
    short twin_bank;
    KMINT32 smallest = KM_TEXTURE_BANK_SIZE;
    KMINT32 twin_offset;

    block = fb_banks[bank].used;
    best = NULL;
    if (block) {
        do {
            if (!(block->flags & BLOCK_LIVE) && (block->flags & BLOCK_PAIRED) &&
                (block->flags & (BLOCK_PLACED0 | BLOCK_PLACED1)) &&
                block->size >= size && block->size < smallest) {
                if (bank == 0) {
                    twin_offset = block->offset + fb_bank0_size;
                } else {
                    twin_offset = block->offset - fb_bank0_size;
                }
                twin = fb_find_used(twin_offset, &twin_bank);
                if (twin && ((KMSURFACEDESC *)twin->owner)->fSurfaceFlags ==
                                surface->fSurfaceFlags) {
                    best = block;
                    smallest = block->size;
                }
            }
            block = block->next;
        } while (block);
    }
    return best;
}

/* the allocated block that starts at offset, or NULL. *bank gets the bank
 * offset lies in, either way */
static struct fb_block *fb_find_used(KMINT32 offset, short *bank)
{
    struct fb_block *block;
    struct fb_block *found;

    if (offset < fb_bank1_base) {
        *bank = 0;
    } else {
        *bank = 1;
    }
    found = NULL;
    block = fb_banks[*bank].used;
    if (block) {
        do {
            if (block->offset == offset) {
                found = block;
                block = NULL;
            } else {
                block = block->next;
            }
        } while (block);
    }
    return found;
}

/* move block from bank's used list to the head of its free list, merged with
 * each free block it touches. each merge returns a record to the pool. the
 * result is KMSTATUS_SUCCESS */
static KMSTATUS fb_release(short bank, struct fb_block *block)
{
    struct fb_block *cursor;
    struct fb_block *touching;

    block->flags &= ~BLOCK_LIVE;
    fb_list_remove(&fb_banks[bank].used, &fb_banks[bank].used_tail, block);
    fb_banks[bank].free_size += block->size;
    touching = NULL;
    cursor = fb_banks[bank].free;
    if (cursor) {
        do {
            if (cursor->offset == block->offset + block->size ||
                block->offset == cursor->offset + cursor->size) {
                touching = cursor;
                cursor = NULL;
            } else {
                cursor = cursor->next;
            }
        } while (cursor);
    }
    if (!touching) {
        fb_list_push(&fb_banks[bank].free, &fb_banks[bank].free_tail, block);
        /* @hack redundant: a never-run store of 0 to the bank's free_size, the
         * same as the other return's, which keeps the two returns' shared exit
         * and fb_banks' address in R11 (0x8ce19514), as the image has them;
         * without it, the registers rotate, the function runs 16 bytes long,
         * the functions after it move, and 89 of 124 words differ */
        if (0) {
            fb_banks[bank].free_size = 0;
        }
        return KMSTATUS_SUCCESS;
    }
    do {
        if (touching->offset == block->offset + block->size) {
            touching->offset = block->offset;
        }
        touching->size += block->size;
        fb_block_drop(block);
        fb_list_remove(&fb_banks[bank].free, &fb_banks[bank].free_tail,
                       touching);
        block = touching;
        touching = NULL;
        cursor = fb_banks[bank].free;
        if (cursor) {
            do {
                if (cursor->offset == block->offset + block->size ||
                    block->offset == cursor->offset + cursor->size) {
                    touching = cursor;
                    cursor = NULL;
                } else {
                    cursor = cursor->next;
                }
            } while (cursor);
        }
    } while (touching);
    fb_list_push(&fb_banks[bank].free, &fb_banks[bank].free_tail, block);
    /* @hack redundant: a never-run store of 0 to the bank's free_size, the same
     * as the other return's, which keeps the two returns' shared exit and
     * fb_banks' address in R11 (0x8ce19514), as the image has them; without it,
     * the registers rotate, the function runs 16 bytes long, the functions
     * after it move, and 89 of 124 words differ */
    if (0) {
        fb_banks[bank].free_size = 0;
    }
    return KMSTATUS_SUCCESS;
}

/* a record from the pool, with BLOCK_TAKEN its only flag, or NULL when every
 * record is taken */
static struct fb_block *fb_block_take(void)
{
    int i;

    for (i = 0; i < BLOCKS; i++) {
        if (!(fb_blocks[i].flags & BLOCK_TAKEN)) {
            fb_blocks[i].flags = BLOCK_TAKEN;
            return &fb_blocks[i];
        }
    }
    return NULL;
}

static void fb_block_drop(struct fb_block *block)
{
    int i;

    for (i = 0; i < BLOCKS; i++) {
        if (&fb_blocks[i] == block) {
            fb_blocks[i].flags &= ~BLOCK_TAKEN;
        }
    }
}

static void fb_list_push(struct fb_block **head, struct fb_block **tail,
                         struct fb_block *block)
{
    struct fb_block *first;

    if (!*head) {
        block->prev = NULL;
        block->next = NULL;
        *head = block;
        *tail = block;
    } else {
        first = *head;
        block->prev = first->prev;
        block->next = *head;
        first->prev = block;
        *head = block;
    }
}

/* insert block after prev, in a list whose tail is *tail */
static void fb_list_insert(struct fb_block **tail, struct fb_block *block,
                           struct fb_block *prev)
{
    struct fb_block *next;

    if (!prev->next) {
        *tail = block;
        block->prev = prev;
        block->next = NULL;
    } else {
        next = prev->next;
        next->prev = block;
        block->prev = prev;
        block->next = prev->next;
    }
    prev->next = block;
}

static void fb_list_remove(struct fb_block **head, struct fb_block **tail,
                           struct fb_block *block)
{
    /* @hack type: volatile on prev, which keeps prev at sp+0 and next
     * at sp+4 (0x8ce19694, 0x8ce1969c); without it, next stays in R7, the
     * function is 4 bytes shorter and 24 of 24 words differ */
    struct fb_block *volatile prev = block->prev;
    struct fb_block *next = block->next;

    if (!block->prev) {
        *head = block->next;
    } else {
        prev->next = block->next;
    }
    if (!block->next) {
        *tail = block->prev;
    } else {
        next->prev = block->prev;
    }
}

/* copy size bytes of video memory from src down to dst through a buffer on the
 * stack. the runs can overlap, but dst must not be above src. src and dst are
 * offsets in the 64-bit view */
static void fb_move(KMDWORD dst, KMDWORD src, KMDWORD size)
{
    KMDWORD buffer[CHUNK / 4];
    KMDWORD *line;
    KMDWORD *data;
    /* @hack type: volatile on last, which keeps the last run's buffer
     * address at sp+4 (0x8ce1973c, 0x8ce1973e, 0x8ce19750); without it, the
     * frame is 4 bytes smaller and 49 of 77 words differ */
    KMDWORD *volatile last;
    int i;

    if (size > CHUNK) {
        data = buffer;
        do {
            kmiCopyFromFrameBuffer(data, src, CHUNK);
            line = data;
            i = CHUNK / CACHE_LINE;
            while (i) {
                kmiPurgeCache(line);
                line += CACHE_LINE / 4;
                i--;
            }
            kmiCopyToFrameBuffer(dst, data, CHUNK);
            size -= CHUNK;
            src = src + CHUNK;
            dst = dst + CHUNK;
        } while (size > CHUNK);
    }
    /* @hack redundant: a never-run store of 0 to size, which keeps size in R13,
     * 0x800 in R14 and src at sp+0 (0x8ce196c6, 0x8ce196e0), as the image has
     * them; without it, size takes R11 and src R8, the buffer moves to sp+4,
     * the function runs 4 bytes short and 72 of 77 words differ */
    if (0) {
        size = 0;
    }
    kmiCopyFromFrameBuffer(last = buffer, src, size);
    line = last;
    i = CHUNK / CACHE_LINE;
    while (i) {
        kmiPurgeCache(line);
        line += CACHE_LINE / 4;
        i--;
    }
    kmiCopyToFrameBuffer(dst, last, size);
}

/* sort a list by offset, lowest first. each pass moves the unsorted part's
 * block with the largest offset to the end of that part */
static void fb_list_sort(struct fb_block **head, struct fb_block **tail)
{
    struct fb_block *cursor;
    struct fb_block *largest;
    struct fb_block *end;
    int count = 0;
    int i;
    int j;
    int left;

    if (!*head) {
        return;
    }
    cursor = *head;
    largest = cursor;
    do {
        if (cursor->offset > largest->offset) {
            largest = cursor;
        }
        cursor = cursor->next;
        count++;
    } while (cursor);
    if (count <= 1) {
        return;
    }
    if (*tail != largest) {
        fb_list_remove(head, tail, largest);
        fb_list_insert(tail, largest, *tail);
    }
    count -= 2;
    left = count;
    for (i = 0; i < count; i++) {
        largest = end = *head;
        for (j = 0; j < left; j++) {
            end = end->next;
            if (end->offset > largest->offset) {
                largest = end;
            }
        }
        if (end != largest) {
            fb_list_remove(head, tail, largest);
            fb_list_insert(tail, largest, end);
        }
        left--;
    }
}

/* what fb_compact does with an allocated block: leaves it, or moves a texture
 * or a VQ page */
enum { STAYS, MOVES_TEXTURE, MOVES_VQ_PAGE };

/* compact a bank whose free and used lists are sorted by offset. an unpaired
 * texture or VQ page that a free block ends at moves down to that block's
 * start, and the free block moves above it. a free block whose offset isn't a
 * multiple of 32 first splits at the next multiple. its bytes below that become
 * a free block of their own. after a move, the free block merges with the next
 * one if they touch. the texture's surface, or the VQ page and each of its
 * slots' surfaces, gets the new offset. frame buffers, the tile accelerator's
 * buffers and paired blocks stay. the result is KMSTATUS_NOT_ENOUGH_MEMORY,
 * with the compaction stopped, when that split finds no record left, and else
 * KMSTATUS_SUCCESS */
static KMSTATUS fb_compact(struct fb_block **free, struct fb_block **free_tail,
                           struct fb_block **used, struct fb_block **used_tail)
{
    struct fb_block *gap;
    struct fb_block *block;
    struct fb_block *head;
    struct fb_block *next;
    struct fb_block *prev;
    KMINT32 kind;
    int i;
    KMDWORD moved;

    gap = *free;
    block = *used;
    if (!gap) {
        return KMSTATUS_SUCCESS;
    }
    if (!block) {
        return KMSTATUS_SUCCESS;
    }
    do {
        kind = STAYS;
        if ((block->flags & BLOCK_TEXTURE0) ||
            (block->flags & BLOCK_TEXTURE1) ||
            (!(block->flags & BLOCK_PAIRED) &&
             (block->flags & BLOCK_PLACED0)) ||
            (!(block->flags & BLOCK_PAIRED) &&
             (block->flags & BLOCK_PLACED1))) {
            kind = MOVES_TEXTURE;
        }
        if ((block->flags & BLOCK_VQ0) || (block->flags & BLOCK_VQ1) ||
            (!(block->flags & BLOCK_PAIRED) &&
             (block->flags & BLOCK_VQ_PLACED0)) ||
            (!(block->flags & BLOCK_PAIRED) &&
             (block->flags & BLOCK_VQ_PLACED1))) {
            kind = MOVES_VQ_PAGE;
        }
        if (kind != STAYS) {
            if (gap->offset + gap->size == block->offset) {
                if (gap->offset % ALIGN) {
                    head = fb_block_take();
                    if (!head) {
                        return KMSTATUS_NOT_ENOUGH_MEMORY;
                    }
                    head->offset = gap->offset;
                    gap->offset = (gap->offset + ALIGN - 1) & ~(ALIGN - 1);
                    head->size = gap->offset - head->offset;
                    gap->size = gap->size - head->size;
                    prev = gap->prev;
                    if (!prev) {
                        fb_list_push(free, free_tail, head);
                    } else {
                        fb_list_insert(free_tail, head, prev);
                    }
                }
                fb_move(gap->offset, block->offset, block->size);
                moved = block->offset - gap->offset;
                block->offset = gap->offset;
                gap->offset = gap->offset + block->size;
                if (kind == MOVES_TEXTURE) {
                    ((KMSURFACEDESC *)block->owner)->pSurface =
                        (PKMDWORD)block->offset;
                } else {
                    ((struct fb_vq_page *)block->owner)->offset = block->offset;
                    for (i = 0; i < SLOTS; i++) {
                        if (((struct fb_vq_page *)block->owner)->slot[i]) {
                            ((struct fb_vq_page *)block->owner)
                                ->slot[i]
                                ->pSurface -= moved >> 2;
                        }
                    }
                }
                next = gap->next;
                if (next && gap->offset + gap->size == next->offset) {
                    gap->size = gap->size + next->size;
                    fb_list_remove(free, free_tail, next);
                    fb_block_drop(next);
                }
            }
            while (block && gap->offset > block->offset) {
                block = block->next;
            }
        } else {
            while (gap && gap->offset < block->offset) {
                gap = gap->next;
            }
            if (!gap) {
                return KMSTATUS_SUCCESS;
            }
            while (block && gap->offset > block->offset) {
                block = block->next;
            }
        }
    } while (block && gap);
    return KMSTATUS_SUCCESS;
}

/* allocate size bytes at offset in bank for page as fb_alloc_at does for a
 * surface, and store the block's offset in page's offset field. the block takes
 * BLOCK_VQ_PLACED0 or BLOCK_VQ_PLACED1 by bank */
static KMSTATUS fb_alloc_vq_at(short bank, KMINT32 offset, KMINT32 size,
                               struct fb_vq_page *page, KMBOOLEAN paired,
                               KMBOOLEAN live)
{
    KMINT32 end = offset + size;
    KMINT32 rest;
    KMINT32 whole;
    KMINT32 before;
    /* @hack spelling: each cut's remainder, the free block's size less size,
     * computed into left before it is stored, which holds fb_banks' address in
     * R8 and the bank's offset in R9 (0x8ce19a30, 0x8ce19a34); stored
     * straight, the two trade, and 32 of 307 words differ */
    KMINT32 left;
    struct fb_block *block;
    struct fb_block *found;
    struct fb_block *taken;
    struct fb_block *after;

    rest = size % ALIGN;
    if (rest) {
        size += ALIGN - rest;
    }
    found = NULL;
    block = fb_banks[bank].free;
    if (block) {
        do {
            if (block->offset <= offset &&
                block->offset + block->size > offset && block->offset <= end &&
                block->offset + block->size >= end) {
                found = block;
                block = NULL;
            } else {
                block = block->next;
            }
        } while (block);
    }
    if (!found) {
        return KMSTATUS_NOT_ENOUGH_MEMORY;
    }
    if (found->size == size) {
        fb_list_remove(&fb_banks[bank].free, &fb_banks[bank].free_tail, found);
        fb_list_push(&fb_banks[bank].used, &fb_banks[bank].used_tail, found);
        page->offset = found->offset;
        found->owner = (KMDWORD)page;
        found->flags = BLOCK_TAKEN;
        if (bank == 0) {
            found->flags |= BLOCK_VQ_PLACED0;
        } else {
            found->flags |= BLOCK_VQ_PLACED1;
        }
        if (paired) {
            found->flags |= BLOCK_PAIRED;
        }
        if (live) {
            found->flags |= BLOCK_LIVE;
        }
        fb_banks[bank].free_size -= found->size;
    } else if (found->offset == offset) {
        taken = fb_block_take();
        if (!taken) {
            return KMSTATUS_NOT_ENOUGH_MEMORY;
        }
        taken->offset = found->offset;
        taken->size = size;
        left = found->size - size;
        found->size = left;
        found->offset += size;
        fb_list_push(&fb_banks[bank].used, &fb_banks[bank].used_tail, taken);
        page->offset = taken->offset;
        taken->owner = (KMDWORD)page;
        if (bank == 0) {
            taken->flags |= BLOCK_VQ_PLACED0;
        } else {
            taken->flags |= BLOCK_VQ_PLACED1;
        }
        if (paired) {
            taken->flags |= BLOCK_PAIRED;
        }
        if (live) {
            taken->flags |= BLOCK_LIVE;
        }
        fb_banks[bank].free_size -= taken->size;
    } else if (found->offset + found->size == end) {
        taken = fb_block_take();
        if (!taken) {
            return KMSTATUS_NOT_ENOUGH_MEMORY;
        }
        left = found->size - size;
        found->size = left;
        taken->offset = offset;
        taken->size = size;
        fb_list_push(&fb_banks[bank].used, &fb_banks[bank].used_tail, taken);
        page->offset = taken->offset;
        taken->owner = (KMDWORD)page;
        if (bank == 0) {
            taken->flags |= BLOCK_VQ_PLACED0;
        } else {
            taken->flags |= BLOCK_VQ_PLACED1;
        }
        if (paired) {
            taken->flags |= BLOCK_PAIRED;
        }
        if (live) {
            taken->flags |= BLOCK_LIVE;
        }
        fb_banks[bank].free_size -= taken->size;
    } else {
        taken = fb_block_take();
        if (!taken) {
            return KMSTATUS_NOT_ENOUGH_MEMORY;
        }
        after = fb_block_take();
        if (!after) {
            fb_block_drop(taken);
            return KMSTATUS_NOT_ENOUGH_MEMORY;
        }
        whole = found->size;
        before = offset - found->offset;
        found->size = before;
        taken->offset = offset;
        taken->size = size;
        fb_list_push(&fb_banks[bank].used, &fb_banks[bank].used_tail, taken);
        after->offset = offset + size;
        after->size = whole - size - before;
        fb_list_insert(&fb_banks[bank].free_tail, after,
                       fb_banks[bank].free_tail);
        page->offset = taken->offset;
        taken->owner = (KMDWORD)page;
        if (bank == 0) {
            taken->flags |= BLOCK_VQ_PLACED0;
        } else {
            taken->flags |= BLOCK_VQ_PLACED1;
        }
        if (paired) {
            taken->flags |= BLOCK_PAIRED;
        }
        if (live) {
            taken->flags |= BLOCK_LIVE;
        }
        fb_banks[bank].free_size -= taken->size;
    }
    return KMSTATUS_SUCCESS;
}

/* fill in shape for the VQ page that surface goes in, by its width and whether
 * it has mipmaps, and return 1. a width other than 8, 16, 32 or 64 returns 0
 * and leaves shape unset */
static KMINT32 fb_vq_shape_for(struct fb_vq_shape *shape,
                               KMSURFACEDESC *surface)
{
    switch (surface->u0.USize) {
    case 8:
        if (surface->fSurfaceFlags & KM_SURFACEFLAGS_MIPMAPED) {
            shape->kind = KIND_8_MIPMAPS;
            shape->codebook = 0x80;
            shape->index = 0x20;
            shape->size = 0xfa0;
        } else {
            shape->kind = KIND_8;
            shape->codebook = 0x80;
            shape->index = 0x20;
            shape->size = 0xf90;
        }
        break;
    case 16:
        if (surface->fSurfaceFlags & KM_SURFACEFLAGS_MIPMAPED) {
            shape->kind = KIND_16_MIPMAPS;
            shape->codebook = 0x80;
            shape->index = 0x60;
            shape->size = 0xfe0;
        } else {
            shape->kind = KIND_16;
            shape->codebook = 0x80;
            shape->index = 0x40;
            shape->size = 0xfc0;
        }
        break;
    case 32:
        if (surface->fSurfaceFlags & KM_SURFACEFLAGS_MIPMAPED) {
            shape->kind = KIND_32_MIPMAPS;
            shape->codebook = 0x200;
            shape->index = 0x160;
            shape->size = 0xf60;
        } else {
            shape->kind = KIND_32;
            shape->codebook = 0x100;
            shape->index = 0x100;
            shape->size = 0x1000;
        }
        break;
    case 64:
        if (surface->fSurfaceFlags & KM_SURFACEFLAGS_MIPMAPED) {
            shape->kind = KIND_64_MIPMAPS;
            shape->codebook = 0x800;
            shape->index = 0x560;
            shape->size = 0xd60;
        } else {
            shape->kind = KIND_64;
            shape->codebook = 0x400;
            shape->index = 0x400;
            shape->size = 0x1000;
        }
        break;
    default:
        return 0;
        /* @hack redundant: a break after the return, which loads __divlu's
         * address into R3 (0x8ce19d56); without it, R2, and 2 of 101 words
         * differ */
        break;
    }
    shape->slots = PAGE_CODEBOOKS / shape->codebook;
    return 1;
}

/* a VQ page record from the pool, with PAGE_TAKEN its only flag, or NULL when
 * every record is taken. its slots hold what they held when it was dropped */
static struct fb_vq_page *fb_vq_take(void)
{
    int i;

    for (i = 0; i < VQ_PAGES; i++) {
        if (!(fb_vq_pool[i].flags & PAGE_TAKEN)) {
            fb_vq_pool[i].flags = PAGE_TAKEN;
            return &fb_vq_pool[i];
        }
    }
    return NULL;
}

static void fb_vq_drop(struct fb_vq_page *page)
{
    int i;

    for (i = 0; i < VQ_PAGES; i++) {
        if (&fb_vq_pool[i] == page) {
            fb_vq_pool[i].flags &= ~PAGE_TAKEN;
        }
    }
}

static void fb_vq_push(struct fb_vq_page **head, struct fb_vq_page *page)
{
    if (!*head) {
        page->next = NULL;
        *head = page;
    } else {
        page->next = *head;
        *head = page;
    }
}

/* unlink page from the list at *head, which must hold it */
static void fb_vq_remove(struct fb_vq_page **head, struct fb_vq_page *page)
{
    struct fb_vq_page *cursor = *head;
    struct fb_vq_page *prev = NULL;

    if (cursor) {
        do {
            if (cursor->next == page) {
                prev = cursor;
                cursor = NULL;
            } else {
                cursor = cursor->next;
            }
        } while (cursor);
    }
    if (*head == page) {
        *head = page->next;
    } else {
        prev->next = page->next;
    }
}

/* the smallest idle half of a VQ page pair in bank that holds size bytes and
 * whose twin's page has page's kind. the result is NULL when there is none.
 * nothing in LOADER.BIN calls it. it has external linkage, since SHC leaves an
 * uncalled static function out of the object file */
struct fb_block *fb_find_idle_vq(short bank, KMINT32 size,
                                 struct fb_vq_page *page)
{
    struct fb_block *block;
    struct fb_block *best;
    struct fb_block *twin;
    short twin_bank;
    KMINT32 smallest = KM_TEXTURE_BANK_SIZE;
    KMINT32 twin_offset;

    block = fb_banks[bank].used;
    best = NULL;
    if (block) {
        do {
            if (!(block->flags & BLOCK_LIVE) && (block->flags & BLOCK_PAIRED) &&
                (block->flags & (BLOCK_VQ_PLACED0 | BLOCK_VQ_PLACED1)) &&
                block->size >= size && block->size < smallest) {
                if (bank == 0) {
                    twin_offset = block->offset + fb_bank0_size;
                } else {
                    twin_offset = block->offset - fb_bank0_size;
                }
                twin = fb_find_used(twin_offset, &twin_bank);
                if (twin) {
                    if ((((struct fb_vq_page *)twin->owner)->flags &
                         PAGE_KIND) == (page->flags & PAGE_KIND)) {
                        smallest = block->size;
                        best = block;
                    }
                }
            }
            block = block->next;
        } while (block);
    }
    return best;
}

/* the offset fb_find_pair returns for size bytes of a surface that isn't a
 * mipmap. nothing in LOADER.BIN calls it. it has external linkage, since SHC
 * leaves an uncalled static function out of the object file */
KMINT32 fb_find_pair_space(KMINT32 size)
{
    struct fb_block *block;
    KMINT32 rest;
    KMINT32 offset;
    KMINT32 last;
    KMSTATUS status;

    rest = size % ALIGN;
    if (rest) {
        size += ALIGN - rest;
    }
    block = fb_banks[0].free;
    if (block) {
        do {
            if (block->size >= size) {
                last = block->offset + block->size - size;
                for (offset = block->offset; offset <= last; offset += ALIGN) {
                    status = fb_check_free(1, offset + fb_bank0_size, size);
                    if (!status) {
                        return offset;
                    }
                }
            }
            block = block->next;
        } while (block);
    }
    return -1;
}

/* convert offset from the 64-bit view of video memory to the 32-bit view, the
 * view kmiFBalloc, kmiFBallocEx and kmiTAallocEx return. the 64-bit view
 * alternates the two halves a long at a time, and the 32-bit view puts the
 * second half after the first. each 8 MB of the 64-bit view maps the same way
 * onto 8 MB of the 32-bit view */
static KMDWORD fb_to_32bit(KMDWORD offset)
{
    KMDWORD converted = offset & ~7UL;

    converted >>= 1;
    converted += offset & 3;
    if (offset & 4) {
        converted += 0x400000;
    }
    if (offset >= 0x800000) {
        converted += 0x400000;
    }
    return converted;
}

/* the inverse of fb_to_32bit: convert offset from the 32-bit view to the 64-bit
 * view. half is 4 for an offset in the second half, whose long lies 4 bytes
 * into each 8 of the 64-bit view */
static KMDWORD fb_to_64bit(KMDWORD offset)
{
    KMDWORD half = 0;
    KMDWORD converted;

    if (offset < 0x800000) {
        if (offset >= 0x400000) {
            half = 4;
            offset -= 0x400000;
        }
        converted = ((offset & ~3UL) << 1) + (offset & 3) + half;
    } else {
        if (offset >= 0xc00000) {
            half = 4;
            offset -= 0x400000;
        }
        converted = ((offset & ~3UL) << 1) + (offset & 3) + half - 0x800000;
    }
    return converted;
}
