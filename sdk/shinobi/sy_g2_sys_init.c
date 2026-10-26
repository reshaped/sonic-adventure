#include <sg_chain.h>
#include <sg_syg2.h>
#include <sg_xpt.h>
#include <string.h>

/* the system bus's G2 DMA registers, through P2 */
#define G2_DMA_REGISTERS ((volatile struct g2_dma_registers *)0xa05f7800)

/* main memory's first and last bytes: G2 DMA may reach all of it */
#define MEMORY_FIRST ((void *)0x0c000000)
#define MEMORY_LAST ((void *)0x0cffffff)

#define DMA_END_PRIORITY 96

SYE_G2_ERR syG2SysInit(void *prm_ptr)
{
    int i;

    sygG2WrkPtr = (struct g2_work *)(((Uint32)sygG2WrkImg + 31) & ~31UL);
    memset(sygG2WrkPtr, 0, sizeof(struct g2_work));
    sygG2WrkPtr->registers = G2_DMA_REGISTERS;
    sygG2WrkPtr->mask_interrupts = true;
    sygG2WrkPtr->unk_e8 = 0;
    for (i = 0; i < G2_DMA_CHANNELS; i++) {
        syG2DmaReset(&sygG2WrkPtr->channels[i]);
        sygG2WrkPtr->trigger_counts[i] = 0;
        sygG2WrkPtr->unk_10c[i] = 0;
        sygG2WrkPtr->end_counts[i] = 0;
    }
    sygG2WrkPtr->fifo_waits = 0;
    sygG2WrkPtr->aica_fifo_waits = 0;
    syG2DmaSetProtect(MEMORY_FIRST, MEMORY_LAST);
    sygG2WrkPtr->mask_interrupts = true;
    sygG2WrkPtr->channels[SYE_G2_DMA_TYPE_AICA].chain = syChainAddHandler(
        SYD_CHAIN_EVENT_IML4_AICADM, syG2DmaEndIntrruptHandler,
        DMA_END_PRIORITY, &sygG2WrkPtr->channels[SYE_G2_DMA_TYPE_AICA]);
    sygG2WrkPtr->channels[SYE_G2_DMA_TYPE_EXT0].chain = syChainAddHandler(
        SYD_CHAIN_EVENT_IML4_EX0DMA, syG2DmaEndIntrruptHandler,
        DMA_END_PRIORITY, &sygG2WrkPtr->channels[SYE_G2_DMA_TYPE_EXT0]);
    sygG2WrkPtr->channels[SYE_G2_DMA_TYPE_EXT1].chain = syChainAddHandler(
        SYD_CHAIN_EVENT_IML4_EX1DMA, syG2DmaEndIntrruptHandler,
        DMA_END_PRIORITY, &sygG2WrkPtr->channels[SYE_G2_DMA_TYPE_EXT1]);
    sygG2WrkPtr->channels[SYE_G2_DMA_TYPE_DEV].chain = syChainAddHandler(
        SYD_CHAIN_EVENT_IML4_DADMAE, syG2DmaEndIntrruptHandler,
        DMA_END_PRIORITY, &sygG2WrkPtr->channels[SYE_G2_DMA_TYPE_DEV]);
    sygG2DmaSuspendAll = false;
    return SYE_G2_ERR_NOTHING;
}
