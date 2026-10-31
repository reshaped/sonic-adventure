/* Kamui driver state, and the hardware's setup and release */
#include <kamui.h>
#include <string.h>

/* the step from bank 0's layout of the tile accelerator's buffers to bank 1's,
 * 4 MB */
#define BANK_STEP 0x400000
/* bank 0's bank_limit, which nothing reads. bank 1's lies BANK_STEP lower */
#define BANK0_LIMIT 0x7d0000

/* @hack type: kmiGetProfileInt under a second name, declared with a KMDWORD
 * fallback. 1ST_READ's symbols.txt gives both names one address. called
 * through this name, SHC loads kmiInitializeDriver's local zero straight into
 * R14 (MOV #0,R14 at 0x8c6633d6). called through kamui.h's declaration, whose
 * fallback is a KMINT32, SHC loads the zero into R11 and copies it to R14, and
 * 247 of 289 words differ. called through a cast of kmiGetProfileInt to this
 * declaration's type, SHC copies the zero from R11 too and saves R8, and 274
 * of 289 words differ */
extern KMINT32 kmi_profile_unsigned_fallback(const char *section,
                                             const char *key, KMDWORD fallback);

static char driver_notice[621] = {
#include "ninja/internal_kamui_lib/driver_notice.inc"
};

/* the pointer itself is volatile: kmiInitializeDriver stores it, then reads
 * it back for memset's argument (0x8c6633ca) */
volatile struct kmi_device_extension *volatile kmiHardwareResource = 0;
#ifdef GCC
/* 1ST_READ.BIN's tables place the two in BSG, between kmvtxbuf_kamui_lib.c's
 * bss and dma_kamui_lib.c's, where no source holds them. the port defines
 * them here, as LOADER.BIN's internal_workaround_for_sh_lnk.c defines its
 * own */
volatile struct kmi_device_extension kmiDeviceExtension;
struct kmi_version_info kmiVersionInfo;
#endif

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

    /* @hack type: a zero of pointer type, which SHC keeps in R14 (MOV #0,R14
     * at 0x8c6633d6) for the fields cleared below and for the OR into
     * ta_options. with integer zeros in its place, SHC also keeps a 1 in R8
     * and saves R8, and 279 of 289 words differ */
    void *zero;

    if (!kmiHardwareResource) {
        kmiHardwareResource = &kmiDeviceExtension;
        /* the image clears the record with memset (0x8c6633c2-0x8c6633ca).
         * memset's parameter isn't volatile, so the cast drops the qualifier */
        memset((void *)kmiHardwareResource, 0, sizeof(kmiDeviceExtension));
    } else {
        /* @hack redundant: an empty else, which puts kmiVersionInfo.major's 3
         * in R3 (MOV #3,R3 at 0x8c6633ce); without it, the 3 takes R2 and 38 of
         * 289 words differ */
    }
    zero = 0;
    kmiVersionInfo.major = 3;
    kmiVersionInfo.minor = 2;
    kmiVersionInfo.revision = 5;
    kmiVersionInfo.build = (KMINT32)zero;
    kmiVersionInfo.copyright = driver_notice;
    kmiDeviceExtension.version = &kmiVersionInfo;
    kmiDeviceExtension.strip_buffer = (KMINT32)zero;
    kmiDeviceExtension.level_adjust = (KMINT32)zero;
    kmiDeviceExtension.skip_lines = (KMINT32)zero;
    kmiDeviceExtension.offset_x = (KMINT32)zero;
    kmiDeviceExtension.offset_y = (KMINT32)zero;
    kmiInitCallback();
    kmiDeviceExtension.render_busy = (KMDWORD)zero;
    kmiDeviceExtension.ta_busy = (KMDWORD)zero;
    kmiDeviceExtension.vsync_wait =
        kmi_profile_unsigned_fallback("RENDER", "VSYNCCount", 1) - 1;
    kmiDeviceExtension.vsync_remaining = kmiDeviceExtension.vsync_wait;
    kmiDeviceExtension.memory_alignment = 0x10000;
    kmiDeviceExtension.texture_memory_size = 0x800000;
    kmiDeviceExtension.unk_01c = 0x800000;
    kmiDeviceExtension.global_clip = (KMINT32)zero;
    kmiDeviceExtension.presort = (KMINT32)zero;
    kmiDeviceExtension.regions_stale = (KMINT32)zero;
    kmiDeviceExtension.strip_length = 3;
    kmiDeviceExtension.display_changed = 1;
    kmiInitializeBackGroundPlane();
    region_size = kmi_profile_unsigned_fallback("RENDER", "RegionSize", 0x6000);
    object_size =
        kmi_profile_unsigned_fallback("RENDER", "ObjListSize", 0x7a000);
    isp_size =
        kmi_profile_unsigned_fallback("RENDER", "ISPParamSize", 0x180000);
    tsp_size =
        kmi_profile_unsigned_fallback("RENDER", "TSPParamSize", 0x180000);
    region_size = (region_size >> 11) << 11;
    object_size = (object_size >> 11) << 11;
    isp_size = (isp_size >> 11) << 11;
    /* @hack spelling: tsp_size rounded down to a multiple of 2 KB in two
     * statements, a shift right and then a shift left. SHC then keeps the
     * region and ISP sizes the profile returns at @(4,R15) and @R15
     * (0x8c66345a, 0x8c66346e). rounded in one statement, as the three sizes
     * above are, the region and ISP sizes trade slots, SHC saves R8 as well,
     * and 278 of 289 words differ */
    tsp_size = tsp_size >> 11;
    tsp_size = tsp_size << 11;
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

    /* @hack redundant: stores that never run, which keep the region, object
     * and ISP sizes the profile returns at @(4,R15), @(8,R15) and @R15
     * (0x8c66345a to 0x8c66346e); without them, 6 of 289 words differ */
    if (0) {
        kmiDeviceExtension.strip_buffer = (KMINT32)zero;
        kmiDeviceExtension.level_adjust = (KMINT32)zero;
    }
    kmiDeviceExtension.latency_model = (KMDWORD)zero;
    kmiDeviceExtension.vertex_buffer_model = (KMDWORD)zero;
    kmiDeviceExtension.slot_state[0] = (KMINT32)zero;
    kmiDeviceExtension.slot_state[1] = SLOT_FREE;
    kmiDeviceExtension.slot_state[2] = SLOT_FREE;
    kmiDeviceExtension.slot_state[3] = SLOT_FREE;
    kmiDeviceExtension.write_slot = (KMINT32)zero;
    kmiDeviceExtension.render_slot = (KMINT32)zero;
    kmiDeviceExtension.ta_slot = (KMINT32)zero;
    kmiDeviceExtension.frame_dma = (KMINT32)zero;
    kmiDeviceExtension.common_modifier = (KMINT32)zero;
    kmiDeviceExtension.frame_dma_wait = 1;
    kmiDeviceExtension.vertex_base =
        kmi_profile_unsigned_fallback("VertexBuffer", "BufferBase", 0xacc00000);
    kmiDeviceExtension.vertex_size =
        kmi_profile_unsigned_fallback("VertexBuffer", "BufferSize", 0x400000);
    kmiDeviceExtension.polling_ta =
        kmi_profile_unsigned_fallback("RENDER", "PoolingTAStatus", 0);
    kmiDeviceExtension.render_timeout =
        kmi_profile_unsigned_fallback("RENDER", "RenderTimeOut", 10);
    kmiDeviceExtension.vertex_timeout =
        kmi_profile_unsigned_fallback("RENDER", "VertexTimeOut", 10);
    kmiDeviceExtension.vsync_timeout =
        kmi_profile_unsigned_fallback("RENDER", "VSYNCTimeOut", 10);
    kmiDeviceExtension.another_modifier = (KMINT32)zero;
    kmiDeviceExtension.another_modifier_list = 1;
    kmiDeviceExtension.ta_options = 15;
    /* kmiDeviceExtension is volatile, so the OR reads ta_options back and
     * stores it unchanged, as the image does (0x8c663672-0x8c663676) */
    kmiDeviceExtension.ta_options |= (KMDWORD)zero;

    kmiDeviceExtension.field_offset[0] = (KMINT32)zero;
    kmiDeviceExtension.field_offset[1] = (KMINT32)zero;

    /* the program calls kmiInitTimer here: 1.20J's kmiInitializeDriver calls
     * it at the same place (BSR at 0x8ce1a58e in LOADER.BIN). under -speed,
     * SHC inlines the call to no code, since kmiInitTimer only returns 0. the
     * call still keeps the store to field_offset[1] out of the delay slot of
     * the JSR to kmiInitRenderer, which holds a NOP (0x8c66367c-0x8c663684) */
    kmiInitTimer();
    /* @hack redundant: the call in a loop of one pass, which keeps it a JSR
     * (0x8c663682); without it, SHC jumps to kmiInitRenderer as a tail call,
     * and 247 of 289 words differ */
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
