/* sound buffer handles, each with the number of a stream ring buffer bank
 * (mw_sdbuf.h) */
#include <mw_sdbuf.h>
#include <sg_xpt.h>
#include <string.h>

#define MWSNDBUF_MAX 8
/* the first buffer's number; the others count down from it */
#define MWSNDBUF_TOP_NO 15

typedef struct {
    Sint32 used;
    Sint32 no;
} MWSNDBUF_OBJ;

MWSNDBUF_OBJ mwg_snd_buf[MWSNDBUF_MAX];

void mwSndBufInit(void)
{
    Sint32 i;

    memset(mwg_snd_buf, 0, sizeof(mwg_snd_buf));
    for (i = 0; i < MWSNDBUF_MAX; i++) {
        mwg_snd_buf[i].no = MWSNDBUF_TOP_NO - i;
    }
}

void *mwSndBufCreate(void)
{
    MWSNDBUF_OBJ *buf = NULL;
    Sint32 i;

    for (i = 0; i < MWSNDBUF_MAX; i++) {
        if (mwg_snd_buf[i].used == 0) {
            buf = &mwg_snd_buf[i];
            buf->used = 1;
            break;
        }
    }
    return buf;
}

void mwSndBufDestroy(void *sndbuf)
{
    MWSNDBUF_OBJ *buf = sndbuf;

    buf->used = 0;
}

Sint32 mwSndBufGetNo(void *sndbuf)
{
    MWSNDBUF_OBJ *buf = sndbuf;

    return buf->no;
}
