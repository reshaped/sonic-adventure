/* the middleware manager. it calls the server, vertical blank and frame start
 * functions each library adds, and passes error messages to one function */
#include <machine.h>
#include <sg_chain.h>
#include <sg_mwmng.h>
#include <sg_mwply.h>
#include <sg_xpt.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

/* how many functions each table holds */
#define MNG_MAX_FUNCS 16

struct mng_func {
    void (*func)(void *obj);
    void *obj;
};

static struct mng_func server_funcs[MNG_MAX_FUNCS];
static struct mng_func vsync_funcs[MNG_MAX_FUNCS];
static struct mng_func frame_funcs[MNG_MAX_FUNCS];
static struct mng_err_func {
    void (*func)(void *obj, char *msg);
    void *obj;
} err_func;
static char err_msg[256];

char *mwg_mng_build = "\nmwMng Ver.1.04 Build:Nov 11 1998 21:18:39\n";
SYCHAIN mwg_mng_chain = 0;
/* the number of mwMngInit calls less the number of mwMngFinish calls */
Sint32 mwg_mng_init_level = 0;

void mwlMngLockCs(Sint32 *mask);
void mwlMngUnlockCs(const Sint32 *mask);

/* enter the manager's critical section: mask every interrupt, and keep the
 * old mask in *mask for mwlMngUnlockCs */
void mwlMngLockCs(Sint32 *mask)
{
    *mask = get_imask();
    set_imask(IMASK_ALL);
}

void mwlMngUnlockCs(const Sint32 *mask)
{
    set_imask(*mask);
}

void mwMngEntryErrFn(void (*func)(void *obj, char *msg), void *obj)
{
    Sint32 mask;

    mwlMngLockCs(&mask);
    err_func.func = func;
    err_func.obj = obj;
    mwlMngUnlockCs(&mask);
}

void mwMngError(const char *format, ...)
{
    va_list args;
    /* @hack spelling: the error function reached through e, whose address SHC
     * keeps in R14 for the reads of func and obj (0x8ced13e8, 0x8ced13ec).
     * with err_func named in place, obj takes a pool word of its own, the
     * object runs 4 bytes long and 10 of 39 words differ */
    struct mng_err_func *e;

    memset(err_msg, 0, sizeof(err_msg));
    va_start(args, format);
    vsprintf(err_msg, format, args);
    va_end(args);
    e = &err_func;
    if (e->func != NULL) {
        e->func(e->obj, err_msg);
    }
}

void mwMngEntryServerFunc(void (*func)(void *obj), void *obj)
{
    Sint32 mask;
    Sint32 i;
    struct mng_func *f;

    mwlMngLockCs(&mask);
    for (i = 0; i < MNG_MAX_FUNCS; ++i) {
        f = &server_funcs[i];
        if (f->func != NULL) {
            continue;
        }
        f->func = func;
        f->obj = obj;
        break;
    }
    if (i == MNG_MAX_FUNCS) {
        mwMngError("mwMng:too many libarary");
    }
    mwlMngUnlockCs(&mask);
}

void mwMngDeleteServerFunc(void (*func)(void *obj))
{
    Sint32 mask;
    Sint32 i;
    struct mng_func *f;

    mwlMngLockCs(&mask);
    for (i = 0; i < MNG_MAX_FUNCS; ++i) {
        f = &server_funcs[i];
        if (f->func != func) {
            continue;
        }
        f->func = NULL;
        f->obj = NULL;
    }
    mwlMngUnlockCs(&mask);
}

void mwMngExecServer(void)
{
    Sint32 i;
    struct mng_func *f;

    for (i = 0; i < MNG_MAX_FUNCS; ++i) {
        f = &server_funcs[i];
        if (f->func == NULL) {
            continue;
        }
        f->func(f->obj);
    }
}

void mwMngEntryVsyncFunc(void (*func)(void *obj), void *obj)
{
    Sint32 mask;
    Sint32 i;
    struct mng_func *f;

    mwlMngLockCs(&mask);
    for (i = 0; i < MNG_MAX_FUNCS; ++i) {
        f = &vsync_funcs[i];
        if (f->func != NULL) {
            continue;
        }
        f->func = func;
        f->obj = obj;
        break;
    }
    if (i == MNG_MAX_FUNCS) {
        mwMngError("mwMng:too many libarary");
    }
    mwlMngUnlockCs(&mask);
}

void mwMngDeleteVsyncFunc(void (*func)(void *obj))
{
    Sint32 mask;
    Sint32 i;
    struct mng_func *f;

    mwlMngLockCs(&mask);
    for (i = 0; i < MNG_MAX_FUNCS; ++i) {
        f = &vsync_funcs[i];
        if (f->func != func) {
            continue;
        }
        f->func = NULL;
        f->obj = NULL;
    }
    mwlMngUnlockCs(&mask);
}

/* the handler mwMngInit adds to the vertical blank chain */
void mwMngVsync(void *obj)
{
    Sint32 i;
    struct mng_func *f;

    for (i = 0; i < MNG_MAX_FUNCS; ++i) {
        f = &vsync_funcs[i];
        if (f->func == NULL) {
            continue;
        }
        f->func(f->obj);
    }
}

void mwMngEntryStartFrameFunc(void (*func)(void *obj), void *obj)
{
    Sint32 mask;
    Sint32 i;
    struct mng_func *f;

    mwlMngLockCs(&mask);
    for (i = 0; i < MNG_MAX_FUNCS; ++i) {
        f = &frame_funcs[i];
        if (f->func != NULL) {
            continue;
        }
        f->func = func;
        f->obj = obj;
        break;
    }
    if (i == MNG_MAX_FUNCS) {
        mwMngError("mwMng:too many libarary");
    }
    mwlMngUnlockCs(&mask);
}

void mwMngDeleteStartFrameFunc(void (*func)(void *obj))
{
    Sint32 mask;
    Sint32 i;
    struct mng_func *f;

    mwlMngLockCs(&mask);
    for (i = 0; i < MNG_MAX_FUNCS; ++i) {
        f = &frame_funcs[i];
        if (f->func != func) {
            continue;
        }
        f->func = NULL;
        f->obj = NULL;
    }
    mwlMngUnlockCs(&mask);
}

void mwMngStartFrame(void)
{
    Sint32 i;
    struct mng_func *f;

    for (i = 0; i < MNG_MAX_FUNCS; ++i) {
        f = &frame_funcs[i];
        if (f->func == NULL) {
            continue;
        }
        f->func(f->obj);
    }
}

void mwMngInit(void)
{
    if (mwg_mng_init_level == 0) {
        memset(server_funcs, 0, sizeof(server_funcs));
        memset(vsync_funcs, 0, sizeof(vsync_funcs));
        memset(frame_funcs, 0, sizeof(frame_funcs));
        memset(&err_func, 0, sizeof(err_func));
        mwg_mng_chain = syChainAddHandler(SYD_CHAIN_EVENT_IML6_VBLANK,
                                          mwMngVsync, MWD_PLY_PRI_VBLNK, NULL);
    }
    ++mwg_mng_init_level;
}

void mwMngFinish(void)
{
    --mwg_mng_init_level;
    if (mwg_mng_init_level == 0) {
        if (mwg_mng_chain != 0) {
            syChainDeleteHandler(mwg_mng_chain);
            mwg_mng_chain = 0;
        }
    }
}
