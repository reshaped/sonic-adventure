/* the Sofdec audio output stage for audio that plays itself. the ADX decoder
 * stage plays its own samples. so this stage only follows its input buffer's
 * flags, and passes standby, start, stop and pause on to the decoder stage */
#include <sg_xpt.h>

#include "movie/sdk/sfd.h"

/* the error each of the stage's ring calls records and returns: the stage has
 * no ring to write or read */
#define SFAOAP_ERR_NORING ((Sint32)0xff000a01)

static Sint32 sfaoap_OutputServer(struct sfd_handle *hn);
static void sfaoap_ChkPrepFlg(struct sfd_handle *hn);
static void sfaoap_ChkTermFlg(struct sfd_handle *hn);

/* the stage's functions are sfd_trfns, and none of them reads its three
 * arguments */
Sint32 SFAOAP_Init(struct sfd_handle *hn, Sint32 arg1, Sint32 arg2, Sint32 arg3)
{
    return 0;
}

Sint32 SFAOAP_Finish(struct sfd_handle *hn, Sint32 arg1, Sint32 arg2,
                     Sint32 arg3)
{
    return 0;
}

Sint32 SFAOAP_ExecServer(struct sfd_handle *hn, Sint32 arg1, Sint32 arg2,
                         Sint32 arg3)
{
    /* @hack spelling: sfaoap_OutputServer's result through err, which keeps
     * the call a BSR (0x8ced737a). returned directly, it becomes a BRA, the
     * object runs 8 bytes short, and 6 of 6 words differ */
    Sint32 err;

    err = sfaoap_OutputServer(hn);
    return err;
}

static Sint32 sfaoap_OutputServer(struct sfd_handle *hn)
{
    sfaoap_ChkPrepFlg(hn);
    sfaoap_ChkTermFlg(hn);
    return 0;
}

/* the output is ready once its input is */
static void sfaoap_ChkPrepFlg(struct sfd_handle *hn)
{
    struct sfd_trn *trn;
    Sint32 bufno;

    if (SFTRN_GetPrepFlg(hn, SFD_TR_AUDIO_OUT) != TRUE) {
        trn = &hn->trntbl.trn[SFD_TR_AUDIO_OUT];
        bufno = trn->ibuf;
        if (SFBUF_GetPrepFlg(hn, bufno) == TRUE) {
            SFTRN_SetPrepFlg(hn, SFD_TR_AUDIO_OUT, TRUE);
        }
    }
}

/* the output has ended once its input has */
static void sfaoap_ChkTermFlg(struct sfd_handle *hn)
{
    struct sfd_trn *trn;
    Sint32 bufno;

    if (SFTRN_GetTermFlg(hn, SFD_TR_AUDIO_OUT) != TRUE) {
        trn = &hn->trntbl.trn[SFD_TR_AUDIO_OUT];
        bufno = trn->ibuf;
        if (SFBUF_GetTermFlg(hn, bufno) == TRUE) {
            SFTRN_SetTermFlg(hn, SFD_TR_AUDIO_OUT, TRUE);
        }
    }
}

Sint32 SFAOAP_Create(struct sfd_handle *hn, Sint32 arg1, Sint32 arg2,
                     Sint32 arg3)
{
    return 0;
}

Sint32 SFAOAP_Destroy(struct sfd_handle *hn, Sint32 arg1, Sint32 arg2,
                      Sint32 arg3)
{
    return 0;
}

/* SFAOAP_Standby, SFAOAP_Start, SFAOAP_Stop and SFAOAP_Pause pass their calls
 * on to the handle's audio decoder stage. each returns that stage's result, or
 * 0 where the handle has none */
Sint32 SFAOAP_Standby(struct sfd_handle *hn, Sint32 arg1, Sint32 arg2,
                      Sint32 arg3)
{
    Sint32 err;

    /* @hack spelling: the result assigned in the test, which SHC tests in R0
     * (TST R0,R0 at 0x8ced7426) and copies to R5 in the branch's delay slot.
     * assigned first, it is copied to R4 to be tested, and 4 of 19 words
     * differ */
    if ((err = SFTRN_CallTrtTrif(hn, SFD_TR_AUDIO, SFD_SLOT_STANDBY, 0, 0)) !=
        0) {
        return err;
    }
    return 0;
}

Sint32 SFAOAP_Start(struct sfd_handle *hn, Sint32 arg1, Sint32 arg2,
                    Sint32 arg3)
{
    Sint32 err;

    /* @hack spelling: the result assigned in the test, which SHC tests in R0
     * (TST R0,R0 at 0x8ced744c) and copies to R5 in the branch's delay slot.
     * assigned first, it is copied to R4 to be tested, and 4 of 19 words
     * differ */
    if ((err = SFTRN_CallTrtTrif(hn, SFD_TR_AUDIO, SFD_SLOT_START, 0, 0)) !=
        0) {
        return err;
    }
    return 0;
}

Sint32 SFAOAP_Stop(struct sfd_handle *hn, Sint32 arg1, Sint32 arg2, Sint32 arg3)
{
    Sint32 err;

    /* @hack spelling: the result assigned in the test, which SHC tests in R0
     * (TST R0,R0 at 0x8ced7472) and copies to R5 in the branch's delay slot.
     * assigned first, it is copied to R4 to be tested, and 4 of 19 words
     * differ */
    if ((err = SFTRN_CallTrtTrif(hn, SFD_TR_AUDIO, SFD_SLOT_STOP, 0, 0)) != 0) {
        return err;
    }
    return 0;
}

Sint32 SFAOAP_Pause(struct sfd_handle *hn, Sint32 arg1, Sint32 arg2,
                    Sint32 arg3)
{
    Sint32 err;

    /* @hack spelling: the result assigned in the test, which SHC tests in R0
     * (TST R0,R0 at 0x8ced7498) and copies to R5 in the branch's delay slot.
     * assigned first, it is copied to R4 to be tested, and 4 of 19 words
     * differ */
    if ((err = SFTRN_CallTrtTrif(hn, SFD_TR_AUDIO, SFD_SLOT_PAUSE, 0, 0)) !=
        0) {
        return err;
    }
    return 0;
}

Sint32 SFAOAP_GetWrite(struct sfd_handle *hn, Sint32 arg1, Sint32 arg2,
                       Sint32 arg3)
{
    return SFLIB_SetErr(hn, SFAOAP_ERR_NORING);
}

Sint32 SFAOAP_AddWrite(struct sfd_handle *hn, Sint32 arg1, Sint32 arg2,
                       Sint32 arg3)
{
    return SFLIB_SetErr(hn, SFAOAP_ERR_NORING);
}

Sint32 SFAOAP_GetRead(struct sfd_handle *hn, Sint32 arg1, Sint32 arg2,
                      Sint32 arg3)
{
    return SFLIB_SetErr(hn, SFAOAP_ERR_NORING);
}

Sint32 SFAOAP_AddRead(struct sfd_handle *hn, Sint32 arg1, Sint32 arg2,
                      Sint32 arg3)
{
    return SFLIB_SetErr(hn, SFAOAP_ERR_NORING);
}

const struct sfd_trif SFD_tr_ao_auto_p = {{
    SFAOAP_Init,
    SFAOAP_Finish,
    SFAOAP_ExecServer,
    SFAOAP_Create,
    SFAOAP_Destroy,
    SFAOAP_Standby,
    SFAOAP_Start,
    SFAOAP_Stop,
    SFAOAP_Pause,
    SFAOAP_GetWrite,
    SFAOAP_AddWrite,
    SFAOAP_GetRead,
    SFAOAP_AddRead,
}};
