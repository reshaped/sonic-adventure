/* SDK 1.20J Kamui frame buffer surfaces and display size. its C is 1.30J's,
 * with the hacks R10 needs in place of R32's */
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
    KMDWORD *source;

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
            for (i = 0; i < 640; i++) {
                zero[i] = 0;
            }
            /* @hack spelling: source set in the loop's first clause, beside i,
             * which gives source R10 and kmiCopyToPVRMemory's address R11
             * (0x8ce1b68c to 0x8ce1b696); set before the loop, the two swap
             * registers and 6 of 178 words differ, and passing zero itself,
             * 14 */
            for (i = 0, source = zero;
                 i < pSurfaceDesc->uSize.dwFrameBufferSize; i += sizeof(zero)) {
                kmiCopyToPVRMemory((char *)pSurfaceDesc->pSurface + i, source,
                                   sizeof(zero));
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
    /* @hack redundant: a never-run read of the table into width, which gives
     * the table R7 (MOV R15,R7 at 0x8ce1b706) and width R14 (0x8ce1b71e,
     * 0x8ce1b724); without it, the two swap registers and 5 of 130 words
     * differ */
    if (0) {
        width = pixel_bits[0];
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
