/* Ninja's system: the display and Kamui's setup, the vertex buffer, each
 * frame's render, and the current vertex context's render state */
#include <kamui.h>
#include <ninja.h>
#include <sg_chain.h>
#include <sg_xpt.h>
#include <shinobi.h>

#include "ninja/njkamui.h"

/* the video chip's sync status. SYNC_VERTICAL is set while a vertical sync
 * lasts */
#define SYNC_STATUS (*(volatile Uint32 *)0xa05f810c)
#define SYNC_VERTICAL 0x2000
/* the video output's control. setting VIDEO_BLANK blanks the picture */
#define VIDEO_CONTROL (*(volatile Uint32 *)0xa05f80e8)
#define VIDEO_BLANK 8
/* the system bus's normal interrupt status. INT_VBLANK_IN is set as a
 * vertical blank starts, and writing the bit clears it */
#define INT_STATUS (*(volatile Uint32 *)0xa05f6900)
#define INT_VBLANK_IN 8
/* an AICA register whose bits AICA_VIDEO_CABLE hold the video cable's type.
 * CABLE_SHIFT moves a SYE_CBL_CABLE_ value into them */
#define AICA_VIDEO (*(volatile Uint32 *)0xa0702c00)
#define AICA_VIDEO_CABLE 0x300
#define CABLE_SHIFT 8

/* the priority of njBlank and of njSetVSyncFunction's function on the vertical
 * blank chain */
#define VBLANK_PRIORITY 88
/* the words njInitVertexBuffer adds to the lists' for Kamui's vertex buffer */
#define BUFFER_EXTRA 88
/* the uncached area, P2, which starts at address P2. an address of main
 * memory ORed with P2 is its P2 address, and so is its cached P1 address ORed
 * with P1_TO_P2. a P2 address of main memory ANDed with P2_TO_P1 is its P1
 * address */
#define P2 0xa0000000
#define P1_TO_P2 0x20000000
#define P2_TO_P1 0x8fffffff
/* the limit njWaitVSync sets on its wait for the last render to end, in the
 * timer counts syTmrMicroToCount returns */
#define RENDER_WAIT_COUNT 166000
/* the bits of a display mode that hold its video output, a KMVIDEOMODE */
#define DSP_VIDEO 3
/* the width and height of the tiles njUserClipping takes, in pixels */
#define TILE_SIZE 32.0F

void njInitGlobal(void);
void njChangeGlobal(void);
void njSetCurrentContext(Int n);
void njInitContext(KMVERTEXCONTEXT *context);
void njExeEORFunction(void);
#ifdef GCC
/* the 32 Floats after _nj_cnk_easy_multilight_buffer_ in njGlobal.c's
 * block, which njInitGlobal's GCC side fills by name */
extern Float _nj_cnk_easy_multilight_buffer_dst_[32];
#endif

const Uint32 _nj_version = 0x00060068;
char *copyright = "SEGA Ninja Library";
char *version = "\nNinja Ver 0x00060068 Build:Nov 04 1998 17:17:49\n";
/* the vertical blanks njBlank has counted */
volatile Int _nj_blank_count_ = 0;
/* the list njInitVertexBuffer gave no buffer under Kamui's 2V latency:
 * NOBUF_OPAQUE or NOBUF_TRANS. it is NOBUF_NONE without 2V latency */
#define NOBUF_NONE 0
#define NOBUF_OPAQUE 1
#define NOBUF_TRANS 2
Int _nj_vertex_2v_type_ = NOBUF_NONE;

/* the handles of njSetVSyncFunction's and njBlank's handlers on the vertical
 * blank chain */
static SYCHAIN vsync_handle;
static SYCHAIN blank_handle;
/* njUserClipping's last rectangle, as the first and the last of the tiles it
 * covers, counted from 0. it starts as the 640 by 480 screen's: tiles 0 to 19
 * across and 0 to 14 down */
static NJS_POINT2 last_rect[2] = {{0.0F, 0.0F}, {19.0F, 14.0F}};

void njSetTextureMemorySize(Uint32 size)
{
    _nj_texture_memory_size = size;
}

/* set the texture memory's size, and give Kamui the system setup again with
 * it. _nj_tex_max_free_size then takes the total free texture memory */
void njChangeTextureMemorySize(Uint32 size)
{
    _nj_texture_memory_size = size;
    /* @hack spelling: the size read back from _nj_texture_memory_size, which
     * keeps the MOV R4,R3 at 0x8c657322 in 1ST_READ.BIN; as = size, the MOV
     * goes, the function is 2 bytes shorter and 17 of 17 words differ */
    _nj_SystemConfigStruct_.nTextureMemorySize = _nj_texture_memory_size;
    kmSetSystemConfiguration(&_nj_SystemConfigStruct_);
    _nj_tex_max_free_size = njCalcTexture(NJD_TEXMEM_FREESIZE);
}

/* make the size bytes at buffer the vertex buffer njInitVertexBuffer gives
 * Kamui, through its uncached address, in place of one from syMalloc. if the
 * vertex buffer needs more than size bytes, Kamui gets none */
/* NOLINTNEXTLINE(readability-non-const-parameter) */
void njSetVertexBuffer(Uint32 *buffer, Sint32 size)
{
    _nj_usr_vertex_buffer_ptr_ = (Uint32 *)((Uint32)buffer | P2);
    _nj_usr_vertex_buffer_size_ = size;
}

/* Kamui calls njSetEORflag at the end of each render */
void njSetEORflag(void *arg)
{
    _nj_eor_flag_ = 0;
    _nj_render_flag_ = 1;
}

/* Kamui calls njSetEOVflag at the end of the vertices */
void njSetEOVflag(void *arg)
{
    _nj_eov_flag_ = 0;
}

/* Kamui calls njSetERRflag on a fatal error, with its code as arg */
void njSetERRflag(void *arg)
{
    _nj_fatal_err_callback_ = (Int)arg;
}

/* njInitVertexBuffer's vertical blank handler, which counts each blank in
 * _nj_blank_count_. once a render has ended, it takes itself off the
 * chain */
void njBlank(void *arg)
{
    if (_nj_render_flag_) {
        syChainDeleteHandler(blank_handle);
    }
    _nj_blank_count_++;
}

/* wait for a vertical sync to end: the one under way, or else the next */
void waitVsync(void)
{
    while (!(SYNC_STATUS & SYNC_VERTICAL)) {
    }
    while (SYNC_STATUS & SYNC_VERTICAL) {
    }
}

/* set the video output for a cable. cable is a SYE_CBL_CABLE_ type moved into
 * AICA_VIDEO_CABLE by CABLE_SHIFT. any other bit cable holds is set in the
 * register too */
void njSetVideoMode(Uint32 cable)
{
    Uint32 value;

    /* @hack spelling: staged through value, which keeps the register's value in
     * R5 and its address in R6 (0x8c6573a0 to 0x8c6573ac in 1ST_READ.BIN); as
     * one expression, 6 of 7 words differ */
    value = AICA_VIDEO;
    value &= ~AICA_VIDEO_CABLE;
    cable |= value;
    AICA_VIDEO = cable;
}

void njInitSystem(Int mode, Int frame, Int count)
{
    KMVIDEOMODE video;
    KMDISPLAYMODE display;
    SYE_CBL_CABLE cable;
    KMBPPMODE bpp;
    KMBOOLEAN anti_alias;

    _nj_display_mode_ = mode;
    _nj_frame_mode_ = frame;
    _nj_waitvsync_count = count;
    video = mode & DSP_VIDEO;
    display = mode & NJD_DSP_MASK;
    if (mode & NJD_DSP_WIDTH_640) {
        _nj_screen_.w = 640.0F;
        _nj_frame_size_w = 640;
    } else {
        _nj_screen_.w = 320.0F;
        _nj_frame_size_w = 320;
    }
    if (mode & NJD_DSP_HEIGHT_480) {
        _nj_screen_.h = 480.0F;
        _nj_frame_size_h = 480;
    } else {
        _nj_screen_.h = 240.0F;
        _nj_frame_size_h = 240;
    }
    if (video == KM_VGA) {
        waitVsync();
        njSetVideoMode(SYE_CBL_CABLE_VGA << CABLE_SHIFT);
        _nj_screen_.w = 640.0F;
        _nj_frame_size_w = 640;
        _nj_screen_.h = 480.0F;
        _nj_frame_size_h = 480;
    } else {
        cable = syCblCheckCable();
        waitVsync();
        switch (cable) {
        case SYE_CBL_CABLE_RGB:
            njSetVideoMode(SYE_CBL_CABLE_RGB << CABLE_SHIFT);
            break;
        case SYE_CBL_CABLE_VBS:
            njSetVideoMode(SYE_CBL_CABLE_VBS << CABLE_SHIFT);
            break;
        case SYE_CBL_CABLE_RESERVED:
            njSetVideoMode(SYE_CBL_CABLE_RESERVED << CABLE_SHIFT);
            break;
        default:
            njSetVideoMode(SYE_CBL_CABLE_VBS << CABLE_SHIFT);
            break;
        }
    }
    if (mode & NJD_DSP_ANTI) {
        _nj_screen_.w *= 2.0F;
        anti_alias = TRUE;
        if (_nj_screen_.h == 240.0F) {
            _nj_screen_.h = 480.0F;
        }
    } else {
        anti_alias = FALSE;
    }
#ifdef GCC
    /* repairs the unset bpp the tag below names: for a format no case lists,
     * GCC passes -1, which kmSetDisplayMode rejects, setting no display
     * mode */
    bpp = (KMBPPMODE)-1;
#endif
    switch (frame) {
    case NJD_FRAMEBUFFER_MODE_RGB565:
        bpp = KM_DSPBPP_RGB565;
        break;
    case NJD_FRAMEBUFFER_MODE_RGB555:
        bpp = KM_DSPBPP_RGB555;
        break;
    case NJD_FRAMEBUFFER_MODE_ARGB1555:
        bpp = KM_DSPBPP_ARGB1555;
        break;
    case NJD_FRAMEBUFFER_MODE_RGB888:
        bpp = KM_DSPBPP_RGB888;
        break;
    case NJD_FRAMEBUFFER_MODE_ARGB8888:
        bpp = KM_DSPBPP_ARGB8888;
        break;
    }
    /* blank the picture, and start Kamui's device as a vertical blank starts */
    /* @hack spelling: VIDEO_CONTROL's or spelled out, which puts the
     * register's address in R5 (0x8c657564 in 1ST_READ.BIN); as |=, the
     * address takes R3, the frame sizes' addresses trade R10 and R11, and 12
     * of 232 words differ */
    VIDEO_CONTROL = VIDEO_CONTROL | VIDEO_BLANK;
    INT_STATUS = INT_VBLANK_IN;
    while (!(INT_STATUS & INT_VBLANK_IN)) {
    }
    kmInitDevice(video);
    /* @bug uninitialized: a frame buffer format no case lists, 2 among them,
     * leaves bpp unset. kmSetDisplayMode then takes as the format the address
     * of _nj_screen_ that R14 holds, rejects it and sets no display mode (the
     * default's BRA at 0x8c65752c, MOV R14,R5 at 0x8c657582 in 1ST_READ.BIN).
     * the calls in 1ST_READ.BIN and LOADER.BIN pass format 0 */
    /* NOLINTNEXTLINE(clang-diagnostic-sometimes-uninitialized) */
    kmSetDisplayMode(display, bpp, TRUE, anti_alias);
    _nj_SystemConfigStruct_.dwSize = sizeof(KMSYSTEMCONFIGSTRUCT);
    _nj_SystemConfigStruct_.flags = KM_CONFIGFLAG_ENABLE_CLEAR_FRAMEBUFFER;
    _nj_ppSurfaceArray[0] = (PKMSURFACEDESC)&_nj_framebuffer_surface0_;
    _nj_ppSurfaceArray[1] = (PKMSURFACEDESC)&_nj_framebuffer_surface1_;
    _nj_SystemConfigStruct_.ppSurfaceDescArray = _nj_ppSurfaceArray;
    _nj_SystemConfigStruct_.nWidth = 0;
    _nj_SystemConfigStruct_.nHeight = 0;
    _nj_SystemConfigStruct_.nTextureMemorySize = _nj_texture_memory_size;
    _nj_vertex_buffer_size_ = 0;
    _nj_SystemConfigStruct_.pBufferDesc = &_nj_vertex_buffer_;
    _nj_SystemConfigStruct_.pVertexBuffer = NULL;
    _nj_SystemConfigStruct_.nVertexBufferSize = 0;
    _nj_SystemConfigStruct_.fBufferSize[0] = 0.0F;
    _nj_SystemConfigStruct_.fBufferSize[1] = 0.0F;
    _nj_SystemConfigStruct_.fBufferSize[2] = 0.0F;
    _nj_SystemConfigStruct_.fBufferSize[3] = 0.0F;
    _nj_SystemConfigStruct_.fBufferSize[4] = 0.0F;
    _nj_render_flag_ = 0;
    _nj_eor_flag_ = 0;
    _nj_eov_flag_ = 1;
    _nj_fatal_err_callback_ = 0;
    kmSetEORCallback(njSetEORflag, NULL);
    kmSetEndOfVertexCallback(njSetEOVflag, NULL);
    kmfSetFatalErrorCallback(njSetERRflag);
    kmSetWaitVsyncCount(_nj_waitvsync_count);
    njInitGlobal();
}

void njChangeSystem(Int mode, Int frame, Int count)
{
    KMVIDEOMODE video;
    KMDISPLAYMODE display;
    SYE_CBL_CABLE cable;
    KMBPPMODE bpp;
    KMBOOLEAN anti_alias;

    _nj_frame_mode_ = frame;
    _nj_waitvsync_count = count;
    video = mode & DSP_VIDEO;
    display = mode & NJD_DSP_MASK;
    if (mode & NJD_DSP_WIDTH_640) {
        _nj_screen_.w = 640.0F;
        _nj_frame_size_w = 640;
    } else {
        _nj_screen_.w = 320.0F;
        _nj_frame_size_w = 320;
    }
    if (mode & NJD_DSP_HEIGHT_480) {
        _nj_screen_.h = 480.0F;
        _nj_frame_size_h = 480;
    } else {
        _nj_screen_.h = 240.0F;
        _nj_frame_size_h = 240;
    }
    if (video == KM_VGA) {
        waitVsync();
        njSetVideoMode(SYE_CBL_CABLE_VGA << CABLE_SHIFT);
        _nj_screen_.w = 640.0F;
        _nj_frame_size_w = 640;
        _nj_screen_.h = 480.0F;
        _nj_frame_size_h = 480;
    } else {
        cable = syCblCheckCable();
        waitVsync();
        switch (cable) {
        case SYE_CBL_CABLE_RGB:
            njSetVideoMode(SYE_CBL_CABLE_RGB << CABLE_SHIFT);
            break;
        case SYE_CBL_CABLE_VBS:
            njSetVideoMode(SYE_CBL_CABLE_VBS << CABLE_SHIFT);
            break;
        case SYE_CBL_CABLE_RESERVED:
            njSetVideoMode(SYE_CBL_CABLE_RESERVED << CABLE_SHIFT);
            break;
        default:
            njSetVideoMode(SYE_CBL_CABLE_VBS << CABLE_SHIFT);
            break;
        }
    }
    if (mode & NJD_DSP_ANTI) {
        _nj_screen_.w *= 2.0F;
        anti_alias = TRUE;
        if (_nj_screen_.h == 240.0F) {
            _nj_screen_.h = 480.0F;
        }
    } else {
        anti_alias = FALSE;
    }
#ifdef GCC
    /* repairs the unset bpp the tag below names: for a format no case lists,
     * GCC passes -1, which kmSetDisplayMode rejects, setting no display
     * mode */
    bpp = (KMBPPMODE)-1;
#endif
    switch (frame) {
    case NJD_FRAMEBUFFER_MODE_RGB565:
        bpp = KM_DSPBPP_RGB565;
        break;
    case NJD_FRAMEBUFFER_MODE_RGB555:
        bpp = KM_DSPBPP_RGB555;
        break;
    case NJD_FRAMEBUFFER_MODE_ARGB1555:
        bpp = KM_DSPBPP_ARGB1555;
        break;
    case NJD_FRAMEBUFFER_MODE_RGB888:
        bpp = KM_DSPBPP_RGB888;
        break;
    case NJD_FRAMEBUFFER_MODE_ARGB8888:
        bpp = KM_DSPBPP_ARGB8888;
        break;
    }
    _nj_SystemConfigStruct_.nTextureMemorySize = _nj_texture_memory_size;
    /* @bug uninitialized: a frame buffer format no case lists, 2 among them,
     * leaves bpp unset. kmSetDisplayMode then takes as the format what
     * njSetVideoMode left in R5: AICA_VIDEO's value with AICA_VIDEO_CABLE
     * clear (the default's BRA at 0x8c657798, the call at 0x8c6577da in
     * 1ST_READ.BIN). MOVIE.BIN's calls pass format 0 or 5 */
    /* NOLINTNEXTLINE(clang-diagnostic-sometimes-uninitialized) */
    kmSetDisplayMode(display, bpp, TRUE, anti_alias);
    kmSetWaitVsyncCount(_nj_waitvsync_count);
    _nj_render_flag_ = 0;
    _nj_eor_flag_ = 0;
    _nj_eov_flag_ = 1;
    _nj_fatal_err_callback_ = 0;
    /* @hack type: a plain 0, not NULL, which SHC keeps in R14 with the flags'
     * zero (0x8c6577e6 in 1ST_READ.BIN) and passes to both callbacks
     * (MOV R14,R5 at 0x8c657800 and 0x8c657808); as NULL, 23 of 181 words
     * differ */
    kmSetEORCallback(njSetEORflag, 0);
    kmSetEndOfVertexCallback(njSetEOVflag, NULL);
    kmfSetFatalErrorCallback(njSetERRflag);
    njChangeGlobal();
}

void njInitGlobal(void)
{
    NJS_POINT2 clip[2];
    Int i;

    _nj_calc_vertex_ = 0;
    _nj_calc_polygon_ = 0;
    _nj_draw_polygon_ = 0;
    _nj_screen_.cx = _nj_screen_.w / 2.0F;
    _nj_screen_.cy = _nj_screen_.h / 2.0F;
    _nj_scr_aspect_x_ = 1.0F;
    _nj_scr_aspect_y_ = 1.0F;
    _nj_screen_.dist = 500.0F;
    _nj_near_clip_ = 1.0F;
    _nj_far_clip_ = 60000.0F;
    _nj_control_3d_flag_ = 0;
    _nj_vsync_count_ = 0;
    _nj_motion_callback_ = NULL;
    /* @hack spelling: the corners through clip, on the stack; stored straight
     * into _nj_2d_clip_, the function is 30 bytes shorter and 138 of 164
     * words differ */
    clip[0].x = 0.0F;
    clip[0].y = 0.0F;
    clip[1].x = _nj_screen_.w - 1.0F;
    clip[1].y = _nj_screen_.h - 1.0F;
    _nj_2d_clip_[0].x = clip[0].x;
    _nj_2d_clip_[0].y = clip[0].y;
    _nj_2d_clip_[1].x = clip[1].x;
    _nj_2d_clip_[1].y = clip[1].y;
    njLightSystemInit();
    for (i = 0; i < NJD_CONTEXT_MAX; i++) {
        njInitContext(&_nj_context_[i]);
        _nj_ctx_color_mode_[i] = NJD_COLOR_MODE_NORMAL;
    }
    njInitContext(&_nj_background_ctx_);
    _nj_background_ctx_.RenderState = KM_SRCBLENDINGMODE | KM_DSTBLENDINGMODE;
    _nj_background_ctx_.SRCBlendingMode = KM_ONE;
    _nj_background_ctx_.DSTBlendingMode = KM_ZERO;
    kmProcessVertexRenderState(&_nj_background_ctx_);
    kmSetBackGroundRenderState(&_nj_background_ctx_);
    njInitSprite();
    kmSetCullingRegister(0.01F);
    njSetCurrentContext(0);
    {
        /* the easy chunk lights' defaults: 16 Floats for
         * _nj_cnk_easy_multilight_buffer_, then 32 more for
         * _nj_cnk_easy_multilight_buffer_dst_, which follows it in memory */
        Float lights[48] = {
            0.0F, 0.0F, -1.0F, 0.0F, 0.0F,     0.0F, 0.0F, 0.0F,
            0.0F, 0.0F, 0.0F,  0.0F, 0.0F,     0.2F, 0.2F, 0.2F,
            0.0F, 0.0F, 0.0F,  1.0F, 0.0F,     1.0F, 1.0F, 1.0F,
            0.0F, 0.0F, 0.0F,  1.0F, 65535.0F, 1.0F, 1.0F, 1.0F,
            0.0F, 0.0F, 0.0F,  1.0F, 65535.0F, 1.0F, 1.0F, 1.0F,
            0.0F, 0.0F, 0.0F,  1.0F, 65535.0F, 1.0F, 1.0F, 1.0F,
        };

        /* @bug memory: the copy writes 48 Floats through
         * _nj_cnk_easy_multilight_buffer_, an array of 16, so its last 32 land
         * in _nj_cnk_easy_multilight_buffer_dst_ (0x8c657980 in 1ST_READ.BIN).
         * the store after the loop puts 3 in dst's first word as index 16
         * (0x8c65798c) */
#ifdef GCC
        /* the copy's last 32 Floats go to _nj_cnk_easy_multilight_buffer_dst_
         * by name, where the tag says the image's land. the 3 then goes over
         * dst's first word, as in the image */
        for (i = 0; i < 16; i++) {
            _nj_cnk_easy_multilight_buffer_[i] = lights[i];
        }
        for (i = 0; i < 32; i++) {
            _nj_cnk_easy_multilight_buffer_dst_[i] = lights[16 + i];
        }
        *(Sint32 *)&_nj_cnk_easy_multilight_buffer_dst_[0] = 3;
#else
        for (i = 0; i < 48; i++) {
            _nj_cnk_easy_multilight_buffer_[i] = lights[i];
        }
        *(Sint32 *)&_nj_cnk_easy_multilight_buffer_[16] = 3;
#endif
    }
    vsync_handle = 0;
}

void njChangeGlobal(void)
{
    NJS_POINT2 clip[2];

    _nj_calc_vertex_ = 0;
    _nj_calc_polygon_ = 0;
    _nj_draw_polygon_ = 0;
    _nj_screen_.cx = _nj_screen_.w / 2.0F;
    _nj_screen_.cy = _nj_screen_.h / 2.0F;
    /* @hack spelling: the corners through clip, on the stack; stored straight
     * into _nj_2d_clip_, the function is 28 bytes shorter and 52 of 52 words
     * differ */
    clip[0].x = 0.0F;
    clip[0].y = 0.0F;
    clip[1].x = _nj_screen_.w - 1.0F;
    clip[1].y = _nj_screen_.h - 1.0F;
    _nj_2d_clip_[0].x = clip[0].x;
    _nj_2d_clip_[0].y = clip[0].y;
    _nj_2d_clip_[1].x = clip[1].x;
    _nj_2d_clip_[1].y = clip[1].y;
}

void njInitVertexBuffer(Sint32 op, Sint32 om, Sint32 tp, Sint32 tm, Sint32 pt)
{
    Int blanks;
    Uint32 *memory;
    Float total;
    Float op_percent;
    Float om_percent;
    Float tp_percent;
    Float tm_percent;
    Float pt_percent;

    _nj_vertex_2v_type_ = NOBUF_NONE;
    if (op < 0) {
        _nj_SystemConfigStruct_.flags = KM_CONFIGFLAG_ENABLE_2V_LATENCY;
        _nj_SystemConfigStruct_.nVbufModel = KM_VERTEXBUFMODEL_NOBUF_OPAQUE;
        op = 100;
        _nj_vertex_2v_type_ = NOBUF_OPAQUE;
    } else if (tp < 0 || pt < 0) {
        _nj_SystemConfigStruct_.flags = KM_CONFIGFLAG_ENABLE_2V_LATENCY;
        _nj_SystemConfigStruct_.nVbufModel = KM_VERTEXBUFMODEL_NOBUF_TRANS;
        tp = 100;
        pt = 0;
        _nj_vertex_2v_type_ = NOBUF_TRANS;
    } else {
        _nj_SystemConfigStruct_.flags = KM_CONFIGFLAG_NOWAIT_FINISH_TEXTUREDMA;
        _nj_SystemConfigStruct_.nVbufModel = KM_VERTEXBUFMODEL_NORMAL;
    }
    tp += pt;
    _nj_vertex_buffer_op_ = op;
    _nj_vertex_buffer_om_ = om;
    _nj_vertex_buffer_tp_ = tp;
    _nj_vertex_buffer_tm_ = tm;
    _nj_vertex_buffer_pt_ = 0;
    _nj_vertex_buffer_size_ = op + om + tp + tm;
    memory = _nj_SystemConfigStruct_.pVertexBuffer;
    /* Kamui gets a buffer from syMalloc by its uncached address. syFree takes
     * the cached address syMalloc returned */
    if (_nj_usr_vertex_buffer_ptr_ == NULL) {
        if (memory) {
            syFree((void *)((Uint32)memory & P2_TO_P1));
        }
        /* @hack spelling: the total converted in each arm (0x8c657b52,
         * 0x8c657b90, 0x8c657bb4 in 1ST_READ.BIN); converted once, after the
         * arms, the function is 66 bytes shorter and 172 of 205 words differ */
        total = (Float)_nj_vertex_buffer_size_;
        _nj_SystemConfigStruct_.pVertexBuffer =
            (Uint32 *)((Uint32)syMalloc((_nj_vertex_buffer_size_ * 4) +
                                        (BUFFER_EXTRA * 4)) |
                       P1_TO_P2);
        _nj_SystemConfigStruct_.nVertexBufferSize =
            _nj_vertex_buffer_size_ + BUFFER_EXTRA;
    } else if (_nj_usr_vertex_buffer_size_ >=
               (_nj_vertex_buffer_size_ * 4) + (BUFFER_EXTRA * 4)) {
        /* @hack redundant: an empty test of memory, which keeps TST R10,R10 at
         * 0x8c657b8c in 1ST_READ.BIN; without it, 113 of 205 words differ */
        if (memory) {
        }
        total = (Float)_nj_vertex_buffer_size_;
        _nj_SystemConfigStruct_.pVertexBuffer = _nj_usr_vertex_buffer_ptr_;
        _nj_SystemConfigStruct_.nVertexBufferSize =
            _nj_vertex_buffer_size_ + BUFFER_EXTRA;
    } else {
        /* @hack redundant: an empty test of memory, which keeps TST R10,R10 at
         * 0x8c657bb0 in 1ST_READ.BIN; without it, 98 of 205 words differ */
        if (memory) {
        }
        total = (Float)_nj_vertex_buffer_size_;
        _nj_SystemConfigStruct_.pVertexBuffer = NULL;
        _nj_SystemConfigStruct_.nVertexBufferSize = 0;
    }
    /* @hack spelling: each list's percentage of the buffer through locals,
     * which interleave the divides and the stores (0x8c657c14 to 0x8c657c50 in
     * 1ST_READ.BIN); stored as they are made, the function is 8 bytes shorter
     * and 64 of 205 words differ */
    op_percent = (Float)op * 100.0F / total;
    om_percent = (Float)om * 100.0F / total;
    tp_percent = (Float)tp * 100.0F / total;
    tm_percent = (Float)tm * 100.0F / total;
    /* tp holds the punch-through list's words with the translucent list's,
     * so the punch-through list's percent is 0 */
    /* @hack spelling: its percent as 0 over the total, which keeps the FDIV at
     * 0x8c657c42 in 1ST_READ.BIN. 0.0F / total is 0.0F for any total but 0,
     * which makes every percent NaN. as 0.0F alone, the FDIV goes, and 43 of
     * 205 words differ */
    pt_percent = 0.0F / total;
    _nj_SystemConfigStruct_.fBufferSize[0] = op_percent;
    _nj_SystemConfigStruct_.fBufferSize[1] = om_percent;
    _nj_SystemConfigStruct_.fBufferSize[2] = tp_percent;
    _nj_SystemConfigStruct_.fBufferSize[3] = tm_percent;
    _nj_SystemConfigStruct_.fBufferSize[4] = pt_percent;
    /* put njBlank on the vertical blank chain, and wait for it to count a
     * blank */
    blank_handle = syChainAddHandler(SYD_CHAIN_EVENT_IML6_VBLANK, njBlank,
                                     VBLANK_PRIORITY, NULL);
    blanks = _nj_blank_count_;
    while (blanks == _nj_blank_count_) {
    }
    kmSetSystemConfiguration(&_nj_SystemConfigStruct_);
    _nj_tex_max_free_size = njCalcTexture(NJD_TEXMEM_FREESIZE);
}

void njSetCurrentContext(Int n)
{
    _nj_current_ctx_ = &_nj_context_[n];
    kmSetVertexRenderState(_nj_current_ctx_);
    _nj_color_mode_ = &_nj_ctx_color_mode_[n];
}

/* the RenderState njInitContext marks for kmProcessVertexRenderState: every
 * bit from KM_DEPTHMODE to KM_DSTSELECT but KM_COLORCLAMP and KM_PALETTEBANK.
 * it sets bColorClamp and PaletteBank too, unmarked */
#define INIT_RENDER_STATE                                                      \
    (((KM_DSTSELECT << 1) - KM_DEPTHMODE) & ~(KM_COLORCLAMP | KM_PALETTEBANK))

void njInitContext(KMVERTEXCONTEXT *context)
{
    context->RenderState = INIT_RENDER_STATE;
    context->ParamType = KM_POLYGON;
    context->ListType = KM_OPAQUE_POLYGON;
    context->ColorType = KM_FLOATINGCOLOR;
    context->UVFormat = KM_32BITUV;
    context->DepthMode = KM_GREATEREQUAL;
    context->CullingMode = KM_CULLSMALL;
    context->ScreenCoordination = KM_SCREEN;
    context->ShadingMode = KM_NOTEXTUREGOURAUD;
    context->SelectModifier = KM_NOMODIFIER;
    context->bZWriteDisable = FALSE;
    context->SRCBlendingMode = KM_SRCALPHA;
    context->DSTBlendingMode = KM_INVSRCALPHA;
    context->bSRCSel = FALSE;
    context->bDSTSel = FALSE;
    context->FogMode = KM_NOFOG;
    context->bUseSpecular = FALSE;
    context->bUseAlpha = FALSE;
    context->bIgnoreTextureAlpha = FALSE;
    context->FlipUV = KM_NOFLIP;
    context->ClampUV = KM_NOCLAMP;
    context->FilterMode = KM_POINT_SAMPLE;
    context->bSuperSample = FALSE;
    context->MipMapAdjust = KM_MIPMAP_D_ADJUST_1_00;
    context->TextureShadingMode = KM_MODULATE_ALPHA;
    context->pTextureSurfaceDesc = NULL;
    context->bColorClamp = FALSE;
    context->PaletteBank = 0;
    context->fFaceColorAlpha = 1.0F;
    context->fFaceColorRed = 1.0F;
    context->fFaceColorGreen = 1.0F;
    context->fFaceColorBlue = 1.0F;
    context->fOffsetColorAlpha = 1.0F;
    context->fOffsetColorRed = 0.0F;
    context->fOffsetColorGreen = 0.0F;
    context->fOffsetColorBlue = 0.0F;
    kmProcessVertexRenderState(context);
}

void njSetEORFunction(void (*func)(void))
{
    _nj_eor_function_ = func;
}

void njExeEORFunction(void)
{
    if (_nj_eor_function_) {
        _nj_eor_function_();
    }
}

void njWaitVSync(void)
{
    Uint32 elapsed;

    elapsed = 0;
    _nj_wait_vsync_in_ = syTmrGetCount();
    if (_nj_render_flag_) {
        while (_nj_eor_flag_ && elapsed < RENDER_WAIT_COUNT) {
            elapsed = syTmrDiffCount(_nj_wait_vsync_in_, syTmrGetCount());
            /* @bug logic: elapsed holds a timer count, which
             * syTmrMicroToCount converts as if it were microseconds. the wait
             * then reaches RENDER_WAIT_COUNT at a count of 212480, about 272 ms
             * (0x8c657de0 in 1ST_READ.BIN) */
            elapsed = syTmrMicroToCount(elapsed);
        }
        if (elapsed < RENDER_WAIT_COUNT) {
            njExeEORFunction();
        }
    }
    _nj_eor_flag_ = 1;
    kmRender();
    _nj_calc_vertex_ = 0;
    _nj_calc_polygon_ = 0;
    _nj_draw_polygon_ = 0;
    _nj_vsync_count_++;
    kmFlipFrameBuffer();
    pdExecPeripheralServer();
    _nj_wait_vsync_out_ = syTmrGetCount();
}

void njSetVSyncFunction(void (*func)(void))
{
    if (vsync_handle) {
        syChainDeleteHandler(vsync_handle);
    }
    if (func) {
        /* @bug mismatched-call: func takes no argument, but the chain calls it
         * as a handler, with the NULL registered here in R4, which func ignores
         * (MOV #0,R7 at 0x8c657e5a and the call's MOV.L @(8,R14),R4 at
         * 0x8c65f26e in 1ST_READ.BIN, 0x8ce0a9e6 and 0x8ce0c942 in
         * LOADER.BIN) */
        vsync_handle =
            syChainAddHandler(SYD_CHAIN_EVENT_IML6_VBLANK,
                              (void (*)(void *))func, VBLANK_PRIORITY, NULL);
    }
}

void njSetBackColor(Uint32 col1, Uint32 col2, Uint32 col3)
{
    KMVERTEX0 vertex[3];
    PVOID plane[3];

    vertex[0].ParamControlWord = KM_VERTEXPARAM_NORMAL;
    vertex[0].fX = 0.0F;
    vertex[0].fY = 0.0F;
    vertex[0].u.fZ = 0.00001F;
    vertex[0].uBaseRGB.dwPacked = col1;
    vertex[1].ParamControlWord = KM_VERTEXPARAM_NORMAL;
    vertex[1].fX = _nj_screen_.w;
    vertex[1].fY = 0.0F;
    vertex[1].u.fZ = 0.00001F;
    vertex[1].uBaseRGB.dwPacked = col2;
    vertex[2].ParamControlWord = KM_VERTEXPARAM_ENDOFSTRIP;
    vertex[2].fX = 0.0F;
    vertex[2].fY = _nj_screen_.h;
    vertex[2].u.fZ = 0.00001F;
    vertex[2].uBaseRGB.dwPacked = col3;
    plane[0] = &vertex[0];
    plane[1] = &vertex[1];
    plane[2] = &vertex[2];
    kmSetBackGroundPlane(plane, KM_VERTEXTYPE_00, sizeof(KMVERTEX0));
}

Uint32 njVersion(void)
{
    return _nj_version;
}

void njExitSystem(void)
{
    kmDiscardVertexBuffer(&_nj_vertex_buffer_);
    if (_nj_usr_vertex_buffer_ptr_ == NULL) {
        syFree(
            (void *)((Uint32)_nj_SystemConfigStruct_.pVertexBuffer & P2_TO_P1));
    }
}

/* the bits of a color mode in the polygon header's global word, and in its TSP
 * word */
#define MODE_GLOBAL 0x020000ff
#define MODE_TSP 0x00180000

void njSetColorMode(Uint32 mode)
{
    Uint32 global;
    Uint32 isp;
    Uint32 tsp;

    if (*_nj_color_mode_ != mode) {
        /* @hack spelling: the global and TSP words read and masked, and the ISP
         * word read, before the mode goes in (0x8c657f6e to 0x8c657f96 in
         * 1ST_READ.BIN); merged as each is read, 21 of 48 words differ */
        global = _nj_current_ctx_->GLOBALPARAMBUFFER & ~MODE_GLOBAL;
        isp = _nj_current_ctx_->ISPPARAMBUFFER;
        tsp = _nj_current_ctx_->TSPPARAMBUFFER & ~MODE_TSP;
        global |= mode & MODE_GLOBAL;
        tsp |= mode & MODE_TSP;
        _nj_current_ctx_->GLOBALPARAMBUFFER = global;
        /* @hack redundant: the ISP word read and written back unchanged;
         * without the read and the write, the function is 12 bytes shorter
         * and 38 of 48 words differ */
        _nj_current_ctx_->ISPPARAMBUFFER = isp;
        _nj_current_ctx_->TSPPARAMBUFFER = tsp;
        kmSetVertexRenderState(_nj_current_ctx_);
        *_nj_color_mode_ = mode;
    }
}

Uint32 njGetColorMode(void)
{
    return *_nj_color_mode_;
}

void njSetIntensityColor(NJS_ARGB *base, NJS_ARGB *offset)
{
    _nj_current_ctx_->fFaceColorAlpha = base->a;
    _nj_current_ctx_->fFaceColorRed = base->r;
    _nj_current_ctx_->fFaceColorGreen = base->g;
    _nj_current_ctx_->fFaceColorBlue = base->b;
    _nj_current_ctx_->fOffsetColorAlpha = offset->a;
    _nj_current_ctx_->fOffsetColorRed = offset->r;
    _nj_current_ctx_->fOffsetColorGreen = offset->g;
    _nj_current_ctx_->fOffsetColorBlue = offset->b;
    kmSetVertexRenderState(_nj_current_ctx_);
}

void njZBufferMode(Int mode)
{
    _nj_current_ctx_->RenderState = KM_DEPTHMODE;
    switch (mode) {
    case 1:
        _nj_current_ctx_->DepthMode = KM_GREATEREQUAL;
        break;
    case 0:
        _nj_current_ctx_->DepthMode = KM_ALWAYS;
        break;
    }
    kmProcessVertexRenderState(_nj_current_ctx_);
    kmSetVertexRenderState(_nj_current_ctx_);
}

void njMipmapAdjust(Int level)
{
    _nj_current_ctx_->RenderState = KM_MIPMAPDADJUST;
    _nj_current_ctx_->MipMapAdjust = level;
    kmProcessVertexRenderState(_nj_current_ctx_);
    kmSetVertexRenderState(_nj_current_ctx_);
}

void njTextureFilterMode(Int mode)
{
    _nj_current_ctx_->RenderState = KM_FILTERMODE;
    switch (mode) {
    case NJD_TEXTUREFILTER_POINT_SAMPLE:
        _nj_current_ctx_->FilterMode = KM_POINT_SAMPLE;
        break;
    case NJD_TEXTUREFILTER_BILINEAR:
        _nj_current_ctx_->FilterMode = KM_BILINEAR;
        break;
    case NJD_TEXTUREFILTER_TRILINEAR:
        _nj_current_ctx_->FilterMode = KM_TRILINEAR_A;
        break;
    }
    kmProcessVertexRenderState(_nj_current_ctx_);
    kmSetVertexRenderState(_nj_current_ctx_);
}

void njSuperSampleMode(Int mode)
{
    _nj_current_ctx_->RenderState = KM_SUPERSAMPLE;
    switch (mode) {
    case 1:
        _nj_current_ctx_->bSuperSample = TRUE;
        break;
    case 0:
        _nj_current_ctx_->bSuperSample = FALSE;
        break;
    }
    kmProcessVertexRenderState(_nj_current_ctx_);
    kmSetVertexRenderState(_nj_current_ctx_);
}

void njTextureClampMode(Int mode)
{
    _nj_current_ctx_->RenderState = KM_CLAMPUV;
    switch (mode) {
    case NJD_TEXTURECLAMP_NOCLAMP:
        _nj_current_ctx_->ClampUV = KM_NOCLAMP;
        break;
    case NJD_TEXTURECLAMP_CLAMP_V:
        _nj_current_ctx_->ClampUV = KM_CLAMP_V;
        break;
    case NJD_TEXTURECLAMP_CLAMP_U:
        _nj_current_ctx_->ClampUV = KM_CLAMP_U;
        break;
    case NJD_TEXTURECLAMP_CLAMP_UV:
        _nj_current_ctx_->ClampUV = KM_CLAMP_UV;
        break;
    }
    kmProcessVertexRenderState(_nj_current_ctx_);
    kmSetVertexRenderState(_nj_current_ctx_);
}

void njTextureFlipMode(Int mode)
{
    _nj_current_ctx_->RenderState = KM_FLIPUV;
    switch (mode) {
    case NJD_TEXTUREFLIP_NOFLIP:
        _nj_current_ctx_->FlipUV = KM_NOFLIP;
        break;
    case NJD_TEXTUREFLIP_FLIP_V:
        _nj_current_ctx_->FlipUV = KM_FLIP_V;
        break;
    case NJD_TEXTUREFLIP_FLIP_U:
        _nj_current_ctx_->FlipUV = KM_FLIP_U;
        break;
    case NJD_TEXTUREFLIP_FLIP_UV:
        _nj_current_ctx_->FlipUV = KM_FLIP_UV;
        break;
    }
    kmProcessVertexRenderState(_nj_current_ctx_);
    kmSetVertexRenderState(_nj_current_ctx_);
}

void njPolygonCullingSize(Float size)
{
    kmSetCullingRegister(size);
}

void njPolygonCullingMode(Int mode)
{
    _nj_current_ctx_->RenderState = KM_CULLINGMODE;
    switch (mode) {
    case NJD_POLYGON_NOCULLING:
        _nj_current_ctx_->CullingMode = KM_NOCULLING;
        break;
    case NJD_POLYGON_CULLINGSMALL:
        _nj_current_ctx_->CullingMode = KM_CULLSMALL;
        break;
    case NJD_POLYGON_CULLINGACW:
        _nj_current_ctx_->CullingMode = KM_CULLCCW;
        break;
    case NJD_POLYGON_CULLINGCW:
        _nj_current_ctx_->CullingMode = KM_CULLCW;
        break;
    }
    kmProcessVertexRenderState(_nj_current_ctx_);
    kmSetVertexRenderState(_nj_current_ctx_);
}

void njColorBlendingMode(Int target, Int mode)
{
    switch (mode) {
    case NJD_COLOR_BLENDING_BOTHINVALPHA:
        mode = KM_BOTHINVSRCALPHA;
        break;
    case NJD_COLOR_BLENDING_BOTHSRCALPHA:
        mode = KM_BOTHSRCALPHA;
        break;
    case NJD_COLOR_BLENDING_DESTALPHA:
        mode = KM_DESTALPHA;
        break;
    case NJD_COLOR_BLENDING_DESTCOLOR:
        mode = KM_DESTCOLOR;
        break;
    case NJD_COLOR_BLENDING_INVDESTALPHA:
        mode = KM_INVDESTALPHA;
        break;
    case NJD_COLOR_BLENDING_INVDESTCOLOR:
        mode = KM_INVDESTCOLOR;
        break;
    case NJD_COLOR_BLENDING_INVSRCALPHA:
        mode = KM_INVSRCALPHA;
        break;
    case NJD_COLOR_BLENDING_INVSRCCOLOR:
        mode = KM_INVSRCCOLOR;
        break;
    case NJD_COLOR_BLENDING_SRCALPHA:
        mode = KM_SRCALPHA;
        break;
    case NJD_COLOR_BLENDING_SRCCOLOR:
        mode = KM_SRCCOLOR;
        break;
    case NJD_COLOR_BLENDING_ONE:
        mode = KM_ONE;
        break;
    case NJD_COLOR_BLENDING_ZERO:
        mode = KM_ZERO;
        break;
    }
    if (target == NJD_SOURCE_COLOR) {
        _nj_current_ctx_->RenderState = KM_SRCBLENDINGMODE;
        _nj_current_ctx_->SRCBlendingMode = mode;
    }
    if (target == NJD_DESTINATION_COLOR) {
        _nj_current_ctx_->RenderState = KM_DSTBLENDINGMODE;
        _nj_current_ctx_->DSTBlendingMode = mode;
    }
    kmProcessVertexRenderState(_nj_current_ctx_);
    kmSetVertexRenderState(_nj_current_ctx_);
}

void njSpecularMode(Int mode)
{
    _nj_current_ctx_->RenderState = KM_USESPECULAR;
    switch (mode) {
    case 1:
        _nj_current_ctx_->bUseSpecular = TRUE;
        break;
    case 0:
        _nj_current_ctx_->bUseSpecular = FALSE;
        break;
    }
    kmProcessVertexRenderState(_nj_current_ctx_);
    kmSetVertexRenderState(_nj_current_ctx_);
}

void njAlphaMode(Int mode)
{
    _nj_current_ctx_->RenderState = KM_USEALPHA;
    switch (mode) {
    case 1:
        _nj_current_ctx_->bUseAlpha = TRUE;
        break;
    case 0:
        _nj_current_ctx_->bUseAlpha = FALSE;
        break;
    }
    kmProcessVertexRenderState(_nj_current_ctx_);
    kmSetVertexRenderState(_nj_current_ctx_);
}

void njIgnoreTextureAlphaMode(Int mode)
{
    _nj_current_ctx_->RenderState = KM_IGNORETEXTUREALPHA;
    switch (mode) {
    case 1:
        _nj_current_ctx_->bIgnoreTextureAlpha = TRUE;
        break;
    case 0:
        _nj_current_ctx_->bIgnoreTextureAlpha = FALSE;
        break;
    }
    kmProcessVertexRenderState(_nj_current_ctx_);
    kmSetVertexRenderState(_nj_current_ctx_);
}

void njTextureShadingMode(Int mode)
{
    _nj_current_ctx_->RenderState = KM_TEXTURESHADINGMODE;
    switch (mode) {
    case NJD_TEX_SHADING_MODE_DECAL:
        _nj_current_ctx_->TextureShadingMode = KM_DECAL;
        break;
    case NJD_TEX_SHADING_MODE_MODULATE:
        _nj_current_ctx_->TextureShadingMode = KM_MODULATE;
        break;
    case NJD_TEX_SHADING_MODE_DECALALPHA:
        _nj_current_ctx_->TextureShadingMode = KM_DECAL_ALPHA;
        break;
    case NJD_TEX_SHADING_MODE_MODULATEALPHA:
        _nj_current_ctx_->TextureShadingMode = KM_MODULATE_ALPHA;
        break;
    }
    kmProcessVertexRenderState(_nj_current_ctx_);
    kmSetVertexRenderState(_nj_current_ctx_);
}

void njSetCheapShadowMode(Int mode)
{
    kmSetCheapShadowMode(mode);
}

void njUserClipping(Int mode, NJS_POINT2 *p)
{
    Uint32 *vertex;

    /* under 2V latency, the list with no buffer takes an empty polygon before
     * the clip rectangles */
    if (_nj_vertex_2v_type_ == NOBUF_OPAQUE) {
        njGetkmPointer();
        njSetkmColorMode(NJD_COLOR_MODE_NORMAL);
        /* @hack spelling: njStartVertexNonTexOpaque's result through
         * vertex; nested in njEndVertex's call, registers and the clip calls'
         * stack slots trade (0x8c658470 on, in 1ST_READ.BIN), and 6 of 400
         * words differ */
        vertex = njStartVertexNonTexOpaque();
        njEndVertex(vertex);
        njSetkmPointer();
    } else if (_nj_vertex_2v_type_ == NOBUF_TRANS) {
        njGetkmPointer();
        njSetkmColorMode(NJD_COLOR_MODE_NORMAL_TRANS);
        /* @hack spelling: njStartVertexNonTexTrans's result through vertex,
         * as above; nested, 6 of 400 words differ */
        vertex = njStartVertexNonTexTrans();
        njEndVertex(vertex);
        njSetkmPointer();
    }
    /* each list with words in the vertex buffer takes the last rectangle
     * again, and then p's, under the new mode */
    if (_nj_vertex_buffer_op_) {
        kmSetUserClipping(&_nj_vertex_buffer_, KM_OPAQUE_POLYGON,
                          last_rect[0].x, last_rect[0].y, last_rect[1].x,
                          last_rect[1].y);
    }
    if (_nj_vertex_buffer_om_) {
        kmSetUserClipping(&_nj_vertex_buffer_, KM_OPAQUE_MODIFIER,
                          last_rect[0].x, last_rect[0].y, last_rect[1].x,
                          last_rect[1].y);
    }
    if (_nj_vertex_buffer_tp_) {
        kmSetUserClipping(&_nj_vertex_buffer_, KM_TRANS_POLYGON, last_rect[0].x,
                          last_rect[0].y, last_rect[1].x, last_rect[1].y);
    }
    if (_nj_vertex_buffer_tm_) {
        kmSetUserClipping(&_nj_vertex_buffer_, KM_TRANS_MODIFIER,
                          last_rect[0].x, last_rect[0].y, last_rect[1].x,
                          last_rect[1].y);
    }
    if (_nj_vertex_buffer_pt_) {
        kmSetUserClipping(&_nj_vertex_buffer_, KM_PUNCHTHROUGH_POLYGON,
                          last_rect[0].x, last_rect[0].y, last_rect[1].x,
                          last_rect[1].y);
    }
    _nj_current_ctx_->RenderState = KM_USERCLIPMODE;
    _nj_current_ctx_->UserClipMode = mode;
    kmProcessVertexRenderState(_nj_current_ctx_);
    kmSetVertexRenderState(_nj_current_ctx_);
    if (_nj_vertex_buffer_op_) {
        kmSetUserClipping(&_nj_vertex_buffer_, KM_OPAQUE_POLYGON, p[0].x,
                          p[0].y, p[1].x, p[1].y);
    }
    if (_nj_vertex_buffer_om_) {
        kmSetUserClipping(&_nj_vertex_buffer_, KM_OPAQUE_MODIFIER, p[0].x,
                          p[0].y, p[1].x, p[1].y);
    }
    if (_nj_vertex_buffer_tp_) {
        kmSetUserClipping(&_nj_vertex_buffer_, KM_TRANS_POLYGON, p[0].x, p[0].y,
                          p[1].x, p[1].y);
    }
    if (_nj_vertex_buffer_tm_) {
        kmSetUserClipping(&_nj_vertex_buffer_, KM_TRANS_MODIFIER, p[0].x,
                          p[0].y, p[1].x, p[1].y);
    }
    if (_nj_vertex_buffer_pt_) {
        kmSetUserClipping(&_nj_vertex_buffer_, KM_PUNCHTHROUGH_POLYGON, p[0].x,
                          p[0].y, p[1].x, p[1].y);
    }
    last_rect[0] = p[0];
    last_rect[1] = p[1];
    if (mode == NJD_CLIP_INSIDE) {
        _nj_2d_clip_[0].x = p[0].x * TILE_SIZE;
        _nj_2d_clip_[0].y = p[0].y * TILE_SIZE;
        _nj_2d_clip_[1].x = (p[1].x * TILE_SIZE) + (TILE_SIZE - 1.0F);
        _nj_2d_clip_[1].y = (p[1].y * TILE_SIZE) + (TILE_SIZE - 1.0F);
    } else {
        _nj_2d_clip_[0].x = 0.0F;
        _nj_2d_clip_[0].y = 0.0F;
        _nj_2d_clip_[1].x = _nj_screen_.w - 1.0F;
        _nj_2d_clip_[1].y = _nj_screen_.h - 1.0F;
    }
}

void njGetSystemAttr(NJS_SYS_ATTR *attr)
{
    njMemCopy(attr, _nj_current_ctx_, sizeof(KMVERTEXCONTEXT));
}

void njSetSystemAttr(NJS_SYS_ATTR *attr)
{
    njMemCopy(_nj_current_ctx_, attr, sizeof(KMVERTEXCONTEXT));
    kmSetVertexRenderState(_nj_current_ctx_);
}

void njStopDisplay(void)
{
    kmStopDisplayFrameBuffer();
}

void njColorClampMode(Int mode)
{
    _nj_current_ctx_->RenderState = KM_COLORCLAMP;
    switch (mode) {
    case 1:
        _nj_current_ctx_->bColorClamp = TRUE;
        break;
    case 0:
        _nj_current_ctx_->bColorClamp = FALSE;
        break;
    }
    kmProcessVertexRenderState(_nj_current_ctx_);
    kmSetVertexRenderState(_nj_current_ctx_);
}

void njSetColorClamp(Uint32 max, Uint32 min)
{
    KMPACKEDARGB high;
    KMPACKEDARGB low;

    high.dwPacked = max;
    low.dwPacked = min;
    kmSetColorClampValue(high, low);
}

void njAdjustDisplay(Sint32 x, Sint32 y)
{
    kmAdjustDisplayCenter(x, y);
}

void njSetBorderColor(Uint32 color)
{
    /* @hack type: color read back through its own address, from the stack slot
     * it is stored to (MOV R15,R2 and MOV.L @R2,R3 at 0x8c65885a in
     * 1ST_READ.BIN); with a KMPACKEDARGB local, 12 of 12 words differ */
    kmSetBorderColor(*(KMPACKEDARGB *)&color);
}
