#include <sg_syg2.h>
#include <sg_xpt.h>

/* the words of a channel's registers, and the index of its enable register
 * among them */
#define REGISTER_WORDS 8
#define REGISTER_ENABLE 5

struct g2_dma_register_words {
    Uint32 channels[G2_DMA_CHANNELS][REGISTER_WORDS];
};

SYE_G2_ERR syG2DmaReset(struct g2_dma_channel *channel)
{
    volatile Uint32 *words;

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
    /* @hack type: the channel's registers taken as words, which sums the bank's
     * address in R0 (ADD R3,R0 at 0x8c123f9e in 1ST_READ.BIN, 0x8ce02086 in
     * LOADER.BIN); through a struct g2_dma_registers pointer, the registers
     * that hold the work's address and the stored zero swap, and 10 of 112
     * words differ in each image */
    words = ((volatile struct g2_dma_register_words *)sygG2WrkPtr->registers)
                ->channels[channel->type];
    words[REGISTER_ENABLE] = 0;
    return SYE_G2_ERR_NOTHING;
}
