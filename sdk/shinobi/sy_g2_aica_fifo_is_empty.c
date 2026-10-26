#include <sg_syg2.h>
#include <sg_xpt.h>

/* the system bus's FIFO status register, through P2. a FIFO's bit is set
 * while the FIFO holds a write */
#define FIFO_STATUS (*(volatile Uint32 *)0xa05f688c)
#define FIFO_AICA 0x01

SYE_G2_ERR syG2AicaFifoIsEmpty(Bool *flg)
{
    /* @hack spelling: the flag as a select, which SHC builds in R3 (MOVT R3
     * at 0x8c126e32, and at 0x8ce03e62 in LOADER.BIN); as !, it builds the
     * flag in R0 and 4 of 7 words differ */
    *flg = (FIFO_STATUS & FIFO_AICA) ? false : true;
    return SYE_G2_ERR_NOTHING;
}
