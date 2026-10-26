/* flash memory play history library, as far as the sources use it */
#ifndef SG_UF_H
#define SG_UF_H
#include <sg_xpt.h>

/* ufPlayTime modes: count a save, count a load, or count neither */
enum { UFD_SAVE, UFD_LOAD, UFD_PLAY };

typedef enum {
    UFD_ERR_USERFLAG_PARITY_ERROR = 1,
    UFD_ERR_BANKNUMBER_ERROR,
    UFD_ERR_PROTECT_COMMON_FLAG,
    UFD_ERR_PROTECT_FLAG,
    UFD_ERR_NO_INIT,
    UFD_ERR_INIT_FAIL,
    UFD_ERR_READ_FAIL,
    UFD_ERR_WRITE_FAIL,
    UFD_ERR_NOT_FOUND,
    UFD_ERR_NO_PACKET_LEFT,
    UFD_ERR_MEMORY_FAIL,
    UFD_ERR_PERIINITFAIL,
    UFD_ERR_REFRESHFAIL,
    UFD_ERR_MAX
} UFS_ERROR;
extern UFS_ERROR ufgErrno;

/* the sizes of ufInitSystem's four work areas: the flash manager work, a
 * one-packet buffer, and the safe refresh's copies of the spare and history
 * partitions */
#define UFD_WORK1_SIZ 65536
#define UFD_WORK2_SIZ 60
#define UFD_WORK3_SIZ 65536
#define UFD_WORK4_SIZ 32768

/* a title's play history record, the four 60-byte packets it holds in flash
 * memory */
typedef struct {
    Uint8 version;
    Uint8 AutoSave;
    Uint8 ProdNum[10];
    Uint8 ProdName[48];
    Uint8 ProdName2[44];
    Uint32 Kind;
    Uint32 FirstStartTime;
    Uint16 CRC;
    Uint8 PeriIdx[6];
    Uint32 PrevStartTime;
    Uint16 StartCount;
    Uint16 PlayTime[24];
    Uint16 LoadCount;
    Uint32 Reserve_2;
    Uint16 SaveCount;
    Uint8 GameEst;
    Uint8 GameCarry;
    Uint32 FirstNetPlay;
    Uint32 PrevNetPlay;
    Uint16 NetPlayCount;
    Uint16 NetPlayTotalTime;
    Uint8 FreeArea[32];
    Uint8 Reserved2[10];
    Uint16 SaveOccur;
} UFS_AllPlayHistory;

Bool ufInitSystem(void *work1, void *work2, void *work3, void *work4);
/* replace each work area passed as non-NULL, and return TRUE. before
 * ufInitSystem it replaces none, and returns FALSE with UFD_ERR_NO_INIT */
Bool ufChangeWorkBuffer(void *work1, void *work2, void *work3, void *work4);
/* count a save or a load in the running title's record, as mode says. unless
 * mode is UFD_LOAD, est and carry become GameEst and GameCarry. param, unless
 * NULL, becomes FreeArea. a synch saves these to flash. ufPlayTime synchs at
 * every count-th call, as ufAutoSynch set count. a synch that ufSaveAction
 * refuses stops these synchs until the next ufAutoSynch. ufPlayTime returns
 * ufgRestPacket, the packets left to write. it returns 0 when its synch fails,
 * and 0 with UFD_ERR_NO_INIT before ufInitSystem succeeds */
Sint32 ufPlayTime(Uint8 mode, Uint8 est, Uint8 carry, Uint8 param[32]);
/* synch the running title's record through ufSaveAction, and return its
 * result. the synch adds the minutes played since the last one to PlayTime,
 * which holds the minutes played in each hour of the day. SaveOccur takes the
 * synchs since ufPlayInit, this one included. packets 2 and 3 of the record
 * then save. ufSynch returns FALSE with UFD_ERR_NO_INIT before ufInitSystem
 * succeeds */
Bool ufSynch(void);
/* have ufPlayTime synch at every count-th call from now; 0 turns its synchs
 * off. the result is TRUE, or FALSE with UFD_ERR_NO_INIT before ufInitSystem
 * succeeds */
Bool ufAutoSynch(Uint32 count);
/* read into *buff the four-packet record of the title whose product number is
 * ProdNum, or of the running title for NULL, and return TRUE. it returns FALSE
 * with UFD_ERR_PROTECT_COMMON_FLAG unless the shared flags have data set. other
 * failures set UFD_ERR_NO_INIT, UFD_ERR_NOT_FOUND or UFD_ERR_READ_FAIL. once
 * ufInitSystem has succeeded, the call opens the history partition,
 * ufSetBank(3), whatever ProdNum is */
Bool ufReadAllHead(Uint8 *ProdNum, UFS_AllPlayHistory *buff);
/* rewrite the history partition with fhSafeRefresh, through all four work
 * areas, and return TRUE. it returns FALSE when the refresh fails. it returns
 * FALSE with UFD_ERR_MEMORY_FAIL without the last two work areas, and with
 * UFD_ERR_NO_INIT until ufInitSystem has succeeded. a success sets ufgErrno
 * too, to UFD_ERR_REFRESHFAIL */
Bool ufSafeRefresh(void);
/* set AutoSave in ufgPlayHistBuffer, save the buffer's first packet, and return
 * TRUE. when fewer than two packets are left to write, it first runs a safe
 * refresh and reopens the history partition. without the last two work areas,
 * that sets UFD_ERR_MEMORY_FAIL, and the save goes ahead. the save goes to the
 * packet numbered by the title's index, not to the title's record, and its
 * result is ignored. it returns FALSE with UFD_ERR_NOT_FOUND for a title with
 * no record, and with UFD_ERR_NO_INIT until ufInitSystem has succeeded */
Bool ufChangeAutoSave(Uint8 Flag);
/* erase the running title's record to 0xff bytes, through ufSaveAction */
Bool ufTitleDelete(void);
/* ufConnect records the start of a network play at time, and ufDisConnect its
 * end, each through ufSaveAction. ufDisConnect saves only after a ufConnect
 * that saved */
Bool ufConnect(Uint32 time);
Bool ufDisConnect(void);

/* the four work areas from ufInitSystem */
extern void *ufgWorkBuff;
extern Uint8 *ufgReadBuff;
extern void *ufgReflash3;
extern void *ufgReflash4;
extern Uint8 *ufgProdNum; /* the running title's product number */
/* the packets the library counts as left to write. ufPlayInit and each synch
 * set it to ufGetFreeBlock's count, and ufSavePacket takes one away for each
 * packet it changes. a safe refresh doesn't renew it */
extern Sint32 ufgRestPacket;
/* the running title's record */
extern UFS_AllPlayHistory ufgPlayHistBuffer;
extern char ufgSafeFlag; /* whether ufInitSystem has succeeded */

/* open flash partition bank, unless it is the partition the last call asked
 * for, and return TRUE. a negative bank opens partition -bank whichever the
 * last call asked for. it returns FALSE with UFD_ERR_BANKNUMBER_ERROR for a
 * bank above 4. it returns FALSE with fmMngInit's error in ufgErrno when the
 * open fails. the failed partition still counts as asked for, so a next call
 * for it returns TRUE without opening it */
Bool ufSetBank(Sint32 bank);
/* read a 60-byte packet into ufgReadBuff, and copy it to buf unless buf is
 * NULL. packet is volatile, as in the definition: SHC refuses a prototype
 * without it, with error 2118 */
Bool ufLoadPacket(volatile Sint32 packet, void *buf);
/* write the FMD_DATA_BYTES at buf to packet, through ufgReadBuff, and return
 * TRUE. a write counts down ufgRestPacket, and a packet that already holds the
 * data doesn't. a write fhFmWriteExec refuses returns FALSE with
 * UFD_ERR_WRITE_FAIL */
Bool ufSavePacket(Sint32 packet, const void *buf);
Sint32 ufGetFreeBlock(void);
/* the index of the title whose record holds ProdNum and a valid CRC, or -1 */
Sint32 ufFindTitleIdx(const Uint8 *ProdNum);
/* read the running title's record and record the peripherals connected now;
 * ufInitSystem calls it. ufPlayInit changes the console's lists of the
 * controllers and expansion devices it has seen, and the record's PeriIdx, only
 * with more than two packets left to write. each controller and expansion
 * device connected now joins its list, if the list lacks it and has room.
 * PeriIdx marks the entries of those connected now, and keeps the marks it
 * held. on success, automatic synchs stop until ufAutoSynch. the result is
 * FALSE when ufReadAllHead fails, or with UFD_ERR_PERIINITFAIL when a port or
 * socket can't be read. either failure leaves the automatic synchs as they
 * were */
Bool ufPlayInit(void);
/* read the first packet of the running title's record into ufgPlayHistBuffer,
 * then return action's result for that packet's number. when fewer than need
 * packets are left to write, and fewer than two, a safe refresh runs first.
 * action runs only while the shared flags have auto_save and data set and the
 * title's AutoSave is set. otherwise it returns FALSE with ufgErrno set, as it
 * does for a title with no record or a refresh without work3 and work4 */
Bool ufSaveAction(Bool (*action)(Sint32 packet), Sint32 need);
/* store the low size bytes of value at data, big-endian */
void SaveNetworkEndiane(Uint32 value, Sint32 size, Uint8 *data);
Bool ufLoadUserFlag(void);
Bool ufSaveUserFlag(void);
/* copy the shared flags to the 60 bytes at buf, an address passed as an
 * integer */
void ufGetUserState(Sint32 buf);
void ufSetAutoSaveFlag(Uint8 flag);
void ufSetVersion(Uint8 version);
void ufSetNetuseFlag(Uint8 flag);
void ufSetData(Uint8 data);

/* flash packet access through fmMngRead and fmMngWrite. each call waits until
 * fmMngExecServer finishes its request. it returns -1 when fmMngRead or
 * fmMngWrite refuses the request. otherwise it returns 0, whether or not the
 * server's request succeeded: a failure shows only in fmMngGetErr */
/* read packet into buf */
Sint32 fhFmReadExec(Sint32 packet, void *buf);
/* write buf to packet. when the packet already holds buf, it writes nothing and
 * returns FM_WRITE_UNCHANGED */
Sint32 fhFmWriteExec(Sint32 packet, const void *buf);
#define FM_WRITE_UNCHANGED 3
/* rewrite the history partition through the four work areas, and return 0, or
 * -1 with ufgErrno set. while it runs, the spare partition keeps the history's
 * copy, and its packet 1020 the refresh's status */
Sint32 fhSafeRefresh(void *work1, void *work2, void *work3, void *work4);
/* write the string for status to the status packet, through buf. it returns -1
 * for an unknown status or a failed write. after a successful write it ends
 * without a return statement, so its result is the 0 or FM_WRITE_UNCHANGED that
 * fhFmWriteExec left in R0. its callers ignore the result */
Sint32 fhSetStatus(Sint32 status, void *buf);
/* how many of the num packets from first differ from the copy at data, reading
 * each through buf; -1 when a read fails */
Sint32 fhPacketNeed(Sint32 first, Sint32 num, void *data, void *buf);

#endif
