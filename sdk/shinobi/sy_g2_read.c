/* G2 bus read, with G2 DMA suspended and the G2 FIFO empty */
#include <machine.h>
#include <sg_syg2.h>
#include <sg_xpt.h>

SYE_G2_ERR syG2Read(void *g2_adr, void *buf_ptr, SYE_G2_DATA_WIDTH data_width,
                    Sint32 data_num, SYE_G2_ADR_MODE src_mode,
                    SYE_G2_ADR_MODE dst_mode)
{
    Sint32 src_step;
    Sint32 dst_step;
    int i;
    Sint32 mask;

    /* @bug uninitialized: a mode its switch doesn't name leaves that step
     * unset. the loops then step by what the caller left in R12 for src or
     * in R13 for dst (the byte loop's ADD R12,R5 and ADD R13,R6 at 0x8c11c496
     * and 0x8c11c49a, and at 0x8ce0056a and 0x8ce0056e in LOADER.BIN) */
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

    /* @hack type: src and dst aren't volatile, though src is on the G2 bus.
     * through a volatile src, SHC counts each loop's i after its copy, not
     * before it as the image does (ADD #1,R4 at 0x8c11c48e), and 11 of 143
     * words differ in each image */
    /* @hack spelling: each case declares dst before src, which keeps g2_adr in
     * R11 and buf_ptr in R10 (MOV R4,R11 and MOV R5,R10 at 0x8c11c402 and
     * 0x8c11c406, and at 0x8ce004d6 and 0x8ce004da in LOADER.BIN); with src
     * first, the two trade and 8 of 143 words differ */
    switch (data_width) {
    case SYE_G2_DATA_WIDTH_8BIT: {
        Uint8 *dst = buf_ptr;
        Uint8 *src = g2_adr;

        for (i = 0; i < data_num; i++) {
            *dst = *src;
            src += src_step;
            dst += dst_step;
        }
        break;
    }
    case SYE_G2_DATA_WIDTH_16BIT: {
        Uint16 *dst = buf_ptr;
        Uint16 *src = g2_adr;

        for (i = 0; i < data_num; i++) {
            *dst = *src;
            src += src_step;
            dst += dst_step;
        }
        break;
    }
    case SYE_G2_DATA_WIDTH_32BIT: {
        Uint32 *dst = buf_ptr;
        Uint32 *src = g2_adr;

        for (i = 0; i < data_num; i++) {
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
