/* Kamui video chip initialization */
#include <kamui.h>
#include <sg_xpt.h>

/* the video chip's registers, as offsets into its register block */
#define CHIP_ID 0x000
#define CHIP_REVISION 0x004
#define SOFT_RESET 0x008
#define SPAN_SORT 0x030
#define FB_READ_CONTROL 0x044
#define FB_WRITE_CONTROL 0x048
#define FB_WRITE_STRIDE 0x04c
#define FB_READ_SIZE 0x05c
#define FB_CLIP_X 0x068
#define FB_CLIP_Y 0x06c
#define SHADOW_SCALE 0x074
#define CULL_VALUE 0x078
#define PARAM_CONFIG 0x07c
#define HALF_OFFSET 0x080
#define PERP_VALUE 0x084
#define BACKGROUND_DEPTH 0x088
#define BACKGROUND_TAG 0x08c
#define FEED_CONFIG 0x098
#define SDRAM_REFRESH 0x0a0
#define SDRAM_CONFIG 0x0a8
#define FOG_TABLE_COLOR 0x0b0
#define FOG_VERTEX_COLOR 0x0b4
#define FOG_DENSITY 0x0b8
#define FOG_CLAMP_MAX 0x0bc
#define FOG_CLAMP_MIN 0x0c0
#define TEXTURE_CONTROL 0x0e4
#define FB_BURST_CONTROL 0x110
#define FOG_TABLE 0x200

/* SOFT_RESET's bits */
#define RESET_TA 1
#define RESET_RENDERER 2

/* a chip is core 6 when CHIP_ID reads CORE6_CHIP_ID and CHIP_REVISION reads
 * at least CORE6_REVISION */
#define CORE6_CHIP_ID 0x17fd11db
#define CORE6_REVISION 0x11

/* video memory's 32-bit view, through the uncached area */
#define VIEW_32BIT 0xa5000000UL
/* video memory's size in longs: 8 MB */
#define VIDEO_MEMORY_LONGS 0x200000

/* 1.0, as a float's bits */
#define FLOAT_ONE 0x3f800000

#define FOG_ENTRIES 0x80

void kmiResetBoard(void)
{
    kmiWriteRegister(SOFT_RESET, RESET_TA | RESET_RENDERER);
    kmiWriteRegister(CHIP_ID, 0);
    kmiWriteRegister(SOFT_RESET, 0);
}

void kmiCheckRenderer(void)
{
}

/* set kmiDeviceExtension's core and feed_config from CHIP_ID and
 * CHIP_REVISION, and its capabilities from BUS_REVISION. then give the video
 * chip's registers their initial values and clear video memory. video_mode
 * must be set first, since FB_READ_CONTROL's value depends on it. on core 6
 * kmiInitializeDevice returns TRUE. on any other core it never returns: it
 * loops forever before it writes PARAM_CONFIG */
KMINT32 kmiInitializeDevice(void)
{
    KMDWORD chip_id;
    KMDWORD chip_revision;
    KMDWORD bus_revision;
    KMDWORD sdram_config;
    KMDWORD span_sort;
    KMDWORD zero;
    int i;

    sdram_config = 0x0dd18957;
    kmiDeviceExtension.feed_config = 0;
    chip_id = kmiReadRegister(CHIP_ID);
    chip_revision = kmiReadRegister(CHIP_REVISION);
    /* core is 5 unless the chip is core 6, as CORE6_CHIP_ID and CORE6_REVISION
     * say. the arms for another CHIP_ID and for revision 1 store 5 again. since
     * kmiDeviceExtension is volatile, a path through either arm stores 5 twice,
     * as the image does (0x8c663f4a, then 0x8c663f58 or 0x8c663f64) */
    kmiDeviceExtension.core = 5;
    if (chip_id != CORE6_CHIP_ID) {
        kmiDeviceExtension.core = 5;
    } else if (chip_revision == 1) {
        kmiDeviceExtension.core = 5;
    } else if (chip_revision >= CORE6_REVISION) {
        kmiDeviceExtension.core = 6;
        kmiDeviceExtension.feed_config = 0x00800408;
        sdram_config = 0x15d1c951;
    }
    /* @hack spelling: the switch on bus_revision, a copy of BUS_REVISION. SHC
     * loads the copy into R5 and copies it to R0 (0x8c663fb8, 0x8c663fba).
     * switching on BUS_REVISION directly, SHC loads it into R0, chip_revision
     * moves from R5 to R4, and 38 of 239 words differ */
    bus_revision = BUS_REVISION;
    switch (bus_revision) {
    case 0:
        kmiDeviceExtension.capabilities = 0;
        span_sort = 0;
        break;
    case 1:
        kmiDeviceExtension.capabilities = 1;
        span_sort = 0;
        break;
    case 2:
        kmiDeviceExtension.capabilities = 3;
        span_sort = 0x101;
        break;
    case 4:
    case 5:
    case 6:
        kmiDeviceExtension.capabilities = 0x7f;
        span_sort = 0x10101;
        break;
    /* case 8, case 9 and the default set KMI_CAP_TWO_ENTRIES and
     * KMI_CAP_DUMMY_OPAQUE in 1.20J, and not in 1.30J */
    case 8:
        kmiDeviceExtension.capabilities = 0x1f;
        span_sort = 0x101;
        break;
    case 9:
        kmiDeviceExtension.capabilities = 0xf;
        span_sort = 0x101;
        break;
    default:
        kmiDeviceExtension.capabilities = 0xf;
        span_sort = 0x101;
        break;
    }
    kmiWriteRegister(SDRAM_CONFIG, sdram_config);
    kmiWriteRegister(SDRAM_REFRESH, 0x20);
    kmiWriteRegister(SOFT_RESET, 0);
    kmiWriteRegister(TEXTURE_CONTROL, 3);
    kmiWriteRegister(FB_WRITE_CONTROL, 9);
    if (kmiDeviceExtension.video_mode & KM_VGA) {
        kmiVerifyWriteRegister(FB_READ_CONTROL, 0x00800000);
    } else {
        kmiVerifyWriteRegister(FB_READ_CONTROL, 0);
    }
    kmiWriteRegister(FB_READ_SIZE, 0);
    kmiWriteRegister(FB_CLIP_X, 640 << 16);
    kmiWriteRegister(FB_CLIP_Y, 480 << 16);
    if (kmiDeviceExtension.core == 6) {
        kmiWriteRegister(FB_BURST_CONTROL, 0x00093f39);
        kmiWriteRegister(FEED_CONFIG, kmiDeviceExtension.feed_config);
    } else {
        kmiWriteRegister(FB_BURST_CONTROL, 0x00092039);
    }
    kmiWriteRegister(FB_WRITE_STRIDE, 640 / 4);
    kmiWriteRegister(SHADOW_SCALE, 0);
    kmiWriteRegister(CULL_VALUE, FLOAT_ONE);
    kmiWriteRegister(PERP_VALUE, 0);
    kmiWriteRegister(SPAN_SORT, span_sort);
    kmiWriteRegister(FOG_TABLE_COLOR, 0x007f7f7f);
    kmiWriteRegister(FOG_VERTEX_COLOR, 0x007f7f7f);
    kmiWriteRegister(FOG_CLAMP_MIN, 0);
    kmiWriteRegister(FOG_CLAMP_MAX, 0xffffffff);
    kmiWriteRegister(HALF_OFFSET, 7);
    kmiWriteRegister(BACKGROUND_DEPTH, 0x3727c5a0);
    kmiWriteRegister(BACKGROUND_TAG, 0x01203000);
    kmiWriteRegister(SHADOW_SCALE, 1);
    if (kmiDeviceExtension.core != 6) {
        while (1) {
        }
    }
    if (kmiDeviceExtension.core == 6) {
        kmiWriteRegister(PARAM_CONFIG, 0x0027df77);
    } else {
        kmiWriteRegister(PARAM_CONFIG, 0x0007df77);
    }
    zero = 0;
    for (i = 0; i < VIDEO_MEMORY_LONGS; i++) {
        kmiMemCopy4((KMDWORD *)VIEW_32BIT + i, &zero, 4);
    }
    return TRUE;
}

/* write FOG_DENSITY and both fog colors, and fill the fog table. table entry i
 * holds 0xff - i in its high byte, and the next entry's high byte, 0xfe - i, in
 * its low byte */
void kmiInitializeFog(void)
{
    int i;

    kmiWriteRegister(FOG_DENSITY, 0xff07);
    kmiWriteRegister(FOG_VERTEX_COLOR, 0x007f7f7f);
    kmiWriteRegister(FOG_TABLE_COLOR, 0x007f7f7f);
    for (i = 0; i < FOG_ENTRIES; i++) {
        kmiWriteRegister(FOG_TABLE + (i * 4),
                         (((0xff - i) & 0xff) << 8 | ((0xfe - i) & 0xff)) &
                             0xffff);
    }
}

void kmiUnloadDevice(void)
{
}
