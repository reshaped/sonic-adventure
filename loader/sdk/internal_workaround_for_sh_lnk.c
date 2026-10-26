/* SDK 1.20J Kamui driver state, and the hardware's setup and release */
#include <kamui.h>
#include <string.h>

/* the step from bank 0's layout of the tile accelerator's buffers to bank 1's,
 * 4 MB */
#define BANK_STEP 0x400000
/* bank 0's bank_limit, which nothing reads. bank 1's lies BANK_STEP lower */
#define BANK0_LIMIT 0x7d0000

static char driver_notice[621] = {
#include "sdk/internal_workaround_for_sh_lnk/driver_notice.inc"
};

/* the pointer itself is volatile: kmiInitializeDriver stores it, then reads
 * it back for memset's argument (0x8ce1a328) */
volatile struct kmi_device_extension *volatile kmiHardwareResource = 0;
volatile struct kmi_device_extension kmiDeviceExtension;
struct kmi_version_info kmiVersionInfo;

KMSTATUS kmiInitTimer(void)
{
    return 0;
}

void kmiInitializeDriver(void)
{
    KMDWORD region_size;
    KMDWORD object_size;
    KMDWORD isp_size;
    KMDWORD tsp_size;
    KMDWORD isp_offset;
    KMDWORD tsp_offset;
    KMDWORD object_offset;
    int i;

    if (!kmiHardwareResource) {
        kmiHardwareResource = &kmiDeviceExtension;
        /* the image clears the record with memset (0x8ce1a320-0x8ce1a328).
         * memset's parameter isn't volatile, so the cast drops the qualifier */
        memset((void *)kmiHardwareResource, 0, sizeof(kmiDeviceExtension));
    } else {
        /* @hack redundant: an empty else, which puts kmiVersionInfo.major's 3
         * in R3 (MOV #3,R3 at 0x8ce1a32a); without it, the 3 takes R2 and 7 of
         * 342 words differ */
    }
    kmiVersionInfo.major = 3;
    kmiVersionInfo.minor = 2;
    kmiVersionInfo.revision = 4;
    kmiVersionInfo.build = 1;
    kmiVersionInfo.copyright = driver_notice;
    kmiDeviceExtension.version = &kmiVersionInfo;
    kmiDeviceExtension.strip_buffer = 0;
    kmiDeviceExtension.level_adjust = 0;
    kmiDeviceExtension.skip_lines = 0;
    kmiDeviceExtension.offset_x = 0;
    kmiDeviceExtension.offset_y = 0;
    kmiInitCallback();
    kmiDeviceExtension.render_busy = 0;
    kmiDeviceExtension.ta_busy = 0;
    kmiDeviceExtension.vsync_wait =
        kmiGetProfileInt("RENDER", "VSYNCCount", 1) - 1;
    kmiDeviceExtension.vsync_remaining = kmiDeviceExtension.vsync_wait;
    kmiDeviceExtension.memory_alignment = 0x10000;
    kmiDeviceExtension.texture_memory_size = 0x800000;
    kmiDeviceExtension.unk_01c = 0x800000;
    kmiDeviceExtension.global_clip = 0;
    kmiDeviceExtension.presort = 0;
    kmiDeviceExtension.regions_stale = 0;
    kmiDeviceExtension.strip_length = 3;
    kmiDeviceExtension.display_changed = 1;
    kmiInitializeBackGroundPlane();
    region_size = kmiGetProfileInt("RENDER", "RegionSize", 0x6000);
    object_size = kmiGetProfileInt("RENDER", "ObjListSize", 0x7a000);
    isp_size = kmiGetProfileInt("RENDER", "ISPParamSize", 0x180000);
    tsp_size = kmiGetProfileInt("RENDER", "TSPParamSize", 0x180000);
    region_size = (region_size >> 11) << 11;
    object_size = (object_size >> 11) << 11;
    isp_size = (isp_size >> 11) << 11;
    tsp_size = (tsp_size >> 11) << 11;
    kmiDeviceExtension.isp_size = isp_size;
    kmiDeviceExtension.tsp_size = tsp_size;
    object_offset = region_size;
    isp_offset = object_offset + object_size;
    tsp_offset = isp_offset + isp_size;
    for (i = 0; i < 2; i++) {
        kmiDeviceExtension.ta_layout[i].region_array = i * BANK_STEP;
        kmiDeviceExtension.ta_layout[i].object_list =
            object_offset + (i * BANK_STEP);
        kmiDeviceExtension.ta_layout[i].isp_params =
            isp_offset + (i * BANK_STEP);
        kmiDeviceExtension.ta_layout[i].tsp_params =
            tsp_offset + (i * BANK_STEP);
        kmiDeviceExtension.bank_limit[i] = BANK0_LIMIT - (i * BANK_STEP);
    }
    kmiDeviceExtension.latency_model = 0;
    kmiDeviceExtension.vertex_buffer_model = 0;
    kmiDeviceExtension.slot_state[0] = SLOT_FILLING;
    kmiDeviceExtension.slot_state[1] = SLOT_FREE;
    kmiDeviceExtension.slot_state[2] = SLOT_FREE;
    kmiDeviceExtension.slot_state[3] = SLOT_FREE;
    kmiDeviceExtension.write_slot = 0;
    kmiDeviceExtension.render_slot = 0;
    kmiDeviceExtension.ta_slot = 0;
    kmiDeviceExtension.frame_dma = 0;
    kmiDeviceExtension.common_modifier = 0;
    kmiDeviceExtension.frame_dma_wait = 1;
    kmiDeviceExtension.vertex_base =
        kmiGetProfileInt("VertexBuffer", "BufferBase", (KMINT32)0xacc00000);
    kmiDeviceExtension.vertex_size =
        kmiGetProfileInt("VertexBuffer", "BufferSize", 0x400000);
    kmiDeviceExtension.polling_ta =
        kmiGetProfileInt("RENDER", "PoolingTAStatus", 0);
    kmiDeviceExtension.render_timeout =
        kmiGetProfileInt("RENDER", "RenderTimeOut", 10);
    kmiDeviceExtension.vertex_timeout =
        kmiGetProfileInt("RENDER", "VertexTimeOut", 10);
    kmiDeviceExtension.vsync_timeout =
        kmiGetProfileInt("RENDER", "VSYNCTimeOut", 10);
    kmiDeviceExtension.another_modifier = 0;
    kmiDeviceExtension.another_modifier_list = 1;
    kmiDeviceExtension.ta_options = 15;
    /* kmiDeviceExtension is volatile, so the OR reads ta_options back and
     * stores it unchanged, as the image does (0x8ce1a582-0x8ce1a586) */
    kmiDeviceExtension.ta_options |= 0;
    kmiDeviceExtension.field_offset[0] = 0;
    kmiDeviceExtension.field_offset[1] = 0;
    kmiInitTimer();
    /* @hack redundant: the call in a loop of one pass, which keeps it a JSR
     * (0x8ce1a594); without it, SHC jumps to kmiInitRenderer as a tail call,
     * the function is 4 bytes shorter, and 58 of 342 words differ */
    do {
        kmiInitRenderer();
    } while (0);
}

void kmiReleaseHardware(void)
{
    if (kmiHardwareResource) {
        kmiUnloadDevice();
        kmiHardwareResource = 0;
    }
}

void kmiInitializeHardware(void)
{
    kmiModuleInitialize();
    kmiInitializeDevice();
    kmiInitializeTA();
    kmiFBinit();
    kmiInitInterrupt();
    kmiInitializeFog();
    kmiDeviceExtension.ta_buffer_size = 0x100000;
    kmiDeviceExtension.tiles_x = 0;
    kmiDeviceExtension.tiles_y = 0;
    kmiDeviceExtension.dummy_translucent = 0;
    kmiSetTAEnvironment(kmiDeviceExtension.ta_layout[0].region_array >> 7,
                        kmiDeviceExtension.ta_layout[0].object_list >> 7,
                        kmiDeviceExtension.ta_layout[0].isp_params >> 7,
                        kmiDeviceExtension.ta_layout[0].tsp_params >> 7,
                        kmiDeviceExtension.ta_options, 0);
}

KMINT32 kmiVerifyWriteRegister(KMDWORD reg, KMDWORD data)
{
    return kmiWriteRegister(reg, data);
}

void kmiWaitAccurate(KMFLOAT seconds)
{
    volatile KMDWORD count;

    for (count = 0; count < (KMDWORD)(KMINT32)(seconds * 2000.0F); count++) {
    }
}

void kmiFatalErrorEntry(KMDWORD code)
{
    kmiDeviceExtension.callback[FATAL_ERROR_CALLBACK].function((void *)code);
}
