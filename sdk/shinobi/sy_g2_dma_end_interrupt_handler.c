#include <sg_syg2.h>
#include <sg_xpt.h>

static volatile Uint32 *interrupt_status = G2_INT_STATUS;

void syG2DmaEndIntrruptHandler(void *arg)
{
    struct g2_dma_channel *channel = arg;
    /* @hack type: the cast drops the registers' volatile, which lets the
     * offset of end_counts load ahead of the enable store (MOV.W at 0x8c123d48,
     * MOV.L R6,@(20,R5) at 0x8c123d5c in 1ST_READ.BIN, 0x8ce01f5c and
     * 0x8ce01f70 in LOADER.BIN); through a volatile pointer, it loads after the
     * stores and 10 of 39 words differ in each image */
    struct g2_dma_registers *registers =
        (struct g2_dma_registers *)sygG2WrkPtr->registers;

    registers[channel->type].enable = 0;
    channel->busy = false;
    channel->ended = true;
    sygG2WrkPtr->end_counts[channel->type]++;
    *interrupt_status = 1 << (channel->type + G2_INT_DMA_END_SHIFT);
    if (channel->callback != NULL) {
        channel->callback(channel->callback_arg);
    }
}
