/* a title's play history record, read from flash */
#include "shinobi/uf_work.h"
#include <sg_uf.h>
#include <sg_xpt.h>
#include <shinobi.h>
#include <string.h>

/* the flash partition that holds the play history */
#define HISTORY_BANK 3
/* each title's record is TITLE_PACKETS packets of the history partition,
 * the first title's from packet TITLE_FIRST_PACKET */
#define TITLE_FIRST_PACKET 24
#define TITLE_PACKETS 4

Bool ufReadAllHead(Uint8 *ProdNum, UFS_AllPlayHistory *buff)
{
    Uint8 *packet_buf = (Uint8 *)buff;
    Bool read;
    Sint32 title;
    Sint32 packet;
    Sint32 i;

    read = false;
    if (ufgSafeFlag) {
        if (ProdNum == NULL) {
            ProdNum = ufgProdNum;
        }
        ufSetBank(HISTORY_BANK);
        if ((title = ufFindTitleIdx(ProdNum)) >= 0) {
            /* a NULL ProdNum took ufgProdNum above, so this branch runs only
             * when ufgProdNum is NULL too. it copies the running title's
             * record from ufgPlayHistBuffer */
            if (ProdNum == NULL) {
                memcpy(buff, &ufgPlayHistBuffer, sizeof(UFS_AllPlayHistory));
                read = true;
            } else {
                packet = (title * TITLE_PACKETS) + TITLE_FIRST_PACKET;
                if (ufgUserFlag.data != 0) {
                    for (i = 0; i < TITLE_PACKETS; i++) {
                        if (ufLoadPacket(packet++, packet_buf) != true) {
                            ufgErrno = UFD_ERR_READ_FAIL;
                            return false;
                        }
                        packet_buf += FMD_DATA_BYTES;
                    }
                    read = true;
                } else {
                    ufgErrno = UFD_ERR_PROTECT_COMMON_FLAG;
                    read = false;
                }
            }
        } else {
            ufgErrno = UFD_ERR_NOT_FOUND;
            read = false;
        }
    } else {
        ufgErrno = UFD_ERR_NO_INIT;
    }
    return read;
}
