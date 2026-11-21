/* run Ninja and the readout against a private translucent list in uncached RAM
 */
#include <kamui.h>
#include <ninja.h>
#include <shinobi.h>
#include <stdarg.h>
#include <string.h>
#include "engine/dc/dreamcast.h"
#include "sdk/ninja/njkamui.h"
#include "stage/stage.h"
#include "system/chord.h"
#include "system/frame.h"
#include "system/timer.h"
#include "system/util.h"

struct kmi_interrupt_statistics kmiIntStatisticData;
volatile struct kmi_device_extension kmiDeviceExtension;
Uint32 _nj_current_texture;
NJS_TEXLIST *_nj_current_texlist;
KMVERTEXCONTEXT *_nj_current_ctx_;
struct nj_parameterkm _nj_parameterkm_;
KMVERTEXBUFFDESC _nj_vertex_buffer_;
KMVERTEXCONTEXT _nj_context_[NJD_CONTEXT_MAX];
Uint32 _nj_vsync_count_;
Uint8 _nj_debug_char_[1];
Sint16 act_current, stage_current;
int frame_interval = 2, frame_step, port_toggles[TOGGLES];
enum wide_mode wide_applied;
Bool draw_off;
Int port_wide_cover = TRUE;

static Uint32 buffer[16384] __attribute__((aligned(32)));
static int formats, figures, fps;
volatile int result;
#define AREA0 (*(volatile Uint32 *)0xff000038)
#define AREA1 (*(volatile Uint32 *)0xff00003c)

void crash_frame(void)
{
}
int port_curtain_level(void)
{
    return 0;
}
enum present_shown present_shown(int late)
{
    return late ? HELD_SECOND : SHOWN_BOTH;
}
int present_figures(struct present_figures *out)
{
    figures++;
    return 0;
}
Uint32 syTmrCountToMicro(Uint32 count)
{
    return count;
}
Uint32 syTmrGetCount(void)
{
    return 0;
}

KMSTATUS kmCreateTextureSurface(PKMSURFACEDESC surface, KMINT32 w, KMINT32 h,
                                KMTEXTURETYPE kind)
{
    return KMSTATUS_SUCCESS;
}

KMSTATUS kmLoadTexture(PKMSURFACEDESC surface, PKMDWORD texture, KMBOOLEAN mip,
                       KMBOOLEAN dither)
{
    return KMSTATUS_SUCCESS;
}

/* count the calls and let the game's formatter write each line */
int string_format(char *out, const char *format, ...)
{
    int length;
    va_list args;
    formats++;
    va_start(args, format);
    length = string_vformat(out, format, args);
    va_end(args);
    if (!strcmp(format, "%2u FPS")) {
        fps = (out[0] == ' ' ? 0 : 10 * (out[0] - '0')) + out[1] - '0';
    }
    return length;
}

static int cycle(void)
{
    return port_toggles[TOGGLE_READOUT] = overlay_page();
}

static int check(void)
{
    Uint32 *list = (Uint32 *)((Uint32)buffer | 0x20000000);
    static PKMDWORD lists[5];
    Uint32 expected[40];
    Uint32 *after;
    Uint32 now = 0xffff0000;

    _nj_vertex_buffer_.pCurrentPtr = lists;
    njInitPrint(NULL, 0, 0);
    static NJS_TEXLIST game_textures;
    _nj_current_ctx_ = &_nj_context_[1];
    _nj_current_ctx_->TSPPARAMBUFFER = 0x94120033;
    _nj_parameterkm_.GLOBALPARAMBUFFER = 0x82940008;
    _nj_current_texlist = &game_textures;
    njPrintColor(0xff137abe);
    njPrintSize(13);
    _nj_vertex_buffer_.pCurrentPtr[KM_TRANS_POLYGON] = list;
    njPrintC(NJM_LOCATION(2, 4), "A");
    memcpy(expected, list, sizeof(expected));
    _nj_current_texture = 37;
    _nj_vertex_buffer_.pCurrentPtr[KM_TRANS_POLYGON] = list;
    AREA0 = 4;
    AREA1 = 12;
    overlay_end(now, 0, 0);
    overlay_draw();
    if (formats || figures ||
        _nj_vertex_buffer_.pCurrentPtr[KM_TRANS_POLYGON] != list) {
        return 1;
    }
    cycle();
    _nj_vsync_count_ = 0xfffffff8;
    overlay_end(now, 0, 0);
    _nj_vsync_count_ += 15;
    overlay_end(now + COUNTS_PER_SECOND / 2 - 1, 0, 1);
    if (formats) {
        return 2;
    }
    now += COUNTS_PER_SECOND / 2;
    overlay_end(now, 0, 0);
    if (fps != 30 || formats != 1 || figures) {
        return 3;
    }
    _nj_vsync_count_ += 30;
    now += COUNTS_PER_SECOND / 2;
    overlay_end(now, 0, 0);
    if (fps != 60 || formats != 2 || figures) {
        return 4;
    }
    frame_interval = 0;
    overlay_end(now + COUNTS_PER_SECOND, 0, 0);
    frame_interval = 2;
    draw_off = TRUE;
    overlay_draw();
    draw_off = FALSE;
    if (formats != 2 ||
        _nj_vertex_buffer_.pCurrentPtr[KM_TRANS_POLYGON] != list) {
        return 5;
    }
    overlay_draw();
    after = _nj_vertex_buffer_.pCurrentPtr[KM_TRANS_POLYGON];
    if (after != list + (5 + 1 + 4 * 6) * 8 || AREA0 != 4 || AREA1 != 12 ||
        !port_wide_cover) {
        return 6;
    }
    if (_nj_current_ctx_ != &_nj_context_[1] ||
        _nj_current_ctx_->TSPPARAMBUFFER != 0x94120033 ||
        _nj_parameterkm_.GLOBALPARAMBUFFER != 0x82940008 ||
        _nj_current_texture != 37 || _nj_current_texlist != &game_textures) {
        return 11;
    }
    njPrintC(NJM_LOCATION(2, 4), "A");
    /* the header defines four words and each textured vertex defines seven */
    for (int i = 0; i < 5; i++) {
        if (memcmp(after + i * 8, expected + i * 8, (i ? 7 : 4) * 4)) {
            return 7;
        }
    }
    cycle();
    _nj_vertex_buffer_.pCurrentPtr[KM_TRANS_POLYGON] = list;
    overlay_draw();
    if (list[6 * 8 + 6] != 0xffffffff) {
        return 8;
    }
    if (!cycle() || cycle() || !cycle()) {
        return 9;
    }
    int before = formats;
    overlay_end(0, 0, 0);
    if (formats != before) {
        return 10;
    }
    return 0;
}

__attribute__((section(".text.boot"))) void start(void)
{
    result = check() + 1;
    for (;;) {
        __asm__ volatile("nop");
    }
}
