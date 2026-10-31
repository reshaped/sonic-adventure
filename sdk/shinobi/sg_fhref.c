/* the safe refresh of the play history partition */
#include "shinobi/fh_status.h"
#include <machine.h>
#include <sg_uf.h>
#include <sg_xpt.h>
#include <shinobi.h>
#include <string.h>

/* flash partitions: the play history's, and the spare partition after it */
#define HISTORY_BANK 3
#define SPARE_BANK 4
/* the packets the refresh copies out of the spare partition, and out of the
 * history partition. HISTORY_COPY is the spare packet where it saves the
 * history's copy */
#define SPARE_PACKETS 1020
#define HISTORY_PACKETS 424
#define HISTORY_COPY (SPARE_PACKETS - HISTORY_PACKETS)
/* the status packet: the spare packet after the copied ones */
#define REFRESH_PACKET SPARE_PACKETS

/* the stages the status packet records */
#define FH_OK 0 /* the refresh is complete */
#define FH_EXIST 1 /* the history's copy is saved in the spare partition */
#define FH_PROCESS1 2 /* the history's copy is being saved */
#define FH_PROCESS2 3 /* the spare partition is being written back */

/* copy the spare partition to work3 and the history partition to work4. save
 * the history's copy in the spare partition, then format the history partition
 * and write work4 back to it. last, write work3 back to the spare partition.
 * the status packet records each stage. a write to the spare partition formats
 * it first when fmMngGetFree reports too few free packets for the packets that
 * differ. work1 is fmMngInit's work, work2 a one-packet buffer, and work3 and
 * work4 must hold SPARE_PACKETS and HISTORY_PACKETS packets */
Sint32 fhSafeRefresh(void *work1, void *work2, void *work3, void *work4)
{
    Sint32 i;

    if (fmMngInit(SPARE_BANK, work1) != 0) {
        ufgErrno = UFD_ERR_WRITE_FAIL;
        return -1;
    }
    for (i = 0; i < SPARE_PACKETS; i++) {
        if (fhFmReadExec(i, work2) != 0) {
            ufgErrno = UFD_ERR_READ_FAIL;
            return -1;
        }
        memcpy((Uint8 *)work3 + (i * FMD_DATA_BYTES), work2, FMD_DATA_BYTES);
    }
    if (fmMngInit(HISTORY_BANK, work1) != 0) {
        ufgErrno = UFD_ERR_WRITE_FAIL;
        return -1;
    }
    for (i = 0; i < HISTORY_PACKETS; i++) {
        if (fhFmReadExec(i, work2) != 0) {
            ufgErrno = UFD_ERR_READ_FAIL;
            return -1;
        }
        memcpy((Uint8 *)work4 + (i * FMD_DATA_BYTES), work2, FMD_DATA_BYTES);
    }
    if (fmMngInit(SPARE_BANK, work1) != 0) {
        ufgErrno = UFD_ERR_WRITE_FAIL;
        return -1;
    }
    if (fhPacketNeed(HISTORY_COPY, HISTORY_PACKETS, work4, work2) + 3 >
        fmMngGetFree()) {
        if (fmMngFormat(FMD_WRITABLE) != 0) {
            ufgErrno = UFD_ERR_WRITE_FAIL;
            return -1;
        }
    }
    fhSetStatus(FH_PROCESS1, work2);
    for (i = 0; i < HISTORY_PACKETS; i++) {
        if (fhFmWriteExec(HISTORY_COPY + i,
                          (Uint8 *)work4 + (i * FMD_DATA_BYTES)) == -1) {
            ufgErrno = UFD_ERR_WRITE_FAIL;
            return -1;
        }
    }
    fhSetStatus(FH_EXIST, work2);
    if (fmMngInit(HISTORY_BANK, work1) != 0) {
        ufgErrno = UFD_ERR_WRITE_FAIL;
        return -1;
    }
    if (fmMngFormat(FMD_WRITABLE) != 0) {
        ufgErrno = UFD_ERR_WRITE_FAIL;
        return -1;
    }
    for (i = 0; i < HISTORY_PACKETS; i++) {
        if (fhFmWriteExec(i, (Uint8 *)work4 + (i * FMD_DATA_BYTES)) == -1) {
            ufgErrno = UFD_ERR_WRITE_FAIL;
            return -1;
        }
    }
    if (fmMngInit(SPARE_BANK, work1) != 0) {
        ufgErrno = UFD_ERR_WRITE_FAIL;
        return -1;
    }
    fhSetStatus(FH_PROCESS2, work2);
    if (fhPacketNeed(0, SPARE_PACKETS, work3, work2) + 2 > fmMngGetFree()) {
        if (fmMngFormat(FMD_WRITABLE) != 0) {
            ufgErrno = UFD_ERR_WRITE_FAIL;
            return -1;
        }
        fhSetStatus(FH_PROCESS2, work2);
    }
    for (i = 0; i < SPARE_PACKETS; i++) {
        if (fhFmWriteExec(i, (Uint8 *)work3 + (i * FMD_DATA_BYTES)) == -1) {
            ufgErrno = UFD_ERR_WRITE_FAIL;
            return -1;
        }
    }
    fhSetStatus(FH_OK, work2);
    if (fmMngInit(HISTORY_BANK, work1) != 0) {
        ufgErrno = UFD_ERR_WRITE_FAIL;
        return -1;
    }
    return 0;
}

Sint32 fhSetStatus(Sint32 status, void *buf)
{
    const char *text;

    memset(buf, 0xff, FMD_DATA_BYTES);
    switch (status) {
    case FH_OK:
        text = fh_ok;
        break;
    case FH_EXIST:
        text = fh_exist;
        break;
    case FH_PROCESS1:
        text = fh_process1;
        break;
    case FH_PROCESS2:
        text = fh_process2;
        break;
    default:
        return -1;
    }
    /* @hack spelling: _builtin_strcpy, which SHC compiles to a call of
     * _slow_strcpy with buf in R0 and text in R1 (JSR @R3 at 0x8c670c42). as
     * strcpy, SHC passes them to strcpy in R4 and R5, the function runs 8 bytes
     * short, and 24 of 48 words differ */
    _builtin_strcpy(buf, text);
#ifdef GCC
    /* keeps the result the tag below names: fhFmWriteExec's, -1 for a
     * failed write */
    return fhFmWriteExec(REFRESH_PACKET, buf);
#else
    if (fhFmWriteExec(REFRESH_PACKET, buf) == -1) {
        return -1;
    }
#endif
    /* @hack implicit: after a successful write, R0 still holds
     * fhFmWriteExec's result, which CMP/EQ #-1,R0 at 0x8c670c4e tests. returned
     * explicitly, through a local, the function runs 12 bytes longer and 16 of
     * 48 words differ */
    /* NOLINTNEXTLINE(clang-diagnostic-return-type) */
}

Sint32 fhPacketNeed(Sint32 first, Sint32 num, void *data, void *buf)
{
    Sint32 i;
    Sint32 need = 0;
    Uint8 *copy;

    for (i = 0; i < num; i++) {
        if (fhFmReadExec(first + i, buf) != 0) {
            return -1;
        }
        copy = (Uint8 *)data + (i * FMD_DATA_BYTES);
        if (memcmp(buf, copy, FMD_DATA_BYTES) != 0) {
            need++;
        }
    }
    return need;
}
