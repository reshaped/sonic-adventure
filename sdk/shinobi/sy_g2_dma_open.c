#include <machine.h>
#include <sg_syg2.h>
#include <sg_xpt.h>

SYE_G2_ERR syG2DmaOpen(SYG2DMA *handle, SYE_G2_DMA_TYPE type)
{
    SYE_G2_ERR err = SYE_G2_ERR_NOTHING;
    /* @hack spelling: the work through a local, which loads the channel
     * size, 56, again for the stores (MOV #56,R1 at 0x8c123dd6); without it,
     * SHC keeps the size in R2, the function is 2 bytes shorter and 112 of
     * 145 words differ */
    struct g2_work *work = sygG2WrkPtr;
    struct g2_dma_channel *channel = &work->channels[type];
    Sint32 mask;

    if (channel->open == true) {
        err = SYE_G2_ERR_DMA_ALREADY_OPEN;
    } else {
        mask = get_imask();
        set_imask(IMASK_ALL);
        sygG2WrkPtr->channels[channel->type].open = false;
        sygG2WrkPtr->channels[channel->type].busy = false;
        sygG2WrkPtr->channels[channel->type].ended = false;
        sygG2WrkPtr->channels[channel->type].type = channel->type;
        sygG2WrkPtr->channels[channel->type].g2_address = 0;
        sygG2WrkPtr->channels[channel->type].root_bus_address = 0;
        sygG2WrkPtr->channels[channel->type].length = 0;
        sygG2WrkPtr->channels[channel->type].direction = 0;
        sygG2WrkPtr->channels[channel->type].flags = 0;
        sygG2WrkPtr->channels[channel->type].unk_1c = 3;
        sygG2WrkPtr->channels[channel->type].callback = NULL;
        sygG2WrkPtr->channels[channel->type].callback_arg = NULL;
        channel->open = true;
        *handle = (SYG2DMA)channel;
        set_imask(mask);
    }
    return err;
}
