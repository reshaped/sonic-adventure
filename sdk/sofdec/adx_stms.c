/* ADX disc streams, each of which reads a GD file system file into a stream
 * joint, one request at a time */
#include <sg_xpt.h>
#include <shinobi.h>
#include <sj.h>
#include <sofdec.h>
#include <string.h>

#define ADXSTM_MAX_OBJ 32
#define ADXSTM_SCT_SIZE 2048
/* maxsct until ADXSTM_SetReqRdSize sets it: no limit */
#define ADXSTM_MAX_SCT 0x7fffffff

typedef struct {
    Sint8 used;
    Sint8 stat;
    Sint8 rqst; /* 1 while a read is in progress */
    SJ sj;
    GDFS gdfs;
    Sint32 stsct; /* the read position, in sectors, when ADXSTM_Start ran */
    Sint32 ofst;
    Sint32 fsize;
    Sint32 rdsize; /* bytes a read asks of the joint's free line */
    Sint32 bufsize; /* the data, in bytes, below which a server pass reads */
    Sint32 rdsct; /* sectors of the read in progress */
    SJCK ck; /* the chunk the read fills */
    Sint32 maxsct; /* the most sectors a read asks for */
    Sint32 eossct; /* the sector reads stop at: the file's end by default */
    Sint32 rdbyte; /* bytes read since ADXSTM_Start */
    /* called with eosobj when a read ends at eossct */
    void (*eosfunc)(void *obj);
    void *eosobj;
    char fname[32];
    Sint32 unk_60;
} ADXSTM_OBJ;

/* the passes that found a stream with no joint, or a joint with no function
 * table */
Sint32 adxstm_sj_internal_error_cnt = 0;
ADXSTM_OBJ adxstmf_obj[ADXSTM_MAX_OBJ];

Sint32 ADXSTM_Init(void)
{
    memset(adxstmf_obj, 0, sizeof(adxstmf_obj));
    return 1;
}

void ADXSTM_Reset(void)
{
}

void ADXSTM_Finish(void)
{
}

/* create a stream that reads fsize bytes of gdfs into sj. returns NULL when
 * all ADXSTM_MAX_OBJ are in use. ADXSTM_OpenFname, ADXSTM_OpenRange and
 * ADXSTM_OpenFileRange leave the result unchecked */
ADXSTM_OBJ *ADXSTMF_Create(GDFS gdfs, Sint32 ofst, Sint32 fsize, SJ sj)
{
    Sint32 i;
    ADXSTM_OBJ *stm;

    for (i = 0; i < ADXSTM_MAX_OBJ; i++) {
        if (adxstmf_obj[i].used == 0) {
            break;
        }
    }
    if (i == ADXSTM_MAX_OBJ) {
        return NULL;
    }
    stm = &adxstmf_obj[i];
    stm->stat = ADXSTM_STAT_STOP;
    stm->rqst = 0;
    stm->sj = sj;
    stm->gdfs = gdfs;
    stm->stsct = 0;
    stm->ofst = ofst;
    stm->fsize = fsize;
    stm->maxsct = ADXSTM_MAX_SCT;
    stm->eossct = (stm->fsize + ADXSTM_SCT_SIZE - 1) / ADXSTM_SCT_SIZE;
    stm->used = 1;
    stm->bufsize = stm->rdsize = sj->vtbl->GetNumData(sj, SJ_LIN_FREE);
    return stm;
}

void ADXSTMF_Destroy(ADXSTM_OBJ *stm)
{
    memset(stm, 0, sizeof(ADXSTM_OBJ));
}

ADXSTM ADXSTM_OpenFname(char *fname, SJ sj)
{
    GDFS gdfs;
    Sint32 fsize;
    ADXSTM_OBJ *stm;

    /* @hack spelling: gdfs set in the test, which tests R0 and copies it to R14
     * in the branch's slot (TST R0,R0 at 0x8c66e0d6, BF/S at 0x8c66e0d8). set
     * before the test, SHC copies it first and 3 of 55 words differ */
    if ((gdfs = gdFsOpen(fname, NULL)) == NULL) {
        ADXERR_CallErrFunc("E8101210: can't open '%s' (ADXSTM_OpenFname)",
                           fname);
        return NULL;
    }
    if (gdFsGetFileSize(gdfs, &fsize) == FALSE) {
        ADXERR_CallErrFunc("E8101211: can't GetFileSize (ADXSTM_OpenFname)");
        gdFsClose(gdfs);
        return NULL;
    }
    /* @bug memory: when every stream is in use, this writes the name at
     * address 0x40, through the null stream (ADD #64,R4 in strncpy's slot at
     * 0x8c66e120), and returns NULL with gdfs left open, which nothing then
     * closes */
    stm = ADXSTMF_Create(gdfs, 0, fsize, sj);
    strncpy(stm->fname, fname, sizeof(stm->fname));
    return stm;
}

ADXSTM ADXSTM_OpenRange(Sint32 fad, Sint32 nsct, SJ sj)
{
    GDFS gdfs;
    ADXSTM_OBJ *stm;

    /* @hack spelling: gdfs set in the test, as in ADXSTM_OpenFname (TST R0,R0
     * at 0x8c66e178, BF/S at 0x8c66e17a). set before the test, 3 of 46 words
     * differ */
    if ((gdfs = gdFsOpenRange(fad, nsct)) == NULL) {
        ADXERR_CallErrFunc(
            "E8101212: can't open fad=%d, nsct=%d (ADXSTM_OpenRange)", fad,
            nsct);
        return NULL;
    }
    /* @bug memory: when every stream is in use, this writes "OpenRange" at
     * address 0x40, through the null stream (ADD #64,R4 in strncpy's slot at
     * 0x8c66e1b2), and returns NULL with the range left open, which nothing
     * then closes */
    stm = ADXSTMF_Create(gdfs, 0, nsct * ADXSTM_SCT_SIZE, sj);
    strncpy(stm->fname, "OpenRange", sizeof(stm->fname));
    return stm;
}

/* open a stream over the first nsct sectors of fname. ofst is not read */
ADXSTM ADXSTM_OpenFileRange(char *fname, Sint32 ofst, Sint32 nsct, SJ sj)
{
    GDFS gdfs;
    ADXSTM_OBJ *stm;

    /* @hack spelling: gdfs set in the test, as in ADXSTM_OpenFname (TST R0,R0
     * at 0x8c66e1d6, BF/S at 0x8c66e1d8). set before the test, 3 of 43 words
     * differ */
    if ((gdfs = gdFsOpen(fname, NULL)) == NULL) {
        ADXERR_CallErrFunc("E8101213: can't open '%s'(ADXSTM_OpenFnameRange)",
                           fname);
        return NULL;
    }
    /* @bug memory: when every stream is in use, this writes the name at
     * address 0x40, through the null stream (ADD #64,R4 in strncpy's slot at
     * 0x8c66e206), and returns NULL with gdfs left open, which nothing then
     * closes */
    stm = ADXSTMF_Create(gdfs, 0, nsct * ADXSTM_SCT_SIZE, sj);
    strncpy(stm->fname, fname, sizeof(stm->fname));
    return stm;
}

void ADXSTM_Close(ADXSTM stm)
{
    ADXSTM_OBJ *obj = stm;

    if (obj != NULL) {
        ADXSTM_Stop(stm);
        ADXCRS_Lock();
        if (obj->gdfs != NULL) {
            gdFsClose(obj->gdfs);
        }
        ADXSTMF_Destroy(obj);
        ADXCRS_Unlock();
    }
}

Sint32 ADXSTM_Start(ADXSTM stm)
{
    ADXSTM_OBJ *obj = stm;

    obj->stat = ADXSTM_STAT_READ;
    obj->rdbyte = 0;
    obj->stsct = gdFsTell(obj->gdfs);
    return 1;
}

void ADXSTM_Stop(ADXSTM stm)
{
    ADXSTM_OBJ *obj = stm;

    if (obj->rqst == 1 && obj->gdfs != NULL) {
        gdFsStopRd(obj->gdfs);
        obj->rqst = 0;
    }
    obj->stat = ADXSTM_STAT_STOP;
    obj->sj->vtbl->Reset(obj->sj);
}

Sint32 ADXSTM_Seek(ADXSTM stm, Sint32 sct)
{
    ADXSTM_OBJ *obj = stm;

    gdFsSeek(obj->gdfs, sct, GDD_SEEK_SET);
    return gdFsTell(obj->gdfs);
}

void ADXSTM_EntryEosFunc(ADXSTM stm, void (*func)(void *obj), void *eosobj)
{
    ADXSTM_OBJ *obj = stm;

    obj->eosfunc = func;
    obj->eosobj = eosobj;
}

/* set the sector at which reads stop: eossct, or the file's end for a negative
 * eossct */
void ADXSTM_SetEos(ADXSTM stm, Sint32 eossct)
{
    ADXSTM_OBJ *obj = stm;

    if (eossct >= 0) {
        obj->eossct = eossct;
    } else {
        obj->eossct = (obj->fsize + ADXSTM_SCT_SIZE - 1) / ADXSTM_SCT_SIZE;
    }
}

void adxstm_sj_internal_error(void)
{
    adxstm_sj_internal_error_cnt++;
}

/* one pass of a reading stream. a completed read puts its sectors on the
 * joint's data line, and calls eosfunc if it ended at eossct. a new read starts
 * when none is in progress and the joint holds less than bufsize bytes of data.
 * it asks for the sectors that fit a free chunk of up to rdsize bytes, but none
 * past eossct and at most maxsct */
void adxstmf_stat_exec(ADXSTM_OBJ *stm)
{
    SJ sj;
    Sint32 nbyte;
    Sint32 nsct;
    Sint32 stat;
    Sint32 cur;
    Sint32 end;
    Sint32 ndata;
    Sint32 rest;
    Sint32 req;
    SJCK ck1;
    SJCK ck2;
    Sint32 eos;

    sj = stm->sj;
    if (stm->rqst == 1) {
        stat = gdFsGetStat(stm->gdfs);
        if (stat == GDD_STAT_COMPLETE) {
            nbyte = stm->rdsct * ADXSTM_SCT_SIZE;
            SJ_SplitChunk(&stm->ck, nbyte, &ck1, &ck2);
            sj->vtbl->PutChunk(sj, SJ_LIN_DATA, &ck1);
            sj->vtbl->UngetChunk(sj, SJ_LIN_FREE, &ck2);
            if (gdFsTell(stm->gdfs) == stm->eossct && stm->eosfunc != NULL) {
                stm->eosfunc(stm->eosobj);
            }
            stm->rdbyte += nbyte;
            /* @hack spelling: rqst cleared and cur read in one statement, which
             * puts cur in R4 and end in R5 (MOV R0,R4 at 0x8c66e396, CMP/GE
             * R5,R4 at 0x8c66e3a4). as two statements, the two trade and 3 of
             * 165 words differ */
            stm->rqst = 0, cur = gdFsTell(stm->gdfs);
            end = (stm->fsize + ADXSTM_SCT_SIZE - 1) / ADXSTM_SCT_SIZE;
            if (cur >= end) {
                stm->stat = ADXSTM_STAT_READEND;
            }
        } else if (stat == GDD_STAT_ERR) {
            ADXERR_CallErrFunc("E8101214: GDFS error (adxstmf_stat_exec)\n");
            stm->stat = ADXSTM_STAT_ERROR;
            stm->rqst = 0;
        }
    }
    if (stm->rqst == 0) {
        /* @hack spelling: the good case tested first, with an empty arm, which
         * loads the joint's interface into R3 and GetNumData into R2
         * (0x8c66e400 to 0x8c66e406). tested the other way, the two trade and
         * 5 of 165 words differ */
        if (sj != NULL && sj->vtbl != NULL) {
        } else {
            adxstm_sj_internal_error();
            return;
        }
        ndata = sj->vtbl->GetNumData(sj, SJ_LIN_DATA);
        if (ndata < stm->bufsize) {
            sj->vtbl->GetChunk(sj, SJ_LIN_FREE, stm->rdsize, &stm->ck);
            nsct = stm->ck.len / ADXSTM_SCT_SIZE;
            /* @hack spelling: eossct read into eos before gdFsTell, which keeps
             * it in the frame across the call (MOV.L R3,@R15 at 0x8c66e42e,
             * MOV.L @R15,R4 at 0x8c66e434). read after the call, the frame is
             * 4 bytes smaller, the function is 8 bytes shorter and 140 of 165
             * words differ */
            eos = stm->eossct;
            cur = gdFsTell(stm->gdfs);
            /* @hack spelling: rest set to eos, then reduced by cur, which
             * subtracts in rest's own register (SUB R5,R4 at 0x8c66e438). as
             * rest = eos - cur, SHC subtracts in R3 and copies the difference
             * to R4, the function is 2 bytes longer and 40 of 165 words
             * differ */
            rest = eos;
            rest -= cur;
            if (nsct < rest) {
                rest = nsct;
            }
            req = stm->maxsct;
            if (rest < req) {
                req = rest;
            }
            /* @hack spelling: a goto to the unget below, which tests req and
             * branches straight to the one UngetChunk call (BT at
             * 0x8c66e44a). with the unget written in each arm, SHC loads the
             * call's address in the first and branches to the second's call,
             * the function is 8 bytes longer and 32 of 165 words differ */
            if (req == 0) {
                goto unget;
            }
            stm->rdsct = gdFsReqRd32(stm->gdfs, req, stm->ck.data);
            if (stm->rdsct <= 0) {
            unget:
                sj->vtbl->UngetChunk(sj, SJ_LIN_FREE, &stm->ck);
            } else {
                stm->rqst = 1;
            }
        }
    }
}

void ADXSTMF_ExecHndl(ADXSTM_OBJ *stm)
{
    if (stm->stat == ADXSTM_STAT_READ) {
        adxstmf_stat_exec(stm);
    }
}

void ADXSTM_ExecServer(void)
{
    Sint32 i;

    for (i = 0; i < ADXSTM_MAX_OBJ; i++) {
        if (adxstmf_obj[i].used == 1) {
            ADXSTMF_ExecHndl(&adxstmf_obj[i]);
        }
    }
}

Sint32 ADXSTM_GetStat(ADXSTM stm)
{
    ADXSTM_OBJ *obj = stm;

    return obj->stat;
}

Sint32 ADXSTM_GetCurOfst(ADXSTM stm, Sint32 *ofst)
{
    ADXSTM_OBJ *obj = stm;

    *ofst = gdFsTell(obj->gdfs) - obj->stsct;
    return 1;
}

Sint32 ADXSTM_GetBufSize(ADXSTM stm, Sint32 *bufsize, Sint32 *rdsize)
{
    ADXSTM_OBJ *obj = stm;

    *bufsize = obj->bufsize;
    *rdsize = obj->rdsize;
    return 1;
}

SJ ADXSTM_GetSj(ADXSTM stm)
{
    ADXSTM_OBJ *obj = stm;

    return obj->sj;
}

Sint32 ADXSTM_SetBufSize(ADXSTM stm, Sint32 bufsize, Sint32 rdsize)
{
    ADXSTM_OBJ *obj = stm;

    obj->bufsize = bufsize;
    obj->rdsize = rdsize;
    return 1;
}

Sint32 ADXSTM_SetReqRdSize(ADXSTM stm, Sint32 maxsct)
{
    ADXSTM_OBJ *obj = stm;

    obj->maxsct = maxsct;
    return 1;
}

void ADXSTM_EntryErrFunc(void)
{
}

Sint32 ADXSTM_Tell(ADXSTM stm)
{
    ADXSTM_OBJ *obj = stm;

    if (obj->gdfs != NULL) {
        return gdFsTell(obj->gdfs);
    }
    return 0;
}

void ADXSTM_GetGdfsStat(ADXSTM stm, Sint32 *stat)
{
    ADXSTM_OBJ *obj = stm;

    *stat = gdFsGetStat(obj->gdfs);
}

Sint32 ADXSTM_GetFad(const char *fname, Sint32 *fad)
{
    GDFS_DIRINFO info;
    Sint32 ret;

    ret = gdFsGetDirInfo(fname, &info);
    *fad = info.fad;
    return ret;
}

Sint32 ADXSTM_GetFsizeSct(const char *fname, Sint32 *fsize)
{
    GDFS_DIRINFO info;
    Sint32 ret;

    ret = gdFsGetDirInfo(fname, &info);
    *fsize = (info.fsize + ADXSTM_SCT_SIZE - 1) / ADXSTM_SCT_SIZE;
    return ret;
}

Sint32 ADXSTM_GetFsizeByte(const char *fname, Sint32 *fsize)
{
    GDFS_DIRINFO info;
    Sint32 ret;

    ret = gdFsGetDirInfo(fname, &info);
    *fsize = info.fsize;
    return ret;
}
