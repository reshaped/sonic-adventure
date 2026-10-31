/* MPEG video library initialization, work area and decoder handles */
#include <machine.h>
#include <sg_xpt.h>
#include <sofdec.h>

#include "movie/sdk/mpv.h"

/* a decoder's stat: free, or handed out by MPV_Create */
#define MPV_STAT_FREE 1
#define MPV_STAT_USED 2

/* the layout errors mpvlib_ChkFatal finds, after which MPV_Init never returns.
 * mpv.h has the one it does return, MPV_ERR_SECTION_SEQ */
#define MPV_ERR_ALIGN ((Sint32)0xff03ff01) /* blocks off a 32-byte boundary */
#define MPV_ERR_GUARD ((Sint32)0xff03ff02) /* the defaults' guard changed */
#define MPV_ERR_VLC_SIZE ((Sint32)0xff03ff03) /* the VLC tables' size wrong */
#define MPV_ERR_SECTION_SIZE ((Sint32)0xff03ff04) /* a section's size wrong */
/* the errors MPV_Destroy and MPV_SetCond return for a decoder not handed out */
#define MPV_ERR_DESTROY_HN ((Sint32)0xff030201)
#define MPV_ERR_SETCOND_HN ((Sint32)0xff030202)

/* the word after the default conditions */
#define MPV_GUARD 0x5a5a5a5a

/* log2 of a decoder's size, 8 KB, to which MPV_Init aligns the decoders */
#define MPV_OBJ_SHIFT 13

/* the size of the aux area between the decoders and the shared tables. with
 * the tables, it takes 8 KB */
#define MPV_AUX_SIZE 0x1560

/* the cache control register, and its bits that put the instruction cache and
 * the operand cache in index mode */
#define CCR (*(volatile Uint32 *)0xff00001c)
#define CCR_IIX 0x8000
#define CCR_OIX 0x80
/* the address bit that picks a cache's half in index mode */
#define IX_BIT 0x02000000

/* the library's record. aux, ixa and obj point into the work MPV_Init is
 * given */
struct mpv_work {
    char version[32];
    char copyright[32];
    Bool iix; /* the instruction cache ran in index mode at MPV_Init */
    Bool oix; /* the operand cache ran in index mode at MPV_Init */
    Uint8 *aux;
    struct mpv_ixa *ixa;
    Sint32 nobj;
    struct mpv_obj *obj;
};

/* the conditions MPV_Create gives a decoder, by MPV_COND_*, then MPV_GUARD */
static const Sint32 mpvlib_default_cond[MPV_COND_MAX + 1] = {
    0, 1, 1, 0, 0, 0, 0, 0, MPV_GUARD,
};
/* the library's record sizes; no source reads them, and nothing in MOVIE.BIN
 * refers to them */
const Sint32 mpvlib_siz_mpvwork = sizeof(struct mpv_work);
const Sint32 mpvlib_siz_mpvobj = sizeof(struct mpv_obj);
const Sint32 mpvlib_siz_mpvobj_mbpos = sizeof(((struct mpv_obj *)0)->mbpos);
const Sint32 mpvlib_siz_mpvobj_dmy1 = sizeof(((struct mpv_obj *)0)->dmy1);
const Sint32 mpvlib_siz_mpvixa = sizeof(struct mpv_ixa);

static struct mpv_work mpvlib_libwork;
/* no source reads these, and nothing in MOVIE.BIN refers to them; the library
 * uses the flags in its work */
Sint32 mpvlib_iix;
Sint32 mpvlib_oix;
/* the last handle MPVLIB_CheckHn was given */
static struct mpv_obj *mpvlib_mpvobj;

/* @hack spelling: the work through its address, which SHC loads again at each
 * use, as at 0x8ced5864 and 0x8ced5870; through mpvlib_libwork, the address
 * stays in a register, and 425 of 486 words differ in the eight functions that
 * use it */
#define WORK (&mpvlib_libwork)

static Sint32 mpvlib_ChkFatal(void);
static void mpvlib_ChkCacheMode(void);
static void mpvlib_InitObjTbl(void);
static void mpvlib_InitObj(struct mpv_obj *mpv);
static void mpvlib_InitWork(Sint32 nobj, void *work);
static void mpvlib_InitDctPa(struct mpv_obj *mpv);

Sint32 MPV_Init(Sint32 nobj, void *work)
{
    Sint32 code;
    struct mpv_ixa *ixa;
    /* @hack spelling: addr, a copy of work, which leaves nobj the lower frame
     * slot, MOV.L R4,@R15 and MOV.L R5,@(4,R15) at 0x8ced572e and 0x8ced5732;
     * through work, 4 of 56 words differ */
    Uint32 addr = (Uint32)work;

    /* @hack spelling: code assigned in the test, which SHC tests in R0 (TST
     * R0,R0 at 0x8ced5734) and copies to R4 in the branch's delay slot;
     * assigned first, 3 of 56 words differ */
    if ((code = mpvlib_ChkFatal()) != 0) {
        if (code == MPV_ERR_SECTION_SEQ) {
            return code;
        }
        for (;;) {
        }
    }
    mpvlib_ChkCacheMode();
    mpvlib_InitWork(nobj, (void *)MPVLIB_ConvOix(addr));
    ixa = WORK->ixa;
    MPVERR_Init();
    MPVFRM_Init();
    MPVVLC_Init(ixa->vlc);
    MPVBDEC_Init(ixa);
    MPVCMC_Init(ixa->clip);
    mpvlib_InitObjTbl();
    DCT_FzInit(ixa->dct);
    return 0;
}

/* the first layout error, recorded as the library's, or 0 */
static Sint32 mpvlib_ChkFatal(void)
{
    /* @hack spelling: blocks' offset through a local, which keeps SHC from
     * folding its test away, MOV #31,R3 and TST R3,R4 at 0x8ced579a and
     * 0x8ced57a2; tested directly, the function runs 20 bytes short and 48 of
     * its 49 words differ */
    Sint32 offset = (Sint32)((struct mpv_obj *)0)->blocks;

    if ((offset & 31) != 0) {
        return MPVERR_SetCode(NULL, MPV_ERR_ALIGN);
    }
    if (mpvlib_default_cond[MPV_COND_MAX] != MPV_GUARD) {
        return MPVERR_SetCode(NULL, MPV_ERR_GUARD);
    }
    if (MPVVLC_IsVlcSizErr() != 0) {
        return MPVERR_SetCode(NULL, MPV_ERR_VLC_SIZE);
    }
#ifdef GCC
    /* keeps the shipped play, in which each of PSGSFD00 to PSGSFD03 takes
     * the size the check wants, so MPV_Init goes on. GCC ignores #pragma
     * section, and mpvadec.c's reservations are SHC's alone, so GCC's link
     * gives the sections other sizes */
#else
    if (MPVASEC_CheckSectionSiz() != 0) {
        return MPVERR_SetCode(NULL, MPV_ERR_SECTION_SIZE);
    }
#endif
    if (MPVASEC_CheckSectionSeq() != 0) {
        return MPVERR_SetCode(NULL, MPV_ERR_SECTION_SEQ);
    }
    return 0;
}

static void mpvlib_ChkCacheMode(void)
{
    Uint32 ccr;

    WORK->iix = FALSE;
    WORK->oix = FALSE;
    ccr = CCR;
    if (ccr & CCR_IIX) {
        WORK->iix = TRUE;
    }
    if (ccr & CCR_OIX) {
        WORK->oix = TRUE;
    }
}

Uint32 MPVLIB_ConvIix(Uint32 addr)
{
    if (WORK->iix) {
        addr |= IX_BIT;
    }
    return addr;
}

Uint32 MPVLIB_ConvOix(Uint32 addr)
{
    if (WORK->oix) {
        addr |= IX_BIT;
    }
    return addr;
}

static void mpvlib_InitObjTbl(void)
{
    Sint32 nobj = WORK->nobj;
    struct mpv_obj *mpv = WORK->obj;
    Sint32 i;

    for (i = 0; i < nobj; i++) {
        mpvlib_InitObj(mpv++);
    }
}

static void mpvlib_InitObj(struct mpv_obj *mpv)
{
    struct mpv_ixa *ixa;
    struct mpv_mb_param *mb;

    mpv->stat = MPV_STAT_FREE;
    ixa = WORK->ixa;
    /* the block decoders index run_level_4, 2 and 1 with the 1 that ends a
     * code's zeros as the index's top bit. so their pointers stand that far
     * below the tables: 8 entries for run_level_4, and 16 for the other two */
    mpv->run_level_8 = mpvvlc_run_level_8;
    mpv->run_level_4 = mpvvlc_run_level_4 - 8;
    mpv->run_level_2 = mpvvlc_run_level_2 - 16;
    mpv->run_level_1 = mpvvlc_run_level_1 - 16;
    mpv->run_level_0a = mpvvlc_run_level_0a;
    mpv->run_level_0b = mpvvlc_run_level_0b;
    mpv->run_level_0c = mpvvlc_run_level_0c;
    mpv->zigzag = ixa->zigzag;
    mpv->ixa = ixa;
    mpv->dct = mpv->ixa->dct;
    mb = &mpv->mb;
    mpv->clip = mpv_clip_0_255_base;
    mb->clip = mpv->clip;
    mb->blocks = mpv->blocks[0];
    mb->pred = mpv->mb_work;
    mb->pred_b = mb->pred + MPV_MB_BYTES;
    /* the inverse DCT writes the four Y blocks, then Cb and Cr */
    mpv->block_ptr[0] = mpv->blocks[2];
    mpv->block_ptr[1] = mpv->blocks[3];
    mpv->block_ptr[2] = mpv->blocks[4];
    mpv->block_ptr[3] = mpv->blocks[5];
    mpv->block_ptr[4] = mpv->blocks[0];
    mpv->block_ptr[5] = mpv->blocks[1];
}

void mpvlib_InitUseLib(struct mpv_ixa *ixa)
{
    DCT_FzInit(ixa->dct);
}

/* clear work, and lay it out: the decoders from its first 8 KB boundary, then
 * the aux area and the shared tables. the clear takes one decoder more than
 * nobj, room for the alignment */
static void mpvlib_InitWork(Sint32 nobj, void *work)
{
    struct mpv_obj *obj;
    Uint8 *aux;
    struct mpv_ixa *ixa;

    UTY_MemsetDword(work, 0,
                    ((nobj * sizeof(struct mpv_obj)) + sizeof(struct mpv_obj) +
                     MPV_AUX_SIZE + sizeof(struct mpv_ixa)) /
                        sizeof(Uint32));
    work =
        (void *)((((Uint32)work + sizeof(struct mpv_obj) - 1) >> MPV_OBJ_SHIFT)
                 << MPV_OBJ_SHIFT);
    obj = work;
    aux = (Uint8 *)(obj + nobj);
    ixa = (struct mpv_ixa *)(aux + MPV_AUX_SIZE);
    /* SHC's intrinsic copies each literal inline, with no call */
    _builtin_strcpy(WORK->version, "MPV Version 1.01 1998-10-14");
    _builtin_strcpy(WORK->copyright, "Copyright (c) 1997,1998 CRI");
    WORK->aux = aux;
    WORK->ixa = ixa;
    WORK->nobj = nobj;
    WORK->obj = obj;
}

void MPV_Finish(void)
{
}

struct mpv_obj *MPV_Create(void)
{
    Sint32 nobj = WORK->nobj;
    struct mpv_obj *mpv = WORK->obj;
    Sint32 i;

    for (i = 0; i < nobj; i++) {
        if (mpv->stat == MPV_STAT_FREE) {
            mpv->stat = MPV_STAT_USED;
            mpv->unk_16c = 0;
            UTY_MemcpyDword(mpv->cond, mpvlib_default_cond, MPV_COND_MAX);
            MPVERR_InitErrInf(&mpv->err);
            MPVCMC_InitObj(mpv);
            mpvlib_InitDctPa(mpv);
            return mpv;
        }
        mpv++;
    }
    return NULL;
}

static void mpvlib_InitDctPa(struct mpv_obj *mpv)
{
    struct dct_fz_param *param = &mpv->dct_param;

    DCT_FzInitPa(param);
    param->scratch = mpv->mb_work;
    param->coef = mpv->coef[0];
    param->blocks = mpv->block_ptr;
}

void MPV_GetDctCnt(struct mpv_obj *mpv, Sint32 *count0, Sint32 *count1)
{
    struct dct_fz_param *param = &mpv->dct_param;

    *count0 = param->count[0];
    *count1 = param->count[1];
}

Sint32 MPV_Destroy(struct mpv_obj *mpv)
{
    /* @hack spelling: a copy of mpv, which SHC keeps in R14 across
     * MPVLIB_CheckHn, MOV R4,R14 at 0x8ced5c60; through mpv, which then goes
     * to the stack, the function runs 2 bytes long and the PSG 4, and 15 of
     * its 19 words differ */
    struct mpv_obj *hn = mpv;

    if (MPVLIB_CheckHn(hn) != 0) {
        return MPVERR_SetCode(NULL, MPV_ERR_DESTROY_HN);
    }
    hn->stat = MPV_STAT_FREE;
    return 0;
}

Sint32 MPV_SetCond(struct mpv_obj *mpv, Sint32 id, Sint32 value)
{
    /* @hack spelling: a copy of mpv, which SHC keeps in R14 across
     * MPVLIB_CheckHn, MOV R4,R14 at 0x8ced5c8c; through mpv, which then goes
     * to the stack, the function runs 2 bytes short and 24 of its 28 words
     * differ */
    struct mpv_obj *hn = mpv;

    if (MPVLIB_CheckHn(hn) != 0) {
        return MPVERR_SetCode(NULL, MPV_ERR_SETCOND_HN);
    }
    hn->cond[id] = value;
    return 0;
}

Sint32 MPVLIB_CheckHn(struct mpv_obj *mpv)
{
    mpvlib_mpvobj = mpv;
    if (mpv == NULL || mpv->stat != MPV_STAT_USED) {
        return -1;
    }
    return 0;
}
