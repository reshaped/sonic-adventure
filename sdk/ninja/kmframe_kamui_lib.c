/* Kamui frame buffer surfaces and display size */
#include <kamui.h>
#include <sg_xpt.h>

/* the kmiSystemConfig request kmCreateFrameBufferSurface fills next, 0 or 1 */
static KMINT32 next_request = 0;

KMSTATUS kmCreateFrameBufferSurface(PKMSURFACEDESC pSurfaceDesc, KMINT32 nWidth,
                                    KMINT32 nHeight, KMBOOLEAN fStripBuffer,
                                    KMBOOLEAN fBufferClear)
{
    KMDWORD zero[640];
    KMDWORD bit_depth[6] = {KMI_BITDEPTH_16, KMI_BITDEPTH_16, KMI_BITDEPTH_16,
                            KMI_BITDEPTH_16, KMI_BITDEPTH_24, KMI_BITDEPTH_32};
    KMSTATUS status;
    KMDWORD i;

    kmiSystemConfig.request[next_request].surface = pSurfaceDesc;
    kmiSystemConfig.request[next_request].width = nWidth;
    kmiSystemConfig.request[next_request].height = nHeight;
    kmiSystemConfig.strip_buffer = fStripBuffer;
    kmiSystemConfig.request[next_request].clear = fBufferClear;
    next_request = (next_request + 1) & 1;
    kmiDeviceExtension.strip_buffer = fStripBuffer;
    if (kmiSystemConfig.buffer_mode) {
        kmiGetDisplayData(DISPLAY_BPP, fStripBuffer, nHeight);
    }
    if (kmiGetSysConfigFlag() == KMI_CONFIGURED) {
        status = kmiFBalloc(kmiSystemConfig.frame_buffer_size, pSurfaceDesc);
        if (status) {
            return status;
        }
        pSurfaceDesc->SurfaceType = KM_SURFACETYPE_FRAMEBUFFER;
        pSurfaceDesc->BitDepth = bit_depth[kmiDeviceExtension.bpp];
        pSurfaceDesc->PixelFormat = kmiDeviceExtension.bpp;
        pSurfaceDesc->u0.Width = kmiSystemConfig.width;
        pSurfaceDesc->u1.Height = kmiSystemConfig.height;
        if (fStripBuffer == TRUE) {
            pSurfaceDesc->fSurfaceFlags = 1;
        } else {
            pSurfaceDesc->fSurfaceFlags = 0;
        }
        if (fBufferClear == TRUE) {
            /* @hack spelling: i = i + 1, not i++, which keeps R5 for
             * KM_SURFACETYPE_FRAMEBUFFER, stored at 0x8c671b9a, and R11 and R10
             * for zero's address and kmiCopyToPVRMemory's (0x8c671c44,
             * 0x8c671c50); with i++, 18 of 165 words differ */
            for (i = 0; i < 640; i = i + 1) {
                zero[i] = 0;
            }
            for (i = 0; i < pSurfaceDesc->uSize.dwFrameBufferSize;
                 i += sizeof(zero)) {
                /* @hack redundant: the call in a loop of one pass, which keeps
                 * the loads of sizeof(zero) and kmiCopyToPVRMemory's address
                 * after the loop's first test (0x8c671c4e, 0x8c671c50);
                 * without it, they come before, and 6 of 165 words differ */
                do {
                    /* @hack spelling: zero passed as &zero[0], which, as the
                     * loop's i = i + 1 does, keeps R5 for
                     * KM_SURFACETYPE_FRAMEBUFFER, stored at 0x8c671b9a, and R11
                     * and R10 for the addresses of zero and kmiCopyToPVRMemory;
                     * as zero, 21 of 165 words differ */
                    kmiCopyToPVRMemory((char *)pSurfaceDesc->pSurface + i,
                                       &zero[0], sizeof(zero));
                } while (0);
            }
        }
    } else {
        kmiAddSysConfigFlag(KMI_CONFIG_FRAME_BUFFER);
    }
    return KMSTATUS_SUCCESS;
}

void kmiGetDisplayData(KMDWORD bpp, KMDWORD strip_buffer, KMDWORD height)
{
    KMDWORD pixel_bits[6] = {16, 16, 16, 16, 24, 32};
    KMINT32 width;

    if (kmiDeviceExtension.video_mode == KM_VGA ||
        (kmiDeviceExtension.display_mode & KM_DSPMODE_WIDTH_640)) {
        width = 640;
    } else {
        width = 320;
    }
    if (strip_buffer == TRUE) {
        height = (height + 31) & ~31;
        kmiDeviceExtension.fb_read_control =
            (kmiDeviceExtension.fb_read_control & 0xffa0ffff) |
            (strip_buffer << 22) | (((height >> 5) & 0x1f) << 16);
    } else if (kmiDeviceExtension.video_mode == KM_VGA) {
        height = 480;
    } else if (FLICKER_FREE) {
        height = 240;
    } else if (kmiDeviceExtension.display_mode & KM_DSPMODE_HEIGHT_480) {
        height = 480;
    } else {
        height = 240;
    }
    kmiSystemConfig.width = width;
    kmiSystemConfig.height = height;
    if (bpp != DISPLAY_BPP) {
        kmiSystemConfig.bytes_per_pixel = pixel_bits[bpp] >> 3;
    } else {
        kmiSystemConfig.bytes_per_pixel =
            pixel_bits[kmiDeviceExtension.bpp] >> 3;
    }
    kmiSystemConfig.frame_buffer_size = kmiSystemConfig.width *
                                        kmiSystemConfig.height *
                                        kmiSystemConfig.bytes_per_pixel;
}
