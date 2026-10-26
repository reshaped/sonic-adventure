/* the movie renderer's textures, and the loading of decoded pictures into
 * them */
#include <kamui.h>
#include <ninja.h>
#include <sg_xpt.h>
#include <shinobi.h>
#include <sj.h>

#include "movie/sdk/rnv.h"

/* the bits that turn a P1 address into its uncached P2 alias, which is the
 * address kmLoadYUVTexture takes the picture from */
#define RNV_UNCACHED 0xa0000000

/* the renderer's surfaces, one for each texture frame */
NJS_TEXSURFACE mwg_MPEGTexSurfaceDesc[RNV_NTXFRM];
/* the Kamui error mwlrnv_km_error_check hangs on */
KMSTATUS Errorstatus;
/* the timer counts before and after the last picture loaded */
Uint32 mw_t11a;
Uint32 mw_t11b;

/* return for KMSTATUS_SUCCESS. any other status hangs the caller in a loop
 * that stores it in Errorstatus */
void mwlrnv_km_error_check(KMSTATUS status)
{
    if (status != 0) {
        for (;;) {
            Errorstatus = status;
        }
    }
}

/* set *tw and *th to the least powers of two at least isx and isy, or to 0
 * for a side of 0 or less */
void mwlrnv_calc_texsize(Sint32 isx, Sint32 isy, Sint32 *tw, Sint32 *th)
{
    Sint32 n;

    if (isx > 0) {
        isx--;
        for (n = 0; isx != 0; n++) {
            isx >>= 1;
        }
        *tw = 1 << n;
    } else {
        *tw = 0;
    }
    if (isy > 0) {
        isy--;
        for (n = 0; isy != 0; n++) {
            isy >>= 1;
        }
        *th = 1 << n;
    } else {
        *th = 0;
    }
}

/* create ntex YUV422 stride surfaces in mwg_MPEGTexSurfaceDesc for an isx by
 * isy movie, and record the sizes in rnv. ntex must be at most RNV_NTXFRM.
 * each surface's sides are isx and isy rounded up to powers of two, and the
 * stride width is isx. a Kamui error hangs the call */
void mwRnvAllocTex(struct rnv_obj *rnv, Sint32 isx, Sint32 isy, Sint32 ntex)
{
    KMSTATUS status;
    KMTEXTURETYPE type;
    Sint32 i;
    Sint32 tw;
    Sint32 th;
    /* @hack redundant: 8 bytes of frame that no code reads, of a type unknown,
     * declared as an SJCK. the image's ADD #-16,R15 and ADD #16,R15 reserve and
     * free them (0x8ced2e20, 0x8ced2e84); without them, 2 of 73 words differ */
    SJCK unused;

    rnv->ntex = ntex;
    rnv->isx = isx;
    rnv->isy = isy;
    rnv->nmbx = rnv->isx / 16;
    rnv->nmby = rnv->isy / 16;
    rnv->srf = mwg_MPEGTexSurfaceDesc;
    type = KM_TEXTURE_STRIDE | KM_TEXTURE_YUV422;
    kmSetStrideWidth(isx);
    for (i = 0; i < ntex; i++) {
        mwlrnv_calc_texsize(isx, isy, &tw, &th);
        rnv->tw = tw;
        rnv->th = th;
        status =
            kmCreateTextureSurface((PKMSURFACEDESC)&rnv->srf[i], tw, th, type);
        mwlrnv_km_error_check(status);
    }
}

void mwRnvFreeTex(struct rnv_obj *rnv)
{
    Sint32 i;
    Sint32 ntex;

    ntex = rnv->ntex;
    for (i = 0; i < ntex; i++) {
        mwlrnv_km_error_check(kmFreeTexture((PKMSURFACEDESC)&rnv->srf[i]));
    }
}

/* the number of texture frames free to load */
Sint32 mwRnvGetNumRoomTex(struct rnv_obj *rnv)
{
    Sint32 nbyte;

    nbyte = rnv->sj_tex->vtbl->GetNumData(rnv->sj_tex, SJ_LIN_FREE);
    return nbyte / sizeof(struct rnv_txfrm);
}

/* load the decoded picture at src, in YUV420 macroblocks, into the next free
 * texture frame. the frame then goes on sj_tex's data line with the picture's
 * number, time and tunit. with no frame free, the picture is dropped. the load
 * waits for its transfer to end, and a Kamui error hangs the call. the surface
 * takes the movie's size for the load, then gets its own back */
void mwRnvLoadTexture(struct rnv_obj *rnv, Uint32 *src, Sint32 time,
                      Sint32 tunit)
{
    SJCK ck;
    /* @hack spelling: src copied to pixels, which puts time at sp+12 and
     * tunit at sp+8 (0x8ced2f14, 0x8ced2f18). without it, the two trade slots,
     * and 9 of 87 words differ */
    Uint32 *pixels = src;
    struct rnv_txfrm *txfrm;
    NJS_TEXSURFACE *srf;
    Uint32 width;
    Uint32 height;
    KMSTATUS status;

    rnv->sj_tex->vtbl->GetChunk(rnv->sj_tex, SJ_LIN_FREE,
                                sizeof(struct rnv_txfrm), &ck);
    if (ck.len == 0) {
        return;
    }
    txfrm = (struct rnv_txfrm *)ck.data;
    srf = txfrm->srf;
    mw_t11a = syTmrGetCount();
    width = srf->nWidth;
    height = srf->nHeight;
    srf->nWidth = rnv->isx;
    srf->nHeight = rnv->isy;
    status = kmLoadYUVTexture((PKMSURFACEDESC)srf,
                              (PKMDWORD)((Uint32)pixels | RNV_UNCACHED), 1, 1,
                              KM_TEXTURE_YUV420, TRUE);
    srf->nWidth = width;
    srf->nHeight = height;
    mw_t11b = syTmrGetCount();
    mwlrnv_km_error_check(status);
    txfrm->dfno = rnv->dfno++;
    txfrm->time = time;
    txfrm->tunit = tunit;
    rnv->sj_tex->vtbl->PutChunk(rnv->sj_tex, SJ_LIN_DATA, &ck);
}
