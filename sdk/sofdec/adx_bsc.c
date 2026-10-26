/* ADX basic decoders, which read an ADX, SPSD or WAV stream's header and turn
 * the data after it into samples in a ring */
#include <sg_xpt.h>
#include <sofdec.h>
#include <string.h>

#define ADXB_MAX_OBJ 8
/* the first halfword of an ADX header, whose fields are big-endian */
#define ADX_MARK 0x8000
/* x with its two low bytes swapped, which reads a big-endian halfword */
#define ADX_SWAP16(x) ((((x) >> 8) & 0xff) | (((x) << 8) & 0xff00))
/* where ADX_DecodeInfoWav reads a WAV file's fmt chunk: past the RIFF header's
 * 12 bytes and the chunk's ID and length */
#define WAV_FMT_BODY 20

/* one decode step's parameters. ADXB_EntryData sets the input. a header
 * reader sets the block layout, the ring and the filter, and get_wr sets where
 * the step writes */
typedef struct {
    Sint8 *ibuf;
    Sint32 nblk;
    Sint32 nch;
    Sint32 blklen;
    Sint32 blksmpl;
    Sint16 *obuf;
    Sint32 obufsize;
    Sint32 obufdist;
    /* get_wr's results: the write position in obuf, the samples of room from
     * there, and nleft, the samples left before decoding stops */
    Sint32 wpos;
    Sint32 nroom;
    Sint32 nleft;
    /* @hack type: k0 and k1 as an array, which SHC stores through coef's
     * address (MOV R14,R3 and ADD #124,R3 at 0x8c67561e, MOV.W R2,@R3 at
     * 0x8c67562c). as two fields, ADXB_DecodeHeaderWav, ADXB_DecodeHeaderSpsd
     * and ADXB_DecodeHeaderAdx are 8 bytes shorter together, and 221 of their
     * 356 words differ */
    Sint16 coef[2];
    Sint16 *dly;
    Sint32 unk_34;
} ADXB_DEC;

typedef struct adxb_obj {
    Sint16 used;
    Sint16 unk_02;
    Sint32 stat;
    ADXPD xpd;
    Sint8 enc;
    Sint8 bps;
    Sint8 nch;
    Sint8 blklen;
    Sint32 blksmpl;
    Sint32 sfreq;
    Sint32 total;
    Sint16 cof;
    Sint16 unk_1e;
    Sint32 lp_ins_nsmpl;
    Sint16 nloop;
    Sint16 lp_type;
    Sint32 lp_start_pos;
    Sint32 lp_start_ofst;
    Sint32 lp_end_pos;
    Sint32 lp_end_ofst;
    Sint32 maxnch;
    Sint16 *pcmbuf; /* the ring the decoder writes */
    Sint32 pcmbsize; /* in samples of one channel */
    Sint32 pcmbdist; /* samples from the first channel's ring to the second's */
    Sint16 k0;
    Sint16 k1;
    Sint16 dly[ADXT_MAX_NCH];
    ADXB_DEC dec;
    ADXB_GETWR get_wr;
    void *get_obj;
    ADXB_ADDWR add_wr;
    void *add_obj;
    Sint32 nsmpl; /* the samples adxb_DefAddWr has counted */
    Sint32 wpos; /* adxb_DefGetWr's write position in pcmbuf */
    Sint32 dec_nsmpl; /* the samples of each channel the last step decoded */
    Sint32 dec_dtlen; /* the input bytes the last step decoded */
    Sint32 format;
} ADXB_OBJ;

ADXB_OBJ adxb_obj[ADXB_MAX_OBJ];
/* the RIFF form type and the data chunk's ID. ADX_DecodeInfoWav looks for the
 * data chunk alone, so adxb_wave_id is unused */
static char *adxb_wave_id = "WAVE";
static char *adxb_data_id = "data";

void ADXB_Init(void)
{
    ADXPD_Init();
    memset(adxb_obj, 0, sizeof(adxb_obj));
}

/* the default get_wr: the room runs from wpos to pcmbuf's end, and decoding
 * stops at the stream's end */
Sint16 *adxb_DefGetWr(void *obj, Sint32 *wpos, Sint32 *nroom, Sint32 *nleft)
{
    ADXB adxb = obj;

    *wpos = adxb->wpos;
    *nroom = adxb->pcmbsize - adxb->wpos;
    *nleft = adxb->total - adxb->nsmpl;
    return adxb->pcmbuf;
}

void adxb_DefAddWr(void *obj, Sint32 wsize, Sint32 wnsmpl)
{
    ADXB adxb = obj;

    adxb->wpos += wnsmpl;
    adxb->nsmpl += wnsmpl;
}

ADXB ADXB_Create(Sint32 maxnch, Sint16 *pcmbuf, Sint32 pcmbsize,
                 Sint32 pcmbdist)
{
    ADXB adxb;
    Sint32 i;

    for (i = 0; i < ADXB_MAX_OBJ; i++) {
        if (adxb_obj[i].used == 0) {
            break;
        }
    }
    if (i == ADXB_MAX_OBJ) {
        return NULL;
    }
    adxb = &adxb_obj[i];
    memset(adxb, 0, sizeof(ADXB_OBJ));
    adxb->used = 1;
    adxb->xpd = ADXPD_Create();
    if (adxb->xpd == NULL) {
        ADXB_Destroy(adxb);
        return NULL;
    }
    adxb->maxnch = maxnch;
    adxb->pcmbuf = pcmbuf;
    adxb->pcmbsize = pcmbsize;
    adxb->pcmbdist = pcmbdist;
    adxb->get_wr = adxb_DefGetWr;
    adxb->get_obj = adxb;
    adxb->add_wr = adxb_DefAddWr;
    adxb->add_obj = adxb;
    return adxb;
}

void ADXB_Destroy(ADXB adxb)
{
    if (adxb != NULL) {
        ADXPD_Destroy(adxb->xpd);
        memset(adxb, 0, sizeof(ADXB_OBJ));
        adxb->used = 0;
    }
}

/* the body of a WAV file's fmt chunk */
typedef struct {
    Sint16 tag;
    Sint16 nch;
    Sint32 sfreq;
    Sint32 byterate;
    Sint16 blkalign;
    Sint16 bps;
} ADXB_WAVFMT;

/* read the WAV header of size bytes at hdr. the format comes from the fmt
 * chunk at WAV_FMT_BODY. the data follows the first 4-byte word that reads
 * "data" and the length after it, and *dtofs gets its offset. for 4-bit data,
 * *blklen gets 2 bytes for each channel and *blksmpl 4, and *total gets each
 * channel's 2-byte units. returns 0, or -1 when no word reads "data", which
 * writes no output */
Sint32 ADX_DecodeInfoWav(Sint8 *hdr, Sint32 size, Sint16 *dtofs, Sint8 *enc,
                         Sint8 *bps, Sint8 *blklen, Sint8 *nch, Sint32 *sfreq,
                         Sint32 *total, Sint32 *blksmpl)
{
    ADXB_WAVFMT *fmt = (ADXB_WAVFMT *)(hdr + WAV_FMT_BODY);
    Sint32 *chunk = (Sint32 *)hdr;
    Sint32 i;
    Uint32 dtlen;

    for (i = 0; i < (Sint32)(size / sizeof(Sint32)); i++) {
        if (chunk[i] == *(Sint32 *)adxb_data_id) {
            break;
        }
    }
    if (i == (Sint32)(size / sizeof(Sint32))) {
        return -1;
    }
    dtlen = chunk[i + 1];
    *dtofs = (i + 2) * 4;
    *enc = -1;
    *sfreq = fmt->sfreq;
    *nch = fmt->nch;
    *bps = fmt->bps;
    *blklen = fmt->blkalign;
    *total = (Sint32)dtlen / *blklen;
    *blksmpl = 1;
    if (*bps == 4) {
        *blklen = *nch * 2;
        *blksmpl = 4;
        *total = dtlen / 2 / *nch;
    }
    return 0;
}

Sint32 ADXB_DecodeHeaderWav(ADXB adxb, Sint8 *hdr, Sint32 size)
{
    Sint16 dtofs;
    Sint32 err;

    adxb->unk_02 = 1;
    err = ADX_DecodeInfoWav(hdr, size, &dtofs, &adxb->enc, &adxb->bps,
                            &adxb->blklen, &adxb->nch, &adxb->sfreq,
                            &adxb->total, &adxb->blksmpl);
    if (err < 0) {
        return 0;
    }
    adxb->cof = 0;
    adxb->k1 = 0;
    adxb->k0 = 0;
    adxb->lp_type = 0;
    adxb->nloop = 0;
    adxb->lp_end_ofst = 0;
    adxb->lp_end_pos = 0;
    adxb->lp_start_ofst = 0;
    adxb->lp_start_pos = 0;
    adxb->lp_ins_nsmpl = 0;
    /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
    adxb->dec.nch = adxb->nch;
    /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
    adxb->dec.blklen = adxb->blklen;
    adxb->dec.blksmpl = adxb->blksmpl;
    adxb->dec.obuf = adxb->pcmbuf;
    adxb->dec.obufsize = adxb->pcmbsize;
    adxb->dec.obufdist = adxb->pcmbdist;
    adxb->dec.coef[0] = adxb->k0;
    adxb->dec.coef[1] = adxb->k1;
    adxb->dec.dly = adxb->dly;
    adxb->wpos = 0;
    adxb->nsmpl = 0;
    adxb->format = ADXB_FORMAT_WAV;
    memset(adxb->dly, 0, sizeof(adxb->dly));
    return dtofs;
}

/* read the SPSD header at hdr. *blklen gets 2 and *blksmpl 1, a block of one
 * 2-byte sample, and *total gets the data's length over 2. returns 0 */
Sint32 ADX_DecodeInfoSpsd(Sint8 *hdr, Sint32 size, Sint16 *dtofs, Sint8 *enc,
                          Sint8 *bps, Sint8 *blklen, Sint8 *nch, Sint32 *sfreq,
                          Sint32 *total, Sint32 *blksmpl)
{
    void *info;
    Sint32 hdsize;

    info = nlsndStreamGetHdInfo(hdr, &hdsize);
    *dtofs = hdsize;
    *nch = nlsndStreamGetParam(info, NLSND_NCH, 0);
    *sfreq = nlsndStreamGetParam(info, NLSND_SFREQ, 0);
    *bps = nlsndStreamGetParam(info, NLSND_BPS, 0);
    /* the switch sets a block for each depth, but the stores below it replace
     * every value it sets */
    /* @hack spelling: the cases from 16 down to 4, which SHC lays out in that
     * order, each branching into the next for the stores they share (BRA at
     * 0x8c6756be and 0x8c6756c8). in ascending order, 91 of 92 words
     * differ */
    switch (*bps) {
    case 16:
        *blklen = *nch * 2;
        *blksmpl = 1;
        *total = nlsndStreamGetParam(info, NLSND_NSMPL, 0);
        break;
    case 8:
        *blklen = *nch;
        *blksmpl = 1;
        *total = nlsndStreamGetParam(info, NLSND_NSMPL, 0);
        break;
    case 4:
        *blklen = *nch;
        *blksmpl = 2;
        *total = nlsndStreamGetParam(info, NLSND_NSMPL, 0);
        break;
    }
    *blklen = 2;
    *blksmpl = 1;
    *total = nlsndStreamGetParam(info, NLSND_DTLEN, 0) / 2;
    *enc = -1;
    return 0;
}

Sint32 ADXB_DecodeHeaderSpsd(ADXB adxb, Sint8 *hdr, Sint32 size)
{
    Sint16 dtofs;
    Sint32 err;

    adxb->unk_02 = 1;
    err = ADX_DecodeInfoSpsd(hdr, size, &dtofs, &adxb->enc, &adxb->bps,
                             &adxb->blklen, &adxb->nch, &adxb->sfreq,
                             &adxb->total, &adxb->blksmpl);
    if (err < 0) {
        return 0;
    }
    adxb->cof = 0;
    adxb->k1 = 0;
    adxb->k0 = 0;
    adxb->lp_type = 0;
    adxb->nloop = 0;
    adxb->lp_end_ofst = 0;
    adxb->lp_end_pos = 0;
    adxb->lp_start_ofst = 0;
    adxb->lp_start_pos = 0;
    adxb->lp_ins_nsmpl = 0;
    /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
    adxb->dec.nch = adxb->nch;
    /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
    adxb->dec.blklen = adxb->blklen;
    adxb->dec.blksmpl = adxb->blksmpl;
    adxb->dec.obuf = adxb->pcmbuf;
    adxb->dec.obufsize = adxb->pcmbsize;
    adxb->dec.obufdist = adxb->pcmbdist;
    adxb->dec.coef[0] = adxb->k0;
    adxb->dec.coef[1] = adxb->k1;
    adxb->dec.dly = adxb->dly;
    adxb->wpos = 0;
    adxb->nsmpl = 0;
    adxb->format = ADXB_FORMAT_SPSD;
    memset(adxb->dly, 0, sizeof(adxb->dly));
    return dtofs;
}

Sint32 ADXB_DecodeHeaderAdx(ADXB adxb, Sint8 *hdr, Sint32 size)
{
    Sint16 dtofs;
    Sint32 err;

    adxb->unk_02 = 1;
    err =
        ADX_DecodeInfo(hdr, size, &dtofs, &adxb->enc, &adxb->bps, &adxb->blklen,
                       &adxb->nch, &adxb->sfreq, &adxb->total, &adxb->blksmpl);
    if (err < 0) {
        return 0;
    }
    err = ADX_DecodeInfoExADPCM2(hdr, size, &adxb->cof);
    if (err < 0) {
        return 0;
    }
    ADXPD_SetCoef(adxb->xpd, adxb->sfreq, adxb->cof);
    ADX_DecodeInfoExLoop(hdr, size, &adxb->lp_ins_nsmpl, &adxb->nloop,
                         &adxb->lp_type, &adxb->lp_start_pos,
                         &adxb->lp_start_ofst, &adxb->lp_end_pos,
                         &adxb->lp_end_ofst);
    /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
    adxb->dec.nch = adxb->nch;
    /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
    adxb->dec.blklen = adxb->blklen;
    adxb->dec.blksmpl = adxb->blksmpl;
    adxb->dec.obuf = adxb->pcmbuf;
    adxb->dec.obufsize = adxb->pcmbsize;
    adxb->dec.obufdist = adxb->pcmbdist;
    adxb->dec.coef[0] = adxb->k0;
    adxb->dec.coef[1] = adxb->k1;
    adxb->dec.dly = adxb->dly;
    adxb->wpos = 0;
    adxb->format = ADXB_FORMAT_ADX;
    memset(adxb->dly, 0, sizeof(adxb->dly));
    return dtofs;
}

/* ADX_MARK starts an ADX header, "SPSD" an SPSD one, and any other header is
 * read as WAV */
Sint32 ADXB_DecodeHeader(ADXB adxb, Sint8 *hdr, Sint32 size)
{
    /* @hack type: the mark through a Uint16 local, which SHC zero-extends
     * before its test (EXTU.W R4,R4 at 0x8c675940). tested in place, the
     * function is 4 bytes shorter and 42 of 54 words differ */
    Uint16 mark;

    mark = ADX_SWAP16(*(Uint16 *)hdr);
    /* @hack redundant: the elses after the returns, which load memcmp's address
     * into R1 (MOV.L at 0x8c67595c). as separate ifs, R3 takes it and 2 of 54
     * words differ */
    if (mark == ADX_MARK) {
        return ADXB_DecodeHeaderAdx(adxb, hdr, size);
    } else if (memcmp(hdr, "SPSD", 4) == 0) {
        return ADXB_DecodeHeaderSpsd(adxb, hdr, size);
    } else {
        return ADXB_DecodeHeaderWav(adxb, hdr, size);
    }
}

void ADXB_EntryGetWrFunc(ADXB adxb, ADXB_GETWR func, void *obj)
{
    adxb->get_wr = func;
    adxb->get_obj = obj;
}

void ADXB_EntryAddWrFunc(ADXB adxb, ADXB_ADDWR func, void *obj)
{
    adxb->add_wr = func;
    adxb->add_obj = obj;
}

Sint16 *ADXB_GetPcmBuf(ADXB adxb)
{
    return adxb->pcmbuf;
}

Sint32 ADXB_GetFormat(ADXB adxb)
{
    return adxb->format;
}

Sint32 ADXB_GetSfreq(ADXB adxb)
{
    return adxb->sfreq;
}

Sint32 ADXB_GetNumChan(ADXB adxb)
{
    return adxb->nch;
}

/* the bits of each output sample: 16 for ADX and for an unknown format, and
 * the stream's own depth for SPSD and WAV */
Sint32 ADXB_GetOutBps(ADXB adxb)
{
    /* @hack redundant: the ADX test, which returns the 16 the last line returns
     * anyway, and is the test SHC makes first (TST R3,R3 at 0x8c6759d6).
     * without it, 15 of 16 words differ */
    if (adxb->format == ADXB_FORMAT_ADX) {
        return 16;
    }
    if (adxb->format == ADXB_FORMAT_SPSD) {
        return adxb->bps;
    }
    if (adxb->format == ADXB_FORMAT_WAV) {
        return adxb->bps;
    }
    return 16;
}

Sint32 ADXB_GetBlkSmpl(ADXB adxb)
{
    return adxb->blksmpl;
}

Sint32 ADXB_GetBlkLen(ADXB adxb)
{
    return adxb->blklen;
}

Sint32 ADXB_GetTotalNumSmpl(ADXB adxb)
{
    return adxb->total;
}

Sint32 ADXB_GetCof(ADXB adxb)
{
    return adxb->cof;
}

Sint32 ADXB_GetLpInsNsmpl(ADXB adxb)
{
    return adxb->lp_ins_nsmpl;
}

Sint32 ADXB_GetNumLoop(ADXB adxb)
{
    return adxb->nloop;
}

Sint32 ADXB_GetLpStartPos(ADXB adxb)
{
    return adxb->lp_start_pos;
}

Sint32 ADXB_GetLpStartOfst(ADXB adxb)
{
    return adxb->lp_start_ofst;
}

Sint32 ADXB_GetLpEndPos(ADXB adxb)
{
    return adxb->lp_end_pos;
}

Sint32 ADXB_GetLpEndOfst(ADXB adxb)
{
    return adxb->lp_end_ofst;
}

Sint32 ADXB_GetStat(ADXB adxb)
{
    return adxb->stat;
}

/* give the decoder ibuflen bytes at ibuf for its next step, and clear the
 * last step's counts. dec.nblk counts ADX data in blocks, and SPSD and WAV
 * data in sets of one 2-byte sample for each channel */
void ADXB_EntryData(ADXB adxb, Sint8 *ibuf, Sint32 ibuflen)
{
    if (adxb->format == ADXB_FORMAT_ADX) {
        adxb->dec.ibuf = ibuf;
        adxb->dec.nblk = ibuflen / adxb->blklen;
    } else {
        adxb->dec.ibuf = ibuf;
        adxb->dec.nblk = ibuflen / (adxb->nch * sizeof(Sint16));
    }
    adxb->dec.unk_34 = 0;
    adxb->dec_nsmpl = 0;
    adxb->dec_dtlen = 0;
}

void ADXB_Start(ADXB adxb)
{
    if (adxb->stat == ADXB_STAT_STOP) {
        adxb->stat = ADXB_STAT_PREP;
    }
}

void ADXB_Stop(ADXB adxb)
{
    ADXPD_Stop(adxb->xpd);
    adxb->stat = ADXB_STAT_STOP;
}

void ADXB_Reset(ADXB adxb)
{
    if (adxb->stat == ADXB_STAT_END) {
        ADXPD_Reset(adxb->xpd);
        adxb->wpos = 0;
        adxb->stat = ADXB_STAT_STOP;
    }
}

Sint32 ADXB_GetDecDtLen(ADXB adxb)
{
    return adxb->dec_dtlen;
}

Sint32 ADXB_GetDecNumSmpl(ADXB adxb)
{
    return adxb->dec_nsmpl;
}

ADXPD ADXB_GetAdxpd(ADXB adxb)
{
    return adxb->xpd;
}

/* start the expander on nblk blocks of dec.ibuf, decoding them into the ring
 * at dec.wpos */
void ADXB_EvokeExpandMono(ADXB adxb, Sint32 nblk)
{
    ADXB_DEC *dec = &adxb->dec;
    ADXPD xpd = adxb->xpd;
    /* @hack spelling: ibuf read through adxb rather than dec, which indexes
     * adxb by R0 (MOV #80,R0 at 0x8c675abe, MOV.L @(R0,R4),R5 at 0x8c675ac8).
     * through dec, the function is 2 bytes shorter and 17 of 23 words differ */
    Sint8 *ibuf = adxb->dec.ibuf;
    Sint16 *obuf = dec->obuf;

    /* @hack spelling: obuf advanced in a statement of its own, which loads obuf
     * before wpos (MOV.L @(20,R6),R7 at 0x8c675aca). advanced at its
     * declaration, 3 of 23 words differ */
    obuf += dec->wpos;
    ADXPD_EntryMono(xpd, ibuf, nblk, obuf);
    ADXPD_Start(xpd);
}

/* as ADXB_EvokeExpandMono for two channels: nblk blocks of each, which
 * alternate in dec.ibuf, into the two rings */
void ADXB_EvokeExpandSte(ADXB adxb, Sint32 nblk)
{
    ADXB_DEC *dec = &adxb->dec;
    ADXPD xpd = adxb->xpd;
    /* @hack spelling: ibuf read through adxb, as in ADXB_EvokeExpandMono (MOV
     * #80,R0 at 0x8c675b12, MOV.L @(R0,R4),R5 at 0x8c675b1c). through dec, the
     * function is 2 bytes shorter and 20 of 28 words differ */
    Sint8 *ibuf = adxb->dec.ibuf;
    Sint16 *obufl = dec->obuf;
    Sint16 *obufr;

    /* @hack spelling: obufl advanced in a statement of its own, as in
     * ADXB_EvokeExpandMono (MOV.L @(20,R6),R7 at 0x8c675b1e). advanced at its
     * declaration, 3 of 28 words differ */
    obufl += dec->wpos;
    obufr = obufl + dec->obufdist;
    ADXPD_EntrySte(xpd, ibuf, nblk * 2, obufl, obufr);
    ADXPD_Start(xpd);
}

/* start the expander on as many blocks of each channel as four limits allow.
 * two are the data ADXB_EntryData gave and the room get_wr gave. the others
 * are the blocks that hold the nleft samples left before decoding stops, and
 * the blocks that start before the ring's end. the last block can run past the
 * ring's end, and ADXB_EndDecode copies what it writes there to the ring's
 * start */
void ADXB_EvokeDecode(ADXB adxb)
{
    ADXB_DEC *dec = &adxb->dec;
    /* @hack spelling: the locals declared in this order, which adds blksmpl to
     * nleft (MOV R10,R1 and ADD R6,R1 at 0x8c675b6c) and sums end - pad + wpos
     * in end's register (SUB R9,R2 and ADD R11,R2 at 0x8c675bb2). declared in
     * the order they are set, 6 of 102 words differ */
    Sint32 nblk;
    Sint32 nfree;
    Sint32 end;
    Sint32 pad;
    Sint32 wpos;
    Sint32 nroom;
    Sint32 nleft;
    Sint32 blksmpl;
    Sint32 nlp;
    Sint32 nrm;

    nblk = dec->nblk / dec->nch;
    nfree = dec->obufsize;
    wpos = dec->wpos;
    nroom = dec->nroom;
    blksmpl = dec->blksmpl;
    nleft = dec->nleft;
    nlp = (nleft + blksmpl - 1) / blksmpl;
    pad = blksmpl - ((nleft + blksmpl - 1) % blksmpl) - 1;
    nfree -= wpos;
    nfree = (nfree + blksmpl - 1) / blksmpl;
    end = nfree * blksmpl;
    if (nlp < nfree) {
        if (end - pad + wpos < dec->obufsize) {
            nfree++;
        }
    }
    if (nleft < nroom) {
        nroom += pad;
    }
    nrm = nroom / blksmpl;
    if (nblk > nrm) {
        nblk = nrm;
    }
    if (nblk > nlp) {
        nblk = nlp;
    }
    if (nblk > nfree) {
        nblk = nfree;
    }
    if (dec->nch == 2) {
        ADXB_EvokeExpandSte(adxb, nblk);
    } else {
        ADXB_EvokeExpandMono(adxb, nblk);
    }
}

void memcpy2(Sint16 *dst, Sint16 *src, Sint32 nword)
{
    for (; nword > 0; nword--) {
        *dst++ = *src++;
    }
}

/* copy the nword samples each of two channels decoded past size, the ring's
 * end, back to the ring's start. the second channel's ring starts dist samples
 * on */
void ADXB_CopyExtraBufSte(Sint16 *obuf, Sint32 size, Sint32 dist, Sint32 nword)
{
    memcpy2(obuf, obuf + size, nword);
    /* @hack spelling: size + dist parenthesized, which adds them before the
     * shift (ADD R3,R5 at 0x8c675c44) in a 12-byte frame. as obuf + size +
     * dist, the frame takes 16 bytes, the function is 4 bytes longer, and 21 of
     * 26 words differ */
    memcpy2(obuf + dist, obuf + (size + dist), nword);
}

void ADXB_CopyExtraBufMono(Sint16 *obuf, Sint32 size, Sint32 dist, Sint32 nword)
{
    memcpy2(obuf, obuf + size, nword);
}

/* account for the step the expander finished. dec_nsmpl gets the samples of
 * each channel it decoded, and dec_dtlen the bytes it read. when the step
 * reaches the last block that holds the nleft samples left before decoding
 * stops, the samples past them don't count. samples past the ring's end go
 * back to its start */
void ADXB_EndDecode(ADXB adxb)
{
    ADXB_DEC *dec = &adxb->dec;
    Sint32 blksmpl = dec->blksmpl;
    Sint32 blklen = dec->blklen;
    Sint32 nleft = dec->nleft;
    Sint32 wpos = dec->wpos;
    Sint16 *obuf = dec->obuf;
    Sint32 pcmbsize = adxb->pcmbsize;
    Sint32 pcmbdist = adxb->pcmbdist;
    Sint32 nlp = (nleft + blksmpl - 1) / blksmpl;
    Sint32 pad = blksmpl - ((nleft + blksmpl - 1) % blksmpl) - 1;
    Sint32 nblk = ADXPD_GetNumBlk(adxb->xpd);
    Sint32 nsmpl = blksmpl * nblk / dec->nch;

    /* @bug logic: for two channels, nblk counts both channels' blocks but
     * nlp one channel's (CMP/GT R5,R2 at 0x8c675cea). a stereo step that
     * decodes nlp / 2 or more blocks of each channel, but fewer than nlp,
     * still leaves its last pad samples of each channel uncounted, for the
     * next step to write over */
    if (nlp <= nblk) {
        nsmpl -= pad;
    }
    /* @hack spelling: wpos = wpos + nsmpl, which keeps adxb in R12, blksmpl in
     * R11 and wpos in R13 (0x8c675c8e, 0x8c675c9a, 0x8c675ca4). as
     * wpos += nsmpl, the three trade registers and 19 of 109 words differ */
    wpos = wpos + nsmpl;
    adxb->dec_nsmpl = nsmpl;
    adxb->dec_dtlen = blklen * nblk;
    if (wpos >= pcmbsize) {
        wpos -= pcmbsize;
        if (dec->nch == 2) {
            ADXB_CopyExtraBufSte(obuf, pcmbsize, pcmbdist, wpos);
        } else {
            ADXB_CopyExtraBufMono(obuf, pcmbsize, pcmbdist, wpos);
        }
    }
}

/* run one step on 16-bit WAV data. it copies as many samples of each channel
 * as the ring's end, the room and the data allow. two channels' interleaved
 * samples go to their two rings */
void ADXB_ExecOneWav16(ADXB adxb)
{
    ADXB_DEC *dec = &adxb->dec;
    Sint16 *ibuf = (Sint16 *)dec->ibuf;
    Sint16 *obuf;
    Sint16 *obufl;
    Sint16 *obufr;
    Sint32 n;
    Sint32 i;

    if (adxb->stat == ADXB_STAT_PREP) {
        if (ADXPD_GetStat(adxb->xpd) == ADXPD_STAT_STOP) {
            adxb->get_wr(adxb->get_obj, &dec->wpos, &dec->nroom, &dec->nleft);
            n = dec->obufsize - dec->wpos;
            if (n > dec->nroom) {
                n = dec->nroom;
            }
            if (n > dec->nblk) {
                n = dec->nblk;
            }
            obuf = dec->obuf;
            obufl = obuf + dec->wpos;
            if (adxb->nch == 2) {
                /* @hack spelling: obufr summed from obuf as obufdist + wpos,
                 * which reads wpos again (MOV.L @(32,R13),R3 at 0x8c675dc6).
                 * as obufl + obufdist, the function is 4 bytes shorter and 76
                 * of 116 words differ */
                obufr = obuf + (dec->obufdist + dec->wpos);
                for (i = 0; i < n; i++) {
                    obufl[i] = ibuf[i * 2];
                    obufr[i] = ibuf[(i * 2) + 1];
                }
            } else {
                for (i = 0; i < n; i++) {
                    obufl[i] = ibuf[i];
                }
            }
            adxb->dec_nsmpl = n;
            adxb->dec_dtlen = n * adxb->nch * 2;
            adxb->stat = ADXB_STAT_EXEC;
        }
    }
    if (adxb->stat == ADXB_STAT_EXEC) {
        adxb->add_wr(adxb->add_obj, adxb->dec_dtlen, adxb->dec_nsmpl);
        adxb->stat = ADXB_STAT_END;
    }
}

/* as ADXB_ExecOneWav16, for the 4-bit WAV data ADXB_ExecHndl hands it. each
 * sample is a low byte and then a high byte. two channels interleave by byte:
 * both low bytes, then both high bytes */
void ADXB_ExecOneWav4(ADXB adxb)
{
    ADXB_DEC *dec = &adxb->dec;
    Uint8 *ibuf = (Uint8 *)dec->ibuf;
    Sint16 *obuf;
    Sint16 *obufl;
    Sint16 *obufr;
    Sint32 n;
    Sint32 i;

    if (adxb->stat == ADXB_STAT_PREP) {
        if (ADXPD_GetStat(adxb->xpd) == ADXPD_STAT_STOP) {
            adxb->get_wr(adxb->get_obj, &dec->wpos, &dec->nroom, &dec->nleft);
            n = dec->obufsize - dec->wpos;
            if (n > dec->nroom) {
                n = dec->nroom;
            }
            if (n > dec->nblk) {
                n = dec->nblk;
            }
            obuf = dec->obuf;
            obufl = obuf + dec->wpos;
            if (adxb->nch == 2) {
                /* @hack spelling: obufr summed as in ADXB_ExecOneWav16, which
                 * reads wpos again (MOV.L @(32,R13),R3 at 0x8c675ebc). as
                 * obufl + obufdist, 122 of 145 words differ */
                obufr = obuf + (dec->obufdist + dec->wpos);
                for (i = 0; i < n; i++) {
                    obufl[i] = (ibuf[(i * 4) + 2] << 8) | ibuf[i * 4];
                    obufr[i] = (ibuf[(i * 4) + 3] << 8) | ibuf[(i * 4) + 1];
                }
            } else {
                for (i = 0; i < n; i++) {
                    obufl[i] = (ibuf[(i * 2) + 1] << 8) | ibuf[i * 2];
                }
            }
            adxb->dec_nsmpl = n;
            adxb->dec_dtlen = n * adxb->nch * 2;
            adxb->stat = ADXB_STAT_EXEC;
        }
    }
    if (adxb->stat == ADXB_STAT_EXEC) {
        adxb->add_wr(adxb->add_obj, adxb->dec_dtlen, adxb->dec_nsmpl);
        adxb->stat = ADXB_STAT_END;
    }
}

/* run one step on SPSD data as one channel of 16-bit samples, as many as the
 * ring's end, the room and the data allow. for two channels, it reports that
 * stereo is unsupported, and copies as many halfwords into the first ring,
 * both channels' samples interleaved. it counts both channels' bytes of that
 * many samples as read, so half of them are never copied */
void ADXB_ExecOneSpsd(ADXB adxb)
{
    ADXB_DEC *dec = &adxb->dec;
    Sint16 *ibuf = (Sint16 *)dec->ibuf;
    Sint16 *obuf;
    Sint16 *obufl;
    Sint32 n;
    Sint32 i;

    if (adxb->stat == ADXB_STAT_PREP) {
        if (ADXPD_GetStat(adxb->xpd) == ADXPD_STAT_STOP) {
            adxb->get_wr(adxb->get_obj, &dec->wpos, &dec->nroom, &dec->nleft);
            n = dec->obufsize - dec->wpos;
            if (n > dec->nroom) {
                n = dec->nroom;
            }
            if (n > dec->nblk) {
                n = dec->nblk;
            }
            obuf = dec->obuf;
            obufl = obuf + dec->wpos;
            if (adxb->nch == 2) {
                ADXERR_CallErrFunc(
                    "E8101209: Stereo is not suport (ADXB_ExecOneSpsd)\n");
            }
            for (i = 0; i < n; i++) {
                obufl[i] = ibuf[i];
            }
            adxb->dec_nsmpl = n;
            adxb->dec_dtlen = n * adxb->nch * 2;
            adxb->stat = ADXB_STAT_EXEC;
        }
    }
    if (adxb->stat == ADXB_STAT_EXEC) {
        adxb->add_wr(adxb->add_obj, adxb->dec_dtlen, adxb->dec_nsmpl);
        adxb->stat = ADXB_STAT_END;
    }
}

void ADXB_ExecOneAdx(ADXB adxb)
{
    ADXB_DEC *dec = &adxb->dec;

    if (adxb->stat == ADXB_STAT_PREP) {
        if (ADXPD_GetStat(adxb->xpd) == ADXPD_STAT_STOP) {
            adxb->get_wr(adxb->get_obj, &dec->wpos, &dec->nroom, &dec->nleft);
            ADXB_EvokeDecode(adxb);
            adxb->stat = ADXB_STAT_EXEC;
        }
    }
    if (adxb->stat == ADXB_STAT_EXEC) {
        ADXPD_ExecHndl(adxb->xpd);
        if (ADXPD_GetStat(adxb->xpd) == ADXPD_STAT_END) {
            ADXB_EndDecode(adxb);
            ADXPD_Reset(adxb->xpd);
            adxb->add_wr(adxb->add_obj, adxb->dec_dtlen, adxb->dec_nsmpl);
            adxb->stat = ADXB_STAT_END;
        }
    }
}

void ADXB_ExecHndl(ADXB adxb)
{
    if (adxb->format == ADXB_FORMAT_ADX) {
        ADXB_ExecOneAdx(adxb);
    } else if (adxb->format == ADXB_FORMAT_SPSD) {
        ADXB_ExecOneSpsd(adxb);
    } else if (adxb->bps == 4) {
        ADXB_ExecOneWav4(adxb);
    } else {
        ADXB_ExecOneWav16(adxb);
    }
}

void ADXB_ExecServer(void)
{
    Sint32 i;

    for (i = 0; i < ADXB_MAX_OBJ; i++) {
        if (adxb_obj[i].used == 1) {
            ADXB_ExecHndl(&adxb_obj[i]);
        }
    }
}
