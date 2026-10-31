/* Kamui access to the video chip's registers, texture memory and frame
 * buffer, and the installation of its interrupt handler */
#include <kamui.h>
#include <machine.h>
#include <sg_xpt.h>

/* the chip's registers, and video memory's 32-bit and 64-bit views, all
 * through the uncached area */
#define REGISTERS 0xa05f8000UL
#define VIEW_32BIT 0xa5000000UL
#define VIEW_64BIT 0xa4000000UL

/* video memory's 64-bit view as a target of DMA through the tile accelerator */
#define TA_VIEW_64BIT 0x11000000UL

/* system bus interrupt registers: the pending status, and the level 6 enables,
 * each for normal and for error interrupts. each enable is followed by the
 * bits of the interrupts Kamui takes on level 6 */
#define INT_STATUS (*(volatile KMDWORD *)0xa05f6900)
#define INT_STATUS_ERROR (*(volatile KMDWORD *)0xa05f6908)
#define INT_LEVEL6 (*(volatile KMDWORD *)0xa05f6930)
#define INT_RENDER_END_TSP 0x04
#define INT_VBLANK_IN 0x08
#define INT_HBLANK_IN 0x20
#define INT_YUV_END 0x40
#define INT_OPAQUE_END 0x80
#define INT_OPAQUE_MODIFIER_END 0x100
#define INT_TRANS_END 0x200
#define INT_TRANS_MODIFIER_END 0x400
#define INT_DMA2_END 0x80000
#define INT_PUNCHTHROUGH_END 0x200000
#define INT_LEVEL6_ERROR (*(volatile KMDWORD *)0xa05f6938)
#define INT_STRIP_OVERRUN 0x02
#define INT_ISP_OVERFLOW 0x04
#define INT_OL_OVERFLOW 0x08

/* where the level 6 interrupt handler's address goes, as an offset from VBR */
#define LEVEL6_HANDLER 0x224

#define CACHE_LINE 32

struct register_setting {
    KMDWORD reg;
    KMDWORD value;
};

void kmiPurgeMemory(KMDWORD addr, KMDWORD size);
void kmiWriteRegisters(KMDWORD count, struct register_setting *settings);
KMINT32 kmiCopyTextureToTexture(void *dst, void *src, KMDWORD size);

void kmiModuleInitialize(void)
{
}

KMINT32 kmiWriteRegister(KMDWORD reg, KMDWORD value)
{
    *(volatile KMDWORD *)(REGISTERS + reg) = value;
    return TRUE;
}

/* write back each dirty operand cache line that holds one of addr,
 * addr + CACHE_LINE and so on through addr + size */
void kmiPurgeMemory(KMDWORD addr, KMDWORD size)
{
    KMDWORD line;
    KMDWORD end = addr + size;

    for (line = addr; line <= end; line += CACHE_LINE) {
        kmiPurgeCache((void *)line);
    }
}

KMDWORD kmiReadRegister(KMDWORD reg)
{
    return *(volatile KMDWORD *)(REGISTERS + reg);
}

KMINT32 kmiCopyToPVRMemory(void *dst, void *src, KMDWORD size)
{
    kmiMemCopy4((char *)dst + VIEW_32BIT, src, size);
    return TRUE;
}

KMINT32 kmiCopyFromPVRMemory(void *dst, void *src, KMDWORD size)
{
    kmiMemCopy4(dst, (char *)src + VIEW_32BIT, size);
    return TRUE;
}

KMINT32 kmiCopyToFrameBuffer(KMDWORD offset, KMDWORD *src, KMDWORD size)
{
    KMDWORD *handle;
    KMDWORD dma_size;
    int mask;

    mask = get_imask();
    if (!(offset & (CACHE_LINE - 1)) && !((KMDWORD)src & (CACHE_LINE - 1)) &&
        mask == 0) {
        kmiPurgeMemory((KMDWORD)src, size);
        dma_size = size & ~(CACHE_LINE - 1);
        while (kmiDMAtoTARequest(offset + TA_VIEW_64BIT, src, dma_size,
                                 DMA_SECOND_QUEUE | DMA_SOURCE_UP,
                                 &handle) == DMA_QUEUE_FULL) {
        }
        /* @bug uninitialized: for a size below 32, dma_size is 0.
         * kmiDMAtoTARequest then finds the length 0 (TST R6,R6 at 0x8c663a52)
         * and returns DMA_EMPTY without setting handle. the loads at
         * 0x8c66649a and 0x8c66649e read the unset pointer and the word it
         * points to */
#ifdef GCC
        /* repairs the read of the unset handle that the tag above names. a
         * copy that makes no request leaves frame_dma as it was, the last DMA
         * transfer's handle */
        if (dma_size != 0) {
            kmiDeviceExtension.frame_dma = *handle;
        }
#else
        kmiDeviceExtension.frame_dma = *handle;
#endif
        if (size & (CACHE_LINE - 1)) {
            offset += dma_size;
            src = (KMDWORD *)((KMDWORD)src + dma_size);
            kmiMemCopy4((void *)(VIEW_64BIT + offset), src,
                        size & (CACHE_LINE - 1));
        }
        if (kmiDeviceExtension.frame_dma_wait == 1) {
            kmiWaitForFinishDMA(kmiDeviceExtension.frame_dma);
        }
    } else {
        kmiMemCopy4((void *)(VIEW_64BIT + offset), src, size);
    }
    return TRUE;
}

KMINT32 kmiCopyFromFrameBuffer(KMDWORD *dst, KMDWORD offset, KMDWORD size)
{
    kmiMemCopy4(dst, (void *)(VIEW_64BIT + offset), size);
    kmiPurgeMemory((KMDWORD)dst, size);
    return TRUE;
}

void kmiWriteRegisters(KMDWORD count, struct register_setting *settings)
{
    KMDWORD i;

    for (i = 0; i < count; i++) {
        kmiWriteRegister(settings[i].reg, settings[i].value);
    }
}

KMINT32 kmiCopyTextureToTexture(void *dst, void *src, KMDWORD size)
{
    return TRUE;
}

void kmiRegisterPhIntHandler(KMINT32 (*handler)(void))
{
    int mask = get_imask();

    set_imask(IMASK_ALL);
    *(KMINT32(**)(void))((char *)get_vbr() + LEVEL6_HANDLER) = handler;
    INT_LEVEL6 = 0;
    INT_LEVEL6_ERROR = 0;
    INT_STATUS = 0xffffffff;
    INT_STATUS_ERROR = 0xffffffff;
    INT_LEVEL6 = INT_RENDER_END_TSP | INT_VBLANK_IN | INT_HBLANK_IN |
                 INT_YUV_END | INT_OPAQUE_END | INT_OPAQUE_MODIFIER_END |
                 INT_TRANS_END | INT_TRANS_MODIFIER_END | INT_DMA2_END |
                 INT_PUNCHTHROUGH_END;
    INT_LEVEL6_ERROR = INT_STRIP_OVERRUN | INT_ISP_OVERFLOW | INT_OL_OVERFLOW;
    set_imask(mask);
}
