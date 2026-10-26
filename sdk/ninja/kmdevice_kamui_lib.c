/* Kamui display setup: the device, display mode, filters, blanking and
 * picture position */
#include <kamui.h>
#include <sg_xpt.h>

/* the video chip's registers, as offsets into its register block */
#define FB_READ_CONTROL 0x044
#define FB_WRITE_CONTROL 0x048
#define FB_WRITE_STRIDE 0x04c
#define FB_READ_SIZE 0x05c
#define FB_CLIP_X 0x068
#define FB_CLIP_Y 0x06c
#define VIDEO_CONTROL 0x0e8
#define VIDEO_START_X 0x0ec
#define VIDEO_START_Y 0x0f0
#define SCALER_CONTROL 0x0f4
#define Y_COEFFICIENTS 0x118

#define FB_READ_ON 1 /* FB_READ_CONTROL: the display reads the frame buffer */
#define VIDEO_BLANK 8 /* VIDEO_CONTROL: the picture is blanked */

/* the bits that hold a video output: all of a KMVIDEOMODE's, and a display
 * mode's lowest two */
#define VIDEO_OUTPUT 3

/* the display mode bits that send the picture as two fields, on any video
 * output but VGA */
#define TWO_FIELD_MODES                                                        \
    (KM_DSPMODE_INTERLACE | KM_DSPMODE_PSEUDONONINTERLACE |                    \
     KM_DSPMODE_HEIGHT_480)

/* write the display registers from Kamui's copies, with frame buffer reads off
 * and the picture blanked */
static void write_display_registers(void)
{
    kmiWriteRegister(FB_CLIP_X, kmiDeviceExtension.fb_clip_x);
    kmiWriteRegister(FB_CLIP_Y, kmiDeviceExtension.fb_clip_y);
    kmiWriteRegister(FB_READ_SIZE, kmiDeviceExtension.fb_read_size);
    kmiWriteRegister(FB_WRITE_STRIDE, kmiDeviceExtension.fb_write_stride);
    kmiWriteRegister(Y_COEFFICIENTS, kmiDeviceExtension.y_coefficients);
    kmiWriteRegister(SCALER_CONTROL, kmiDeviceExtension.scaler_control);
    kmiWriteRegister(FB_WRITE_CONTROL, kmiDeviceExtension.fb_write_control);
    kmiWriteRegister(FB_READ_CONTROL,
                     kmiDeviceExtension.fb_read_control & ~FB_READ_ON);
    kmiWriteRegister(VIDEO_CONTROL,
                     kmiDeviceExtension.video_control | VIDEO_BLANK);
}

void kmiWaitVlank(void)
{
    KMDWORD count;

    count = kmiDeviceExtension.vsync_count;
    while (count == kmiDeviceExtension.vsync_count) {
    }
}

KMSTATUS kmInitDevice(KMVIDEOMODE nVideoMode)
{
    kmiInitializeDriver();
    if (nVideoMode > KM_PAL || nVideoMode < KM_NTSC) {
        return KMSTATUS_INVALID_VIDEO_MODE;
    }
    kmiDeviceExtension.video_mode =
        kmiGetProfileInt("KAMUI", "VideoMode", nVideoMode & VIDEO_OUTPUT);
    kmiInitializeHardware();
    return KMSTATUS_SUCCESS;
}

KMSTATUS kmSetDisplayMode(KMDISPLAYMODE nDisplayMode, KMBPPMODE nBpp,
                          KMBOOLEAN fDither, KMBOOLEAN fAntiAlias)
{
    KMSTATUS status = KMSTATUS_SUCCESS;
    KMBOOLEAN anti_alias;
    KMBOOLEAN dither;
    KMBOOLEAN interlace;
    /* @hack type: width and height KMDWORD, though kmiFBControl takes them as
     * KMINT32, which keeps the image's registers and its pool's 480 ahead of
     * its 640 (0x8c661920); as KMINT32, LOADER.BIN's function changes size, and
     * 40 of 164 words differ in 1ST_READ.BIN */
    KMDWORD width;
    KMDWORD height;

    kmiDiscardRenderPipeline();
    nDisplayMode = kmiGetProfileInt("KAMUI", "DisplayMode", nDisplayMode);
    fAntiAlias = kmiGetProfileInt("KAMUI", "AntiAlias", fAntiAlias);
    fDither = kmiGetProfileInt("KAMUI", "Dither", fDither);
    kmiDeviceExtension.video_mode = nDisplayMode & VIDEO_OUTPUT;
    switch (nBpp) {
    case KM_DSPBPP_RGB565:
    case KM_DSPBPP_RGB555:
    case KM_DSPBPP_ARGB1555:
    case KM_DSPBPP_RGB888:
    case KM_DSPBPP_ARGB8888:
        break;
    default:
        status = KMSTATUS_INVALID_DISPLAY_MODE;
        break;
    }
    if (fAntiAlias) {
        anti_alias = TRUE;
    } else {
        anti_alias = FALSE;
    }
    if (fDither) {
        dither = TRUE;
    } else {
        dither = FALSE;
    }
    if (status == KMSTATUS_SUCCESS) {
        kmiDeviceExtension.display_mode = nDisplayMode;
        kmiDeviceExtension.bpp = nBpp;
        kmiDeviceExtension.dither = dither;
        kmiDeviceExtension.anti_alias = anti_alias;
        if (kmiDeviceExtension.display_mode == KM_DSPMODE_VGA) {
            width = 640;
            height = 480;
            interlace = FALSE;
            kmiDeviceExtension.picture_lines = height;
        } else {
            /* @hack redundant: a third condition, && 1, which SHC keeps as a
             * test of a register that holds 1 (TST R4,R4 at 0x8c661904 in
             * 1ST_READ.BIN, 0x8ce0f2da in LOADER.BIN); without it,
             * LOADER.BIN's function changes size, and 13 of 164 words differ
             * in 1ST_READ.BIN */
            if ((kmiDeviceExtension.display_mode & KM_VGA) != KM_VGA &&
                (kmiDeviceExtension.display_mode & TWO_FIELD_MODES) && 1) {
                interlace = TRUE;
            } else {
                interlace = FALSE;
            }
            if (kmiDeviceExtension.display_mode & KM_DSPMODE_WIDTH_640) {
                width = 640;
            } else {
                width = 320;
            }
            if (kmiDeviceExtension.display_mode & KM_DSPMODE_HEIGHT_480) {
                height = 480;
            } else {
                height = 240;
            }
            kmiDeviceExtension.picture_lines = 240;
        }
        switch (kmiDeviceExtension.video_mode) {
        case KM_VGA:
            status = KMSTATUS_SUCCESS;
            kmiInitVGA();
            break;
        case KM_NTSC:
            status = KMSTATUS_SUCCESS;
            kmiInitNTSC();
            break;
        case KM_PAL:
            status = KMSTATUS_SUCCESS;
            kmiInitPAL();
            break;
        default:
            status = KMSTATUS_INVALID_DISPLAY_MODE;
            break;
        }
        kmiFBControl(width, height, nBpp, interlace, dither, anti_alias);
        write_display_registers();
    }
    return status;
}

KMSTATUS kmBlankScreen(KMBOOLEAN bBlanking)
{
    if (bBlanking == TRUE) {
        kmiWriteRegister(FB_READ_CONTROL,
                         kmiDeviceExtension.fb_read_control & ~FB_READ_ON);
        kmiWriteRegister(VIDEO_CONTROL,
                         kmiDeviceExtension.video_control | VIDEO_BLANK);
    } else {
        kmiWriteRegister(FB_READ_CONTROL,
                         kmiDeviceExtension.fb_read_control & ~FB_READ_ON);
        kmiWriteRegister(VIDEO_CONTROL, kmiDeviceExtension.video_control);
    }
    kmiWriteRegister(FB_READ_CONTROL, kmiDeviceExtension.fb_read_control);
    return KMSTATUS_SUCCESS;
}

/* set the display mode's frame buffer up again, dithered and anti-aliased as
 * fDither and fAntiAlias say. the registers change after a vertical blank, and
 * the function returns after the next */
KMSTATUS kmChangeDisplayFilterMode(KMBOOLEAN fDither, KMBOOLEAN fAntiAlias)
{
    /* @hack redundant: a table of bits per pixel for each KMBPPMODE, which the
     * function fills on its stack and never reads; without it, count
     * unmeasured: the function changes size in both images */
    KMDWORD depth[6] = {16, 16, 16, 16, 24, 32};
    KMBOOLEAN anti_alias;
    KMBOOLEAN dither;
    KMBOOLEAN interlace;
    KMINT32 width;
    KMINT32 height;

    kmiDiscardRenderPipeline();
    if (fAntiAlias) {
        anti_alias = TRUE;
    } else {
        anti_alias = FALSE;
    }
    if (fDither) {
        dither = TRUE;
    } else {
        dither = FALSE;
    }
    kmiDeviceExtension.dither = dither;
    kmiDeviceExtension.anti_alias = anti_alias;
    if (kmiDeviceExtension.display_mode == KM_DSPMODE_VGA) {
        width = 640;
        height = 480;
        interlace = FALSE;
    } else {
        /* @hack redundant: a third condition, && 1, which SHC keeps as a test
         * of a register that holds 1 (TST R14,R14 at 0x8c661af2 in
         * 1ST_READ.BIN, TST R9,R9 at 0x8ce0f48e in LOADER.BIN); without it,
         * LOADER.BIN's function changes size, and 35 of 148 words differ in
         * 1ST_READ.BIN */
        if ((kmiDeviceExtension.display_mode & KM_VGA) != KM_VGA &&
            (kmiDeviceExtension.display_mode & TWO_FIELD_MODES) && 1) {
            interlace = TRUE;
        } else {
            interlace = FALSE;
        }
        if (kmiDeviceExtension.display_mode & KM_DSPMODE_WIDTH_640) {
            width = 640;
        } else {
            width = 320;
        }
        if (kmiDeviceExtension.display_mode & KM_DSPMODE_HEIGHT_480) {
            height = 480;
        } else {
            height = 240;
        }
    }
    kmiWaitVlank();
    kmiFBControl(width, height, kmiDeviceExtension.bpp, interlace, dither,
                 anti_alias);
    write_display_registers();
    kmiWriteRegister(FB_READ_CONTROL,
                     kmiDeviceExtension.fb_read_control & ~FB_READ_ON);
    kmiWriteRegister(VIDEO_CONTROL, kmiDeviceExtension.video_control);
    kmiWriteRegister(FB_READ_CONTROL, kmiDeviceExtension.fb_read_control);
    kmiResetBoard();
    kmiSetTAEnvironment(kmiDeviceExtension.ta_layout[0].region_array >> 7,
                        kmiDeviceExtension.ta_layout[0].object_list >> 7,
                        kmiDeviceExtension.ta_layout[0].isp_params >> 7,
                        kmiDeviceExtension.ta_layout[0].tsp_params >> 7,
                        kmiDeviceExtension.ta_options, 0);
    kmiDeviceExtension.display_changed = TRUE;
    kmiWaitVlank();
    return KMSTATUS_SUCCESS;
}

KMSTATUS kmStopDisplayFrameBuffer(void)
{
    kmiWriteRegister(FB_READ_CONTROL,
                     kmiDeviceExtension.fb_read_control & ~FB_READ_ON);
    kmiWriteRegister(VIDEO_CONTROL,
                     kmiDeviceExtension.video_control | VIDEO_BLANK);
    kmiDiscardRenderPipeline();
    kmiResetBoard();
    kmiWriteRegister(FB_READ_CONTROL,
                     kmiDeviceExtension.fb_read_control & ~FB_READ_ON);
    kmiWriteRegister(VIDEO_CONTROL,
                     kmiDeviceExtension.video_control | VIDEO_BLANK);
    kmiWaitVlank();
    kmiDeviceExtension.display_changed = TRUE;
    return KMSTATUS_SUCCESS;
}

/* @hack type: nXAdjust and nYAdjust long, not KMINT32, which is int, here and
 * in kamui.h's declaration. SHC then loads the first test's 10 and origin_x's
 * address among the register saves (0x8c661ca2 to 0x8c661cb4); as KMINT32 in
 * both, 9 of 137 words differ in 1ST_READ.BIN and 9 of 133 in LOADER.BIN */
KMSTATUS kmAdjustDisplayCenter(long nXAdjust, long nYAdjust)
{
    KMSTATUS status = KMSTATUS_SUCCESS;
    KMDWORD size;
    KMDWORD lines;

    if (kmiDeviceExtension.origin_x + nXAdjust > 10) {
        if (kmiDeviceExtension.offset_x != nXAdjust) {
            kmiDeviceExtension.offset_x = nXAdjust;
            kmiDeviceExtension.video_start_x =
                kmiDeviceExtension.origin_x + kmiDeviceExtension.offset_x;
            kmiWriteRegister(VIDEO_START_X, kmiDeviceExtension.video_start_x);
        }
    } else {
        /* @hack redundant: an else that stores the KMSTATUS_SUCCESS status
         * already holds, which keeps the image's branches; without it, 92 of
         * 137 words differ in 1ST_READ.BIN and 20 of 133 in LOADER.BIN */
        status = KMSTATUS_SUCCESS;
    }
    if (kmiDeviceExtension.origin_y1 + nYAdjust > 0 ||
        kmiDeviceExtension.origin_y2 + nYAdjust > 0) {
        if (kmiDeviceExtension.offset_y != nYAdjust) {
            kmiDeviceExtension.offset_y = nYAdjust;
            kmiDeviceExtension.video_start_y =
                (kmiDeviceExtension.origin_y1 + kmiDeviceExtension.offset_y) |
                ((kmiDeviceExtension.origin_y2 + kmiDeviceExtension.offset_y)
                 << 16);
            kmiWriteRegister(VIDEO_START_Y, kmiDeviceExtension.video_start_y);
            if (nYAdjust > 0) {
                size = kmiDeviceExtension.fb_read_size &
                       ~(READ_LINES << READ_LINES_SHIFT);
                lines = kmiDeviceExtension.fb_lines;
                if (kmiDeviceExtension.skip_lines <
                    kmiDeviceExtension.offset_y) {
                    lines -= kmiDeviceExtension.offset_y;
                } else {
                    lines -= kmiDeviceExtension.skip_lines;
                }
                if (kmiDeviceExtension.offset_y + lines >
                    kmiDeviceExtension.fb_lines) {
                    lines = kmiDeviceExtension.fb_lines -
                            kmiDeviceExtension.skip_lines;
                }
                if (kmiDeviceExtension.fb_lines < lines) {
                    kmiDeviceExtension.fb_lines = lines;
                }
                kmiDeviceExtension.fb_read_size =
                    ((lines & READ_LINES) << READ_LINES_SHIFT) | size;
                /* frame buffer reads are stopped while the read size changes */
                kmiWriteRegister(FB_READ_CONTROL,
                                 kmiDeviceExtension.fb_read_control &
                                     ~FB_READ_ON);
                kmiWriteRegister(FB_READ_SIZE, kmiDeviceExtension.fb_read_size);
                kmiWriteRegister(FB_READ_CONTROL,
                                 kmiDeviceExtension.fb_read_control);
            }
        }
    } else {
        /* @hack redundant: an else that stores the KMSTATUS_SUCCESS status
         * already holds, which keeps the image's branches; without it, count
         * unmeasured: the function changes size in both images */
        status = KMSTATUS_SUCCESS;
    }
    return status;
}
