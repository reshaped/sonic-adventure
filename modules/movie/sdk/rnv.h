/* the movie renderer, which loads decoded pictures into textures and draws
 * each as a textured strip through Kamui */
#ifndef MOVIE_SDK_RNV_H
#define MOVIE_SDK_RNV_H
#include <kamui.h>
#include <ninja.h>
#include <sg_xpt.h>
#include <sj.h>

/* how mwRnvCalcDef places the movie: by its size, or stretched over the whole
 * screen. any value but MWD_RNV_DTYPE_FULL places it by its size */
#define MWD_RNV_DTYPE_AUTO 0
#define MWD_RNV_DTYPE_FULL 1

/* the draw callback's results for the frame it is offered. on WAIT, the
 * renderer puts the frame back to offer again, and draws the frame on screen
 * once more. on SKIP, it counts the frame drawn without drawing it, and offers
 * the next at once. on OK, or any other value, it draws the frame */
#define MWD_RNV_DRAWCB_OK 0
#define MWD_RNV_DRAWCB_WAIT 1
#define MWD_RNV_DRAWCB_SKIP 2

#define RNV_NTXFRM 6 /* the texture frames */
/* the drawn frames sj_dsp holds. drawing a frame while it holds RNV_NDSP frees
 * the oldest */
#define RNV_NDSP 2

/* a texture frame: a surface, and the picture last loaded into it, with its
 * number and its time in units of 1/tunit second */
struct rnv_txfrm {
    NJS_TEXSURFACE *srf;
    Sint32 dfno; /* the count of pictures loaded before it since mwRnvCreate */
    Sint32 time;
    Sint32 tunit;
};

/* the renderer's state. the draw callback takes draw_obj, and the display
 * callback disp_obj, each with the frame's time, tunit and dfno */
struct rnv_obj {
    Uint8 used; /* TRUE from mwRnvCreate to mwRnvDestroy */
    Sint32 stat;
    Sint32 unk_08;
    NJS_TEXSURFACE *srf; /* the ntex surfaces, mwg_MPEGTexSurfaceDesc */
    /* the sj_tex chunk drawn or skipped last, or zeros before the first */
    SJCK cur;
    Sint32 ntex;
    /* the movie's size in macroblocks and in pixels, then its textures' size,
     * rounded up to powers of two */
    Sint32 nmbx;
    Sint32 nmby;
    Sint32 isx;
    Sint32 isy;
    Sint32 tw;
    Sint32 th;
    /* the screen size, then the movie's position and size on it */
    Float ssx;
    Float ssy;
    Float plx;
    Float ply;
    Float psx;
    Float psy;
    /* the part of the texture shown, tsu by tsv from tu, tv, in texture
     * coordinates. plz is the strip's depth */
    Float tu;
    Float tv;
    Float tsu;
    Float tsv;
    Float plz;
    /* the strip's base and offset colors */
    KMPACKEDARGB bright;
    KMPACKEDARGB bright_ofst;
    Sint32 (*draw_fn)(void *obj, Sint32 time, Sint32 tunit, Sint32 dfno);
    void *draw_obj;
    void (*disp_fn)(void *obj, Sint32 time, Sint32 tunit, Sint32 dfno);
    void *disp_obj;
    /* the strip's corners: bottom left, top left, bottom right, top right */
    KMVERTEX3 vtx[4];
    Sint32 dfno; /* the number the next picture loaded takes */
    /* a joint of rnv_txfrm records: free texture frames on its free line, and
     * frames loaded and waiting to be drawn on its data line. a drawn frame is
     * on neither until sj_dsp frees it */
    SJ sj_tex;
    /* a joint of the sj_tex chunks drawn, as SJCKs, on its data line */
    SJ sj_dsp;
    /* the areas of sj_tex's and sj_dsp's rings */
    struct rnv_txfrm txfrm[RNV_NTXFRM];
    SJCK dsp[RNV_NDSP];
};

/* mwRnvCreate's parameters: the movie's size in macroblocks, and an
 * MWD_RNV_DTYPE_* placement */
struct rnv_cprm {
    Sint32 nmbx;
    Sint32 nmby;
    Sint32 dtype;
};

/* the renderer's life and its server. there is one renderer, mwg_rnv_obj,
 * which mwRnvCreate returns */
void mwRnvInit(void);
void mwRnvFinish(void);
struct rnv_obj *mwRnvCreate(void *reserved, struct rnv_cprm *cprm);
void mwRnvDestroy(struct rnv_obj *rnv);
void mwRnvStart(struct rnv_obj *rnv);
void mwRnvStop(struct rnv_obj *rnv);
void mwRnvExecServer(void);
void mwRnvLoad(struct rnv_obj *rnv, Uint32 *src, Sint32 time, Sint32 tunit);
void mwRnvEntryDrawCallbackFn(struct rnv_obj *rnv,
                              Sint32 (*fn)(void *obj, Sint32 time, Sint32 tunit,
                                           Sint32 dfno),
                              void *obj);
void mwRnvEntryDispCallbackFn(struct rnv_obj *rnv,
                              void (*fn)(void *obj, Sint32 time, Sint32 tunit,
                                         Sint32 dfno),
                              void *obj);

/* drawing the renderer's frames through Kamui */
void mwRnvInitVertexContextOpaq(KMVERTEXCONTEXT *ctx);
void mwRnvInitDisp(void);
void mwRnvSetVertexBuffer(KMVERTEXBUFFDESC *vtxbuf);
void mwRnvSetBilinearFilterSw(Sint32 sw);
Sint32 mwRnvGetBilinearFilterSw(void);
void mwRnvCalcDef(struct rnv_obj *rnv, Sint32 dtype);
void mwRnvCalcVtex(struct rnv_obj *rnv);
void mwRnvSetDispPos(struct rnv_obj *rnv, Float lx, Float ly);
void mwRnvSetDispSize(struct rnv_obj *rnv, Float sx, Float sy);
/* mwRnvSetBright sets the strip's base color, which multiplies the texture,
 * and mwRnvSetBrightOfst its offset color, which is added to it. each color is
 * an opaque gray whose level is val's low byte, and each set remakes the
 * corners. the getters return the level, from the color's blue */
void mwRnvSetBright(struct rnv_obj *rnv, Sint32 val);
void mwRnvSetBrightOfst(struct rnv_obj *rnv, Sint32 val);
Sint32 mwRnvGetBright(struct rnv_obj *rnv);
Sint32 mwRnvGetBrightOfst(struct rnv_obj *rnv);
void mwRnvDispMovieTexture(struct rnv_obj *rnv, NJS_TEXSURFACE *srf);
void mwRnvDispMbTexture(NJS_TEXSURFACE *srf, Sint32 lx, Sint32 ly, Float sx,
                        Float sy, Sint32 isx, Sint32 isy);
void mwRnvCheckSj(struct rnv_obj *rnv);
void mwRnvDisp(struct rnv_obj *rnv);
void mwRnvDraw(struct rnv_obj *rnv);

/* the renderer's textures */
void mwlrnv_km_error_check(KMSTATUS status);
void mwlrnv_calc_texsize(Sint32 isx, Sint32 isy, Sint32 *tw, Sint32 *th);
void mwRnvAllocTex(struct rnv_obj *rnv, Sint32 isx, Sint32 isy, Sint32 ntex);
void mwRnvFreeTex(struct rnv_obj *rnv);
Sint32 mwRnvGetNumRoomTex(struct rnv_obj *rnv);
void mwRnvLoadTexture(struct rnv_obj *rnv, Uint32 *src, Sint32 time,
                      Sint32 tunit);

#endif
