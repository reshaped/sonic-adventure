#include <sg_syg2.h>
#include <sg_xpt.h>

/* the G2 DMA protection register, through P2. it takes PROTECTION_KEY in its
 * high half, then the first and the last megabyte a transfer may reach, as
 * MEGABYTE numbers them */
#define G2_PROTECTION (*(volatile Uint32 *)0xa05f78bc)
#define PROTECTION_KEY 0x46590000UL
/* the number of the megabyte that holds address: its bits 20 to 26 */
#define MEGABYTE(address) (((Uint32)(address) & 0x07f00000UL) >> 20)

Uint32 syG2DmaProtect = 0;

SYE_G2_ERR syG2DmaSetProtect(void *first, void *last)
{
    Uint32 lower = MEGABYTE(first);
    Uint32 upper = MEGABYTE(last);
    Uint32 bounds = (lower << 8) | upper;

    /* @hack redundant: a label nothing jumps to, which keeps the bounds in R6,
     * a register of their own (MOV R4,R6 at 0x8c12400c, and at 0x8ce020f4 in
     * LOADER.BIN); without it, SHC ORs them into R4, first's register, and
     * 8 of 20 words differ */
publish: /* NOLINT(clang-diagnostic-unused-label) */
    syG2DmaProtect = (G2_PROTECTION = bounds | PROTECTION_KEY);
    return SYE_G2_ERR_NOTHING;
}
