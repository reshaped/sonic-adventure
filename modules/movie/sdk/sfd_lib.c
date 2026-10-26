/* the Sofdec library's setup and shutdown, its errors, and its check of a
 * handle */
#include <sg_xpt.h>
#include <sofdec.h>
#include <string.h>

#include "movie/sdk/sfd.h"

/* the errors SFD_SetErrFn and SFD_GetErrInf record for the library when
 * given a handle that isn't in use */
#define SFD_ERR_SETERRFN ((Sint32)0xff000101)
#define SFD_ERR_GETERRINF ((Sint32)0xff000102)

static Sint32 sflib_InitLibWork(struct sfd_config *config);
static void sflib_InitErr(struct sfd_err *err);
static void sflib_SetErrSub(struct sfd_err *err, Sint32 ecode);
static void sflib_SetErrFnSub(struct sfd_err *err, sfd_errfn func, void *obj);

struct sflib_work SFLIB_libwork;
/* the last handle SFLIB_CheckHn checked, which it stores and no function
 * reads */
static struct sfd_handle *sfd_hn_last;

Sint32 SFD_Init(struct sfd_config *config)
{
    Sint32 err;

    /* @hack spelling: the result assigned in the test, which SHC tests in R0
     * (TST R0,R0 at 0x8ced7dee) and copies to R5 in the branch's delay slot.
     * assigned first, it is copied to R4 to be tested, and 4 of 16 words
     * differ */
    if ((err = sflib_InitLibWork(config)) != 0) {
        return err;
    }
    SFPLY_Init();
    return 0;
}

/* set the library's record up from config. the handles at config->hn are
 * zeroed and marked free, and SFTRN_Init calls each stage's init. it returns
 * SFTRN_Init's result */
static Sint32 sflib_InitLibWork(struct sfd_config *config)
{
    struct sfd_handle *hn;
    Sint32 nhn;
    Sint32 i;
    Sint32 err;

    UTY_MemsetDword(&SFLIB_libwork, 0,
                    sizeof(struct sflib_work) / sizeof(Uint32));
    /* @hack spelling: every use of the record through (&SFLIB_libwork)->, which
     * has SHC load its address from the pool for each (MOV.L @(0x8ced7ef0),R4
     * at 0x8ced7e20 and 0x8ced7e2c). without it, SHC holds the address in R14,
     * and 73 of 89 words differ */
    strncpy((&SFLIB_libwork)->ver, "SFD Version 1.01 1998-10-14",
            sizeof(SFLIB_libwork.ver));
    strncpy((&SFLIB_libwork)->copyright, "Copyright (c) 1997,1998 CRI",
            sizeof(SFLIB_libwork.copyright));
    (&SFLIB_libwork)->config = *config;
    (&SFLIB_libwork)->svrstat = 0;
    sflib_InitErr(&(&SFLIB_libwork)->err);
    SFTIM_Init(&(&SFLIB_libwork)->clock, config->tunit);
    SFBUF_Init(&(&SFLIB_libwork)->buf);
    nhn = config->nhn;
    hn = config->hn;
    (&SFLIB_libwork)->nhn = nhn;
    (&SFLIB_libwork)->hn = hn;
    UTY_MemsetDword(hn, 0, (nhn * sizeof(struct sfd_handle)) / sizeof(Uint32));
    for (i = 0; i < nhn; ++i) {
        hn[i].stat = SFD_STAT_FREE;
    }
    err = SFTRN_Init(&(&SFLIB_libwork)->trlist, config->trlist);
    if (err != 0) {
        return err;
    }
    return 0;
}

/* destroy each handle in use, and call each stage's finish. it returns the
 * stages' error, or else the last handle's SFD_Destroy result, so an earlier
 * handle's error is lost */
Sint32 SFD_Finish(void)
{
    Sint32 hnerr = 0;
    /* @hack spelling: every use of the record through (&SFLIB_libwork)->, which
     * has SHC load its address from the pool for each (0x8ced7ed4,
     * 0x8ced7ed8). without it, SHC holds the address in R10, the object runs
     * 4 bytes short, and 58 of 66 words differ */
    Sint32 nhn = (&SFLIB_libwork)->nhn;
    struct sfd_handle *hn = (&SFLIB_libwork)->hn;
    Sint32 i;
    Sint32 err;

    for (i = 0; i < nhn; ++i) {
        if (hn[i].stat == SFD_STAT_FREE) {
            continue;
        }
        hnerr = SFD_Destroy(&hn[i]);
    }
    SFTIM_Finish(&(&SFLIB_libwork)->clock);
    SFBUF_Finish(&(&SFLIB_libwork)->buf);
    err = SFTRN_Finish(&(&SFLIB_libwork)->trlist);
    if (err != 0) {
        return err;
    }
    if (hnerr != 0) {
        return hnerr;
    }
    return 0;
}

static void sflib_InitErr(struct sfd_err *err)
{
    SFLIB_InitErrInf(err);
}

void SFLIB_InitErrInf(struct sfd_err *err)
{
    err->func = NULL;
    err->obj = NULL;
    err->ecode = 0;
    err->unk_0c = 0;
    err->unk_10 = 0;
}

Sint32 SFLIB_SetErr(struct sfd_handle *hn, Sint32 ecode)
{
    if (hn == NULL) {
        /* @hack spelling: err's address formed through (&SFLIB_libwork)->,
         * which has SHC load the record's address and add err's offset (ADD
         * #84,R4 at 0x8ced7f94). without it, SHC loads err's address from the
         * pool, and 22 of 29 words differ */
        sflib_SetErrSub(&(&SFLIB_libwork)->err, ecode);
    } else {
        sflib_SetErrSub(&hn->err, ecode);
        if (hn->stat > 0) {
            hn->stat = -hn->stat;
        }
    }
    return ecode;
}

static void sflib_SetErrSub(struct sfd_err *err, Sint32 ecode)
{
    err->ecode = ecode;
    if (ecode != 0) {
        sfd_errfn func = err->func;

        if (func != NULL) {
            func(err->obj, ecode);
        }
    }
}

Sint32 SFD_SetErrFn(struct sfd_handle *hn, sfd_errfn func, void *obj)
{
    if (hn == NULL) {
        /* @hack spelling: err's address formed through (&SFLIB_libwork)->,
         * which has SHC load the record's address and add err's offset (ADD
         * #84,R4 at 0x8ced7ff2). without it, SHC loads err's address from the
         * pool, and 23 of 34 words differ */
        sflib_SetErrFnSub(&(&SFLIB_libwork)->err, func, obj);
    } else {
        if (SFLIB_CheckHn(hn) != 0) {
            return SFLIB_SetErr(NULL, SFD_ERR_SETERRFN);
        }
        sflib_SetErrFnSub(&hn->err, func, obj);
    }
    return 0;
}

static void sflib_SetErrFnSub(struct sfd_err *err, sfd_errfn func, void *obj)
{
    err->func = func;
    err->obj = obj;
}

Sint32 SFD_GetErrInf(struct sfd_handle *hn, struct sfd_err *err)
{
    if (hn == NULL) {
        /* @hack spelling: err read through (&SFLIB_libwork)->, which has SHC
         * load the record's address and add err's offset (ADD #84,R2 at
         * 0x8ced8036). without it, SHC loads err's address from the pool, and
         * 25 of 36 words differ */
        *err = (&SFLIB_libwork)->err;
    } else {
        if (SFLIB_CheckHn(hn) != 0) {
            return SFLIB_SetErr(NULL, SFD_ERR_GETERRINF);
        }
        *err = hn->err;
    }
    return 0;
}

Sint32 SFLIB_CheckHn(struct sfd_handle *hn)
{
    sfd_hn_last = hn;
    if (hn == NULL) {
        return -1;
    }
    if (hn->stat == SFD_STAT_FREE) {
        return -1;
    }
    return 0;
}
