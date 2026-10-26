/* SPSD stream header readers: the header's size, and each of its parameters by
 * number */
#include <sg_xpt.h>
#include <sofdec.h>

/* byte offsets in the header */
#define SPSD_HDSIZE 0x07 /* the header's size, in SPSD_HDSIZE_UNIT bytes */
#define SPSD_FORMAT 0x08 /* the sample format's code */
/* the channel code, in the bits SPSD_CHCODE_MASK selects */
#define SPSD_CHCODE 0x09
#define SPSD_DTLEN 0x0c /* the data's length in bytes */
#define SPSD_UNK_16 0x16
#define SPSD_UNK_1E 0x1e
#define SPSD_UNK_1F 0x1f
#define SPSD_SFREQ 0x2a /* the sample rate, in hertz */
#define SPSD_CH 0x30 /* a table of SPSD_CH_LEN bytes for each channel */

#define SPSD_HDSIZE_UNIT 16
#define SPSD_CHCODE_MASK 3
#define SPSD_CH_LEN 4

void *nlsndStreamGetHdInfo(void *info, Sint32 *hdsize)
{
    *hdsize = ((Uint8 *)info)[SPSD_HDSIZE] * SPSD_HDSIZE_UNIT;
    return info;
}

Sint32 nlsndStreamGetParam(void *info, Sint32 no, Sint16 ch)
{
    Sint8 *hdr = info;
    Sint32 nch;
    Sint32 bits;
    Sint32 len;

    len = *(Sint32 *)(hdr + SPSD_DTLEN);
    /* @bug uninitialized: a channel code of 3 sets no nch, so NLSND_NSMPL
     * and NLSND_NCH read R14 as the caller left it (0x8c677966, 0x8c67796e).
     * ADX_DecodeInfoSpsd keeps info in R14, so its channel count is the low
     * byte of the header's address (0x8c67568a, 0x8c675690) */
    switch (hdr[SPSD_CHCODE] & SPSD_CHCODE_MASK) {
    case 0:
        nch = 1;
        break;
    case 1:
        nch = 2;
        break;
    case 2:
        nch = 4;
        break;
    }
    /* @bug uninitialized: a format other than 0 to 3 sets no bits, so
     * NLSND_BPS and NLSND_NSMPL read R7 as the caller left it (0x8c677952,
     * 0x8c67795e). ADX_DecodeInfoSpsd leaves its enc pointer in R7, so its
     * bits per sample are that pointer's low byte (0x8c6756a4, 0x8c6756a8) */
    switch (hdr[SPSD_FORMAT]) {
    case 0:
        bits = 16;
        break;
    case 1:
        bits = 8;
        break;
    case 2:
    case 3:
        bits = 4;
        break;
    }
    switch (no) {
    case NLSND_SFREQ:
        return *(Uint16 *)(hdr + SPSD_SFREQ);
    case NLSND_BPS:
        return bits;
    case NLSND_NSMPL:
        return len * 8 / bits / nch;
    case NLSND_DTLEN:
        return len;
    case NLSND_NCH:
        return nch;
    case NLSND_CH_BYTE_1:
        return *(Uint8 *)(hdr + SPSD_CH + (ch * SPSD_CH_LEN) + 1);
    case NLSND_CH_BYTE_2:
        return *(Uint8 *)(hdr + SPSD_CH + (ch * SPSD_CH_LEN) + 2);
    case NLSND_CH0_BYTE_3_LOW:
        return ((Uint8 *)hdr)[SPSD_CH + 3] & 0x0f;
    case NLSND_CH0_BYTE_3_HIGH:
        return (((Uint8 *)hdr)[SPSD_CH + 3] & 0xf0) >> 4;
    case NLSND_UNK_16:
        if (((Uint8 *)hdr)[SPSD_UNK_1F] != 0) {
            return 0;
        }
        /* @hack spelling: the value through len, the local case NLSND_DTLEN
         * returns, so that both cases share one return of it (0x8c6779b6).
         * SHC then also keeps hdr + SPSD_CH + 3 in R11 (saved at 0x8c6778c2).
         * in a local of its own, 129 of 138 words differ */
        len = *(Uint16 *)(hdr + SPSD_UNK_16) >> 6;
        if (len == 0) {
            return 1;
        }
        return len;
    case NLSND_UNK_1E:
        return ((Uint8 *)hdr)[SPSD_UNK_1E];
    }
    return -1;
}
