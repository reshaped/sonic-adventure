/* flash packet read and write, waiting on fmMngExecServer */
/* fh_status.h gives this object its own copy of the safe refresh status
 * strings, though no function here reads them */
#include "shinobi/fh_status.h" /* NOLINT(misc-include-cleaner) */
#include <sg_uf.h>
#include <sg_xpt.h>
#include <shinobi.h>
#include <string.h>

Sint32 fhFmReadExec(Sint32 packet, void *buf)
{
    Sint32 busy;

    if (fmMngRead(packet, buf) != 0) {
        return -1;
    }
    while ((busy = (fmMngExecServer() == FMD_STAT_RUNNING))) {
    }
    /* @bug logic: busy holds whether the server's status was
     * FMD_STAT_RUNNING, not the status itself, and the loop ends with busy 0.
     * the test for FMD_STAT_ERR (CMP/EQ #3,R0 at 0x8c670968) never holds, and
     * a read the server fails returns 0 */
    if (busy == FMD_STAT_ERR) {
        return -1;
    }
    return 0;
}

Sint32 fhFmWriteExec(Sint32 packet, const void *buf)
{
    Uint32 old[16];
    Sint32 busy;

    if (fhFmReadExec(packet, old) != 0) {
        return -1;
    }
    if (memcmp(old, buf, FMD_DATA_BYTES) == 0) {
        return FM_WRITE_UNCHANGED;
    }
    if (fmMngWrite(packet, (void *)buf) != 0) {
        return -1;
    }
    while ((busy = (fmMngExecServer() == FMD_STAT_RUNNING))) {
    }
    /* @bug logic: as in fhFmReadExec, busy is 0 here, so the test for
     * FMD_STAT_ERR (CMP/EQ #3,R0 at 0x8c6709ca) never holds, and a write the
     * server fails returns 0 */
    if (busy == FMD_STAT_ERR) {
        return -1;
    }
    return 0;
}
