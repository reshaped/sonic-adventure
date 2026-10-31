/* Kamui interrupt callbacks and their defaults, the line that raises the
 * horizontal blank interrupt, and the scan line being displayed */
#include <kamui.h>
#include <machine.h>

/* the video chip's registers, as offsets into its register block */
#define HBLANK_INTERRUPT 0x0c8
#define SYNC_STATUS 0x10c

/* the current scan line, in SYNC_STATUS */
#define SCAN_LINE 0x3ff

/* HBLANK_INTERRUPT's fields: the line that raises the interrupt, and above it
 * the start of the horizontal blank */
#define HBLANK_LINE 0x3ff
#define HBLANK_START 0x03ff0000

static void callback_end_render(void *argument);
static void callback_vsync(void *argument);
static void callback_hsync(void *argument);
static void callback_texture_overflow(void *argument);
static void callback_strip_overrun(void *argument);
static void callback_end_vertex(void *argument);
static void callback_end_yuv(void *argument);
static void callback_wait_vsync(void *argument);
static void callback_fatal_error(void *error);

void kmiInitCallback(void)
{
    kmiDeviceExtension.unk_2a4 = 0;
    kmiDeviceExtension.callback[END_RENDER_CALLBACK].function =
        callback_end_render;
    kmiDeviceExtension.callback[END_RENDER_CALLBACK].argument = 0;
    kmiDeviceExtension.callback[VSYNC_CALLBACK].function = callback_vsync;
    kmiDeviceExtension.callback[VSYNC_CALLBACK].argument = 0;
    kmiDeviceExtension.callback[HSYNC_CALLBACK].function = callback_hsync;
    kmiDeviceExtension.callback[HSYNC_CALLBACK].argument = 0;
    kmiDeviceExtension.callback[TEXTURE_OVERFLOW_CALLBACK].function =
        callback_texture_overflow;
    kmiDeviceExtension.callback[TEXTURE_OVERFLOW_CALLBACK].argument = 0;
    kmiDeviceExtension.callback[STRIP_OVERRUN_CALLBACK].function =
        callback_strip_overrun;
    kmiDeviceExtension.callback[STRIP_OVERRUN_CALLBACK].argument = 0;
    kmiDeviceExtension.callback[END_VERTEX_CALLBACK].function =
        callback_end_vertex;
    kmiDeviceExtension.callback[END_VERTEX_CALLBACK].argument = 0;
    kmiDeviceExtension.callback[END_YUV_CALLBACK].function = callback_end_yuv;
    kmiDeviceExtension.callback[END_YUV_CALLBACK].argument = 0;
    kmiDeviceExtension.callback[WAIT_VSYNC_CALLBACK].function =
        callback_wait_vsync;
    kmiDeviceExtension.callback[WAIT_VSYNC_CALLBACK].argument = 0;
    kmiDeviceExtension.callback[FATAL_ERROR_CALLBACK].function =
        callback_fatal_error;
    kmiDeviceExtension.callback[FATAL_ERROR_CALLBACK].argument = 0;
}

static void reset_callback(volatile struct kmi_device_extension *device,
                           int slot)
{
    switch (slot) {
    case END_RENDER_CALLBACK:
        device->callback[END_RENDER_CALLBACK].function = callback_end_render;
        break;
    case VSYNC_CALLBACK:
        device->callback[VSYNC_CALLBACK].function = callback_vsync;
        break;
    case HSYNC_CALLBACK:
        device->callback[HSYNC_CALLBACK].function = callback_hsync;
        break;
    case TEXTURE_OVERFLOW_CALLBACK:
        device->callback[TEXTURE_OVERFLOW_CALLBACK].function =
            callback_texture_overflow;
        break;
    case STRIP_OVERRUN_CALLBACK:
        device->callback[STRIP_OVERRUN_CALLBACK].function =
            callback_strip_overrun;
        break;
    case END_VERTEX_CALLBACK:
        device->callback[END_VERTEX_CALLBACK].function = callback_end_vertex;
        break;
    case END_YUV_CALLBACK:
        device->callback[END_YUV_CALLBACK].function = callback_end_yuv;
        break;
    case WAIT_VSYNC_CALLBACK:
        device->callback[WAIT_VSYNC_CALLBACK].function = callback_wait_vsync;
        break;
    case FATAL_ERROR_CALLBACK:
        device->callback[FATAL_ERROR_CALLBACK].function = callback_fatal_error;
        break;
    }
}

void kmiRegisterCallback(volatile struct kmi_device_extension *device,
                         KMINT32 slot, PKMCALLBACKFUNC callback, void *argument)
{
    int mask = _builtin_get_imask();
    _builtin_set_imask(IMASK_ALL);
    if (callback) {
        /* @hack type: the handler stored through a view of the record
         * without volatile. SHC then forms the argument store's base (MOV.W
         * and ADD at 0x8c6213e4 to 0x8c6213ec) before it stores the handler
         * at 0x8c6213ee. through the volatile record, the handler store comes
         * first, and 7 of 42 words differ */
        ((struct kmi_device_extension *)device)->callback[slot].function =
            callback;
        device->callback[slot].argument = argument;
    } else {
        reset_callback(device, slot);
    }
    _builtin_set_imask(mask);
}

KMSTATUS kmSetEORCallback(PKMCALLBACKFUNC callback, void *argument)
{
    kmiRegisterCallback(kmiHardwareResource, END_RENDER_CALLBACK, callback,
                        argument);
    return 0;
}

KMSTATUS kmSetVSyncCallback(PKMCALLBACKFUNC callback, void *argument)
{
    kmiEnableVSyncInterrupt(1);
    kmiRegisterCallback(kmiHardwareResource, VSYNC_CALLBACK, callback,
                        argument);
    return 0;
}

KMSTATUS kmSetHSyncCallback(PKMCALLBACKFUNC callback, void *argument)
{
    kmiEnableHSyncInterrupt(callback != 0);
    kmiRegisterCallback(kmiHardwareResource, HSYNC_CALLBACK, callback,
                        argument);
    return 0;
}

KMSTATUS kmSetHSyncLine(KMINT32 line)
{
    KMINT32 origin;
    KMINT32 lines;
    KMINT32 position;
    KMINT32 low;
    KMINT32 value;

    origin = kmiDeviceExtension.origin_y1 + kmiDeviceExtension.offset_y;
    if (TWO_FIELDS) {
        lines = kmiDeviceExtension.sync_load >> 17;
    } else {
        lines = kmiDeviceExtension.sync_load >> 16;
    }
    if (VGA_MODE) {
        position = line + origin;
    } else if (kmiDeviceExtension.display_mode & KM_DSPMODE_HEIGHT_480) {
        position = (line / 2) + origin;
    } else {
        position = line + origin;
    }
    if (position < 0) {
        position += lines;
    }
    if (position > lines) {
        position = lines;
    }
    if (position < 0) {
        position = 0;
    }
    /* @hack spelling: position masked into low in a statement of its own, which
     * masks it (MOV.W and AND at 0x8c621594 to 0x8c62159a) before
     * hblank_interrupt is loaded (0x8c62159c); masked in the OR, 16 of 76
     * words differ and the function is 2 bytes shorter */
    low = position & HBLANK_LINE;
    value = kmiDeviceExtension.hblank_interrupt & HBLANK_START;
    value |= low;
    kmiDeviceExtension.hblank_interrupt = value;
    kmiWriteRegister(HBLANK_INTERRUPT, value);
    return 0;
}

KMSTATUS kmGetCurrentScanline(KMINT32 *line)
{
    /* @hack redundant: register on status, which gives it R4 (MOV R0,R4 at
     * 0x8c6215d0) and position R5; without it, status takes R14 and position
     * R4, and 23 of 98 words differ */
    register KMDWORD status;
    KMINT32 position;
    KMINT32 origin;
    KMINT32 visible;
    KMINT32 lines;
    KMFLOAT scale;

    if (kmiDeviceExtension.capabilities & KMI_CAP_SYNC_STATUS) {
        status = kmiReadRegister(SYNC_STATUS);
        position = status & SCAN_LINE;
    } else {
        position = 0;
#ifdef GCC
        /* repairs the unset status the tag below names: GCC's status is 0,
         * so the test of its bit 18 below adds no line */
        status = 0;
#endif
    }
    scale = 1.0F;
    origin = kmiDeviceExtension.origin_y1 + kmiDeviceExtension.offset_y;
    if (VGA_MODE) {
        visible = 480;
    } else if (TWO_FIELDS) {
        if (kmiDeviceExtension.display_mode & KM_DSPMODE_HEIGHT_480) {
            visible = 480;
            /* @hack redundant: scale, already 1.0F, set to it again, FMOV
             * FR5,FR4 at 0x8c621660; without it, 59 of 98 words differ */
            scale = 1.0F;
            origin += origin;
        } else {
            visible = 240;
            origin += origin;
            scale = 0.5F;
        }
    } else {
        visible = 240;
    }
    lines = kmiDeviceExtension.sync_load >> 16;
    if (TWO_FIELDS) {
        /* @bug uninitialized: on a chip without KMI_CAP_SYNC_STATUS, the BT at
         * 0x8c6215c4 skips the read that sets status. in a two-field mode, TST
         * R2,R4 at 0x8c621692 then tests bit 18 of R4, which still holds the
         * pointer line from entry. kmiInitializeDevice gives no chip of bus
         * revision 0 or 1 that bit */
        if (status & 0x40000) {
            position++;
        }
    }
    /* @hack spelling: the origin subtracted in a statement of its own, which
     * keeps position in R5 through the conversion (SUB R6,R5 at 0x8c621698, STS
     * FPUL,R5 at 0x8c6216a2); as (position - origin) * scale, 19 of 98 words
     * differ */
    position -= origin;
    position *= scale;
    if (position <= visible) {
        *line = position;
    } else {
        *line = position - lines;
    }
    return 0;
}

KMSTATUS kmSetTexOverflowCallback(PKMCALLBACKFUNC callback, void *argument)
{
    kmiRegisterCallback(kmiHardwareResource, TEXTURE_OVERFLOW_CALLBACK,
                        callback, argument);
    return 0;
}

KMSTATUS kmSetStripOverRunCallback(PKMCALLBACKFUNC callback, void *argument)
{
    kmiRegisterCallback(kmiHardwareResource, STRIP_OVERRUN_CALLBACK, callback,
                        argument);
    return 0;
}

KMSTATUS kmSetEndOfVertexCallback(PKMCALLBACKFUNC callback, void *argument)
{
    kmiRegisterCallback(kmiHardwareResource, END_VERTEX_CALLBACK, callback,
                        argument);
    return 0;
}

KMSTATUS kmSetEndOfYUVCallback(PKMCALLBACKFUNC callback, void *argument)
{
    kmiRegisterCallback(kmiHardwareResource, END_YUV_CALLBACK, callback,
                        argument);
    return 0;
}

KMSTATUS kmSetWaitVsyncCallback(PKMCALLBACKFUNC callback, void *argument)
{
    kmiRegisterCallback(kmiHardwareResource, WAIT_VSYNC_CALLBACK, callback,
                        argument);
    return 0;
}

KMSTATUS kmfSetFatalErrorCallback(PKMCALLBACKFUNC callback)
{
    kmiRegisterCallback(kmiHardwareResource, FATAL_ERROR_CALLBACK, callback, 0);
    return 0;
}

static void callback_end_render(void *argument)
{
}

static void callback_vsync(void *argument)
{
}

static void callback_hsync(void *argument)
{
}

static void callback_texture_overflow(void *argument)
{
}

static void callback_strip_overrun(void *argument)
{
}

static void callback_end_vertex(void *argument)
{
}

static void callback_end_yuv(void *argument)
{
}

static void callback_wait_vsync(void *argument)
{
}

static void callback_fatal_error(void *error)
{
    KMPACKEDARGB color;

    /* @bug uninitialized: for a code other than 1 to 4, no case sets color.
     * kmSetBorderColor then gets color's unwritten stack word: the default's
     * BRA at 0x8c621806 skips every store, and MOV.L @R14,R2 at 0x8c621822
     * loads the word. 1ST_READ.BIN's calls pass 1 to 4 */
    switch ((KMDWORD)error) {
    case 1:
        color.dwPacked = 0xff0000ff;
        break;
    case 2:
        color.dwPacked = 0xffff0000;
        break;
    case 3:
        color.dwPacked = 0xff00ff00;
        break;
    case 4:
        color.dwPacked = 0xffff00ff;
        break;
    }
    kmSetBorderColor(color);
    for (;;) {
    }
}
