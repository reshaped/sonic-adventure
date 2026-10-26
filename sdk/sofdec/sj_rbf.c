/* ring buffer stream joints. extra bytes past the buffer's end mirror its
 * start, so a chunk can run past the end instead of wrapping */
#include <machine.h>
#include <sg_xpt.h>
#include <sj.h>
#include <string.h>

#define SJRBF_MAX 32

typedef struct {
    SJ_OBJ sj;
    Sint32 used;
    UUID *uuid;
    Sint32 data_len;
    Sint32 free_len;
    Sint32 free_pos; /* the free line's offset in buf */
    Sint32 data_pos; /* the data line's offset in buf */
    char *buf;
    Sint32 bsize;
    Sint32 xsize; /* bytes past bsize that mirror the first xsize */
    void (*err_func)(void *obj, Sint32 ecode);
    void *err_obj;
} SJRBF_OBJ;
typedef SJRBF_OBJ *SJRBF;

#define MIN(a, b) (((a) < (b)) ? (a) : (b))

void SJRBF_Destroy(SJ sj);
UUID *SJRBF_GetUuid(SJ sj);
void SJRBF_Reset(SJ sj);
void SJRBF_GetChunk(SJ sj, Sint32 id, Sint32 nbyte, SJCK *ck);
void SJRBF_UngetChunk(SJ sj, Sint32 id, SJCK *ck);
void SJRBF_PutChunk(SJ sj, Sint32 id, SJCK *ck);
Sint32 SJRBF_GetNumData(SJ sj, Sint32 id);
Sint32 SJRBF_IsGetChunk(SJ sj, Sint32 id, Sint32 nbyte, Sint32 *rbyte);
void SJRBF_EntryErrFunc(SJ sj, void (*func)(void *obj, Sint32 ecode),
                        void *obj);
Sint32 SJRBF_Lock(void);
void SJRBF_Unlock(SJRBF sjrbf, Sint32 mask);
void SJRBF_Error(void *obj, Sint32 ecode);

static const UUID sjrbf_uuid = {
    0x3b9a9e81,
    0x0dbb,
    0x11d2,
    {0xa6, 0xbf, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00}};
SJ_IF sjrbf_vtbl = {NULL,
                    NULL,
                    NULL,
                    SJRBF_Destroy,
                    SJRBF_GetUuid,
                    SJRBF_Reset,
                    SJRBF_GetChunk,
                    SJRBF_UngetChunk,
                    SJRBF_PutChunk,
                    SJRBF_GetNumData,
                    SJRBF_IsGetChunk,
                    SJRBF_EntryErrFunc};
SJRBF_OBJ sjrbf_obj[SJRBF_MAX];

/* mask every interrupt, and return the old mask for SJRBF_Unlock */
/* @hack type: SJRBF_Lock takes no joint, though SJRBF_Unlock does. given one
 * and passed sj, three callers allocate other registers: 7 of 105 words differ
 * in SJRBF_GetChunk, 6 of 93 in SJRBF_PutChunk and 49 of 67 in
 * SJRBF_IsGetChunk */
Sint32 SJRBF_Lock(void)
{
    Sint32 mask = get_imask();

    set_imask(IMASK_ALL);
    return mask;
}

void SJRBF_Unlock(SJRBF sjrbf, Sint32 mask)
{
    set_imask(mask);
}

/* the default error function, which loops forever */
void SJRBF_Error(void *obj, Sint32 ecode)
{
    for (;;) {
    }
}

void SJRBF_Init(void)
{
    memset(sjrbf_obj, 0, sizeof(sjrbf_obj));
}

void SJRBF_Finish(void)
{
}

SJ SJRBF_Create(char *buf, Sint32 bsize, Sint32 xsize)
{
    SJRBF sjrbf;
    int i;

    for (i = 0; i < SJRBF_MAX; i++) {
        if (sjrbf_obj[i].used == false) {
            break;
        }
    }
    if (i == SJRBF_MAX) {
        return NULL;
    }
    sjrbf = &sjrbf_obj[i];
    sjrbf->used = true;
    sjrbf->sj.vtbl = &sjrbf_vtbl;
    sjrbf->buf = buf;
    sjrbf->bsize = bsize;
    sjrbf->xsize = xsize;
    sjrbf->uuid = (UUID *)&sjrbf_uuid;
    sjrbf->err_func = SJRBF_Error;
    sjrbf->err_obj = sjrbf;
    SJRBF_Reset((SJ)sjrbf);
    return (SJ)sjrbf;
}

void SJRBF_Destroy(SJ sj)
{
    SJRBF sjrbf = (SJRBF)sj;

    if (sjrbf != NULL) {
        memset(sjrbf, 0, sizeof(SJRBF_OBJ));
        sjrbf->used = false;
    }
}

UUID *SJRBF_GetUuid(SJ sj)
{
    return ((SJRBF)sj)->uuid;
}

void SJRBF_EntryErrFunc(SJ sj, void (*func)(void *obj, Sint32 ecode), void *obj)
{
    SJRBF sjrbf = (SJRBF)sj;

    sjrbf->err_func = func;
    sjrbf->err_obj = obj;
}

void SJRBF_Reset(SJ sj)
{
    SJRBF sjrbf = (SJRBF)sj;

    sjrbf->data_len = 0;
    sjrbf->free_len = sjrbf->bsize;
    sjrbf->free_pos = 0;
    sjrbf->data_pos = 0;
}

Sint32 SJRBF_GetNumData(SJ sj, Sint32 id)
{
    SJRBF sjrbf = (SJRBF)sj;

    if (id == SJ_LIN_DATA) {
        return sjrbf->data_len;
    }
    if (id == SJ_LIN_FREE) {
        return sjrbf->free_len;
    }
    if (sjrbf->err_func != NULL) {
        sjrbf->err_func(sjrbf->err_obj, SJ_ERR_PRM);
    }
    return 0;
}

/* the chunk ends no further than xsize bytes past the buffer's end */
void SJRBF_GetChunk(SJ sj, Sint32 id, Sint32 nbyte, SJCK *ck)
{
    SJRBF sjrbf = (SJRBF)sj;
    Sint32 mask = SJRBF_Lock();

    if (id == SJ_LIN_FREE) {
        ck->len =
            MIN(sjrbf->free_len, sjrbf->bsize - sjrbf->free_pos + sjrbf->xsize);
        ck->len = MIN(ck->len, nbyte);
        ck->data = sjrbf->buf + sjrbf->free_pos;
        sjrbf->free_pos = (sjrbf->free_pos + ck->len) % sjrbf->bsize;
        sjrbf->free_len -= ck->len;
    } else if (id == SJ_LIN_DATA) {
        ck->len =
            MIN(sjrbf->data_len, sjrbf->bsize - sjrbf->data_pos + sjrbf->xsize);
        ck->len = MIN(ck->len, nbyte);
        ck->data = sjrbf->buf + sjrbf->data_pos;
        sjrbf->data_pos = (sjrbf->data_pos + ck->len) % sjrbf->bsize;
        sjrbf->data_len -= ck->len;
    } else {
        ck->len = 0;
        ck->data = NULL;
        if (sjrbf->err_func != NULL) {
            sjrbf->err_func(sjrbf->err_obj, SJ_ERR_PRM);
        }
    }
    SJRBF_Unlock(sjrbf, mask);
}

void SJRBF_PutChunk(SJ sj, Sint32 id, SJCK *ck)
{
    SJRBF sjrbf;
    Sint32 mask;
    Sint32 offset;
    Sint32 len;

    sjrbf = (SJRBF)sj;
    mask = SJRBF_Lock();
    if (id == SJ_LIN_DATA) {
        sjrbf->data_len += ck->len;
        /* data written in the buffer's first xsize bytes is copied after its
         * end */
        offset = ck->data - sjrbf->buf;
        if (offset < sjrbf->xsize) {
            len = MIN(ck->len, sjrbf->xsize - offset);
            /* @hack spelling: the destination as &buf[bsize + offset], which
             * adds offset to bsize before buf (ADD R12,R4 at 0x8c6705ea). as
             * buf + bsize + offset, SHC adds buf first and 7 of 93 words
             * differ */
            memcpy(&sjrbf->buf[sjrbf->bsize + offset], ck->data, len);
        }
        /* data written past the buffer's end is copied to its start */
        offset = ck->data - sjrbf->buf + ck->len;
        if (offset > sjrbf->bsize) {
            len = MIN(ck->len, offset - sjrbf->bsize);
            /* @hack spelling: the source as &buf[offset - len], which subtracts
             * len from offset before it adds buf (SUB R13,R12 at
             * 0x8c670616). as buf + offset - len, SHC adds first and 4 of 93
             * words differ */
            memcpy(sjrbf->buf, &sjrbf->buf[offset - len], len);
        }
    } else if (id == SJ_LIN_FREE) {
        sjrbf->free_len += ck->len;
    } else {
        ck->len = 0;
        ck->data = NULL;
        if (sjrbf->err_func != NULL) {
            sjrbf->err_func(sjrbf->err_obj, SJ_ERR_PRM);
        }
    }
    SJRBF_Unlock(sjrbf, mask);
}

/* return ck to the start of line id. ck must end where the line now starts,
 * as the last chunk taken from the line does, and as any tail of that chunk
 * does. for any other chunk, the joint reports SJ_ERR_PRM. for an id other
 * than SJ_LIN_FREE and SJ_LIN_DATA, it clears ck and reports SJ_ERR_PRM */
void SJRBF_UngetChunk(SJ sj, Sint32 id, SJCK *ck)
{
    SJRBF sjrbf;
    Sint32 mask;
    Sint32 pos;
    Sint32 offset;

    sjrbf = (SJRBF)sj;
    mask = SJRBF_Lock();
    if (id == SJ_LIN_FREE) {
        pos = (sjrbf->free_pos + sjrbf->bsize - ck->len) % sjrbf->bsize;
        offset = (ck->data - sjrbf->buf) % sjrbf->bsize;
        if (pos == offset) {
            sjrbf->free_pos = pos;
            sjrbf->free_len += ck->len;
        } else if (sjrbf->err_func != NULL) {
            sjrbf->err_func(sjrbf->err_obj, SJ_ERR_PRM);
        }
    } else if (id == SJ_LIN_DATA) {
        pos = (sjrbf->data_pos + sjrbf->bsize - ck->len) % sjrbf->bsize;
        offset = (ck->data - sjrbf->buf) % sjrbf->bsize;
        if (pos == offset) {
            sjrbf->data_pos = pos;
            sjrbf->data_len += ck->len;
        } else if (sjrbf->err_func != NULL) {
            sjrbf->err_func(sjrbf->err_obj, SJ_ERR_PRM);
        }
    } else {
        ck->len = 0;
        ck->data = NULL;
        if (sjrbf->err_func != NULL) {
            sjrbf->err_func(sjrbf->err_obj, SJ_ERR_PRM);
        }
    }
    SJRBF_Unlock(sjrbf, mask);
}

Sint32 SJRBF_IsGetChunk(SJ sj, Sint32 id, Sint32 nbyte, Sint32 *rbyte)
{
    SJRBF sjrbf;
    Sint32 mask;
    Sint32 len;

    sjrbf = (SJRBF)sj;
    mask = SJRBF_Lock();
    if (id == SJ_LIN_FREE) {
        len =
            MIN(sjrbf->free_len, sjrbf->bsize - sjrbf->free_pos + sjrbf->xsize);
        len = MIN(len, nbyte);
    } else if (id == SJ_LIN_DATA) {
        len =
            MIN(sjrbf->data_len, sjrbf->bsize - sjrbf->data_pos + sjrbf->xsize);
        len = MIN(len, nbyte);
    } else {
        len = 0;
        if (sjrbf->err_func != NULL) {
            sjrbf->err_func(sjrbf->err_obj, SJ_ERR_PRM);
        }
    }
    *rbyte = len;
    SJRBF_Unlock(sjrbf, mask);
    return len == nbyte;
}

char *SJRBF_GetBufPtr(SJ sj)
{
    return ((SJRBF)sj)->buf;
}

Sint32 SJRBF_GetBufSize(SJ sj)
{
    return ((SJRBF)sj)->bsize;
}

Sint32 SJRBF_GetXtrSize(SJ sj)
{
    return ((SJRBF)sj)->xsize;
}
