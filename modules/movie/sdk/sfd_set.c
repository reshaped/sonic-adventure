/* Sofdec handle state and conditions, as callers read and set them */
#include <sg_xpt.h>

#include "movie/sdk/sfd.h"

/* error codes for a call on an invalid handle */
#define SFD_ERR_GETHNSTAT ((Sint32)0xff000103)
#define SFD_ERR_SETCOND ((Sint32)0xff000104)
#define SFD_ERR_GETCOND ((Sint32)0xff000105)
#define SFD_ERR_GETINF ((Sint32)0xff000106)
#define SFD_ERR_AUTOVIDEO ((Sint32)0xff000107)
#define SFD_ERR_AUTOAUDIO ((Sint32)0xff000108)

Sint32 SFD_GetSvrStat(void)
{
    /* @hack spelling: the result through stat, which keeps the call a JSR
     * (0x8ced4b50); returned directly, it becomes a tail jump to
     * SFPLY_DecideSvrStat and 7 of 7 words differ */
    Sint32 stat;

    stat = SFPLY_DecideSvrStat();
    return stat;
}

Sint32 SFD_GetHnStat(struct sfd_handle *hn)
{
    if (SFLIB_CheckHn(hn) != 0) {
        return SFLIB_SetErr(NULL, SFD_ERR_GETHNSTAT);
    }
    return hn->stat;
}

Sint32 SFD_SetCond(struct sfd_handle *hn, Sint32 no, Sint32 val)
{
    if (SFLIB_CheckHn(hn) != 0) {
        return SFLIB_SetErr(NULL, SFD_ERR_SETCOND);
    }
    SFSET_SetCond(hn, no, val);
    return 0;
}

/* setting SFD_COND_TIME_SRC switches the time source at once */
void SFSET_SetCond(struct sfd_handle *hn, Sint32 no, Sint32 val)
{
    hn->cond[no] = val;
    if (no == SFD_COND_TIME_SRC) {
        SFTIM_SetMasterTimeFn(hn);
    }
}

Sint32 SFD_GetCond(struct sfd_handle *hn, Sint32 no, Sint32 *val)
{
    if (SFLIB_CheckHn(hn) != 0) {
        return SFLIB_SetErr(NULL, SFD_ERR_GETCOND);
    }
    *val = SFSET_GetCond(hn, no);
    return 0;
}

Sint32 SFSET_GetCond(struct sfd_handle *hn, Sint32 no)
{
    return hn->cond[no];
}

Sint32 SFD_GetMvInf(struct sfd_handle *hn, struct sfd_mvinf *mvinf)
{
    if (SFLIB_CheckHn(hn) != 0) {
        return SFLIB_SetErr(NULL, SFD_ERR_GETINF);
    }
    *mvinf = hn->mvinf;
    return 0;
}

Sint32 SFD_GetPlyInf(struct sfd_handle *hn, struct sfd_plyinf *plyinf)
{
    if (SFLIB_CheckHn(hn) != 0) {
        return SFLIB_SetErr(NULL, SFD_ERR_GETINF);
    }
    *plyinf = hn->plyinf;
    return 0;
}

/* SFD_SetAutoVideo and SFD_SetAutoAudio only check the handle; they store
 * nothing */
Sint32 SFD_SetAutoVideo(struct sfd_handle *hn, Bool on)
{
    if (SFLIB_CheckHn(hn) != 0) {
        return SFLIB_SetErr(NULL, SFD_ERR_AUTOVIDEO);
    }
    return 0;
}

Sint32 SFD_SetAutoAudio(struct sfd_handle *hn, Bool on)
{
    if (SFLIB_CheckHn(hn) != 0) {
        return SFLIB_SetErr(NULL, SFD_ERR_AUTOAUDIO);
    }
    return 0;
}
