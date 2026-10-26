/* the movie renderer: its one object, and its server. the server waits until
 * every texture frame holds a picture, then draws a frame each call */
#include <machine.h>
#include <sg_xpt.h>
#include <sj.h>
#include <string.h>

#include "movie/sdk/rnv.h"

/* the states in rnv_obj's stat: stopped, waiting for every texture frame to
 * be loaded, and drawing. stat is never 2 */
#define RNV_STAT_STOP 0
#define RNV_STAT_FILL 1
#define RNV_STAT_DRAW 3

void mwRnvLockCs(Sint32 *mask);
void mwRnvUnlockCs(const Sint32 *mask);
void mwlRnvExecOne(struct rnv_obj *rnv);

const char mwg_rnv_build[] = "\nmwRnv Ver.1.04 Build:Nov 11 1998 21:19:25\n";
struct rnv_obj mwg_rnv_obj;

/* enter the renderer's critical section: mask every interrupt, and keep the
 * old mask in *mask for mwRnvUnlockCs */
void mwRnvLockCs(Sint32 *mask)
{
    *mask = get_imask();
    set_imask(IMASK_ALL);
}

void mwRnvUnlockCs(const Sint32 *mask)
{
    set_imask(*mask);
}

void mwRnvInit(void)
{
    memset(&mwg_rnv_obj, 0, sizeof(mwg_rnv_obj));
    mwRnvInitDisp();
}

void mwRnvFinish(void)
{
    memset(&mwg_rnv_obj, 0, sizeof(mwg_rnv_obj));
}

/* set up mwg_rnv_obj, the one renderer, for cprm's movie, and start it. the
 * renderer draws on a 640 by 480 screen, at brightness 0xc0 with no offset.
 * returns &mwg_rnv_obj. reserved is unread. a joint that can't be created
 * hangs the call in a loop */
struct rnv_obj *mwRnvCreate(void *reserved, struct rnv_cprm *cprm)
{
    Sint32 mask;
    struct rnv_obj *rnv;
    /* @hack redundant: 12 bytes of frame that no code reads, of a type unknown,
     * declared as a Sint32[3]. the image's ADD #-16,R15 and ADD #16,R15 reserve
     * and free them (0x8ced1fc0, 0x8ced207e); without them, 2 of 106 words
     * differ */
    Sint32 unused[3];

    mwRnvLockCs(&mask);
    rnv = &mwg_rnv_obj;
    rnv->used = TRUE;
    rnv->stat = RNV_STAT_STOP;
    rnv->sj_tex = SJRBF_Create((char *)&rnv->txfrm, sizeof(rnv->txfrm), 0);
    if (rnv->sj_tex == NULL) {
        for (;;) {
        }
    }
    rnv->sj_dsp = SJRBF_Create((char *)&rnv->dsp, sizeof(rnv->dsp), 0);
    if (rnv->sj_dsp == NULL) {
        for (;;) {
        }
    }
    rnv->ssx = 640.0F;
    rnv->ssy = 480.0F;
    rnv->isx = cprm->nmbx * 16;
    rnv->isy = cprm->nmby * 16;
    rnv->dfno = 0;
    rnv->draw_fn = NULL;
    rnv->draw_obj = NULL;
    rnv->disp_fn = NULL;
    rnv->disp_obj = NULL;
    rnv->bright.dwPacked = 0xffc0c0c0;
    rnv->bright_ofst.dwPacked = 0xff000000;
    mwRnvAllocTex(rnv, rnv->isx, rnv->isy, RNV_NTXFRM);
    mwRnvCalcDef(rnv, cprm->dtype);
    memset(&rnv->cur, 0, sizeof(rnv->cur));
    mwRnvCalcVtex(rnv);
    mwRnvStart(rnv);
    mwRnvUnlockCs(&mask);
    return rnv;
}

void mwRnvDestroy(struct rnv_obj *rnv)
{
    Sint32 mask;

    mwRnvLockCs(&mask);
    if (rnv != NULL) {
        rnv->used = FALSE;
        rnv->sj_tex->vtbl->Destroy(rnv->sj_tex);
        rnv->sj_dsp->vtbl->Destroy(rnv->sj_dsp);
        mwRnvFreeTex(rnv);
    }
    mwRnvUnlockCs(&mask);
}

/* stop the renderer, and start it again with every texture frame free. frame
 * i goes back on sj_tex's free line with its surface, rnv->srf[i], and the
 * renderer waits for all of them to be loaded */
void mwRnvStart(struct rnv_obj *rnv)
{
    SJCK ck;
    struct rnv_txfrm *txfrm;
    Sint32 i;

    mwRnvStop(rnv);
    rnv->sj_tex->vtbl->Reset(rnv->sj_tex);
    rnv->sj_dsp->vtbl->Reset(rnv->sj_dsp);
    /* put all of sj_tex's free space on its data line, so that the loop
     * below takes each frame back as a chunk of its own */
    for (;;) {
        rnv->sj_tex->vtbl->GetChunk(rnv->sj_tex, SJ_LIN_FREE,
                                    sizeof(struct rnv_txfrm), &ck);
        if (ck.len == 0) {
            break;
        }
        rnv->sj_tex->vtbl->PutChunk(rnv->sj_tex, SJ_LIN_DATA, &ck);
    }
    for (i = 0; i < RNV_NTXFRM; i++) {
        rnv->sj_tex->vtbl->GetChunk(rnv->sj_tex, SJ_LIN_DATA,
                                    sizeof(struct rnv_txfrm), &ck);
        if (ck.len == 0) {
            for (;;) {
            }
        }
        txfrm = (struct rnv_txfrm *)ck.data;
        txfrm->srf = &rnv->srf[i];
        rnv->sj_tex->vtbl->PutChunk(rnv->sj_tex, SJ_LIN_FREE, &ck);
    }
    rnv->stat = RNV_STAT_FILL;
}

void mwRnvStop(struct rnv_obj *rnv)
{
    rnv->sj_tex->vtbl->Reset(rnv->sj_tex);
    rnv->sj_dsp->vtbl->Reset(rnv->sj_dsp);
    rnv->stat = RNV_STAT_STOP;
}

/* one server step: draw a frame, or while the renderer waits, set it drawing
 * from the next step once every texture frame is loaded */
void mwlRnvExecOne(struct rnv_obj *rnv)
{
    Sint32 nload;

    if (rnv->stat == RNV_STAT_DRAW) {
        mwRnvDraw(rnv);
    } else if (rnv->stat == RNV_STAT_FILL) {
        nload = rnv->sj_tex->vtbl->GetNumData(rnv->sj_tex, SJ_LIN_DATA) /
                sizeof(struct rnv_txfrm);
        if (nload >= RNV_NTXFRM) {
            rnv->stat = RNV_STAT_DRAW;
        }
    }
}

void mwRnvExecServer(void)
{
    Sint32 mask;
    struct rnv_obj *rnv;

    mwRnvLockCs(&mask);
    rnv = &mwg_rnv_obj;
    if (rnv->used == TRUE) {
        mwlRnvExecOne(rnv);
    }
    mwRnvUnlockCs(&mask);
}

void mwRnvLoad(struct rnv_obj *rnv, Uint32 *src, Sint32 time, Sint32 tunit)
{
    mwRnvLoadTexture(rnv, src, time, tunit);
}

void mwRnvEntryDrawCallbackFn(struct rnv_obj *rnv,
                              Sint32 (*fn)(void *obj, Sint32 time, Sint32 tunit,
                                           Sint32 dfno),
                              void *obj)
{
    rnv->draw_fn = fn;
    rnv->draw_obj = obj;
}

void mwRnvEntryDispCallbackFn(struct rnv_obj *rnv,
                              void (*fn)(void *obj, Sint32 time, Sint32 tunit,
                                         Sint32 dfno),
                              void *obj)
{
    rnv->disp_fn = fn;
    rnv->disp_obj = obj;
}
