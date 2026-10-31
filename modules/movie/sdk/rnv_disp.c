/* movie renderer drawing, each frame a strip of two textured triangles through
 * Kamui */
#include <kamui.h>
#include <machine.h>
#include <ninja.h>
#include <sg_xpt.h>
#include <shinobi.h>
#include <sj.h>
#include <string.h>

#include "movie/sdk/rnv.h"

extern KMVERTEXBUFFDESC _nj_vertex_buffer_;

/* the vertex buffer the strips go into: Ninja's, until mwRnvSetVertexBuffer
 * sets another */
KMVERTEXBUFFDESC *mwg_VertexBufferDescPtr = &_nj_vertex_buffer_;
/* TRUE to filter the texture bilinearly, FALSE to point sample it */
Bool mwg_bilinear_flag = TRUE;
/* the counts mwRnvCheckSj took last. rnv_nwait_disp counts the texture frames
 * loaded and waiting to be drawn, and rnv_nwait_dec the free ones. rnv_nrend
 * counts the drawn frames sj_dsp holds, and rnv_nrend_empty its free slots */
Sint32 rnv_nwait_disp = 0;
Sint32 rnv_nwait_dec = 0;
Sint32 rnv_nrend = 0;
Sint32 rnv_nrend_empty = 0;
/* the timer counts before and after the last strip mwRnvDisp or mwRnvDraw
 * sent, and the microseconds between them */
Uint32 mw_t10a = 0;
Uint32 mw_t10b = 0;
Uint32 mw_drawtime = 0;

/* the texture's corners as u and v, in the order of the strip's corners. no
 * code reads the table */
/* NOLINTNEXTLINE(clang-diagnostic-unused-const-variable) */
static const Float corner_uv[4][2] = {
    {0.0F, 1.0F},
    {0.0F, 0.0F},
    {1.0F, 1.0F},
    {1.0F, 0.0F},
};

KMVERTEXCONTEXT mwg_VertexContext01;

void mwRnvSetVertexBuffer(KMVERTEXBUFFDESC *vtxbuf)
{
    mwg_VertexBufferDescPtr = vtxbuf;
}

/* set ctx up for an opaque textured strip in screen coordinates, and process
 * it. each corner's base color multiplies the texture's colors, and its offset
 * color is added to them. the texture is filtered bilinearly while
 * mwg_bilinear_flag is TRUE */
void mwRnvInitVertexContextOpaq(KMVERTEXCONTEXT *ctx)
{
    /* every RenderState bit from KM_DEPTHMODE to KM_DSTSELECT, but
     * KM_COLORCLAMP and KM_PALETTEBANK. kmProcessVertexRenderState applies
     * the fields those bits mark, so it skips the bColorClamp set below. it
     * also applies bIgnoreTextureAlpha, which this function doesn't set, as
     * ctx already holds it */
    ctx->RenderState = ((KM_DSTSELECT << 1) - KM_DEPTHMODE) &
                       ~(KM_COLORCLAMP | KM_PALETTEBANK);
    ctx->ParamType = KM_POLYGON;
    ctx->ListType = KM_OPAQUE_POLYGON;
    ctx->ColorType = KM_PACKEDCOLOR;
    ctx->bUseSpecular = TRUE;
    ctx->UVFormat = KM_32BITUV;
    ctx->DepthMode = KM_GREATER;
    ctx->CullingMode = KM_NOCULLING;
    ctx->ScreenCoordination = KM_SCREEN;
    ctx->ShadingMode = KM_TEXTUREGOURAUD;
    ctx->SelectModifier = KM_NOMODIFIER;
    ctx->bZWriteDisable = FALSE;
    ctx->SRCBlendingMode = KM_ONE;
    ctx->DSTBlendingMode = KM_ZERO;
    ctx->bSRCSel = FALSE;
    ctx->bDSTSel = FALSE;
    ctx->FogMode = KM_NOFOG;
    ctx->bUseSpecular = TRUE;
    ctx->bUseAlpha = TRUE;
    ctx->FlipUV = KM_NOFLIP;
    ctx->ClampUV = KM_NOCLAMP;
    if (mwg_bilinear_flag) {
        ctx->FilterMode = KM_BILINEAR;
    } else {
        ctx->FilterMode = KM_POINT_SAMPLE;
    }
    ctx->bSuperSample = FALSE;
    ctx->MipMapAdjust = KM_MIPMAP_D_ADJUST_1_00;
    ctx->TextureShadingMode = KM_MODULATE;
    ctx->bColorClamp = FALSE;
    ctx->PaletteBank = 0;
    ctx->fFaceColorAlpha = 1.0F;
    ctx->fFaceColorRed = 1.0F;
    ctx->fFaceColorGreen = 1.0F;
    ctx->fFaceColorBlue = 1.0F;
    ctx->fOffsetColorAlpha = 1.0F;
    ctx->fOffsetColorRed = 1.0F;
    ctx->fOffsetColorGreen = 1.0F;
    ctx->fOffsetColorBlue = 1.0F;
    kmProcessVertexRenderState(ctx);
}

/* set up mwg_VertexContext01, the movie's vertex context: an opaque strip as
 * mwRnvInitVertexContextOpaq makes it, with u and v clamped. its RenderState is
 * left marking only the shading mode and the clamp */
void mwRnvInitDisp(void)
{
    memset(&mwg_VertexContext01, 0, sizeof(mwg_VertexContext01));
    mwRnvInitVertexContextOpaq(&mwg_VertexContext01);
    mwg_VertexContext01.RenderState = 0;
    mwg_VertexContext01.RenderState |= KM_SHADINGMODE;
    mwg_VertexContext01.ShadingMode = KM_TEXTUREGOURAUD;
    mwg_VertexContext01.RenderState |= KM_CLAMPUV;
    mwg_VertexContext01.ClampUV = KM_CLAMP_UV;
    kmProcessVertexRenderState(&mwg_VertexContext01);
}

/* filter bilinearly for sw 1, and point sample for any other value.
 * mwg_VertexContext01 is set up again as mwRnvInitDisp sets it up, but without
 * being zeroed first. its clamp takes effect at the next strip's
 * kmProcessVertexRenderState */
void mwRnvSetBilinearFilterSw(Sint32 sw)
{
    if (sw == 1) {
        mwg_bilinear_flag = TRUE;
        /* @hack redundant: a goto to the statement after the else, which has
         * the else arm store its FALSE through R3 (MOV #0,R3 at 0x8ced235c).
         * without it, the store goes through R1, and 2 of 28 words differ */
        goto reset;
    } else {
        mwg_bilinear_flag = FALSE;
    }
reset:
    mwRnvInitVertexContextOpaq(&mwg_VertexContext01);
    mwg_VertexContext01.RenderState = 0;
    mwg_VertexContext01.RenderState |= KM_SHADINGMODE;
    mwg_VertexContext01.ShadingMode = KM_TEXTUREGOURAUD;
    mwg_VertexContext01.RenderState |= KM_CLAMPUV;
    mwg_VertexContext01.ClampUV = KM_CLAMP_UV;
}

Sint32 mwRnvGetBilinearFilterSw(void)
{
    return mwg_bilinear_flag;
}

/* set the movie's rectangle on the screen, and the part of its texture shown,
 * from isx and isy. mwRnvCalcVtex makes the corners from them. the strip's
 * depth is always 1/65536. MWD_RNV_DTYPE_FULL stretches the whole picture over
 * the whole screen. any other dtype sets each side by its length n in pixels.
 * across, n is isx:
 * - up to 320: n - 1 texels from a quarter texel in, at twice their size,
 *   starting where 2n pixels would start centered
 * - 352: texels 16.25 to 336.25, over the screen's width
 * - any other n up to 640: all n texels at their size, centered
 * - over 640: all n texels, over the screen's width
 * down, n is isy:
 * - up to 240: as across up to 320
 * - 241 to 320, and 361 to 480: all n texels at their size, centered
 * - 321 to 360: n - 1 texels from v = 0.375 / (n - 1), at 4/3 their size,
 *   starting where 4n / 3 pixels would start centered
 * - over 480: all n texels, over the screen's height */
void mwRnvCalcDef(struct rnv_obj *rnv, Sint32 dtype)
{
    Float span;

    rnv->plz = 1.0F / 65536.0F;
    if (dtype == MWD_RNV_DTYPE_FULL) {
        rnv->plx = 0.0F;
        rnv->ply = 0.0F;
        rnv->psx = rnv->ssx;
        rnv->psy = rnv->ssy;
        rnv->tu = 0.0F;
        rnv->tv = 0.0F;
        rnv->tsu = (Float)rnv->isx / (Float)rnv->tw;
        rnv->tsv = (Float)rnv->isy / (Float)rnv->th;
    } else {
        if (rnv->isx <= 320) {
            span = (Float)(rnv->isx - 1);
            rnv->plx = (rnv->ssx - (Float)(rnv->isx * 2)) / 2.0F;
            rnv->psx = 2.0F * span;
            rnv->tu = 0.25F / (Float)rnv->tw;
            rnv->tsu = span / (Float)rnv->tw;
        } else if (rnv->isx == 352) {
            span = 320.0F;
            rnv->plx = 0.0F;
            rnv->psx = rnv->ssx;
            rnv->tu = 16.25F / (Float)rnv->tw;
            rnv->tsu = span / (Float)rnv->tw;
        } else if (rnv->isx <= 640) {
            rnv->plx = (rnv->ssx - (Float)rnv->isx) / 2.0F;
            rnv->psx = (Float)rnv->isx;
            rnv->tu = 0.0F;
            rnv->tsu = (Float)rnv->isx / (Float)rnv->tw;
        } else {
            rnv->plx = 0.0F;
            rnv->psx = rnv->ssx;
            rnv->tu = 0.0F;
            rnv->tsu = (Float)rnv->isx / (Float)rnv->tw;
        }
        if (rnv->isy <= 240) {
            span = (Float)(rnv->isy - 1);
            rnv->ply = (rnv->ssy - (Float)(rnv->isy * 2)) / 2.0F;
            rnv->psy = 2.0F * span;
            rnv->tv = 0.25F / (Float)rnv->th;
            rnv->tsv = span / (Float)rnv->th;
        } else if (rnv->isy <= 320) {
            rnv->ply = (rnv->ssy - (Float)rnv->isy) / 2.0F;
            rnv->psy = (Float)rnv->isy;
            rnv->tv = 0.0F;
            rnv->tsv = (Float)rnv->isy / (Float)rnv->th;
        } else if (rnv->isy <= 360) {
            span = (Float)(rnv->isy - 1);
            rnv->ply = (rnv->ssy - ((Float)rnv->isy * 4.0F / 3.0F)) / 2.0F;
            rnv->psy = span * 4.0F / 3.0F;
            rnv->tv = 0.375F / span;
            rnv->tsv = span / (Float)rnv->th;
        } else if (rnv->isy <= 480) {
            rnv->ply = (rnv->ssy - (Float)rnv->isy) / 2.0F;
            rnv->psy = (Float)rnv->isy;
            rnv->tv = 0.0F;
            rnv->tsv = (Float)rnv->isy / (Float)rnv->th;
        } else {
            rnv->ply = 0.0F;
            rnv->psy = rnv->ssy;
            rnv->tv = 0.0F;
            rnv->tsv = (Float)rnv->isy / (Float)rnv->th;
        }
    }
}

void mwRnvSetDispPos(struct rnv_obj *rnv, Float lx, Float ly)
{
    rnv->plx = lx;
    rnv->ply = ly;
    mwRnvCalcVtex(rnv);
}

void mwRnvSetDispSize(struct rnv_obj *rnv, Float sx, Float sy)
{
    rnv->psx = sx;
    rnv->psy = sy;
    mwRnvCalcVtex(rnv);
}

/* set the strip's corners from the movie's position, size and depth, the part
 * of the texture shown, and the base and offset colors */
void mwRnvCalcVtex(struct rnv_obj *rnv)
{
    Sint32 i;
    KMVERTEX3 *vtx;
    Uint32 base = rnv->bright.dwPacked;
    Uint32 ofst = rnv->bright_ofst.dwPacked;

    vtx = rnv->vtx;
    for (i = 0; i < 4; i++) {
        if (i == 3) {
            vtx[i].ParamControlWord = KM_VERTEXPARAM_ENDOFSTRIP;
        } else {
            vtx[i].ParamControlWord = KM_VERTEXPARAM_NORMAL;
        }
        if (i == 0) {
            vtx[i].fX = rnv->plx;
            vtx[i].fY = rnv->ply + rnv->psy;
            vtx[i].fU = rnv->tu;
            vtx[i].fV = rnv->tv + rnv->tsv;
            vtx[i].uBaseRGB.dwPacked = base;
            vtx[i].uOffsetRGB.dwPacked = ofst;
        } else if (i == 1) {
            vtx[i].fX = rnv->plx;
            vtx[i].fY = rnv->ply;
            vtx[i].fU = rnv->tu;
            vtx[i].fV = rnv->tv;
            vtx[i].uBaseRGB.dwPacked = base;
            vtx[i].uOffsetRGB.dwPacked = ofst;
        } else if (i == 2) {
            vtx[i].fX = rnv->plx + rnv->psx;
            vtx[i].fY = rnv->ply + rnv->psy;
            vtx[i].fU = rnv->tu + rnv->tsu;
            vtx[i].fV = rnv->tv + rnv->tsv;
            vtx[i].uBaseRGB.dwPacked = base;
            vtx[i].uOffsetRGB.dwPacked = ofst;
        } else if (i == 3) {
            vtx[i].fX = rnv->plx + rnv->psx;
            vtx[i].fY = rnv->ply;
            vtx[i].fU = rnv->tu + rnv->tsu;
            vtx[i].fV = rnv->tv;
            vtx[i].uBaseRGB.dwPacked = base;
            vtx[i].uOffsetRGB.dwPacked = ofst;
        }
        vtx[i].u.fZ = rnv->plz;
#ifdef GCC
        /* the port's 16:9 squeezes the movie about the screen's middle, as
         * it squeezes the 2D draws, so the movie keeps its shape. the bands
         * beside it show movie_play's back color, black */
        if (port_wide_squeezes()) {
            vtx[i].fX = port_wide_x(vtx[i].fX);
        }
#endif
    }
}

void mwRnvSetBright(struct rnv_obj *rnv, Sint32 val)
{
    Uint32 level;

    level = (Uint8)val;
    rnv->bright.dwPacked = (level << 16) | (level << 8) | level | 0xff000000;
    mwRnvCalcVtex(rnv);
}

void mwRnvSetBrightOfst(struct rnv_obj *rnv, Sint32 val)
{
    Uint32 level;

    level = (Uint8)val;
    rnv->bright_ofst.dwPacked =
        (level << 16) | (level << 8) | level | 0xff000000;
    mwRnvCalcVtex(rnv);
}

Sint32 mwRnvGetBright(struct rnv_obj *rnv)
{
    return rnv->bright.byte.bBlue;
}

Sint32 mwRnvGetBrightOfst(struct rnv_obj *rnv)
{
    return rnv->bright_ofst.byte.bBlue;
}

/* send the strip, textured with srf, to mwg_VertexBufferDescPtr */
void mwRnvDispMovieTexture(struct rnv_obj *rnv, NJS_TEXSURFACE *srf)
{
    Sint32 i;
    /* @hack spelling: srf copied to tex, which SHC also copies to R4 for
     * nothing (MOV R5,R4 at 0x8ced275a). without it, that copy goes, the
     * object runs 4 bytes short, and 36 of 44 words differ */
    NJS_TEXSURFACE *tex;

    tex = srf;
    mwg_VertexContext01.pTextureSurfaceDesc = (PKMSURFACEDESC)tex;
    kmProcessVertexRenderState(&mwg_VertexContext01);
    kmSetVertexRenderState(&mwg_VertexContext01);
    kmStartVertexStrip(mwg_VertexBufferDescPtr);
    for (i = 0; i < 4; i++) {
        kmSetVertex(mwg_VertexBufferDescPtr, &rnv->vtx[i], KM_VERTEXTYPE_03,
                    sizeof(KMVERTEX3));
    }
}

/* send a strip of isx - 1 by isy - 1 texels of srf, scaled by sx and sy. its
 * top left is at lx, ly. its u starts at 0.25 / (isx - 1) and its v at
 * 0.25 / (isy - 1). its depth is 0.01, its base color opaque white and its
 * offset color opaque black */
void mwRnvDispMbTexture(NJS_TEXSURFACE *srf, Sint32 lx, Sint32 ly, Float sx,
                        Float sy, Sint32 isx, Sint32 isy)
{
    KMVERTEX3 vtx;
    Float tw;
    Float th;
    Float w;
    Float h;
    Sint32 i;
    /* @hack spelling: srf copied to tex, which keeps srf at sp+36 and sy at
     * sp+24 (0x8ced27e2, 0x8ced27e8). without it, the two trade slots, the
     * loop's 4 and -1 move to other registers, and 14 of 227 words differ */
    NJS_TEXSURFACE *tex;

    tex = srf;
    w = (Float)(isx - 1);
    h = (Float)(isy - 1);
    tw = (Float)tex->nWidth;
    th = (Float)tex->nHeight;
    mwg_VertexContext01.pTextureSurfaceDesc = (PKMSURFACEDESC)tex;
    kmProcessVertexRenderState(&mwg_VertexContext01);
    kmSetVertexRenderState(&mwg_VertexContext01);
    kmStartVertexStrip(mwg_VertexBufferDescPtr);
    for (i = 0; i < 4; i++) {
        if (i == 3) {
            vtx.ParamControlWord = KM_VERTEXPARAM_ENDOFSTRIP;
        } else {
            vtx.ParamControlWord = KM_VERTEXPARAM_NORMAL;
        }
        if (i == 0) {
            vtx.fX = (Float)lx + 0.0F;
            vtx.fY = (sy * h) + (Float)ly;
            vtx.fU = (0.25F / w) + 0.0F;
            vtx.fV = (h / th) + (0.25F / h);
        } else if (i == 1) {
            vtx.fX = (Float)lx + 0.0F;
            vtx.fY = (Float)ly + 0.0F;
            vtx.fU = (0.25F / w) + 0.0F;
            vtx.fV = (0.25F / h) + 0.0F;
        } else if (i == 2) {
            vtx.fX = (sx * w) + (Float)lx;
            vtx.fY = (sy * h) + (Float)ly;
            vtx.fU = (w / tw) + (0.25F / w);
            vtx.fV = (h / th) + (0.25F / h);
        } else if (i == 3) {
            vtx.fX = (sx * w) + (Float)lx;
            vtx.fY = (Float)ly + 0.0F;
            vtx.fU = (w / tw) + (0.25F / w);
            vtx.fV = (0.25F / h) + 0.0F;
        }
        vtx.u.fZ = 0.01F;
        vtx.uBaseRGB.dwPacked = 0xffffffff;
        vtx.uOffsetRGB.dwPacked = 0xff000000;
        kmSetVertex(mwg_VertexBufferDescPtr, &vtx, KM_VERTEXTYPE_03,
                    sizeof(KMVERTEX3));
    }
}

/* take the counts rnv_nwait_disp to rnv_nrend_empty, with every interrupt
 * masked */
void mwRnvCheckSj(struct rnv_obj *rnv)
{
    Sint32 mask;
    Sint32 txfrm_size;
    Sint32 ck_size;

    txfrm_size = sizeof(struct rnv_txfrm);
    ck_size = sizeof(SJCK);
    mask = get_imask();
    set_imask(IMASK_ALL);
    rnv_nwait_disp =
        rnv->sj_tex->vtbl->GetNumData(rnv->sj_tex, SJ_LIN_DATA) / txfrm_size;
    rnv_nwait_dec =
        rnv->sj_tex->vtbl->GetNumData(rnv->sj_tex, SJ_LIN_FREE) / txfrm_size;
    rnv_nrend =
        rnv->sj_dsp->vtbl->GetNumData(rnv->sj_dsp, SJ_LIN_DATA) / ck_size;
    rnv_nrend_empty =
        rnv->sj_dsp->vtbl->GetNumData(rnv->sj_dsp, SJ_LIN_FREE) / ck_size;
    set_imask(mask);
}

/* draw the next loaded frame, or with none loaded, the frame on screen again.
 * with neither, it hangs in a loop. each new frame drawn goes on sj_dsp, and
 * when sj_dsp already holds RNV_NDSP, the oldest there is freed first. the
 * first frame drawn after mwRnvCreate never goes on sj_dsp, so its texture
 * frame never comes free */
void mwRnvDisp(struct rnv_obj *rnv)
{
    struct rnv_txfrm txfrm;
    SJCK ck;
    SJCK dsp;
    SJCK old;

    mwRnvCheckSj(rnv);
    rnv->sj_tex->vtbl->GetChunk(rnv->sj_tex, SJ_LIN_DATA,
                                sizeof(struct rnv_txfrm), &ck);
    if (ck.len == 0) {
        if (rnv->cur.data == NULL) {
            for (;;) {
            }
        }
        ck = rnv->cur;
    } else if (rnv->cur.len != 0) {
        if (rnv->sj_dsp->vtbl->GetNumData(rnv->sj_dsp, SJ_LIN_FREE) == 0) {
            rnv->sj_dsp->vtbl->GetChunk(rnv->sj_dsp, SJ_LIN_DATA, sizeof(SJCK),
                                        &dsp);
            old = *(SJCK *)dsp.data;
            rnv->sj_dsp->vtbl->PutChunk(rnv->sj_dsp, SJ_LIN_FREE, &dsp);
            rnv->sj_tex->vtbl->PutChunk(rnv->sj_tex, SJ_LIN_FREE, &old);
        }
        rnv->sj_dsp->vtbl->GetChunk(rnv->sj_dsp, SJ_LIN_FREE, sizeof(SJCK),
                                    &dsp);
        if (dsp.len == 0) {
            for (;;) {
            }
        }
        *(SJCK *)dsp.data = ck;
        rnv->sj_dsp->vtbl->PutChunk(rnv->sj_dsp, SJ_LIN_DATA, &dsp);
    }
    txfrm = *(struct rnv_txfrm *)ck.data;
    mw_t10a = syTmrGetCount();
    mwRnvDispMovieTexture(rnv, txfrm.srf);
    mw_t10b = syTmrGetCount();
    mw_drawtime = syTmrCountToMicro(syTmrDiffCount(mw_t10a, mw_t10b));
    rnv->cur = ck;
}

/* draw as mwRnvDisp does, with the callbacks. the draw callback is offered
 * each loaded frame, and its MWD_RNV_DRAWCB_* result decides what is drawn.
 * the display callback takes each frame sj_dsp frees, before its texture frame
 * comes free. unlike mwRnvDisp, this puts the first frame on sj_dsp too */
void mwRnvDraw(struct rnv_obj *rnv)
{
    struct rnv_txfrm txfrm;
    SJCK ck;
    SJCK dsp;
    SJCK old;
    struct rnv_txfrm *done;
    Sint32 ret;

    /* @hack spelling: a skip starts over at retry, a loop written as a goto.
     * as a do-while loop, the object runs 20 bytes short and 205 of 216 words
     * differ */
retry:
    mwRnvCheckSj(rnv);
    rnv->sj_tex->vtbl->GetChunk(rnv->sj_tex, SJ_LIN_DATA,
                                sizeof(struct rnv_txfrm), &ck);
    if (ck.len == 0) {
        ret = MWD_RNV_DRAWCB_WAIT;
    } else {
        txfrm = *(struct rnv_txfrm *)ck.data;
        if (rnv->draw_fn != NULL) {
            ret = rnv->draw_fn(rnv->draw_obj, txfrm.time, txfrm.tunit,
                               txfrm.dfno);
        } else {
            ret = MWD_RNV_DRAWCB_OK;
        }
    }
    if (ret == MWD_RNV_DRAWCB_WAIT) {
        if (ck.len > 0) {
            rnv->sj_tex->vtbl->UngetChunk(rnv->sj_tex, SJ_LIN_DATA, &ck);
        }
        if (rnv->cur.data == NULL) {
            for (;;) {
            }
        }
        ck = rnv->cur;
    } else {
        if (rnv->sj_dsp->vtbl->GetNumData(rnv->sj_dsp, SJ_LIN_FREE) == 0) {
            rnv->sj_dsp->vtbl->GetChunk(rnv->sj_dsp, SJ_LIN_DATA, sizeof(SJCK),
                                        &dsp);
            old = *(SJCK *)dsp.data;
            rnv->sj_dsp->vtbl->PutChunk(rnv->sj_dsp, SJ_LIN_FREE, &dsp);
            done = (struct rnv_txfrm *)old.data;
            if (rnv->disp_fn != NULL) {
                rnv->disp_fn(rnv->disp_obj, done->time, done->tunit,
                             done->dfno);
            }
            rnv->sj_tex->vtbl->PutChunk(rnv->sj_tex, SJ_LIN_FREE, &old);
        }
        rnv->sj_dsp->vtbl->GetChunk(rnv->sj_dsp, SJ_LIN_FREE, sizeof(SJCK),
                                    &dsp);
        if (dsp.len == 0) {
            for (;;) {
            }
        }
        *(SJCK *)dsp.data = ck;
        rnv->sj_dsp->vtbl->PutChunk(rnv->sj_dsp, SJ_LIN_DATA, &dsp);
    }
    rnv->cur = ck;
    if (ret == MWD_RNV_DRAWCB_SKIP) {
        goto retry;
    }
    txfrm = *(struct rnv_txfrm *)ck.data;
    mw_t10a = syTmrGetCount();
    mwRnvDispMovieTexture(rnv, txfrm.srf);
    mw_t10b = syTmrGetCount();
    mw_drawtime = syTmrCountToMicro(syTmrDiffCount(mw_t10a, mw_t10b));
}
