/* ADX file reads: a disc file, or a file of an AFS archive, read by sectors
 * into a stream joint */
#include <sg_xpt.h>
#include <shinobi.h>
#include <sj.h>
#include <sofdec.h>
#include <string.h>

#define ADXF_OBJ_MAX 16
#define ADXF_PART_MAX 256
#define ADXF_SCT_SIZE 2048
/* the most server passes adxf_LoadData makes while its request fails to
 * start, and again while it waits for the sectors */
#define ADXF_RETRY_MAX 10000
/* the failed sector reads in a row after which a partition's load gives up */
#define ADXF_LOAD_RETRY 3
/* an AFS archive's header, in Sint32s. a mark the loaders don't check comes
 * first, then the file count at AFS_NFILE, and from AFS_FILES each file's
 * offset and size in bytes */
#define AFS_NFILE 1
#define AFS_FILES 2
/* the most files an archive's header may list */
#define ADXF_AFS_FILE_MAX 0x10000

/* a file's states, which adxf_ExecOne copies from its request's stream */
#define ADXF_STAT_STOP ADXSTM_STAT_STOP
#define ADXF_STAT_READING ADXSTM_STAT_READ
#define ADXF_STAT_READEND ADXSTM_STAT_READEND
#define ADXF_STAT_ERROR ADXSTM_STAT_ERROR

/* the errors. ADXF_ERR_FATAL is a file that can't be found, opened or read,
 * or a read request that can't start or isn't there to stop.
 * ADXF_ERR_INTERNAL is a joint SJRBF_Create can't make, and ADXF_ERR_PRM a bad
 * parameter. ADXF_ERR_AFS_FILE is an archive that lists over
 * ADXF_AFS_FILE_MAX files */
#define ADXF_ERR_FATAL (-1)
#define ADXF_ERR_INTERNAL (-2)
#define ADXF_ERR_PRM (-3)
#define ADXF_ERR_AFS_FILE (-4)

/* ADXF_Seek's origins: the file's start, the read position and the file's
 * end */
#define ADXF_SEEK_SET 0
#define ADXF_SEEK_CUR 1
#define ADXF_SEEK_END 2

/* an open file, and its read request */
typedef struct adxf_obj {
    Sint8 used;
    Sint8 stat;
    ADXSTM stm; /* the request's stream */
    SJ sj; /* the joint ADXF_ReadNw32 made for the request, or NULL */
    Sint32 fad; /* the frame address of the file's first sector */
    Sint32 fsize_sct;
    Sint32 fsize_byte;
    Sint32 skpos; /* the read position, in sectors from fad */
    Sint32 rdstpos; /* the frame address of the request's first sector */
    Sint32 rqnsct; /* the request's sectors */
    Sint32 rdnsct; /* the sectors of the request read */
} ADXF_OBJ;
typedef ADXF_OBJ *ADXF;

/* an added archive's entry for a file its header gives a nonzero offset */
typedef struct adxf_entry {
    Uint16 fno;
    Uint16 fsize; /* in sectors */
    Sint32 ofst; /* in sectors from the archive's first */
} ADXF_ENTRY;

/* a partition: this header, then an archive's file table, in storage the
 * caller gives. a loaded archive's table holds Uint16s: its first file's
 * offset, then each file's size, all in sectors. an added archive's table holds
 * an ADXF_ENTRY for each file with a nonzero offset */
typedef struct adxf_ptinfo {
    struct adxf_ptinfo *next; /* the partition this one adds to, or NULL */
    Sint32 size; /* the bytes of this partition and of those it adds to */
    Sint32 top; /* the frame address of the archive's first sector */
    Sint32 nfile;
    Uint16 nentry;
    Sint8 type;
    Sint8 rsv;
} ADXF_PTINFO;
/* a partition's types */
#define ADXF_PT_LOADED 0
#define ADXF_PT_ADDED 1

const char adxf_copyright[32] = "Copyright (c) 1998 CRI";
const char adxf_version[32] = "ADXF Version 5.30";
char *adxf_build = "\nADXF Ver.5.30 Build:Nov 10 1998 18:25:25\n";

ADXF_OBJ adxf_obj[ADXF_OBJ_MAX];
ADXF_PTINFO *adxf_ptinfo[ADXF_PART_MAX];
/* the sector buffers of ADXF_LoadPartition and ADXF_AddPartition, each used
 * from its first 32-byte boundary past its start */
static Sint32 adxf_load_buf[0x820];
static Sint32 adxf_add_buf[0x820];

ADXF ADXF_Open(char *fname, void *atr);
void ADXF_Close(ADXF adxf);
Sint32 ADXF_ReadNw32(ADXF adxf, Sint32 nsct, void *buf);
Sint32 ADXF_Stop(ADXF adxf);
Sint32 ADXF_GetNumReadSct(ADXF adxf);
Sint32 ADXF_GetStat(ADXF adxf);

void adxf_wait_1ms(void)
{
    Sint32 i;

    for (i = 0; i < 50000; i++) {
#ifdef GCC
        /* GCC deletes an empty loop. the asm keeps each pass, so the wait
         * takes as long as the shipped loop */
        __asm__ volatile("");
#endif
    }
}

void ADXF_Init(void)
{
    memset(adxf_obj, 0, sizeof(adxf_obj));
    memset((void *)adxf_ptinfo, 0, sizeof(adxf_ptinfo));
}

void ADXF_Finish(void)
{
    memset((void *)adxf_ptinfo, 0, sizeof(adxf_ptinfo));
    memset(adxf_obj, 0, sizeof(adxf_obj));
}

Sint32 adxf_ChkPrmPt(Sint32 ptid, void *ptinfo)
{
    if (ptid < 0 || ptid >= ADXF_PART_MAX || ptinfo == NULL) {
        return ADXF_ERR_PRM;
    }
    return 0;
}

/* read nsct sectors of adxf into buf, running the servers itself. returns the
 * sectors read, or 0 when the request doesn't start within ADXF_RETRY_MAX
 * passes. the wait for the sectors ends early only at ADXF_STAT_READEND, so a
 * failed read takes all ADXF_RETRY_MAX passes */
Sint32 adxf_LoadData(ADXF adxf, Sint32 nsct, void *buf)
{
    Sint32 i;
    Sint32 n;

    for (i = 0; i < ADXF_RETRY_MAX; i++) {
        n = ADXF_ReadNw32(adxf, nsct, buf);
        if (n > 0) {
            break;
        }
        gdFsExecServer();
        ADXSTM_ExecServer();
        ADXF_ExecServer();
        adxf_wait_1ms();
    }
    if (i == ADXF_RETRY_MAX) {
        return 0;
    }
    for (i = 0; i < ADXF_RETRY_MAX; i++) {
        gdFsExecServer();
        ADXSTM_ExecServer();
        ADXF_ExecServer();
        if (ADXF_GetStat(adxf) == ADXF_STAT_READEND) {
            break;
        }
        adxf_wait_1ms();
    }
    return ADXF_GetNumReadSct(adxf);
}

Sint32 ADXF_LoadPartition(Sint32 ptid, char *fname, void *ptinfo, Sint32 nmax)
{
    Sint32 ret;
    Sint32 fad;
    ADXF adxf;
    Sint32 atr;
    ADXF_PTINFO *pt;
    Uint16 *first;
    Uint16 *fsize;
    Sint32 *buf;
    Sint32 file;
    Sint32 retry;
    Sint32 i;

    ret = adxf_ChkPrmPt(ptid, ptinfo);
    if (ret < 0) {
        return ret;
    }
    if (ADXSTM_GetFad(fname, &fad) < 0) {
        return ADXF_ERR_FATAL;
    }
    if ((adxf = ADXF_Open(fname, &atr)) == NULL) {
        return ADXF_ERR_FATAL;
    }
    ret = ADXF_ERR_FATAL;
    pt = adxf_ptinfo[ptid] = ptinfo;
    pt->next = NULL;
    pt->top = fad;
    pt->type = ADXF_PT_LOADED;
    /* @hack redundant: a store of nmax to ret that never runs, which puts 0x800
     * in R6 and 0x200 in R7 (0x8c65c38e, 0x8c65c3dc); without it, 6 of 160
     * words differ */
    if (0) {
        ret = nmax;
    }
    first = (Uint16 *)((char *)pt + sizeof *pt);
    fsize = first + 1;
    buf = (Sint32 *)(((Uint32)adxf_load_buf + 32) & ~31);
    file = 0;
    pt->nfile = 0;
    for (retry = 0; retry < ADXF_LOAD_RETRY;) {
        if (adxf_LoadData(adxf, 1, buf) > 0) {
            if (pt->nfile == 0) {
                if (buf[AFS_NFILE] > ADXF_AFS_FILE_MAX) {
                    buf[AFS_NFILE] = ADXF_AFS_FILE_MAX;
                    ret = ADXF_ERR_AFS_FILE;
                    goto done;
                }
                pt->nfile = pt->nentry =
                    (nmax < buf[AFS_NFILE]) ? nmax : buf[AFS_NFILE];
                /* the header and its nfile + 1 Uint16s, rounded up to a
                 * multiple of 4. the sum is even, so adding 2 before the
                 * division rounds it up */
                pt->size = (sizeof(ADXF_PTINFO) +
                            ((pt->nfile + 1) * sizeof(Uint16)) + 2) /
                           4 * 4;
                *first = buf[AFS_FILES] / ADXF_SCT_SIZE;
                i = AFS_FILES + 1;
            } else {
                i = 1;
            }
            /* the header's table pairs each file's offset word with its size
             * word, and a sector holds whole pairs, so the sizes are its odd
             * words */
            for (; i < ADXF_SCT_SIZE / 4; i += 2) {
                fsize[file++] = (buf[i] + ADXF_SCT_SIZE - 1) / ADXF_SCT_SIZE;
                if (file >= pt->nfile) {
                    ret = 0;
                    goto done;
                }
            }
            retry = 0;
        } else {
            retry++;
        }
    }
done:
    ADXF_Close(adxf);
    return ret;
}

/* add the AFS archive fname to partition ptid, which must hold one already.
 * ptinfo must hold an ADXF_PTINFO, then an ADXF_ENTRY for each of the
 * archive's first nmax files with a nonzero offset. ADXF_GetFileRange looks
 * there before the archives ptid held. ptinfo stays in use as
 * ADXF_LoadPartition's does. returns what ADXF_LoadPartition does. once fname
 * opens, ptid keeps ptinfo even on failure */
Sint32 ADXF_AddPartition(Sint32 ptid, char *fname, void *ptinfo, Sint32 nmax)
{
    ADXF_PTINFO *pt = ptinfo;
    Sint32 ret;
    Sint32 fad;
    ADXF adxf;
    Sint32 atr;
    void *table_start;
    ADXF_ENTRY *files;
    ADXF_ENTRY *entry;
    Sint32 *buf;
    Sint32 file;
    Sint32 nentry;
    Sint32 retry;
    Sint32 i;

    ret = adxf_ChkPrmPt(ptid, ptinfo);
    if (ret < 0) {
        return ret;
    }
    if (ADXSTM_GetFad(fname, &fad) < 0) {
        return ADXF_ERR_FATAL;
    }
    if ((adxf = ADXF_Open(fname, &atr)) == NULL) {
        return ADXF_ERR_FATAL;
    }
    pt->next = adxf_ptinfo[ptid];
    ret = ADXF_ERR_FATAL;
    adxf_ptinfo[ptid] = ptinfo;
    pt->top = fad;
    pt->type = ADXF_PT_ADDED;
    /* @hack spelling: the table in two steps, table_start and then files,
     * which puts ret in R12 and file in R11, and nmax and adxf at frame
     * offsets 12 and 16, from 0x8c65c474; through files alone, 131 of 172
     * words differ */
    table_start = (char *)pt + sizeof *pt;
    buf = (Sint32 *)(((Uint32)adxf_add_buf + 32) & ~31);
    nentry = file = 0;
    pt->nfile = 0;
    files = (ADXF_ENTRY *)table_start;
    for (retry = 0; retry < ADXF_LOAD_RETRY;) {
        if (adxf_LoadData(adxf, 1, buf) > 0) {
            i = 0;
            if (pt->nfile == 0) {
                if (buf[AFS_NFILE] > ADXF_AFS_FILE_MAX) {
                    buf[AFS_NFILE] = ADXF_AFS_FILE_MAX;
                    ret = ADXF_ERR_AFS_FILE;
                    goto done;
                }
                pt->nfile = (nmax < buf[AFS_NFILE]) ? nmax : buf[AFS_NFILE];
                i = AFS_FILES;
            }
            while (i < ADXF_SCT_SIZE / 4) {
                if (buf[i] == 0) {
                    i += 2;
                } else {
                    Sint32 sector_round;

                    entry = &files[nentry++];
                    entry->fno = file;
                    /* @hack spelling: the rounding through sector_round,
                     * which makes the pool word at 0x8c65c638 2047; as the
                     * literal, it is 2048 and 5 of 172 words differ */
                    sector_round = ADXF_SCT_SIZE - 1;
                    entry->ofst = (buf[i++] + sector_round) / ADXF_SCT_SIZE;
                    entry->fsize = (buf[i++] + sector_round) / ADXF_SCT_SIZE;
                }
                file++;
                if (file >= pt->nfile) {
                    ret = 0;
                    goto done;
                }
            }
            retry = 0;
        } else {
            retry++;
        }
    }
done:
    pt->nentry = nentry;
    pt->size = pt->next->size + (pt->nentry * sizeof(ADXF_ENTRY)) +
               sizeof(ADXF_PTINFO);
    ADXF_Close(adxf);
    return ret;
}

/* the bytes of partition ptid and of each partition it adds to. ptid must hold
 * a partition */
Sint32 ADXF_GetPtinfoSize(Sint32 ptid)
{
    ADXF_PTINFO *pt;

    pt = adxf_ptinfo[ptid];
    return pt->size;
}

ADXF adxf_AllocAdxFs(void)
{
    ADXF adxf;
    Sint32 i;

    adxf = NULL;
    for (i = 0; i < ADXF_OBJ_MAX; i++) {
        if (adxf_obj[i].used == 0) {
            adxf = &adxf_obj[i];
            break;
        }
    }
    return adxf;
}

ADXF adxf_CreateAdxFs(void)
{
    ADXF adxf;

    adxf = adxf_AllocAdxFs();
    if (adxf == NULL) {
        return NULL;
    }
    adxf->stat = ADXF_STAT_STOP;
    adxf->stm = NULL;
    adxf->rdstpos = 0;
    adxf->rqnsct = 0;
    adxf->rdnsct = 0;
    adxf->used = 1;
    return adxf;
}

Sint32 adxf_SetFileInfo(ADXF adxf, char *fname)
{
    Sint32 ret;
    Sint32 fad;
    Sint32 fsize_sct;
    Sint32 fsize_byte;

    ret = ADXSTM_GetFad(fname, &fad);
    if (ret < 0) {
        return ret;
    }
    ret = ADXSTM_GetFsizeSct(fname, &fsize_sct);
    if (ret < 0) {
        return ret;
    }
    ret = ADXSTM_GetFsizeByte(fname, &fsize_byte);
    if (ret < 0) {
        return ret;
    }
    adxf->fad = fad;
    adxf->fsize_sct = fsize_sct;
    adxf->fsize_byte = fsize_byte;
    return 0;
}

/* open the disc file fname. returns its handle, or NULL when all ADXF_OBJ_MAX
 * are open or ADXSTM can't find fname's frame address and sizes. atr is
 * unused */
ADXF ADXF_Open(char *fname, void *atr)
{
    ADXF adxf;
    Sint32 ret;

    ADXCRS_Lock();
    adxf = adxf_CreateAdxFs();
    /* @hack redundant: the empty else, which puts ADXCRS_Unlock's address in R3
     * (0x8c65c6c6); without it, R2 takes it and 2 of 29 words differ */
    if (adxf != NULL) {
        ret = adxf_SetFileInfo(adxf, fname);
        if (ret < 0) {
            ADXF_Close(adxf);
            adxf = NULL;
        }
    } else {
    }
    ADXCRS_Unlock();
    return adxf;
}

Sint32 adxf_SetAfsFileInfo(ADXF adxf, Sint32 ptid, Sint32 flid)
{
    Sint32 ret;
    Sint32 ofst;
    Sint32 nsct;

    ret = ADXF_GetFileRange(ptid, flid, &ofst, &nsct);
    if (ret < 0) {
        return ADXF_ERR_PRM;
    }
    adxf->fad = ofst;
    adxf->fsize_sct = nsct;
    adxf->fsize_byte = (Sint32)((Uint32)nsct * ADXF_SCT_SIZE);
    return 0;
}

/* open file flid of partition ptid. returns its handle, or NULL when all
 * ADXF_OBJ_MAX are open or ADXF_GetFileRange refuses flid */
ADXF ADXF_OpenAfs(Sint32 ptid, Sint32 flid)
{
    ADXF adxf;
    Sint32 ret;

    ADXCRS_Lock();
    adxf = adxf_CreateAdxFs();
    /* @hack redundant: the empty else, which puts ADXCRS_Unlock's address in R3
     * (0x8c65c748); without it, R2 takes it and 2 of 31 words differ */
    if (adxf != NULL) {
        ret = adxf_SetAfsFileInfo(adxf, ptid, flid);
        if (ret < 0) {
            ADXF_Close(adxf);
            adxf = NULL;
        }
    } else {
    }
    ADXCRS_Unlock();
    return adxf;
}

void adxf_CloseSjStm(ADXF adxf)
{
    if (adxf->stm != NULL) {
        ADXSTM_Close(adxf->stm);
        adxf->stm = NULL;
    }
    if (adxf->sj != NULL) {
        SJ_Destroy(adxf->sj);
        adxf->sj = NULL;
    }
}

void ADXF_Close(ADXF adxf)
{
    if (adxf != NULL) {
        ADXCRS_Lock();
        if (adxf->stat == ADXF_STAT_READING) {
            ADXF_Stop(adxf);
        }
        memset(adxf, 0, sizeof(ADXF_OBJ));
        ADXCRS_Unlock();
    }
}

/* start reading nsct sectors from the read position into sj, or up to the
 * file's end if that comes first. returns the sectors requested, which are 0
 * at the file's end, though the request still starts. it returns 0 while a
 * request reads. it returns ADXF_ERR_PRM for a NULL adxf or sj or a negative
 * nsct. it returns ADXF_ERR_FATAL while a request's stream stays open, or
 * when this one's can't open */
Sint32 ADXF_ReadSj32(ADXF adxf, Sint32 nsct, SJ sj)
{
    ADXSTM stm;
    Sint32 rest;

    if (adxf == NULL) {
        return ADXF_ERR_PRM;
    }
    if (nsct < 0) {
        return ADXF_ERR_PRM;
    }
    if (sj == NULL) {
        return ADXF_ERR_PRM;
    }
    if (adxf->stat == ADXF_STAT_READING) {
        return 0;
    }
    if (adxf->stm != NULL) {
        return ADXF_ERR_FATAL;
    }
    ADXCRS_Lock();
    adxf->rdstpos = (Sint32)((Uint32)adxf->fad + (Uint32)adxf->skpos);
    /* @hack spelling: rest set in the test, which puts the minimum in R3
     * (0x8c65c810); set before it, 8 of 71 words differ */
    adxf->rqnsct =
        (nsct <
         (rest = (Sint32)((Uint32)adxf->fsize_sct - (Uint32)adxf->skpos)))
            ? nsct
            : rest;
    adxf->rdnsct = 0;
    stm = ADXSTM_OpenRange(adxf->rdstpos, adxf->rqnsct, sj);
    if (stm == NULL) {
        nsct = ADXF_ERR_FATAL;
    } else {
        adxf->stm = stm;
        adxf->stat = ADXF_STAT_READING;
        ADXSTM_Start(adxf->stm);
        nsct = adxf->rqnsct;
    }
    ADXCRS_Unlock();
    return nsct;
}

/* as ADXF_ReadSj32, into the nsct sectors at buf through a joint of its own.
 * it returns ADXF_ERR_PRM for a NULL buf too, ADXF_ERR_FATAL while its last
 * joint stays, and ADXF_ERR_INTERNAL when SJRBF_Create fails */
Sint32 ADXF_ReadNw32(ADXF adxf, Sint32 nsct, void *buf)
{
    SJ sj;
    Sint32 n;

    if (adxf == NULL) {
        return ADXF_ERR_PRM;
    }
    if (nsct < 0) {
        return ADXF_ERR_PRM;
    }
    if (buf == NULL) {
        return ADXF_ERR_PRM;
    }
    if (adxf->stat == ADXF_STAT_READING) {
        return 0;
    }
    if (adxf->sj != NULL) {
        return ADXF_ERR_FATAL;
    }
    sj = SJRBF_Create(buf, (Sint32)((Uint32)nsct * ADXF_SCT_SIZE), 0);
    if (sj == NULL) {
        return ADXF_ERR_INTERNAL;
    }
    ADXCRS_Lock();
    if ((n = ADXF_ReadSj32(adxf, nsct, sj)) <= 0) {
        SJ_Destroy(sj);
    } else {
        adxf->sj = sj;
    }
    ADXCRS_Unlock();
    return n;
}

/* stop adxf's request and return the read position, which the sectors read
 * don't advance. returns ADXF_ERR_PRM for a NULL adxf, and ADXF_ERR_FATAL
 * without a request's stream */
Sint32 ADXF_Stop(ADXF adxf)
{
    Sint32 ofst;

    if (adxf == NULL) {
        return ADXF_ERR_PRM;
    }
    if (adxf->stm == NULL) {
        return ADXF_ERR_FATAL;
    }
    /* @hack spelling: the empty arm, which puts ADXCRS_Lock, ADXSTM_Stop and
     * ADXSTM_GetCurOfst in R3, R2 and R3, from 0x8c65c926; as one negated
     * test, 8 of 45 words differ */
    if (adxf->stat == ADXF_STAT_STOP) {
    } else {
        ADXCRS_Lock();
        ADXSTM_Stop(adxf->stm);
        ADXSTM_GetCurOfst(adxf->stm, &ofst);
        adxf->rdnsct = ofst;
        adxf_CloseSjStm(adxf);
        adxf->stat = ADXF_STAT_STOP;
        ADXCRS_Unlock();
    }
    return adxf->skpos;
}

/* update adxf's state and its sectors read from its request's stream. at the
 * request's end or failure, advance the read position by the sectors read,
 * and close the request's stream and joint */
void adxf_ExecOne(ADXF adxf)
{
    Sint32 ofst;

    if (adxf->stat == ADXF_STAT_READING) {
        adxf->stat = ADXSTM_GetStat(adxf->stm);
        ADXSTM_GetCurOfst(adxf->stm, &ofst);
        adxf->rdnsct = ofst;
        if (adxf->stat == ADXF_STAT_READEND || adxf->stat == ADXF_STAT_ERROR) {
            adxf->skpos = (Sint32)((Uint32)adxf->skpos + (Uint32)adxf->rdnsct);
            adxf_CloseSjStm(adxf);
        }
    }
}

void ADXF_ExecServer(void)
{
    Sint32 i;

    ADXCRS_Lock();
    for (i = 0; i < ADXF_OBJ_MAX; i++) {
        if (adxf_obj[i].used == 1) {
            adxf_ExecOne(&adxf_obj[i]);
        }
    }
    ADXCRS_Unlock();
}

/* move the read position pos sectors from the origin type names, and hold it
 * within the file. any other type only holds it there. returns the position,
 * or ADXF_ERR_PRM for a NULL adxf */
Sint32 ADXF_Seek(ADXF adxf, Sint32 pos, Sint32 type)
{
    if (adxf == NULL) {
        return ADXF_ERR_PRM;
    }
    if (type == ADXF_SEEK_SET) {
        adxf->skpos = pos;
    } else if (type == ADXF_SEEK_CUR) {
        adxf->skpos += pos;
    } else if (type == ADXF_SEEK_END) {
        adxf->skpos = adxf->fsize_sct + pos;
    }
    if (adxf->skpos < 0) {
        adxf->skpos = 0;
    } else if (adxf->skpos > adxf->fsize_sct) {
        adxf->skpos = adxf->fsize_sct;
    }
    return adxf->skpos;
}

Sint32 ADXF_Tell(ADXF adxf)
{
    if (adxf == NULL) {
        return ADXF_ERR_PRM;
    }
    return adxf->skpos;
}

Sint32 ADXF_GetFsizeSct(ADXF adxf)
{
    if (adxf == NULL) {
        return ADXF_ERR_PRM;
    }
    return adxf->fsize_sct;
}

Sint32 ADXF_GetFsizeByte(ADXF adxf)
{
    if (adxf == NULL) {
        return ADXF_ERR_PRM;
    }
    return adxf->fsize_byte;
}

/* the request's sectors, and in *rdstpos the frame address of its first.
 * returns ADXF_ERR_PRM for a NULL adxf, with *rdstpos 0 */
Sint32 ADXF_GetNumReqSct(ADXF adxf, Sint32 *rdstpos)
{
    if (adxf == NULL) {
        *rdstpos = 0;
        return ADXF_ERR_PRM;
    }
    *rdstpos = adxf->rdstpos;
    return adxf->rqnsct;
}

Sint32 ADXF_GetNumReadSct(ADXF adxf)
{
    if (adxf == NULL) {
        return ADXF_ERR_PRM;
    }
    return adxf->rdnsct;
}

Sint32 ADXF_GetStat(ADXF adxf)
{
    if (adxf == NULL) {
        return ADXF_ERR_PRM;
    }
    return adxf->stat;
}

Sint32 adxf_ChkPrmGfr(Sint32 ptid, Sint32 flid)
{
    ADXF_PTINFO *pt;

    if (ptid >= ADXF_PART_MAX) {
        return ADXF_ERR_PRM;
    }
    pt = adxf_ptinfo[ptid];
    if (flid >= pt->nfile) {
        return ADXF_ERR_PRM;
    }
    return 0;
}

Sint32 ADXF_GetFileRange(Sint32 ptid, Sint32 flid, Sint32 *fad, Sint32 *fnsct)
{
    Sint32 ret;
    ADXF_PTINFO *pt;
    ADXF_ENTRY *base;
    ADXF_ENTRY *entry;
    Uint16 *tbl;
    Uint16 *fsize;
    Sint32 top;
    Sint32 i;

    ret = adxf_ChkPrmGfr(ptid, flid);
    if (ret < 0) {
        *fad = -1;
        *fnsct = -1;
        goto end;
    }
    for (pt = adxf_ptinfo[ptid]; pt->next != NULL; pt = pt->next) {
        base = (ADXF_ENTRY *)((char *)pt + sizeof *pt);
        for (i = 0; i < pt->nentry; i++) {
            entry = &base[i];
            if (entry->fno == flid) {
                *fad = pt->top + entry->ofst;
                *fnsct = entry->fsize;
                goto end;
            }
        }
    }
    tbl = (Uint16 *)((char *)pt + sizeof *pt);
    top = *tbl;
    fsize = tbl + 1;
    for (i = 0; i < flid; i++) {
        top += fsize[i];
    }
    *fad = top + pt->top;
    *fnsct = fsize[flid];
end:
    return ret;
}
