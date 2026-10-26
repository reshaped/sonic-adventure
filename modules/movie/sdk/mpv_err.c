/* MPEG video decoder error reporting */
#include <sg_xpt.h>

#include "movie/sdk/mpv.h"

/* the errors MPV_SetErrFunc and MPV_GetErrInf return for a decoder not handed
 * out */
#define MPV_ERR_SETERRFUNC_HN ((Sint32)0xff030203)
#define MPV_ERR_GETERRINF_HN ((Sint32)0xff030204)

/* the library's own error record, for errors with no decoder */
static struct mpv_err_info mpverrinf;

static void mpverr_SetCodeSub(struct mpv_err_info *err, Sint32 code);

void MPVERR_Init(void)
{
    MPVERR_InitErrInf(&mpverrinf);
}

void MPVERR_InitErrInf(struct mpv_err_info *err)
{
    err->func = NULL;
    err->obj = NULL;
    err->code = 0;
    err->misses = 0;
    err->recoveries = 0;
}

Sint32 MPV_SetErrFunc(struct mpv_obj *mpv, void (*func)(void *obj, Sint32 code),
                      void *obj)
{
    /* @hack spelling: a copy of mpv, which SHC keeps in R14 across
     * MPVLIB_CheckHn, MOV R4,R14 at 0x8ced54e6; through mpv, which then goes
     * to the stack, the function runs 2 bytes short and 22 of its 27 words
     * differ */
    struct mpv_obj *hn = mpv;
    struct mpv_err_info *err;

    if (MPVLIB_CheckHn(hn) != 0) {
        return MPVERR_SetCode(NULL, MPV_ERR_SETERRFUNC_HN);
    }
    err = &hn->err;
    err->func = func;
    err->obj = obj;
    return 0;
}

Sint32 MPV_GetErrInf(struct mpv_obj *mpv, struct mpv_err_info *err)
{
    /* @hack spelling: a copy of mpv, which SHC keeps in R14 across
     * MPVLIB_CheckHn, MOV R4,R14 at 0x8ced551a; through mpv, which then goes
     * to the stack, the function runs 2 bytes short and 21 of its 33 words
     * differ */
    struct mpv_obj *hn = mpv;

    if (MPVLIB_CheckHn(hn) != 0) {
        return MPVERR_SetCode(NULL, MPV_ERR_GETERRINF_HN);
    }
    *err = hn->err;
    return 0;
}

Sint32 MPVERR_SetCode(struct mpv_obj *mpv, Sint32 code)
{
    if (mpv == NULL) {
        mpverr_SetCodeSub(&mpverrinf, code);
    } else {
        mpverr_SetCodeSub(&mpv->err, code);
    }
    return code;
}

/* record code in err, and pass a nonzero code to err's function, if any */
static void mpverr_SetCodeSub(struct mpv_err_info *err, Sint32 code)
{
    void (*func)(void *obj, Sint32 code);

    err->code = code;
    if (code != 0) {
        func = err->func;
        if (func != NULL) {
            func(err->obj, code);
        }
    }
}
