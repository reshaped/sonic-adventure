/* G2 DMA: Shinobi's trigger, on the port's bus (g2.h). the transfer is a
 * copy, done at once, between root bus memory and sound RAM, and the
 * channel's end interrupt follows at the next blank, which runs
 * syG2DmaEndIntrruptHandler through the channel's chain as the console's
 * interrupt does. a transfer to the AICA's registers goes through the host,
 * a word at a time */
#include <sg_syg2.h>
#include <sg_xpt.h>

#include "xbox/sdk/shinobi/g2.h"

volatile Uint32 port_g2_dma_ended;

SYE_G2_ERR syG2DmaTriger(SYG2DMA handle, SYE_G2_DMA_TRIGGER triger_mode)
{
    return syG2DmaTrigger(handle, triger_mode);
}

SYE_G2_ERR syG2DmaTrigger(SYG2DMA handle, SYE_G2_DMA_TRIGGER triger_mode)
{
    struct g2_dma_channel *channel = (struct g2_dma_channel *)handle;
    Uint32 length = channel->length & ~LENGTH_END;
    Uint8 *root = (Uint8 *)(channel->root_bus_address | 0xa0000000u);
    Uint8 *g2 = (Uint8 *)(channel->g2_address | 0xa0000000u);
    Uint32 k;

    (void)triger_mode;
    if (channel->busy == true) {
        return SYE_G2_ERR_DMA_BUSY;
    }
    channel->busy = true;
    sygG2WrkPtr->trigger_counts[channel->type]++;
    for (k = 0; k < length; k += 4) {
        if (channel->direction == SYE_G2_DMA_DIR_ROOT_TO_G2) {
            g2_write(g2 + k, *(Uint32 *)(root + k), 4);
        } else {
            *(Uint32 *)(root + k) = g2_read(g2 + k, 4);
        }
    }
    port_g2_dma_ended |= 1u << channel->type;
    return SYE_G2_ERR_NOTHING;
}
