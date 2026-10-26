/* sending the commands waiting on the second host command list to the
 * sound driver */
#include "shinobi/sd_work.h"
#include <sg_xpt.h>
#include <shinobi.h>

SDE_ERR sd2ndHostCmdSendList(void)
{
    struct sd_work *work = sdgWrkPtr;
    struct sd_host_commands *host;
    struct sd_host_command_entry *entry;
    SDE_ERR err;
    Sint32 sent;
    Sint32 max;
    Sint32 i;
    Uint8 status;

    if (work == NULL) {
        err = SDE_ERR_NO_INIT;
        goto error;
    }
    host = &work->host;
    /* the last primary list is still in transfer */
    if (host->busy == true) {
        err = SDE_ERR_SND_DRV_BUSY;
        goto error;
    }
    /* the status byte keeps LIST_WAITING until the driver has taken the last
     * primary list */
    sdSndMemReadUint8(work->status, &status);
    if (status != 0) {
        err = SDE_ERR_SND_DRV_BUSY;
        goto error;
    }
    sent = 0;
    max = sdgWrkPtr->host.second_max;
    for (i = 0; i < max; i++) {
        entry = &host->second[host->second_read++];
        if (host->second_read == max) {
            host->second_read = 0;
        }
        if (entry->pending == true) {
            err = sdPrimaryHostCmdWrite(entry);
            if (err != SDE_ERR_NOTHING) {
                goto error;
            }
            sent++;
            /* @bug logic: sent is compared with the commands still
             * waiting, which sdPrimaryHostCmdWrite counts down, not with those
             * waiting at the start. with 2n waiting, the loop stops after n
             * (CMP/EQ R1,R12 at 0x8c65dbe0), and the rest wait for a later
             * call */
            if (sent == SD_PRIMARY_COMMAND_MAX ||
                sent == sdgWrkPtr->host.second_num) {
                break;
            }
        }
    }
    err = sdPrimaryHostCmdFlush();
    /* @hack redundant: a goto to the next label, whose test SHC keeps: TST
     * R14,R14 at 0x8c65dbf2, ahead of the label's at 0x8c65dbf4. without it,
     * the function runs 4 bytes short and 32 of 97 words differ */
    if (err != SDE_ERR_NOTHING) {
        goto error;
    }

error:
    if (err != SDE_ERR_NOTHING) {
        sdSysErrTrap((void *)sd2ndHostCmdSendList, err);
    }
    return err;
}
