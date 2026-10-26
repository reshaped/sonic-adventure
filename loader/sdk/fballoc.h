/* the allocator calls that SDK 1.20J's other Kamui members make. kamui.h, which
 * both SDK releases share, doesn't declare them: 1.20J's take their offsets as
 * integers, and 1.30J's as pointers */
#ifndef LOADER_SDK_FBALLOC_H
#define LOADER_SDK_FBALLOC_H
#include <kamui.h>

/* allocate the tile accelerator's two interleaved buffers in bank 0 of video
 * memory, size bytes each, rounded up to a multiple of 32. on success, *first
 * and *second get their offsets in the 32-bit view. the result is
 * KMSTATUS_NOT_ENOUGH_MEMORY, with nothing stored, when no free block holds
 * both buffers or no block record is left, and else KMSTATUS_SUCCESS */
KMSTATUS kmiTAallocEx(KMINT32 size, KMDWORD *first, KMDWORD *second);
/* free the block that holds two interleaved buffers, given their offsets in the
 * 32-bit view as kmiFBalloc, kmiFBallocEx and kmiTAallocEx store them.
 * kmiFBfreeEx frees the block at first, or when kmiFBfree fails there, the
 * block at second. the result is the last kmiFBfree's status */
KMSTATUS kmiFBfreeEx(KMDWORD first, KMDWORD second);

#endif
