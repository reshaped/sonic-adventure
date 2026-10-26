#include <sg_syg2.h>
#include <sg_xpt.h>

SYE_G2_ERR syG2DmaResumeAll(void)
{
    int i;
    /* @hack spelling: the handle through a local, which loads the work's
     * address through R12 and calls through R11 (0x8c123fc0, 0x8c123fca);
     * passed straight to the call, the two trade and 2 of 30 words differ */
    SYG2DMA handle;

    for (i = 0; i < G2_DMA_CHANNELS; i++) {
        handle = (SYG2DMA)&sygG2WrkPtr->channels[i];
        syG2DmaResume(handle);
    }
    sygG2DmaSuspendAll = false;
    return SYE_G2_ERR_NOTHING;
}
