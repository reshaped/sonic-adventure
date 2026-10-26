/* play history library initialization, work areas and flash packet access */
/* for the status strings alone, which this object holds and no code here
 * reads */
#include "shinobi/fh_status.h" /* NOLINT(misc-include-cleaner) */
#include <ninja.h>
#include <sg_uf.h>
#include <sg_xpt.h>
#include <shinobi.h>
#include <string.h>

/* the flash partition that holds the play history, and the highest
 * partition number */
#define HISTORY_BANK 3
#define BANK_MAX 4
#define PACKET_LONGS (FMD_DATA_BYTES / 4)
#define TITLES 100
#define TITLE_FIRST_PACKET 24
#define TITLE_PACKETS 4

/* the first two packets of a title's record, as UFS_AllPlayHistory lays
 * them out. CRC covers body, from ProdNum through FirstStartTime */
struct title_record {
    Uint8 version;
    Uint8 AutoSave;
    Uint8 body[110];
    Uint16 CRC;
    Uint8 PeriIdx[6];
};

char *ufgBuild = "\nuf Ver 1.04 Build:Dec 07 1998 20:59:33\n";
char ufgSafeFlag = false;

UFS_ERROR ufgErrno;
void *ufgWorkBuff;
Uint8 *ufgReadBuff;
void *ufgReflash3;
void *ufgReflash4;
Uint8 *ufgProdNum;
Sint32 ufgRestPacket;
UFS_AllPlayHistory ufgPlayHistBuffer;
Sint32 ufgProdIdx;
/* the open flash partition. ufInitSystem sets -1, for none */
static Sint32 open_bank;
static SYS_BT_SYSTEMID console_id;

Bool ufSetBank(Sint32 bank)
{
    Sint32 err;

    /* @hack redundant: the else after the return, which loads open_bank into R2
     * (MOV.L @R5,R2 at 0x8c65d5f8). without it, R2 and R3 trade and 8 of 33
     * words differ */
    if (bank > BANK_MAX) {
        ufgErrno = UFD_ERR_BANKNUMBER_ERROR;
        return false;
    } else if (open_bank != bank) {
        if (bank < 0) {
            bank = -bank;
        }
        open_bank = bank;
        /* @hack spelling: err assigned in the test, which SHC tests in R0 and
         * copies to R5 in the BT/S's slot (TST R0,R0 at 0x8c65d60e). assigned
         * apart, SHC copies it to R4 first and tests that, and 4 of 33 words
         * differ */
        if ((err = fmMngInit(bank, ufgWorkBuff))) {
            ufgErrno = err;
            return false;
        }
    }
    return true;
}

/* @hack type: packet volatile, which SHC keeps in the frame and reads
 * back for the call (MOV.L R4,@(4,R15) at 0x8c65d62e, MOV.L @(4,R15),R4 at
 * 0x8c65d636). without it, the function runs 4 bytes short and 30 of 32 words
 * differ */
Bool ufLoadPacket(volatile Sint32 packet, void *buf)
{
    if (fhFmReadExec(packet, ufgReadBuff)) {
        ufgErrno = UFD_ERR_READ_FAIL;
        return false;
    }
    if (buf != NULL) {
        memcpy(buf, ufgReadBuff, FMD_DATA_BYTES);
    }
    return true;
}

Bool ufSavePacket(Sint32 packet, const void *buf)
{
    Bool saved = true;

    memset(ufgReadBuff, 0xff, FMD_DATA_BYTES);
    njMemCopy4(ufgReadBuff, buf, PACKET_LONGS);
    switch (fhFmWriteExec(packet, ufgReadBuff)) {
    case 0:
        ufgRestPacket--;
        break;
    case FM_WRITE_UNCHANGED:
        break;
    default:
        ufgErrno = UFD_ERR_WRITE_FAIL;
        saved = false;
        break;
    }
    return saved;
}

Sint32 ufGetFreeBlock(void)
{
    Sint32 blocks;

    /* @hack spelling: blocks assigned in the test, which SHC tests in R0 and
     * copies to R4 in the BT/S's slot (CMP/PZ R0 at 0x8c65d6c6). assigned
     * apart, SHC copies it first, 8 of 13 words differ, and 1 of
     * ufSavePacket's 44 */
    if ((blocks = fmMngGetFree()) < 0) {
        ufgErrno = UFD_ERR_NO_INIT;
    }
    return blocks;
}

Bool ufInitSystem(void *work1, void *work2, void *work3, void *work4)
{
    Bool ready;

    ufgWorkBuff = work1;
    ufgReadBuff = work2;
    ufgReflash3 = work3;
    ufgReflash4 = work4;
    open_bank = -1;
    ufSetBank(HISTORY_BANK);
    ufLoadUserFlag();
    ufgSafeFlag = true;
    syBtGetBootSystemID(&console_id);
    ufgProdNum = (Uint8 *)console_id.szProduct;
    ready = ufPlayInit();
    if (!ready) {
        ufgSafeFlag = false;
    }
    return ready;
}

Bool ufChangeWorkBuffer(void *work1, void *work2, void *work3, void *work4)
{
    Bool changed = false;

    if (ufgSafeFlag) {
        if (work1 != NULL) {
            ufgWorkBuff = work1;
        }
        if (work2 != NULL) {
            ufgReadBuff = work2;
        }
        if (work3 != NULL) {
            ufgReflash3 = work3;
        }
        if (work4 != NULL) {
            ufgReflash4 = work4;
        }
        changed = true;
    } else {
        ufgErrno = UFD_ERR_NO_INIT;
    }
    return changed;
}

Sint32 ufFindTitleIdx(const Uint8 *ProdNum)
{
    Sint32 found = -1;
    Sint32 i;
    Sint32 packet;
    /* a title's two packets, which hold its record */
    union {
        struct title_record record;
        Uint8 bytes[2 * FMD_DATA_BYTES];
    } title;
    Uint8 *second;
    struct title_record *record;
    Uint32 crc;

    if (ufgSafeFlag) {
        /* @hack spelling: i zeroed ahead of second, which SHC does first (MOV
         * #0,R14 at 0x8c65d7ba). zeroed in the for, it comes after, and 2 of
         * 90 words differ */
        i = 0;
        /* @hack spelling: the address of title's second packet through a
         * local, which SHC keeps in the frame (MOV.L R3,@(4,R15) at
         * 0x8c65d7c8). used directly, the function runs 36 bytes short and 79
         * of 90 words differ */
        second = title.bytes + FMD_DATA_BYTES;
        for (; i < TITLES; i++) {
            packet = TITLE_FIRST_PACKET + (i * TITLE_PACKETS);
            if (ufLoadPacket(packet, NULL) == true) {
                if (memcmp(ufgReadBuff + 2, ProdNum, 10) == 0) {
                    memcpy(title.bytes, ufgReadBuff, FMD_DATA_BYTES);
                    packet++;
                    if (ufLoadPacket(packet, NULL) == true) {
                        /* @hack spelling: the record through a local, which
                         * SHC copies to R12 after the second read (MOV R11,R12
                         * at 0x8c65d7fe). used directly, the function runs 4
                         * bytes short and 62 of 90 words differ */
                        record = &title.record;
                        memcpy(second, ufgReadBuff, FMD_DATA_BYTES);
                        crc = fmcalccrc(sizeof(record->body), record->body);
                        if (record->CRC == (Uint16)crc) {
                            found = i;
                            break;
                        }
                    }
                }
            }
        }
    } else {
        ufgErrno = UFD_ERR_NO_INIT;
    }
    return found;
}

Bool ufSafeRefresh(void)
{
    Bool refreshed;

    if (ufgSafeFlag) {
        if (ufgReflash3 == NULL || ufgReflash4 == NULL) {
            refreshed = false;
            ufgErrno = UFD_ERR_MEMORY_FAIL;
        } else if (fhSafeRefresh(ufgWorkBuff, ufgReadBuff, ufgReflash3,
                                 ufgReflash4) == 0) {
            /* a successful refresh sets UFD_ERR_REFRESHFAIL: the result, not
             * ufgErrno, tells success */
            refreshed = true;
            ufgErrno = UFD_ERR_REFRESHFAIL;
        } else {
            refreshed = false;
        }
    } else {
        ufgErrno = UFD_ERR_NO_INIT;
        refreshed = false;
    }
    return refreshed;
}
