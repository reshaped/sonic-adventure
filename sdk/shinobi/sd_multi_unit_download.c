#include "shinobi/sd_work.h"
#include <sg_xpt.h>
#include <shinobi.h>

/* a multi unit's header. a record for each of its banks follows it */
struct multi_unit {
    SDE_DATA_TYPE type;
    Sint8 version;
    Sint8 unk_05[3];
    Sint32 bank_num;
    Sint32 unk_0c;
    /* in a unit older than version 1, the bank records start at this word
     * unless it is -1 */
    Sint32 unk_10;
    Sint32 unk_14[3];
};

struct multi_unit_bank {
    SDE_DATA_TYPE type;
    Sint32 num;
    Sint32 address; /* an offset in sound memory */
    Sint32 size; /* negative when the bank takes no allocation */
    Sint32 data_offset; /* from the start of the unit */
    Sint32 data_size;
    Sint32 unk_18[2];
};

/* the source file name the library prints */
#define SOURCE "sd_multi_unit_download_.c"

/* how much shorter the header is when the bank records start at unk_10 */
#define OLD_HEADER_SHORTFALL 16

SDE_ERR sdMultiUnitDownload(SDMEMBLK handle)
{
    SDE_ERR err = SDE_ERR_NOTHING;
    SDMEMBLK block = NULL;
    struct multi_unit *unit;
    struct multi_unit_bank *bank;
    Sint32 bank_num;
    Sint32 i;

    sdDebugPrintf("Enter:%s\n", SOURCE);
    if (sdgWrkPtr == NULL) {
        err = SDE_ERR_NO_INIT;
    } else if (handle == NULL) {
        err = SDE_ERR_HANDLE_NULL;
    } else {
        unit = ((struct sd_memblk *)handle)->data;
        if (unit->type != SDE_DATA_TYPE_MULTI_UNIT) {
            err = SDE_ERR_DATA_ILLEGAL_TYPE;
        } else {
            sdMemBlkCreate(&block);
            bank_num = unit->bank_num;
            bank = (struct multi_unit_bank *)(unit + 1);
            if (unit->version < 1) {
                sdDebugPrintf("Warning:Illegal MultiUnit version.\n");
                /* @hack spelling: bank set by a select whose first arm keeps
                 * it, which loads the next debug format's address into R3
                 * (0x8c61888a). as an if, R2 and R3 trade and 8 of 134 words
                 * differ */
                bank = unit->unk_10 == -1
                           ? bank
                           : (struct multi_unit_bank *)((char *)bank -
                                                        OLD_HEADER_SHORTFALL);
                err = SDE_ERR_DATA_ILLEGAL_VER;
            }
            sdDebugPrintf("  Block#%02lX Download Adr:%08X\n", bank_num, bank);
            /* @hack spelling: an if around a do, not a for, which sets i's 0 in
             * the slot of the count's test (MOV #0,R9 at 0x8c618896). as a for
             * loop, 4 of 134 words differ */
            i = 0;
            if (bank_num > 0) {
                do {
                    if (bank->size >= 0) {
                        err = sdSndMemAllocateBank(bank->type, bank->num,
                                                   bank->address, bank->size);
                        if (err != SDE_ERR_NOTHING) {
                            break;
                        }
                    }
                    if (bank->data_size > 0) {
                        /* @hack spelling: data and size through locals,
                         * which SHC computes before it pushes the last
                         * argument (MOV.L R8,@-R15 at 0x8c6188c2). passed
                         * directly, the push comes first and 12 of 134 words
                         * differ */
                        void *data = (char *)unit + bank->data_offset;
                        Sint32 size = bank->data_size;

                        err = sdMemBlkSetPrm(block, data, size,
                                             SDD_MEMBLK_SYNC_FUNC, NULL);
                        if (err != SDE_ERR_NOTHING) {
                            break;
                        }
                        err = sdBankDownload(block, bank->type, bank->num);
                        if (err != SDE_ERR_NOTHING) {
                            break;
                        }
                    }
                    bank++;
                } while (++i < bank_num);
            }
        }
    }
    sdDebugPrintf("Exit :%s(%08X)\n", SOURCE, err);
    if (block != NULL) {
        sdMemBlkDestroy(block);
    }
    return err;
}
