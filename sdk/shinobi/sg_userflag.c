/* play history shared flags, in flash packet 0 with a parity byte */
#include "shinobi/uf_work.h"
#include <ninja.h>
#include <sg_uf.h>
#include <sg_xpt.h>
#include <shinobi.h>
#include <string.h>

#define FLAG_PACKET 0
#define PACKET_LONGS (FMD_DATA_BYTES / 4)

/* the bytes the parity covers */
#define PARITY_BYTES 8

/* 1 where a nibble holds an even number of set bits */
static const Uint8 nibble_parity[16] = {1, 0, 0, 1, 0, 1, 1, 0,
                                        0, 1, 1, 0, 1, 0, 0, 1};

struct uf_user_flag ufgUserFlag;

/* one bit for each of the PARITY_BYTES bytes at data, the first's highest.
 * a bit is set when its byte's two nibbles have the same parity */
static Uint32 parity(const Uint8 *data)
{
    Uint32 bits = 0;
    int i;
    Uint8 c;

    for (i = 0; i < PARITY_BYTES; i++) {
        c = data[i];
        if (nibble_parity[c & 15] == nibble_parity[c >> 4]) {
            bits |= 1;
        }
        bits <<= 1;
    }
    bits >>= 1;
    return bits;
}

/* read packet 0 and copy it to ufgUserFlag as the shared flags, unless the
 * parity check refuses it. the check refuses it only when the first byte is
 * 0xff, the parity computed isn't 0xff and the parity stored differs. a
 * refusal returns false with UFD_ERR_USERFLAG_PARITY_ERROR, and leaves
 * ufgUserFlag as it was */
Bool ufLoadUserFlag(void)
{
    Uint8 check;

    ufLoadPacket(FLAG_PACKET, NULL);
    check = parity(ufgReadBuff);
    /* @hack redundant: an else after the return; without it, memcpy's address
     * and ufgReadBuff's trade R2 and R3 (0x8c6708c0), and 4 of 41 words
     * differ */
    if (check != 0xff && ufgReadBuff[0] == 0xff &&
        check != ufgReadBuff[FMD_DATA_BYTES - 1]) {
        ufgErrno = UFD_ERR_USERFLAG_PARITY_ERROR;
        return false;
    } else {
        memcpy(&ufgUserFlag, ufgReadBuff, FMD_DATA_BYTES);
        return true;
    }
}

Bool ufSaveUserFlag(void)
{
    /* @hack spelling: the flags through a pointer, which SHC keeps in R14
     * and adds the parity's offset to after the call (MOV #59,R1 and ADD
     * R14,R1 at 0x8c6708de); named directly, the function is 6 bytes
     * shorter and 13 of 14 words differ */
    struct uf_user_flag *flag = &ufgUserFlag;

    flag->parity = parity((const Uint8 *)flag);
    return ufSavePacket(FLAG_PACKET, flag);
}

/* @hack type: buf an integer, which keeps a frame for the call (ADD #-4,R15
 * and MOV.L R4,@R15 at 0x8c67090e and 0x8c670914); as a pointer, the call
 * is a bare jump, the function is 6 bytes shorter and 6 of 7 words differ */
void ufGetUserState(Sint32 buf)
{
    njMemCopy4((void *)buf, &ufgUserFlag, PACKET_LONGS);
}

void ufSetAutoSaveFlag(Uint8 flag)
{
    ufgUserFlag.auto_save = flag;
}

void ufSetVersion(Uint8 version)
{
    ufgUserFlag.version = version;
}

void ufSetNetuseFlag(Uint8 flag)
{
    ufgUserFlag.netuse = flag;
}

void ufSetData(Uint8 data)
{
    ufgUserFlag.data = data;
}
