#include "shinobi/sd_work.h"
#include <sg_xpt.h>
#include <shinobi.h>

SDE_ERR sdPstmGetCurAdr(SDPSTM handle, const Sint8 target_ch,
                        Sint32 *cur_smp_adr)
{
    struct sd_work *work = sdgWrkPtr;
    SDE_ERR err = SDE_ERR_NOTHING;
    struct sd_stream *stream;
    Sint8 number;
    Sint32 ring_ptr;
    Sint32 ring_size;
    Sint32 cur_adr;

    if (work == NULL) {
        err = SDE_ERR_NO_INIT;
        goto error;
    }
    if (!handle) {
        err = SDE_ERR_HANDLE_NULL;
        goto error;
    }
    if (cur_smp_adr == NULL) {
        err = SDE_ERR_PTR_NULL;
        goto error;
    }
    number = ((struct sd_port *)handle)->number;
    stream = &work->streams[number];
    sdSndMemReadSint32(SD_PSTM_PORT_STATUS(work, number, target_ch, cur_adr),
                       &cur_adr);
    /* the ring buffer's bank is looked up, but its offset and size are never
     * read */
    sdSndMemGetBankStat(SDE_DATA_TYPE_PSTM_RING_BUF, stream->ch[target_ch],
                        &ring_ptr, &ring_size);
    switch (stream->pcm_type) {
    case SDE_PCM_TYPE_8BIT_PCM:
        break;
    case SDE_PCM_TYPE_16BIT_PCM:
        /* @hack spelling: the doubling written as a sum, which leaves the
         * halving below its value in R1 and its 0 in R3 (0x8c676dd4 to
         * 0x8c676dde); as cur_adr *= 2, the two trade and 6 of 103 words
         * differ */
        cur_adr = cur_adr + cur_adr;
        break;
    case SDE_PCM_TYPE_4BIT_ADPCM:
        cur_adr /= 2;
        break;
    }
    *cur_smp_adr = cur_adr;

error:
    if (err != SDE_ERR_NOTHING) {
        sdSysErrTrap((void *)sdPstmGetCurAdr, err);
    }
    return err;
}
