/* ADX header readers, and the coefficients of the filter that predicts each
 * sample from the two before it */
#include <math.h>
#include <sg_xpt.h>
#include <sofdec.h>

/* an ADX header starts with ADX_MARK, then the data's offset counted from
 * ADX_DTOFS_BASE, the end of those two halfwords. every field is big-endian */
#define ADX_MARK 0x8000
#define ADX_DTOFS_BASE 4
/* the shortest header that holds the sample format */
#define ADX_HDR_INFO_LEN 16
/* the shortest headers that hold the cutoff and the loop. a reader needs
 * hdrsize at least that long, and data that starts no sooner */
#define ADX_HDR_COF_LEN 18
#define ADX_HDR_LOOP_LEN 48

typedef struct {
    Uint16 mark;
    Sint16 dtofs;
    Sint8 enc;
    Sint8 blklen;
    Sint8 bps;
    Sint8 nch;
    Sint32 sfreq;
    Sint32 total;
    Uint16 cof;
    Sint8 ver;
    Sint8 rev;
    Sint16 ins_nsmpl;
    Sint16 nloop;
    Sint16 reserved;
    Sint16 type;
    Sint32 start_pos;
    Sint32 start_ofst;
    Sint32 end_pos;
    Sint32 end_ofst;
} ADX_HDR;
#define HDR(hdr) ((const ADX_HDR *)(hdr))

#define ADX_SWAP16(x) ((((x) >> 8) & 0xff) | (((x) << 8) & 0xff00))
#define ADX_SWAP32(x)                                                          \
    ((Uint8)((x) >> 24) | (((x) >> 8) & 0xff00) | (((x) << 8) & 0xff0000) |    \
     ((x) << 24))

void ADX_GetCoefficient(Sint32 cutoff, Sint32 sfreq, Sint16 *k0, Sint16 *k1)
{
    Float a;
    Float b;
    Float c;

    a = sqrt(2.0) - cos(6.283185307 * cutoff / sfreq);
    b = sqrt(2.0) - 1.0;
    c = (a - sqrt((a + b) * (a - b))) / b;
    *k0 = (Sint16)(c * 8192.0);
    *k1 = (Sint16)(-(c * c * 4096.0));
}

Sint32 ADX_DecodeInfo(const Sint8 *hdr, Sint32 hdrsize, Sint16 *dtofs,
                      Sint8 *enc, Sint8 *bps, Sint8 *blklen, Sint8 *nch,
                      Sint32 *sfreq, Sint32 *total, Sint32 *blksmpl)
{
    /* @hack spelling: the fields after the offset read as data[pos], where data
     * is a copy of hdr and pos a cursor in R4 (MOV.B @(R0,R14),R3 at
     * 0x8c66cf7c). read through HDR(hdr), the function is 36 bytes shorter
     * and 130 of 133 words differ */
    const Sint8 *data = hdr;
    Uint16 mark;
    Sint32 pos;

    if (hdrsize < ADX_HDR_INFO_LEN) {
        return -1;
    }
    mark = ADX_SWAP16(HDR(hdr)->mark);
    if (mark != ADX_MARK) {
        return -2;
    }
    *dtofs = ADX_SWAP16(HDR(data)->dtofs) + ADX_DTOFS_BASE;
    pos = ADX_DTOFS_BASE;
    *enc = data[pos++];
    *blklen = data[pos++];
    *bps = data[pos++];
    *nch = data[pos++];
    *sfreq = ADX_SWAP32(*(const Sint32 *)(data + pos));
    pos += 4;
    *total = ADX_SWAP32(*(const Sint32 *)(data + pos));
    *blksmpl = (*blklen - 2) * 8 / *bps;
    return 0;
}

Sint32 ADX_DecodeInfoExADPCM2(const Sint8 *hdr, Sint32 hdrsize, Sint16 *cof)
{
    Sint32 dtofs;
    Uint16 mark;

    if (hdrsize < ADX_HDR_COF_LEN) {
        return -1;
    }
    mark = ADX_SWAP16(HDR(hdr)->mark);
    if (mark != ADX_MARK) {
        return -2;
    }
    dtofs = ADX_SWAP16(HDR(hdr)->dtofs);
    if (dtofs < ADX_HDR_COF_LEN - ADX_DTOFS_BASE) {
        return -1;
    }
    *cof = ADX_SWAP16(HDR(hdr)->cof);
    return 0;
}

Sint32 ADX_DecodeInfoExLoop(const Sint8 *hdr, Sint32 hdrsize, Sint32 *ins_nsmpl,
                            Sint16 *nloop, Sint16 *type, Sint32 *start_pos,
                            Sint32 *start_ofst, Sint32 *end_pos,
                            Sint32 *end_ofst)
{
    /* @hack spelling: ADX_HDR_LOOP_LEN through len, which the data's offset
     * then reuses, so SHC keeps it at @R15 (MOV.L R2,@R15 at 0x8c66d0c0). with
     * the constant in the test and the offset in a local of its own, the
     * function is 4 bytes shorter and 165 of 169 words differ */
    Sint32 len;
    Uint16 mark;

    *nloop = 0;
    len = ADX_HDR_LOOP_LEN;
    if (hdrsize < len) {
        return -1;
    }
    mark = ADX_SWAP16(HDR(hdr)->mark);
    if (mark != ADX_MARK) {
        return -2;
    }
    len = ADX_SWAP16(HDR(hdr)->dtofs);
    if (len < ADX_HDR_LOOP_LEN - ADX_DTOFS_BASE) {
        return -1;
    }
    *ins_nsmpl = ADX_SWAP16(HDR(hdr)->ins_nsmpl);
    *nloop = ADX_SWAP16(HDR(hdr)->nloop);
    if (*nloop != 1) {
        return -2;
    }
    *type = ADX_SWAP16(HDR(hdr)->type);
    *start_pos = ADX_SWAP32(HDR(hdr)->start_pos);
    *start_ofst = ADX_SWAP32(HDR(hdr)->start_ofst);
    *end_pos = ADX_SWAP32(HDR(hdr)->end_pos);
    *end_ofst = ADX_SWAP32(HDR(hdr)->end_ofst);
    return 0;
}
