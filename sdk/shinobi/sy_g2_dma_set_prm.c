#include <sg_syg2.h>
#include <sg_xpt.h>

/* the masks round down to a multiple of 32. ADDRESS_MASK keeps an
 * address's 29-bit physical address, and LENGTH_MASK keeps a length below
 * 32 MB */
#define ADDRESS_MASK 0x1fffffe0UL
#define LENGTH_MASK 0x01ffffe0UL

/* bits of g2_dma_channel.flags, which syG2DmaTrigger writes to the trigger
 * register. FLAG_INT_TRIGGER is the bit it sets there for
 * SYE_G2_DMA_TRIGGER_INT */
#define FLAG_TRANSFER_CTL 0x1UL
#define FLAG_INT_TRIGGER 0x2UL
#define FLAG_SUSPEND_MODE 0x4UL
#define FLAG_MASK 0x7

SYE_G2_ERR syG2DmaSetPrm(SYG2DMA handle, void *g2_adr, void *root_bus_adr,
                         Sint32 sz, SYE_G2_DMA_DIR dir, Bool transfer_ctl,
                         Bool suspend_mode, SYE_G2_CALLBACK_FUNC callback,
                         void *callback_1st_arg)
{
    struct g2_dma_channel *channel = (struct g2_dma_channel *)handle;

    channel->g2_address = (Uint32)g2_adr & ADDRESS_MASK;
    channel->root_bus_address = (Uint32)root_bus_adr & ADDRESS_MASK;
    channel->length = ((Uint32)sz & LENGTH_MASK) | LENGTH_END;
    channel->direction = dir & 1;
    if (transfer_ctl == true) {
        channel->flags |= FLAG_TRANSFER_CTL;
    } else {
        /* @bug logic: a false transfer_ctl clears FLAG_INT_TRIGGER, not the
         * FLAG_TRANSFER_CTL its if sets (MOV #-3,R3 at 0x8c126ea2) */
        channel->flags &= ~FLAG_INT_TRIGGER;
    }
    if (suspend_mode == true) {
        channel->flags |= FLAG_SUSPEND_MODE;
    } else {
        /* @bug logic: a false suspend_mode clears FLAG_TRANSFER_CTL, not the
         * FLAG_SUSPEND_MODE its if sets (MOV #-2,R3 at 0x8c126eba) */
        channel->flags &= ~FLAG_TRANSFER_CTL;
    }
    channel->flags &= FLAG_MASK;
    channel->callback = callback;
    channel->callback_arg = callback_1st_arg;
    channel->ended = false;
    return SYE_G2_ERR_NOTHING;
}
