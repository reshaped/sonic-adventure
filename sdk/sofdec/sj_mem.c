/* memory stream joints: a buffer's bytes on the data line, read through once,
 * and a free line that stays empty */
#include <machine.h>
#include <sg_xpt.h>
#include <sj.h>
#include <string.h>

#define SJMEM_MAX 32

typedef struct {
    SJ_OBJ sj;
    Sint32 used;
    UUID *uuid;
    Sint32 rest; /* bytes left on the data line */
    Sint32 pos; /* the data line's offset in buf */
    char *buf;
    Sint32 bsize;
    void (*err_func)(void *obj, Sint32 ecode);
    void *err_obj;
} SJMEM_OBJ;
typedef SJMEM_OBJ *SJMEM;

void SJMEM_Finish(void);
void SJMEM_Destroy(SJ sj);
UUID *SJMEM_GetUuid(SJ sj);
void SJMEM_EntryErrFunc(SJ sj, void (*func)(void *obj, Sint32 ecode),
                        void *obj);
void SJMEM_Reset(SJ sj);
Sint32 SJMEM_GetNumData(SJ sj, Sint32 id);
void SJMEM_GetChunk(SJ sj, Sint32 id, Sint32 nbyte, SJCK *ck);
void SJMEM_PutChunk(SJ sj, Sint32 id, SJCK *ck);
void SJMEM_UngetChunk(SJ sj, Sint32 id, SJCK *ck);
Sint32 SJMEM_IsGetChunk(SJ sj, Sint32 id, Sint32 nbyte, Sint32 *rbyte);
char *SJMEM_GetBufPtr(SJ sj);
Sint32 SJMEM_GetBufSize(SJ sj);
Sint32 SJMEM_Lock(SJ sj);
void SJMEM_Unlock(SJ sj, Sint32 mask);
void SJMEM_Error(void *obj, Sint32 ecode);

static const UUID sjmem_uuid = {
    0xdd9eee41,
    0x1679,
    0x11d2,
    {0x93, 0x6c, 0x00, 0x60, 0x08, 0x94, 0x48, 0xbc}};
SJ_IF sjmem_vtbl = {NULL,
                    NULL,
                    NULL,
                    SJMEM_Destroy,
                    SJMEM_GetUuid,
                    SJMEM_Reset,
                    SJMEM_GetChunk,
                    SJMEM_UngetChunk,
                    SJMEM_PutChunk,
                    SJMEM_GetNumData,
                    SJMEM_IsGetChunk,
                    SJMEM_EntryErrFunc};
SJMEM_OBJ sjmem_obj[SJMEM_MAX];

/* mask every interrupt, and return the old mask for SJMEM_Unlock. sj is not
 * read */
Sint32 SJMEM_Lock(SJ sj)
{
    Sint32 mask = get_imask();

    set_imask(IMASK_ALL);
    return mask;
}

void SJMEM_Unlock(SJ sj, Sint32 mask)
{
    set_imask(mask);
}

/* the default error function, which loops forever */
void SJMEM_Error(void *obj, Sint32 ecode)
{
    for (;;) {
    }
}

void SJMEM_Init(void)
{
    memset(sjmem_obj, 0, sizeof(sjmem_obj));
}

void SJMEM_Finish(void)
{
}

SJ SJMEM_Create(char *data, Sint32 bsize)
{
    int i;
    SJMEM sjmem;

    for (i = 0; i < SJMEM_MAX; i++) {
        if (sjmem_obj[i].used == false) {
            break;
        }
    }
    if (i == SJMEM_MAX) {
        return NULL;
    }
    sjmem = &sjmem_obj[i];
    sjmem->used = true;
    sjmem->sj.vtbl = &sjmem_vtbl;
    sjmem->buf = data;
    sjmem->bsize = bsize;
    sjmem->uuid = (UUID *)&sjmem_uuid;
    sjmem->err_func = SJMEM_Error;
    sjmem->err_obj = sjmem;
    SJMEM_Reset(&sjmem->sj);
    return &sjmem->sj;
}

void SJMEM_Destroy(SJ sj)
{
    SJMEM sjmem = (SJMEM)sj;

    if (sjmem != NULL) {
        memset(sjmem, 0, sizeof(*sjmem));
        sjmem->used = false;
    }
}

UUID *SJMEM_GetUuid(SJ sj)
{
    return ((SJMEM)sj)->uuid;
}

void SJMEM_EntryErrFunc(SJ sj, void (*func)(void *obj, Sint32 ecode), void *obj)
{
    SJMEM sjmem = (SJMEM)sj;

    sjmem->err_func = func;
    sjmem->err_obj = obj;
}

void SJMEM_Reset(SJ sj)
{
    SJMEM sjmem = (SJMEM)sj;

    sjmem->rest = sjmem->bsize;
    sjmem->pos = 0;
}

Sint32 SJMEM_GetNumData(SJ sj, Sint32 id)
{
    SJMEM sjmem = (SJMEM)sj;

    if (id == SJ_LIN_DATA) {
        return sjmem->rest;
    }
    if (id == SJ_LIN_FREE) {
        return 0;
    }
    if (sjmem->err_func != NULL) {
        sjmem->err_func(sjmem->err_obj, SJ_ERR_PRM);
    }
    return 0;
}

void SJMEM_GetChunk(SJ sj, Sint32 id, Sint32 nbyte, SJCK *ck)
{
    SJMEM sjmem = (SJMEM)sj;
    Sint32 rest;
    Sint32 mask;

    /* @hack type: sjmem cast back to SJ for SJMEM_Lock and SJMEM_Unlock,
     * which lets sj and sjmem share R14 (MOV R4,R14 at 0x8c6701fa). passed
     * sj, the calls keep it in a saved register of its own and 52 of 57
     * words differ */
    mask = SJMEM_Lock((SJ)sjmem);
    if (id == SJ_LIN_FREE) {
        ck->len = 0;
        ck->data = NULL;
    } else if (id == SJ_LIN_DATA) {
        /* @hack spelling: sjmem->rest read once, through a local, which puts
         * id in the frame at 4 and nbyte at 0 (0x8c6701f4, 0x8c6701f6). read
         * in place twice, the two trade slots and 6 of 57 words differ */
        rest = sjmem->rest;
        ck->len = rest < nbyte ? rest : nbyte;
        ck->data = sjmem->buf + sjmem->pos;
        sjmem->pos += ck->len;
        sjmem->rest -= ck->len;
    } else {
        ck->len = 0;
        ck->data = NULL;
        if (sjmem->err_func != NULL) {
            sjmem->err_func(sjmem->err_obj, SJ_ERR_PRM);
        }
    }
    SJMEM_Unlock((SJ)sjmem, mask);
}

/* a chunk put on either line changes nothing: the data line's bytes are fixed,
 * and the free line stays empty */
void SJMEM_PutChunk(SJ sj, Sint32 id, SJCK *ck)
{
    SJMEM sjmem = (SJMEM)sj;
    /* @hack type: sjmem cast back to SJ for SJMEM_Lock and SJMEM_Unlock,
     * which lets sj and sjmem share R14 (MOV R4,R14 at 0x8c670280). passed
     * sj, the calls keep it in a saved register of its own and 30 of 32
     * words differ */
    Sint32 mask = SJMEM_Lock((SJ)sjmem);

    /* @hack spelling: the two empty arms, which keep ck in R3 for the stores
     * (0x8c670290, 0x8c670294). with both lines tested in one condition, R2
     * takes it and 2 of 32 words differ */
    if (id == SJ_LIN_FREE) {
    } else if (id == SJ_LIN_DATA) {
    } else {
        ck->len = 0;
        ck->data = NULL;
        if (sjmem->err_func != NULL) {
            sjmem->err_func(sjmem->err_obj, SJ_ERR_PRM);
        }
    }
    SJMEM_Unlock((SJ)sjmem, mask);
}

void SJMEM_UngetChunk(SJ sj, Sint32 id, SJCK *ck)
{
    SJMEM sjmem = (SJMEM)sj;
    /* @hack type: sjmem cast back to SJ for SJMEM_Lock and SJMEM_Unlock,
     * which lets sj and sjmem share R14 (MOV R4,R14 at 0x8c6702c2). passed
     * sj, the calls keep it in a saved register of its own and 60 of 64
     * words differ */
    Sint32 mask = SJMEM_Lock((SJ)sjmem);

    if (id == SJ_LIN_FREE) {
        if (sjmem->err_func != NULL) {
            sjmem->err_func(sjmem->err_obj, SJ_ERR_PRM);
        }
    } else if (id == SJ_LIN_DATA) {
        Sint32 bsize;
        Sint32 rest;
        Sint32 offset;
        Sint32 pos = sjmem->pos - ck->len;

        if (pos <= 0) {
            pos = 0;
        }
        sjmem->pos = pos;
        /* @hack spelling: the size read through a local, which loads it
         * before the sum it is compared with (MOV.L @(24,R14),R5 at
         * 0x8c6702ea). read in place, it loads after the sum and 7 of 64
         * words differ */
        bsize = sjmem->bsize;
        /* @hack spelling: the sum through rest, which passes the minimum
         * through R2 (0x8c6702f8 to 0x8c6702fc). written in place, R3 takes
         * it and 3 of 64 words differ */
        rest = sjmem->rest + ck->len;
        sjmem->rest = bsize < rest ? bsize : rest;
        /* @hack spelling: the chunk's offset through a local, which computes
         * it in R5 (SUB R3,R5 at 0x8c670302). compared in place, R1 takes it
         * and 5 of 64 words differ */
        offset = ck->data - sjmem->buf;
        if (pos != offset && sjmem->err_func != NULL) {
            sjmem->err_func(sjmem->err_obj, SJ_ERR_PRM);
        }
    } else {
        ck->len = 0;
        ck->data = NULL;
        if (sjmem->err_func != NULL) {
            sjmem->err_func(sjmem->err_obj, SJ_ERR_PRM);
        }
    }
    SJMEM_Unlock((SJ)sjmem, mask);
}

Sint32 SJMEM_IsGetChunk(SJ sj, Sint32 id, Sint32 nbyte, Sint32 *rbyte)
{
    SJMEM sjmem = (SJMEM)sj;
    Sint32 len;
    /* @hack type: sjmem cast back to SJ for SJMEM_Lock and SJMEM_Unlock,
     * which lets sj and sjmem share R14 (MOV R4,R14 at 0x8c670346). passed
     * sj, the calls keep it in a saved register of its own and 44 of 47
     * words differ */
    Sint32 mask = SJMEM_Lock((SJ)sjmem);

    if (id == SJ_LIN_FREE) {
        len = 0;
    } else if (id == SJ_LIN_DATA) {
        len = sjmem->rest < nbyte ? sjmem->rest : nbyte;
    } else {
        len = 0;
        if (sjmem->err_func != NULL) {
            sjmem->err_func(sjmem->err_obj, SJ_ERR_PRM);
        }
    }
    *rbyte = len;
    SJMEM_Unlock((SJ)sjmem, mask);
    return len == nbyte;
}

char *SJMEM_GetBufPtr(SJ sj)
{
    return ((SJMEM)sj)->buf;
}

Sint32 SJMEM_GetBufSize(SJ sj)
{
    return ((SJMEM)sj)->bsize;
}
