#include <sg_syg2.h>
#include <sg_xpt.h>

/* the words of a channel's registers, and the index of its start register
 * among them. the start register's START_BUSY bit is set while a transfer
 * runs */
#define REGISTER_WORDS 8
#define REGISTER_START 6
#define START_BUSY 1

struct g2_dma_register_words {
    Uint32 channels[G2_DMA_CHANNELS][REGISTER_WORDS];
};

SYE_G2_ERR syG2DmaIsBusy(SYG2DMA handle, Bool *flg)
{
    /* @hack type: the channel's registers read as words, which sums the bank's
     * address in R0, the base added to the channel's offset (ADD R3,R0 at
     * 0x8c127520 in 1ST_READ.BIN, 0x8ce03fe8 in LOADER.BIN); through a struct
     * g2_dma_registers pointer, the two add in the other order, 18 of 19 words
     * differ in each image */
    volatile Uint32 *words =
        ((volatile struct g2_dma_register_words *)sygG2WrkPtr->registers)
            ->channels[((struct g2_dma_channel *)handle)->type];

    *flg = (words[REGISTER_START] & START_BUSY) ? true : false;
    return SYE_G2_ERR_NOTHING;
}
