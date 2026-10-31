/* G2 bus write, with G2 DMA suspended and the FIFOs drained */
#include <machine.h>
#include <sg_syg2.h>
#include <sg_xpt.h>

/* syG2Write waits for both FIFOs to empty before its first write, then
 * again every WRITES_PER_WAIT writes. WRITES_PER_WAIT must be a power of 2:
 * the loops mask i with WRITES_PER_WAIT - 1 */
#define WRITES_PER_WAIT 8

SYE_G2_ERR syG2Write(void *buf_ptr, void *g2_adr, SYE_G2_DATA_WIDTH data_width,
                     Sint32 data_num, SYE_G2_ADR_MODE src_mode,
                     SYE_G2_ADR_MODE dst_mode)
{
    Sint32 src_step;
    Sint32 dst_step;
    int i;
    Sint32 mask;

#ifdef GCC
    /* repairs the unset steps the tag below names: GCC steps a mode its
     * switch doesn't name by 0, as SYE_G2_ADR_MODE_FIX. GCC can't see that
     * every path that restores mask sets it: both run under one test of
     * mask_interrupts, which nothing between changes */
    src_step = 0;
    dst_step = 0;
    mask = 0;
#endif
    /* @bug uninitialized: a mode its switch doesn't name leaves that step
     * unset. the loops then step by what the caller left in R10 for src or
     * in R9 for dst (the byte loop's ADD R10,R3 and ADD R9,R2 at 0x8c11c744
     * and 0x8c11c74a, and at 0x8ce00818 and 0x8ce0081e in LOADER.BIN) */
    switch (src_mode) {
    case SYE_G2_ADR_MODE_FIX:
        src_step = 0;
        break;
    case SYE_G2_ADR_MODE_ADD:
        src_step = 1;
        break;
    case SYE_G2_ADR_MODE_SUB:
        src_step = -1;
        break;
    }
    switch (dst_mode) {
    case SYE_G2_ADR_MODE_FIX:
        dst_step = 0;
        break;
    case SYE_G2_ADR_MODE_ADD:
        dst_step = 1;
        break;
    case SYE_G2_ADR_MODE_SUB:
        dst_step = -1;
        break;
    }

    if (sygG2WrkPtr->mask_interrupts == true) {
        mask = get_imask();
        set_imask(IMASK_ALL);
    }
    syG2DmaSuspendAll();
    syG2FifoWaitEmpty();

    /* @hack type: src and dst aren't volatile, though dst is on the G2 bus.
     * through a volatile dst, SHC counts each loop's i after its copy, not
     * before it as the image does (ADD #1,R8 at 0x8c11c736), and 21 of 187
     * words differ in each image */
    /* @hack spelling: each case declares dst before src, which takes buf_ptr
     * into R8 at entry (MOV R4,R8 at 0x8c11c696, and at 0x8ce0076a in
     * LOADER.BIN); with src first, R8 takes g2_adr, the function is 4 bytes
     * shorter and 95 of 187 words differ */
    switch (data_width) {
    case SYE_G2_DATA_WIDTH_8BIT: {
        Uint8 *dst = g2_adr;
        Uint8 *src = buf_ptr;

        for (i = 0; i < data_num; i++) {
            if ((i & (WRITES_PER_WAIT - 1)) == 0) {
                syG2FifoWaitEmpty();
                syG2AicaFifoWaitEmpty();
            }
            *dst = *src;
            src += src_step;
            dst += dst_step;
        }
        break;
    }
    case SYE_G2_DATA_WIDTH_16BIT: {
        Uint16 *dst = g2_adr;
        Uint16 *src = buf_ptr;

        for (i = 0; i < data_num; i++) {
            if ((i & (WRITES_PER_WAIT - 1)) == 0) {
                syG2FifoWaitEmpty();
                syG2AicaFifoWaitEmpty();
            }
            *dst = *src;
            src += src_step;
            dst += dst_step;
        }
        break;
    }
    case SYE_G2_DATA_WIDTH_32BIT: {
        Uint32 *dst = g2_adr;
        Uint32 *src = buf_ptr;

        for (i = 0; i < data_num; i++) {
            if ((i & (WRITES_PER_WAIT - 1)) == 0) {
                syG2FifoWaitEmpty();
                syG2AicaFifoWaitEmpty();
            }
            *dst = *src;
            src += src_step;
            dst += dst_step;
        }
        break;
    }
    }

    syG2DmaResumeAll();
    if (sygG2WrkPtr->mask_interrupts == true) {
        set_imask(mask);
    }
    return SYE_G2_ERR_NOTHING;
}
