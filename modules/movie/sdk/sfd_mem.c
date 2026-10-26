/* the Sofdec input stage for a stream in memory. the caller writes the stream
 * into the input ring itself, through SFD_GetWritePtr and SFD_AddWritePtr, and
 * ends it with SFD_TermInput */
#include <sg_xpt.h>

#include "movie/sdk/sfd.h"

/* the error SFMEM_GetRead and SFMEM_AddRead record and return. the stage after
 * this one reads the input ring itself */
#define SFMEM_ERR_NOREAD ((Sint32)0xff000501)

/* the stage's functions are sfd_trfns. an argument named arg1, arg2 or arg3 is
 * one the function doesn't read */
Sint32 SFMEM_Init(struct sfd_handle *hn, Sint32 arg1, Sint32 arg2, Sint32 arg3)
{
    return 0;
}

Sint32 SFMEM_Finish(struct sfd_handle *hn, Sint32 arg1, Sint32 arg2,
                    Sint32 arg3)
{
    return 0;
}

/* a stream in memory is ready at once, so each server step marks the input
 * ring ready to read */
Sint32 SFMEM_ExecServer(struct sfd_handle *hn, Sint32 arg1, Sint32 arg2,
                        Sint32 arg3)
{
    struct sfd_trn *trn;
    Sint32 bufno;

    trn = &hn->trntbl.trn[SFD_TR_INPUT];
    bufno = trn->obuf[0];
    SFBUF_SetPrepFlg(hn, bufno, TRUE);
    return 0;
}

Sint32 SFMEM_Create(struct sfd_handle *hn, Sint32 arg1, Sint32 arg2,
                    Sint32 arg3)
{
    return 0;
}

Sint32 SFMEM_Destroy(struct sfd_handle *hn, Sint32 arg1, Sint32 arg2,
                     Sint32 arg3)
{
    return 0;
}

Sint32 SFMEM_Standby(struct sfd_handle *hn, Sint32 arg1, Sint32 arg2,
                     Sint32 arg3)
{
    return 0;
}

Sint32 SFMEM_Start(struct sfd_handle *hn, Sint32 arg1, Sint32 arg2, Sint32 arg3)
{
    return 0;
}

Sint32 SFMEM_Stop(struct sfd_handle *hn, Sint32 arg1, Sint32 arg2, Sint32 arg3)
{
    return 0;
}

Sint32 SFMEM_Pause(struct sfd_handle *hn, Sint32 arg1, Sint32 arg2, Sint32 arg3)
{
    return 0;
}

/* store the input ring's room to write in a struct sfbuf_ring_inf. inf is
 * that struct's address, passed as an sfd_trfn's Sint32 */
Sint32 SFMEM_GetWrite(struct sfd_handle *hn, Sint32 inf, Sint32 arg2,
                      Sint32 arg3)
{
    struct sfd_trn *trn;
    Sint32 bufno;

    trn = &hn->trntbl.trn[SFD_TR_INPUT];
    bufno = trn->obuf[0];
    return SFBUF_RingGetWrite(hn, bufno, (struct sfbuf_ring_inf *)inf);
}

/* count len bytes written to the input ring, as SFBUF_RingAddWrite does */
Sint32 SFMEM_AddWrite(struct sfd_handle *hn, Sint32 len, Sint32 written,
                      Sint32 arg3)
{
    struct sfd_trn *trn;
    Sint32 bufno;

    trn = &hn->trntbl.trn[SFD_TR_INPUT];
    bufno = trn->obuf[0];
    return SFBUF_RingAddWrite(hn, bufno, len, written);
}

Sint32 SFMEM_GetRead(struct sfd_handle *hn, Sint32 arg1, Sint32 arg2,
                     Sint32 arg3)
{
    return SFLIB_SetErr(hn, SFMEM_ERR_NOREAD);
}

Sint32 SFMEM_AddRead(struct sfd_handle *hn, Sint32 arg1, Sint32 arg2,
                     Sint32 arg3)
{
    return SFLIB_SetErr(hn, SFMEM_ERR_NOREAD);
}

const struct sfd_trif SFD_tr_in_mem = {{
    SFMEM_Init,
    SFMEM_Finish,
    SFMEM_ExecServer,
    SFMEM_Create,
    SFMEM_Destroy,
    SFMEM_Standby,
    SFMEM_Start,
    SFMEM_Stop,
    SFMEM_Pause,
    SFMEM_GetWrite,
    SFMEM_AddWrite,
    SFMEM_GetRead,
    SFMEM_AddRead,
}};
