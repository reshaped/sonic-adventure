/* Kamui's frame buffer and video signal setup for a display mode. kmiFBControl
 * records the frame buffer's register values without writing them. kmiInitPAL,
 * kmiInitNTSC and kmiInitVGA record a signal's register values and write
 * them */
#include <kamui.h>
#include <sg_xpt.h>

/* the video chip's registers, as the offsets kmiReadRegister and
 * kmiVerifyWriteRegister take */
#define FB_READ_CONTROL 0x044
#define FB_READ_SIZE 0x05c
#define HBLANK_INTERRUPT 0x0c8
#define VBLANK_INTERRUPT 0x0cc
#define SYNC_CONTROL 0x0d0
#define HBLANK 0x0d4
#define SYNC_LOAD 0x0d8
#define VBLANK 0x0dc
#define SYNC_WIDTH 0x0e0
#define VIDEO_CONTROL 0x0e8
#define VIDEO_START_X 0x0ec
#define VIDEO_START_Y 0x0f0
#define SYNC_STATUS 0x10c

/* FB_READ_CONTROL's bit for VGA's full-rate pixel clock */
#define FB_READ_FULL_CLOCK 0x00800000
/* VIDEO_CONTROL's bit that blanks the picture */
#define VIDEO_BLANK 8
/* SYNC_STATUS's field for the scan line the signal is on */
#define SCAN_LINE 0x3ff

/* the frame buffer's pixel formats: a WRITE_ code as the renderer writes the
 * pixels, and a READ_ code as the display reads them */
#define WRITE_RGB555 0
#define WRITE_RGB565 1
#define WRITE_ARGB1555 3
#define WRITE_RGB888 4
#define WRITE_ARGB8888 6
#define READ_RGB555 0
#define READ_RGB565 1
#define READ_RGB888 2
#define READ_RGB0888 3

/* a tile's width and height in pixels, and the most tiles a frame holds from
 * top to bottom */
#define TILE_SIZE 32
#define MOST_TILES_Y 15

/* a frame buffer's setup, field by field as store_frame packs it into
 * kmiDeviceExtension's register values */
struct frame_setup {
    /* the 4-byte words of each line the display reads, less one */
    KMINT32 width;
    KMINT32 lines; /* the lines the display reads, less one */
    /* the words the display skips between one read line and the next, plus
     * one */
    KMINT32 modulus;
    KMINT32 hscale; /* 1 has the scaler halve each line */
    /* the scaler's vertical factor: 0x400 keeps the lines one to one */
    KMINT32 vscale;
    /* the flicker filter's weights */
    KMINT32 filter0;
    KMINT32 filter1;
    KMINT32 scaler_field;
    KMINT32 enable;
    KMINT32 interlace;
    KMINT32 repeat_lines;
    KMINT32 dither;
    KMINT32 write_format;
    KMINT32 pad_value;
    KMINT32 alpha_cutoff;
    KMINT32 depth;
    KMINT32 concat;
    KMINT32 stride; /* a written line's length, in 8-byte units */
    KMINT32 unk_48;
    KMINT32 unk_4c;
    KMINT32 unk_50;
    KMINT32 clip_x_max;
    KMINT32 clip_x_min;
    KMINT32 clip_y_max;
    KMINT32 clip_y_min;
    KMINT32 strip_buffer;
    KMINT32 strip_lines;
    KMINT32 full_clock; /* the full-rate pixel clock, for VGA */
    KMINT32 key_threshold;
};

/* a video signal's setup, field by field as write_signal packs it into
 * kmiDeviceExtension's register values. its timing counts pixel clocks along
 * a line and lines down the screen */
struct signal_setup {
    KMINT32 hblank_end;
    KMINT32 hblank_start;
    KMINT32 line_clocks; /* a line's pixel clocks, less one */
    KMINT32 vblank_end;
    KMINT32 vblank_start;
    /* the lines of a frame of two fields, or else of one field, less one */
    KMINT32 lines;
    KMINT32 hsync_width;
    KMINT32 vsync_width;
    KMINT32 broad_width;
    KMINT32 equalize_width;
    KMINT32 ntsc;
    KMINT32 pal;
    KMINT32 interlace;
    KMINT32 hsync_polarity;
    KMINT32 vsync_polarity;
    KMINT32 blank_polarity;
    KMINT32 blank;
    KMINT32 field_source;
    KMINT32 drive_sync;
    KMINT32 composite_on_hsync;
    KMINT32 master_hsync;
    KMINT32 master_vsync;
    KMINT32 unk_58;
    KMINT32 master_csync;
    KMINT32 second_field_only;
    KMINT32 unk_64;
    KMINT32 sync_lock;
    KMINT32 repeat_pixels; /* 1 shows each pixel twice, for 320-pixel lines */
    KMINT32 pixel_delay;
    KMINT32 vblank_in_line;
    KMINT32 vblank_out_line;
    KMINT32 unk_7c[29];
    /* where the picture starts: origin_y1 and origin_y2 give its first line
     * in each field, and origin_x its first clock on each line */
    KMINT32 origin_y1;
    KMINT32 origin_y2;
    KMINT32 origin_x;
};

void kmiDACControl(KMINT32 level, KMINT32 green_sync, KMINT32 power_down);

static void store_frame(struct frame_setup setup)
{
    kmiDeviceExtension.fb_clip_x =
        (setup.clip_x_min & 0x7ff) | ((setup.clip_x_max & 0x7ff) << 16);
    kmiDeviceExtension.fb_clip_y =
        (setup.clip_y_min & 0x7ff) | ((setup.clip_y_max & 0x7ff) << 16);
    kmiDeviceExtension.unk_2b4 = (setup.unk_48 & 15) |
                                 ((setup.unk_4c & 31) << 4) |
                                 ((setup.unk_50 & 15) << 9);
    kmiDeviceExtension.fb_width = setup.width;
    kmiDeviceExtension.fb_lines = setup.lines;
    if (kmiDeviceExtension.offset_y > 0) {
        kmiDeviceExtension.fb_read_size =
            (setup.width & 0x3ff) |
            (((setup.lines - kmiDeviceExtension.offset_y) & READ_LINES)
             << READ_LINES_SHIFT) |
            ((setup.modulus & 0x3ff) << 20);
    } else {
        kmiDeviceExtension.fb_read_size =
            (setup.width & 0x3ff) |
            ((setup.lines & READ_LINES) << READ_LINES_SHIFT) |
            ((setup.modulus & 0x3ff) << 20);
    }
    kmiDeviceExtension.y_coefficients =
        (setup.filter0 & 255) | ((setup.filter1 & 255) << 8);
    kmiDeviceExtension.scaler_control = (setup.vscale & 0xffff) |
                                        ((setup.hscale & 1) << 16) |
                                        ((setup.scaler_field & 1) << 18);
    kmiDeviceExtension.fb_write_stride = setup.stride & 0x1ff;
    kmiDeviceExtension.fb_write_control =
        (setup.write_format & 7) | ((setup.dither & 1) << 3) |
        ((setup.pad_value & 255) << 8) | ((setup.alpha_cutoff & 255) << 16);
    kmiDeviceExtension.fb_read_control =
        (setup.enable & 1) | ((setup.repeat_lines & 1) << 1) |
        ((setup.depth & 3) << 2) | ((setup.concat & 7) << 4) |
        ((setup.key_threshold & 255) << 8) | ((setup.strip_lines & 31) << 16) |
        ((setup.strip_buffer & 1) << 22) | ((setup.full_clock & 1) << 23);
}

KMINT32 kmiFBControl(KMINT32 width, KMINT32 height, KMBPPMODE bpp,
                     KMINT32 interlace, KMBOOLEAN dither, KMBOOLEAN anti_alias)
{
    struct frame_setup setup;
    const KMINT32 pixel_bits[6] = {16, 16, 16, 16, 24, 32};
    KMINT32 bits = pixel_bits[bpp];
    KMINT32 fields;
    KMINT32 extra_tiles;
    KMINT32 scale;
    KMINT32 line_bytes;

    if (anti_alias && height == 240) {
        height <<= 1;
    }
    kmiDeviceExtension.display_width = width;
    kmiDeviceExtension.display_height = height;
    setup.enable = 1;
    setup.alpha_cutoff = 0;
    setup.pad_value = 0;
    setup.concat = 0;
    setup.key_threshold = 0;
    setup.strip_lines = 0;
    setup.strip_buffer = 0;
    if (kmiDeviceExtension.display_mode & KM_VGA) {
        setup.full_clock = 1;
        fields = 1;
        extra_tiles = 0;
    } else {
        setup.full_clock = 0;
        if (interlace == 1) {
            fields = 2;
            extra_tiles = 0;
        } else {
            fields = 1;
            extra_tiles = 1;
        }
    }
    /* @hack redundant: a never-run store of extra_tiles to setup.width, which
     * keeps extra_tiles in R8 under R32. LOADER.BIN, which R10 builds, doesn't
     * change; without it, 53 of 428 words differ in 1ST_READ.BIN */
    if (0) {
        setup.width = extra_tiles;
    }
    switch (bits) {
    case 16:
        /* bpp 2, which pixel_bits counts as 16 bits but KMBPPMODE doesn't
         * name, gets RGB565 from the final else. kmSetDisplayMode refuses
         * it */
        if (bpp == KM_DSPBPP_ARGB1555) {
            setup.write_format = WRITE_ARGB1555;
            setup.depth = READ_RGB555;
        } else if (bpp == KM_DSPBPP_RGB565) {
            setup.write_format = WRITE_RGB565;
            setup.depth = READ_RGB565;
        } else if (bpp == KM_DSPBPP_RGB555) {
            setup.write_format = WRITE_RGB555;
            setup.depth = READ_RGB555;
        } else {
            setup.write_format = WRITE_RGB565;
            setup.depth = READ_RGB565;
        }
        setup.dither = dither;
        break;
    case 24:
        setup.write_format = WRITE_RGB888;
        setup.depth = READ_RGB888;
        setup.dither = 0;
        break;
    case 32:
        setup.write_format = WRITE_ARGB8888;
        setup.depth = READ_RGB0888;
        setup.dither = 0;
        break;
    }
    setup.repeat_lines = 0;
    setup.interlace = interlace;
    setup.scaler_field = 0;
    setup.stride = width * bits / 64;
    setup.width = (width * bits / 32) - 1;
    line_bytes = width * (bits / 8);
    if (interlace == 1) {
        if (height == 480) {
            setup.lines = 240 - 1;
            setup.modulus = (width * bits / 32) + 1;
            /* @hack redundant: a test of KM_PAL whose two arms store the
             * same. SHC keeps the test, TST #2,R0 at 0x8c664618 and at
             * 0x8ce1d992 in LOADER.BIN, and no branch reads it. without it,
             * the section runs 32 bytes short in 1ST_READ.BIN and 8 in
             * LOADER.BIN, and 281 of 428 words differ in 1ST_READ.BIN and 260
             * of 399 in LOADER.BIN */
            if (kmiDeviceExtension.display_mode & KM_PAL) {
                kmiDeviceExtension.field_offset[0] = 0;
                kmiDeviceExtension.field_offset[1] = line_bytes;
            } else {
                kmiDeviceExtension.field_offset[0] = 0;
                kmiDeviceExtension.field_offset[1] = line_bytes;
            }
        } else {
            setup.lines = height - 1;
            kmiDeviceExtension.field_offset[0] = 0;
            kmiDeviceExtension.field_offset[1] = line_bytes;
            setup.modulus = 1;
        }
    } else if (kmiDeviceExtension.display_mode & KM_VGA) {
        setup.lines = height - 1;
        kmiDeviceExtension.field_offset[0] = 0;
        kmiDeviceExtension.field_offset[1] = line_bytes;
        setup.modulus = 1;
    } else if (height == 480) {
        setup.lines = 240 - 1;
        kmiDeviceExtension.field_offset[0] = 0;
        kmiDeviceExtension.field_offset[1] = line_bytes;
        setup.modulus = (width * bits / 32) + 1;
    } else {
        setup.lines = height - 1;
        kmiDeviceExtension.field_offset[0] = 0;
        kmiDeviceExtension.field_offset[1] = line_bytes;
        setup.modulus = 1;
    }
    if (anti_alias) {
        kmiDeviceExtension.ta_options |= OPTION_BY_COLUMNS;
        setup.hscale = 1;
        scale = 2;
        if (kmiDeviceExtension.display_mode & KM_VGA) {
            setup.vscale = 0x401;
            setup.lines -= 2;
        } else {
            /* @bug uninitialized: a display mode whose bits other than KM_VGA,
             * KM_PAL and KM_DSPMODE_WIDTH_640 match no case leaves
             * setup.vscale unset. the BRA at 0x8c664730 skips every store to
             * it. the copy of setup at 0x8c66492c passes the unset word on,
             * and store_frame reads it at 0x8c6643e4. LOADER.BIN does the same
             * at 0x8ce1da16, 0x8ce1db96 and 0x8ce1d770 */
            switch (kmiDeviceExtension.display_mode &
                    ~(KM_VGA | KM_PAL | KM_DSPMODE_WIDTH_640)) {
            case KM_DSPMODE_INTERLACE | KM_DSPMODE_HEIGHT_480:
            case KM_DSPMODE_PSEUDONONINTERLACE | KM_DSPMODE_HEIGHT_480:
                setup.vscale = 0x401;
                setup.modulus = (width * bits / 32) + 1;
                setup.lines -= 2;
                break;
            case 0:
            case KM_DSPMODE_INTERLACE:
            case KM_DSPMODE_HEIGHT_480:
                kmiDeviceExtension.field_offset[0] = 0;
                kmiDeviceExtension.field_offset[1] = 0;
                setup.vscale = 0x800;
                setup.modulus = 1;
                setup.lines -= 2;
                break;
            }
        }
    } else {
        setup.hscale = 0;
        if (FLICKER_FREE) {
            setup.vscale = 0x800;
            setup.lines = 238;
            kmiDeviceExtension.field_offset[0] = 0;
            kmiDeviceExtension.field_offset[1] = 0;
            setup.modulus = 1;
        } else if (kmiDeviceExtension.display_mode & KM_VGA) {
            setup.vscale = 0x400;
        } else if (kmiDeviceExtension.display_mode & KM_DSPMODE_HEIGHT_480) {
            setup.vscale = 0x401;
            setup.lines -= 2;
        } else {
            setup.vscale = 0x400;
            setup.lines -= 1;
        }
        scale = 1;
    }
    setup.filter0 = 64;
    setup.filter1 = 128;
    kmiDeviceExtension.tiles_x = width / TILE_SIZE * scale;
    kmiDeviceExtension.tiles_y = (height * fields / TILE_SIZE) + extra_tiles;
    if (kmiDeviceExtension.tiles_y > MOST_TILES_Y) {
        kmiDeviceExtension.tiles_y = MOST_TILES_Y;
    }
    setup.unk_4c = 9;
    setup.unk_48 = 6;
    if (bits == 16) {
        setup.unk_50 = 8;
    } else {
        setup.unk_50 = 11;
    }
    setup.clip_x_min = 0;
    setup.clip_x_max = width - 1;
    setup.clip_y_min = 0;
    if (FLICKER_FREE) {
        setup.clip_y_max = (height / 2) - 1;
    } else {
        setup.clip_y_max = height - 1;
    }
    store_frame(setup);
    kmiDeviceExtension.skip_lines = 0;
    kmiDeviceExtension.level_adjust = KM_LEVEL_ADJUST_NORMAL;
    return 0;
}

void kmiDACControl(KMINT32 level, KMINT32 green_sync, KMINT32 power_down)
{
}

/* record setup's register values in kmiDeviceExtension and write them to the
 * chip. the picture is left blanked, and the frame buffer unread */
static void write_signal(struct signal_setup setup)
{
    kmiDeviceExtension.origin_x = setup.origin_x;
    kmiDeviceExtension.origin_y1 = setup.origin_y1;
    kmiDeviceExtension.origin_y2 = setup.origin_y2;
    kmiDeviceExtension.sync_control =
        setup.master_hsync | (setup.master_vsync << 1) |
        (setup.master_csync << 2) | (setup.sync_lock << 3) |
        (setup.interlace << 4) | (setup.second_field_only << 5) |
        (setup.ntsc << 6) | (setup.pal << 7) | (setup.drive_sync << 8) |
        (setup.composite_on_hsync << 9);
    kmiDeviceExtension.hblank =
        (setup.hblank_start & 0x7ff) | ((setup.hblank_end & 0x7ff) << 16);
    kmiDeviceExtension.hblank_interrupt = (setup.hblank_start & 0x7ff) << 16;
    kmiDeviceExtension.sync_load =
        (setup.line_clocks & 0x7ff) | ((setup.lines & 0x7ff) << 16);
    kmiDeviceExtension.vblank =
        (setup.vblank_start & 0x7ff) | ((setup.vblank_end & 0x7ff) << 16);
    kmiDeviceExtension.vblank_interrupt =
        (setup.vblank_in_line & 0x7ff) |
        ((setup.vblank_out_line & 0x7ff) << 16);
    kmiDeviceExtension.sync_width = (setup.hsync_width & 127) |
                                    ((setup.vsync_width & 15) << 8) |
                                    ((setup.broad_width & 0x3ff) << 12) |
                                    ((setup.equalize_width & 127) << 22);
    kmiDeviceExtension.video_control =
        (setup.hsync_polarity & 1) | ((setup.vsync_polarity & 1) << 1) |
        ((setup.blank_polarity & 1) << 2) | ((setup.blank & 1) << 3) |
        ((setup.field_source & 15) << 4) | ((setup.repeat_pixels & 1) << 8) |
        ((setup.pixel_delay & 63) << 16);
    kmiDeviceExtension.video_start_x =
        kmiDeviceExtension.origin_x + kmiDeviceExtension.offset_x;
    kmiDeviceExtension.video_start_y =
        (kmiDeviceExtension.origin_y1 + kmiDeviceExtension.offset_y) |
        ((kmiDeviceExtension.origin_y2 + kmiDeviceExtension.offset_y) << 16);
    /* a chip with KMI_CAP_SYNC_STATUS reports the scan line in SYNC_STATUS. on
     * such a chip, wait for the line to leave 0 and come back to it */
    if (kmiDeviceExtension.capabilities & KMI_CAP_SYNC_STATUS) {
        while (!(kmiReadRegister(SYNC_STATUS) & SCAN_LINE)) {
        }
        while (kmiReadRegister(SYNC_STATUS) & SCAN_LINE) {
        }
    }
    kmiVerifyWriteRegister(VIDEO_CONTROL,
                           kmiDeviceExtension.video_control | VIDEO_BLANK);
    if (kmiDeviceExtension.video_mode & KM_VGA) {
        kmiVerifyWriteRegister(FB_READ_CONTROL, FB_READ_FULL_CLOCK);
    } else {
        kmiVerifyWriteRegister(FB_READ_CONTROL, 0);
    }
    kmiVerifyWriteRegister(FB_READ_SIZE, 0);
    kmiVerifyWriteRegister(SYNC_CONTROL, kmiDeviceExtension.sync_control);
    kmiVerifyWriteRegister(SYNC_LOAD, kmiDeviceExtension.sync_load);
    kmiVerifyWriteRegister(VBLANK_INTERRUPT,
                           kmiDeviceExtension.vblank_interrupt);
    kmiVerifyWriteRegister(HBLANK, kmiDeviceExtension.hblank);
    kmiVerifyWriteRegister(VBLANK, kmiDeviceExtension.vblank);
    kmiVerifyWriteRegister(SYNC_WIDTH, kmiDeviceExtension.sync_width);
    kmiVerifyWriteRegister(VIDEO_START_X, kmiDeviceExtension.video_start_x);
    kmiVerifyWriteRegister(VIDEO_START_Y, kmiDeviceExtension.video_start_y);
    kmiVerifyWriteRegister(HBLANK_INTERRUPT,
                           kmiDeviceExtension.hblank_interrupt);
}

void kmiInitPAL(void)
{
    struct signal_setup setup;

    setup.line_clocks = 864 - 1;
    if (TWO_FIELDS) {
        setup.lines = 625 - 1;
    } else {
        setup.lines = 313 - 1;
    }
    setup.hblank_end = 141;
    setup.hblank_start = 843;
    setup.vblank_end = 44;
    setup.vblank_start = 620;
    setup.hsync_width = 63;
    setup.vsync_width = 5;
    setup.vblank_in_line = 310;
    setup.vblank_out_line = 21;
    if (TWO_FIELDS) {
        setup.broad_width = 362;
    } else {
        setup.broad_width = 799;
    }
    if (TWO_FIELDS) {
        setup.interlace = 1;
    } else {
        setup.interlace = 0;
    }
    setup.hsync_polarity = 0;
    setup.vsync_polarity = 0;
    setup.blank_polarity = 0;
    setup.blank = 0;
    if (TWO_FIELDS) {
        setup.field_source = 0;
    } else {
        setup.field_source = 2;
    }
    if (kmiDeviceExtension.core == 6) {
        setup.pixel_delay = 22;
    } else {
        setup.pixel_delay = 63;
    }
    if (kmiDeviceExtension.display_mode & KM_DSPMODE_WIDTH_640) {
        setup.repeat_pixels = 0;
    } else {
        setup.repeat_pixels = 1;
    }
    setup.equalize_width = 31;
    setup.ntsc = 0;
    setup.pal = 1;
    setup.drive_sync = 1;
    setup.composite_on_hsync = 0;
    setup.master_hsync = 0;
    setup.master_vsync = 0;
    setup.unk_58 = 0;
    setup.master_csync = 0;
    setup.sync_lock = 0;
    setup.second_field_only = 0;
    setup.unk_64 = 240;
    setup.origin_x = 174;
    if (TWO_FIELDS) {
        setup.origin_y1 = 45;
        setup.origin_y2 = 46;
    } else {
        setup.origin_y1 = 46;
        setup.origin_y2 = 46;
    }
    kmiDeviceExtension.picture_lines = 240;
    kmiDeviceExtension.picture_end =
        (setup.vblank_end / 2) + kmiDeviceExtension.picture_lines;
    kmiDeviceExtension.picture_start = setup.vblank_end;
    write_signal(setup);
}

void kmiInitNTSC(void)
{
    struct signal_setup setup;

    setup.line_clocks = 858 - 1;
    if (TWO_FIELDS) {
        setup.lines = 525 - 1;
    } else {
        setup.lines = 263 - 1;
    }
    setup.hblank_end = 126;
    setup.hblank_start = 837;
    if (TWO_FIELDS) {
        setup.vblank_end = 36;
        setup.vblank_start = 516;
    } else {
        setup.vblank_end = 18;
        setup.vblank_start = 258;
    }
    setup.hsync_width = 63;
    if (kmiDeviceExtension.display_mode & KM_DSPMODE_HEIGHT_480) {
        setup.vblank_in_line = 260;
        setup.vblank_out_line = 21;
    } else {
        setup.vblank_in_line = 258;
        setup.vblank_out_line = 21;
    }
    if (TWO_FIELDS) {
        setup.vsync_width = 6;
    } else {
        setup.vsync_width = 3;
    }
    if (TWO_FIELDS) {
        setup.broad_width = 364;
    } else {
        setup.broad_width = 793;
    }
    if (TWO_FIELDS) {
        setup.equalize_width = 31;
    } else {
        setup.equalize_width = 15;
    }
    setup.ntsc = 1;
    setup.pal = 0;
    if (TWO_FIELDS) {
        setup.interlace = 1;
    } else {
        setup.interlace = 0;
    }
    setup.hsync_polarity = 0;
    setup.vsync_polarity = 0;
    setup.blank_polarity = 0;
    setup.blank = 0;
    setup.field_source = 0;
    if (kmiDeviceExtension.core == 6) {
        setup.pixel_delay = 22;
    } else {
        setup.pixel_delay = 31;
    }
    if (kmiDeviceExtension.display_mode & KM_DSPMODE_WIDTH_640) {
        setup.repeat_pixels = 0;
    } else {
        setup.repeat_pixels = 1;
    }
    setup.drive_sync = 1;
    setup.composite_on_hsync = 0;
    setup.master_hsync = 0;
    setup.master_vsync = 0;
    setup.unk_58 = 0;
    setup.master_csync = 0;
    setup.sync_lock = 0;
    setup.origin_x = 164;
    setup.unk_64 = 480;
    setup.second_field_only = 0;
    if (TWO_FIELDS) {
        setup.origin_y1 = 18;
        setup.origin_y2 = 18;
    } else {
        setup.origin_y1 = 17;
        setup.origin_y2 = 18;
    }
    kmiDeviceExtension.picture_lines = 240;
    kmiDeviceExtension.picture_end =
        (setup.vblank_end / 2) + kmiDeviceExtension.picture_lines;
    kmiDeviceExtension.picture_start = setup.vblank_end;
    write_signal(setup);
}

void kmiInitVGA(void)
{
    struct signal_setup setup;

    setup.line_clocks = 858 - 1;
    setup.lines = 525 - 1;
    setup.hblank_end = 126;
    setup.hblank_start = 837;
    setup.vblank_end = 40;
    setup.vblank_start = 520;
    setup.hsync_width = 63;
    setup.vsync_width = 3;
    setup.broad_width = 793;
    setup.equalize_width = 15;
    setup.vblank_in_line = 520;
    setup.vblank_out_line = 21;
    setup.hsync_polarity = 0;
    setup.vsync_polarity = 0;
    setup.blank_polarity = 0;
    setup.blank = 0;
    setup.field_source = 0;
    if (kmiDeviceExtension.core == 6) {
        setup.pixel_delay = 22;
    } else {
        setup.pixel_delay = 31;
    }
    setup.repeat_pixels = 0;
    setup.ntsc = 0;
    setup.pal = 0;
    setup.interlace = 0;
    setup.drive_sync = 1;
    setup.composite_on_hsync = 0;
    setup.master_hsync = 0;
    setup.master_vsync = 0;
    setup.unk_58 = 0;
    setup.master_csync = 0;
    setup.sync_lock = 0;
    setup.second_field_only = 0;
    setup.unk_64 = 480;
    /* @hack redundant: the first origins, which the next three stores
     * replace. SHC stores them, and forms the second origin_x, 168, by adding
     * 24 to the first's 144 (ADD #24,R3 at 0x8c665228, and at 0x8ce1e336 in
     * LOADER.BIN). without them, the section runs 36 bytes short in
     * 1ST_READ.BIN and 16 in LOADER.BIN, and 68 of 116 words differ in
     * 1ST_READ.BIN and 41 of 117 in LOADER.BIN */
    setup.origin_x = 144;
    setup.origin_y1 = 35;
    setup.origin_y2 = 35;
    setup.origin_x = 168;
    setup.origin_y1 = 40;
    setup.origin_y2 = 40;
    kmiDeviceExtension.picture_lines = 480;
    kmiDeviceExtension.picture_end =
        setup.vblank_end + kmiDeviceExtension.picture_lines;
    kmiDeviceExtension.picture_start = setup.vblank_end;
    write_signal(setup);
}

KMINT32 kmiEnableVSyncInterrupt(KMBOOLEAN enable)
{
    return TRUE;
}

KMINT32 kmiEnableHSyncInterrupt(KMBOOLEAN enable)
{
    return TRUE;
}
