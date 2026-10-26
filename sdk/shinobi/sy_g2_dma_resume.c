#include <sg_syg2.h>
#include <sg_xpt.h>

SYE_G2_ERR syG2DmaResume(SYG2DMA handle)
{
    Bool busy;

    syG2DmaIsBusy(handle, &busy);
    if (busy == true) {
        /* @hack spelling: the registers through a local, which loads the
         * zero after the work pointer and the channel's type (MOV #0,R2 at
         * 0x8c126e5a, and at 0x8ce03e8a in LOADER.BIN); reached through the
         * work, the zero loads first and 8 of 24 words differ */
        volatile struct g2_dma_registers *registers = sygG2WrkPtr->registers;

        registers[((struct g2_dma_channel *)handle)->type].suspend = 0;
    }
    return SYE_G2_ERR_NOTHING;
}
