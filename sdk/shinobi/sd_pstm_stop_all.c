#include "shinobi/sd_work.h"
#include <sg_xpt.h>
#include <shinobi.h>

#define HOST_PSTM_STOP_ALL 0x2f

/* the port numbers sdPstmStopAll walks, though only SDD_PSTM_PORT_MAX
 * exist */
#define STOPPED_PORTS 16

SDE_ERR sdPstmStopAll(void)
{
    static struct sd_host_command command;
    SDE_ERR err;
    struct sd_stream *stream;
    int number;
    int i;
    Sint32 ch_num;

    if (sdgWrkPtr == NULL) {
        err = SDE_ERR_NO_INIT;
        goto error;
    }
    /* @bug memory: ports 4 to 15 are past streams' end. SHC computes
     * number * 44 in a byte (EXTU.B R3,R3 at 0x8c6777e2), so each such port's
     * record starts below 256 bytes from streams: past its end, or inside
     * another port's record. the loop reads ch_num there and writes stopped,
     * unk_14 and unk_24 for each channel it gives */
#ifdef GCC
    /* repairs the reads and writes past streams the tag names: GCC's loop
     * stops at streams' last port. no path reaches the loop: sdPstmStopAll's
     * one caller, mwSndStopAll in mw_sdcs.c, has no caller */
    for (number = 0; number < SDD_PSTM_PORT_MAX; number++) {
#else
    for (number = 0; number < STOPPED_PORTS; number++) {
#endif
        stream = &sdgWrkPtr->streams[number];
        /* @hack spelling: ch_num through a local, which SHC reads once for
         * each port into R1 (MOV R0,R1 at 0x8c6777ea). tested in place, the
         * function runs 8 bytes short and 66 of 70 words differ */
        /* @hack type: ch_num a Sint32, which the byte read extends once
         * (MOV.B @(4,R6),R0 at 0x8c6777e8). as a Sint8, SHC extends it again
         * for the test, and 48 of 70 words differ */
        /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
        ch_num = stream->ch_num;
        for (i = 0; i < ch_num; i++) {
            stream->stopped[i] = true;
            stream->unk_14[i] = 0;
            stream->unk_24[i] = 0;
        }
    }
    command.command = HOST_PSTM_STOP_ALL;
    err = sdSetHostCmd(&command);

error:
    if (err != SDE_ERR_NOTHING) {
        sdSysErrTrap((void *)sdPstmStopAll, err);
    }
    return err;
}
