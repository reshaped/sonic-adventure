/* Sofdec manual video output stage, which hands each decoded picture to the
 * caller through SFD_GetFrm once it is due, and takes it back through
 * SFD_RelFrm */
#include <sg_xpt.h>

#include "movie/sdk/sfd.h"

/* the error either write slot returns: the output stage takes no writes */
#define SFVOM_ERR_NOWRITE ((Sint32)0xff000701)

static void sfvom_ChkTermFlg(struct sfd_handle *hn);
static Bool sfvom_IsTerm(struct sfd_handle *hn);
static void sfvom_ChkPrepFlg(struct sfd_handle *hn);
static Bool sfvom_IsPrepEnd(struct sfd_handle *hn);
static Sint32 sfvom_OutputServer(struct sfd_handle *hn);
static Bool sfvom_IsDispTime(struct sfd_handle *hn, struct sfd_frm *frm);

/* each function fills a slot of SFD_tr_vo_manu, so it takes sfd_trfn's
 * arguments, whether or not it reads them */
Sint32 SFVOM_Init(struct sfd_handle *hn, Sint32 arg1, Sint32 arg2, Sint32 arg3)
{
    return 0;
}

Sint32 SFVOM_Finish(struct sfd_handle *hn, Sint32 arg1, Sint32 arg2,
                    Sint32 arg3)
{
    return 0;
}

Sint32 SFVOM_ExecServer(struct sfd_handle *hn, Sint32 arg1, Sint32 arg2,
                        Sint32 arg3)
{
    Sint32 err;

    sfvom_ChkTermFlg(hn);
    err = sfvom_OutputServer(hn);
    sfvom_ChkPrepFlg(hn);
    return err;
}

/* the output stage ends once its input buffer has ended */
static void sfvom_ChkTermFlg(struct sfd_handle *hn)
{
    struct sfd_trn *trn;
    Sint32 bufno;

    /* @hack spelling: the stage and its input buffer through trn and bufno,
     * which has SHC add the stage's offsets to hn in R4 (ADD R14,R4 at
     * 0x8ced935e, ADD R3,R4 at 0x8ced9362) before it reads ibuf; read through
     * hn, the offsets fold into one and 25 of 28 words differ */
    trn = &hn->trntbl.trn[SFD_TR_VIDEO_OUT];
    bufno = trn->ibuf;
    if (SFBUF_GetTermFlg(hn, bufno) == TRUE) {
        if (sfvom_IsTerm(hn)) {
            SFTRN_SetTermFlg(hn, SFD_TR_VIDEO_OUT, TRUE);
        }
    }
}

static Bool sfvom_IsTerm(struct sfd_handle *hn)
{
    return TRUE;
}

/* the output stage is ready once its input buffer is */
static void sfvom_ChkPrepFlg(struct sfd_handle *hn)
{
    struct sfd_trn *trn;
    Sint32 bufno;

    /* @hack spelling: the stage and its input buffer through trn and bufno,
     * which has SHC add the stage's offsets to hn in R4 (ADD R14,R4 at
     * 0x8ced939a, ADD R3,R4 at 0x8ced939e) before it reads ibuf; read through
     * hn, the offsets fold into one and 25 of 28 words differ */
    trn = &hn->trntbl.trn[SFD_TR_VIDEO_OUT];
    bufno = trn->ibuf;
    if (SFBUF_GetPrepFlg(hn, bufno) == TRUE) {
        if (sfvom_IsPrepEnd(hn)) {
            SFTRN_SetPrepFlg(hn, SFD_TR_VIDEO_OUT, TRUE);
        }
    }
}

static Bool sfvom_IsPrepEnd(struct sfd_handle *hn)
{
    return TRUE;
}

static Sint32 sfvom_OutputServer(struct sfd_handle *hn)
{
    return 0;
}

Sint32 SFVOM_Create(struct sfd_handle *hn, Sint32 arg1, Sint32 arg2,
                    Sint32 arg3)
{
    return 0;
}

Sint32 SFVOM_Destroy(struct sfd_handle *hn, Sint32 arg1, Sint32 arg2,
                     Sint32 arg3)
{
    return 0;
}

Sint32 SFVOM_Standby(struct sfd_handle *hn, Sint32 arg1, Sint32 arg2,
                     Sint32 arg3)
{
    return 0;
}

Sint32 SFVOM_Start(struct sfd_handle *hn, Sint32 arg1, Sint32 arg2, Sint32 arg3)
{
    return 0;
}

Sint32 SFVOM_Stop(struct sfd_handle *hn, Sint32 arg1, Sint32 arg2, Sint32 arg3)
{
    return 0;
}

Sint32 SFVOM_Pause(struct sfd_handle *hn, Sint32 arg1, Sint32 arg2, Sint32 arg3)
{
    return 0;
}

Sint32 SFVOM_GetWrite(struct sfd_handle *hn, Sint32 arg1, Sint32 arg2,
                      Sint32 arg3)
{
    return SFLIB_SetErr(hn, SFVOM_ERR_NOWRITE);
}

Sint32 SFVOM_AddWrite(struct sfd_handle *hn, Sint32 arg1, Sint32 arg2,
                      Sint32 arg3)
{
    return SFLIB_SetErr(hn, SFVOM_ERR_NOWRITE);
}

/* arg1 is a struct sfd_frm **. while the handle plays, store through it the
 * input buffer's next picture if that picture is due, and NULL otherwise.
 * returns 0, or the error of the buffer's read, after which *arg1 is as that
 * read left it */
/* @hack redundant: register on hn, which puts hn in R14 and frm in R13 (MOV
 * R4,R14 at 0x8ced93fa, MOV R5,R13 at 0x8ced9406); without it, the two trade
 * and 11 of 36 words differ */
Sint32 SFVOM_GetRead(register struct sfd_handle *hn, Sint32 arg1, Sint32 arg2,
                     Sint32 arg3)
{
    struct sfd_frm **frm = (struct sfd_frm **)arg1;
    Sint32 err;
    struct sfd_trn *trn;
    Sint32 bufno;

    if (hn->stat == SFD_STAT_PLAY) {
        trn = &hn->trntbl.trn[SFD_TR_VIDEO_OUT];
        bufno = trn->ibuf;
        if ((err = SFBUF_VfrmGetRead(hn, bufno, (Sint32)frm)) != 0) {
            return err;
        } else if (sfvom_IsDispTime(hn, *frm)) {
            return 0;
        }
    }
    *frm = NULL;
    return 0;
}

/* TRUE when frm is due: always under SFD_COND_DISP_ALWAYS, and otherwise once
 * the handle's time has started and reaches frm's, or is SFTIM_NONE. FALSE
 * for a NULL frm */
static Bool sfvom_IsDispTime(struct sfd_handle *hn, struct sfd_frm *frm)
{
    Sint32 time;
    Sint32 tunit;
    Sint32 frm_time;
    Sint32 frm_tunit;

    if (frm == NULL) {
        return FALSE;
    }
    if (hn->cond[SFD_COND_DISP_ALWAYS] != 0) {
        return TRUE;
    }
    frm_time = frm->time;
    frm_tunit = frm->tunit;
    SFTIM_GetTime(hn, &time, &tunit);
    if (time == SFTIM_NOT_YET) {
        return FALSE;
    }
    if (time == SFTIM_NONE) {
        return TRUE;
    }
    return SFD_IsDispTime(frm_time, frm_tunit, time, tunit);
}

/* release arg1, the struct sfd_frm * SFVOM_GetRead stored, through the input
 * buffer. returns 0 or the buffer's error */
Sint32 SFVOM_AddRead(struct sfd_handle *hn, Sint32 arg1, Sint32 arg2,
                     Sint32 arg3)
{
    struct sfd_trn *trn;
    Sint32 bufno;

    trn = &hn->trntbl.trn[SFD_TR_VIDEO_OUT];
    bufno = trn->ibuf;
    return SFBUF_VfrmAddRead(hn, bufno, arg1);
}

const struct sfd_trif SFD_tr_vo_manu = {{
    SFVOM_Init,
    SFVOM_Finish,
    SFVOM_ExecServer,
    SFVOM_Create,
    SFVOM_Destroy,
    SFVOM_Standby,
    SFVOM_Start,
    SFVOM_Stop,
    SFVOM_Pause,
    SFVOM_GetWrite,
    SFVOM_AddWrite,
    SFVOM_GetRead,
    SFVOM_AddRead,
}};
