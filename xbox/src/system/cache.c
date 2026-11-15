/* operand-cache control and the matrix below Ninja's stack top. make pc
 * compiles this in place of src/system/cache.src.
 *
 * the three RAM-mode entries return at once, as cache.src's do. x86 keeps
 * its caches coherent and has no operand cache to empty, so cache_empty
 * keeps only what the game can see of it: interrupts unmasked after, as the
 * port's mask models them */
#include <machine.h>
#include <ninja.h>
#include <sg_xpt.h>

#include "system/cache.h"
#include "xbox/include/sh4.h"

void cache_ram_init(void)
{
}

void cache_ram_restore(void)
{
}

void cache_ram_end(void)
{
}

void cache_empty(void)
{
    set_imask(0);
}

/* cache.src's paired loads fill XF0 to XF15 from the matrix's floats in
 * order, into the port's XMTRX (xbox/include/sh4.h) */
Bool matrix_load_parent(void)
{
    sh4_xmtrx_set((const Float *)(_nj_current_matrix_ptr_ - 1));
    return TRUE;
}
