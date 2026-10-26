/* the console settings, kept in a flash memory packet and copied into
 * kbCfgGetId's record */
#include <sg_xpt.h>
#include <shinobi.h>

/* the partition and packet that hold the settings */
#define CFG_PART 2
#define CFG_PACKET 5
/* where the settings lie in kbCfgGetId's record, and their size */
#define SETTINGS_OFFSET 16
#define SETTINGS_BYTES 8
/* permit's SYD_BT_CFGWRT_ bits run from AT's, bit 0, to TM's, bit 4 */
#define PERMIT_BITS 5
#define TM_BIT 4

const char *syBtCfggBuild = "\nsyBtCfg Ver 1.00 Build:Sep 29 1998 15:00:47\n";

Sint32 syBtCfgSet(Sint32 permit, void *work)
{
    char packet[FMD_DATA_BYTES];
    char *settings;
    Sint32 result;
    Sint32 status;

    result = fmMngInit(CFG_PART, work);
    if (result == FMD_BAD_HEADER) {
        /* @hack spelling: the format's result in a local of its own, which
         * SHC tests apart from fmMngInit's (TST R4,R4 at 0x8c11b82e); through
         * result, the two share one test, the function is 8 bytes shorter
         * and 100 of 116 words differ */
        Sint32 error = fmMngFormat(FMD_WRITABLE);
        if (error != 0) {
            return SYD_BT_CFG_ERR;
        }
    } else if (result != 0) {
        return SYD_BT_CFG_ERR;
    }
    result = fmMngRead(CFG_PACKET, packet);
    if (result != 0) {
        return SYD_BT_CFG_ERR;
    }
    while ((status = fmMngExecServer()) == FMD_STAT_RUNNING) {
    }
    if (status != FMD_STAT_COMPLETE) {
        return SYD_BT_CFG_ERR;
    }

    /* @hack redundant: a goto to the next statement, which keeps kbCfgGetId's
     * address in R3 (MOV.L at 0x8c11b85e); without it, the address, the byte
     * copy and the permit test take R2, and 6 of 116 words differ */
    goto find_settings;
find_settings:
    settings = (char *)kbCfgGetId() + SETTINGS_OFFSET;
    /* @hack redundant: a goto to the next statement, which keeps the byte copy
     * and the permit test in R3 (0x8c11b874, 0x8c11b890); without it, both take
     * R2 and 4 of 116 words differ */
    goto copy;
copy:
    if (permit == SYD_BT_CFGINI) {
        char *from = packet;
        Sint32 count;
        for (count = 0; count < SETTINGS_BYTES; count++) {
            /* @hack spelling: the pointers stepped in statements of their own,
             * which keeps the byte in R3 (MOV.B @R6+,R3 at 0x8c11b874); as
             * *settings++ = *from++, the byte copy and the permit test take
             * R2, the permit loop's 5 and 1 trade R11 and R7, and 7 of 116
             * words differ */
            *settings = *from;
            ++from;
            ++settings;
        }
    } else {
        char *to = packet + (SETTINGS_BYTES - 1);
        Sint32 bit;
        settings += SETTINGS_BYTES - 1;
        for (bit = 0; bit < PERMIT_BITS; bit++) {
            if (permit & 1) {
                *to = *settings;
                if (bit == TM_BIT) {
                    *--to = *--settings;
                    *--to = *--settings;
                    *--to = *--settings;
                }
            }
            /* @hack spelling: the shift spelled permit = permit >> 1, which
             * keeps the copy loop's from in R6 and its count in R5
             * (0x8c11b86c, 0x8c11b870); as >>=, the two trade and 5 of 116
             * words differ */
            permit = permit >> 1;
            /* after TM's four bytes, the last step takes to one byte below
             * packet's start. a pointer there is undefined in C, so the step
             * subtracts from the address as an integer */
            to = (char *)((Uint32)to - 1);
            --settings;
        }
        result = fmMngWrite(CFG_PACKET, packet);
        if (result != 0) {
            return SYD_BT_CFG_ERR;
        }
        while ((status = fmMngExecServer()) == FMD_STAT_RUNNING) {
        }
        if (status != FMD_STAT_COMPLETE) {
            return SYD_BT_CFG_ERR;
        }
    }
    return SYD_BT_CFG_OK;
}
