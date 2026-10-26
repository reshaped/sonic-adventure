#include <sg_syg2.h>
#include <sg_xpt.h>
#include <shinobi.h>

SYE_G2_ERR syG2DmaTriger(SYG2DMA handle, SYE_G2_DMA_TRIGGER triger_mode)
{
    return syG2DmaTrigger(handle, triger_mode);
}

SYE_G2_ERR syG2DmaTrigger(SYG2DMA handle, SYE_G2_DMA_TRIGGER triger_mode)
{
    struct g2_dma_channel *channel = (struct g2_dma_channel *)handle;
    SYE_G2_ERR err = SYE_G2_ERR_NOTHING;
    struct g2_dma_registers *registers =
        G2_DMA_CHANNEL_REGISTERS(channel->type);
    Bool busy;

    syG2DmaIsBusy(handle, &busy);
    if (busy == true) {
        err = SYE_G2_ERR_DMA_BUSY;
    } else {
        syCacheOCWB((void *)channel->root_bus_address,
                    channel->length & ~LENGTH_END);
        registers->enable = 0;
        registers->device = channel->g2_address;
        registers->memory = channel->root_bus_address;
        registers->length = channel->length;
        registers->direction = channel->direction;
        registers->trigger =
            channel->flags |
            (triger_mode == SYE_G2_DMA_TRIGGER_CPU ? 0
                                                   : SYE_G2_DMA_TRIGGER_INT);
        registers->enable = 1;
        channel->busy = true;
        sygG2WrkPtr->trigger_counts[channel->type]++;
        registers->start = 1;
    }
    return err;
}
