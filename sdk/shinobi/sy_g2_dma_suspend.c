#include <sg_syg2.h>
#include <sg_xpt.h>

SYE_G2_ERR syG2DmaSuspend(SYG2DMA handle)
{
    struct g2_dma_channel *channel = (struct g2_dma_channel *)handle;
    Bool busy;

    syG2DmaIsBusy(handle, &busy);
    if (busy == true) {
        struct g2_dma_registers *registers =
            G2_DMA_CHANNEL_REGISTERS(channel->type);

        registers->suspend = 1;
        channel->unk_1c = 2;
    }
    return SYE_G2_ERR_NOTHING;
}
