/* the sound ports (mw_snd.h): stream ports, their rings in sound memory, and
 * the transfers that fill the rings */
#include <mw_rna.h>
#include <mw_sdbuf.h>
#include <mw_snd.h>
#include <sg_xpt.h>
#include <shinobi.h>
#include <string.h>
#ifdef NONMATCHING
#include "shinobi/sd_work.h"
#include <machine.h>
#include <sg_syg2.h>
#endif

#define MWSND_MAX_OBJ 8

/* sound buffer n's ring is the first MWSND_RING_SIZE bytes of bank n. bank n
 * starts MWSND_NBANK - n banks of MWSND_BANK_SIZE bytes before the end of
 * sound memory. a bank's last 0x40 bytes take the 4-byte copies mw_rna.c
 * writes past a ring's end */
#define MWSND_SND_MEM_SIZE 0x200000
#define MWSND_BANK_SIZE 0x4040
#define MWSND_RING_SIZE 0x4000
#define MWSND_NBANK 16

/* the fewest bytes mwSndTrns copies without a report. for a smaller copy, it
 * sets the CPU transfer mode for every memory block, and reports an error */
#define MWSND_TRNS_MIN 32

struct MwSnd {
    Sint32 used;
    Sint32 nch; /* the channels opened, 1 or 2 */
    Sint32 usech; /* the channels played, as mwSndSetUseCh last set */
    Sint32 bps; /* bits per sample */
    void *buf[2];
    SDMEMBLK mblk[2];
    SDPSTM port;
    /* never read or written: mwSndSetTrnsMode ignores its mode */
    Sint32 trnsmode;
};

static MwSnd mwg_snd_obj[MWSND_MAX_OBJ];
static Uint32 mwg_snd_tcnt[3];

#define TEXT_1_LENGTH 45
#define TEXT_2_LENGTH 49
#define TEXT_3_LENGTH 43
#define TEXT_4_LENGTH 56
#define TEXT_5_LENGTH 50
#define TEXT_6_LENGTH 51
#define TEXT_7_LENGTH 50
#define TEXT_8_LENGTH 50
#define TEXT_9_LENGTH 52
#define TEXT_10_LENGTH 52
#define TEXT_11_LENGTH 51
#define TEXT_12_LENGTH 52
#define TEXT_13_LENGTH 42
#define TEXT_14_LENGTH 42
#define TEXT_15_LENGTH 48
#define TEXT_16_LENGTH 53
#define TEXT_17_LENGTH 46
#define TEXT_18_LENGTH 54
#define TEXT_19_LENGTH 46
#define TEXT_20_LENGTH 50
#define TEXT_21_LENGTH 50
#define TEXT_22_LENGTH 53
#define TEXT_23_LENGTH 43
#define TEXT_24_LENGTH 46
#define TEXT_25_LENGTH 48
#define TEXT_26_LENGTH 54
#define TEXT_27_LENGTH 57
#include "sofdec/mw_sdcs/text.inc"

#ifdef NONMATCHING
/* SEGA's CPU transfer copies a word at a time through syG2Write, with every
 * interrupt masked. in Shinobi's CPU transfer mode, this side's mwSndTrns
 * sends a copy by the AICA's G2 DMA channel instead. it does so where the
 * copy's source, destination and size are multiples of MWSND_DMA_UNIT. it
 * returns without waiting for the copy's end. in CPU mode Shinobi starts no
 * DMA. once Shinobi's last DMA has ended, the channel is this file's alone.
 *
 * outside an interrupt handler, mwSndTrns, mwSndGetMblkStat and
 * mwSndClosePort first wait for the end of the block's copy. so their reads
 * of its status there find the copy finished, as SEGA's CPU transfer left it.
 * sdMemBlkSetPrm's read is one of these.
 *
 * a handler, which runs while syIntNestCount isn't 0, waits for no copy. a
 * wait there could break two ways. after syChainClx1Handler reads SB_ISTNRM
 * and before it calls the channel's end handler, the wait would end the
 * copy. the chain would then call the end handler again and end the next
 * copy early. the end handler clears the end bit before it calls its
 * callback. a wait between the two would spin masked on a bit no copy will
 * set.
 *
 * so to a handler, a block whose copy the ring holds is busy.
 * mwSndGetMblkStat reads it in progress, as it reads a block in Shinobi's
 * DMA. mwSndTrns refuses it, as sdMemBlkSetPrm refuses a block in progress.
 * mwSndTrns and mwSndClosePort return SDE_ERR_MEMBLK_ALREADY_TRANSFER and
 * change nothing. so mwSndClosePort neither destroys a block nor frees the
 * sound buffer a copy writes while the copy runs. a copy sent from a handler
 * keeps SEGA's transfer, as does one the ring has no room for.
 *
 * the mode becomes DMA only while no port is open: in sound_init_driver at
 * boot, and in movie_main. movie_play enters movie_main after stream_destroy
 * has closed every port. so no copy of this file's runs once Shinobi's DMA
 * may start.
 *
 * SEGA's close destroys a block in progress. here a handler's close of a busy
 * block fails instead. its caller, mwRnaDestroy, then forgets a port that
 * stays open with its buffers. in the shipped game no handler meets a busy
 * block. none feeds a port while the mode is CPU. game_init turns ADXT's
 * vsync server off with ADXT_ConfigVsyncSvr(0). movie_vsync's server feeds
 * only the ports movie_main opens after it sets DMA. mwSndTrns sets the mode
 * back to CPU only for a copy of under MWSND_TRNS_MIN bytes. mw_rna.c copies
 * 16-bit samples in multiples of 32 bytes. its only smaller copy is 4 bytes
 * past a ring's end, at a rate other than 44.1 kHz. every movie on the disc
 * streams at 44.1 kHz */

/* syG2DmaSetPrm rounds the addresses and the length down to a multiple of
 * MWSND_DMA_UNIT */
#define MWSND_DMA_UNIT 32
/* one copy for each channel of each port: mwSndTrns ends a block's copy
 * before it starts the next */
#define MWSND_DMA_MAX (MWSND_MAX_OBJ * 2)
/* the interrupt levels running, which syIntMasterISR_2 counts */
extern Sint32 syIntNestCount;
/* SB_ISTNRM, the system bus's normal interrupt status, is sg_syg2.h's
 * G2_INT_STATUS. MWSND_INT_AICA_DMA_END is its bit for the end of the AICA's
 * G2 DMA, which syG2DmaEndIntrruptHandler clears for that channel */
#define MWSND_INT_AICA_DMA_END                                                 \
    (1UL << (SYE_G2_DMA_TYPE_AICA + G2_INT_DMA_END_SHIFT))

/* a copy that runs on the channel or waits for it */
struct MwSndDma {
    SDMEMBLK mblk;
    void *src;
    Sint32 dst; /* the offset in sound memory */
    Sint32 size;
};

/* the copies, in order, as a ring of mwg_snd_dma_num from mwg_snd_dma_first.
 * the first runs on the channel, and the rest wait for it. the ring changes
 * only with every interrupt masked */
static struct MwSndDma mwg_snd_dma[MWSND_DMA_MAX];
static Sint32 mwg_snd_dma_first;
static Sint32 mwg_snd_dma_num;

static void mwlSndDmaEnd(void *arg);

/* start the ring's first copy on the free channel, with the flags
 * sdMemBlkDmaTransfer gives its transfers. syG2DmaTrigger writes the operand
 * cache back over the source first */
static void mwlSndDmaStart(void)
{
    struct MwSndDma *dma = &mwg_snd_dma[mwg_snd_dma_first];

    syG2DmaSetPrm(sdgG2DmaHandle, (void *)(SD_SND_MEM + dma->dst), dma->src,
                  dma->size, SYE_G2_DMA_DIR_ROOT_TO_G2, true, true,
                  mwlSndDmaEnd, NULL);
    /* syG2DmaTrigger does nothing while the busy bit of SB_ADST, the
     * channel's start register, reads set. mwlSndDmaWait starts the next copy
     * once the end bit is set, without reading the busy bit */
    while (syG2DmaTrigger(sdgG2DmaHandle, SYE_G2_DMA_TRIGGER_CPU) !=
           SYE_G2_ERR_NOTHING) {
    }
}

/* the channel's callback, which syG2DmaEndIntrruptHandler calls at the end of
 * the ring's first copy */
static void mwlSndDmaEnd(void *arg)
{
    int mask = get_imask();

    set_imask(IMASK_ALL);
    sdMemBlkSetStat(mwg_snd_dma[mwg_snd_dma_first].mblk,
                    SDE_MEMBLK_STAT_TRANSFER_FINISHED);
    mwg_snd_dma_first = (mwg_snd_dma_first + 1) % MWSND_DMA_MAX;
    mwg_snd_dma_num--;
    if (mwg_snd_dma_num > 0) {
        mwlSndDmaStart();
    }
    set_imask(mask);
}

/* take a copy mwSndTrns has set up in mblk for the channel. the result is
 * TRUE for a copy taken. its block is then in progress, and the copy has
 * started or waits in the ring. the result is FALSE, with nothing done, for a
 * copy that keeps SEGA's transfer. a taken copy sets its block's status and
 * skips the rest of sdMemBlkTransfer's work. mwSndTrns passes
 * SDD_MEMBLK_NO_FUNC, so no callback or done flag waits on the block. nothing
 * reads Shinobi's transfer counts. only a transfer in DMA mode fills
 * Shinobi's queue, which is empty wherever a copy is taken */
static Bool mwlSndDmaTake(SDMEMBLK mblk, Sint32 dst, void *src, Sint32 size)
{
    struct MwSndDma *dma;
    int mask;

    if (size <= 0 || ((Uint32)src | dst | size) % MWSND_DMA_UNIT != 0 ||
        sdgWrkPtr->transfer_mode != SDE_MEMBLK_TRANSFER_MODE_CPU ||
        syIntNestCount != 0) {
        return false;
    }
    mask = get_imask();
    set_imask(IMASK_ALL);
    /* with the ring empty, a busy channel holds Shinobi's DMA. that DMA
     * runs, or has ended with its end handler yet to run. the busy flag,
     * unlike SB_ADST, stays set until that handler runs. a copy started
     * before then would replace the channel's callback with mwlSndDmaEnd.
     * the pending end would then cut this copy short and mark it finished */
    if (mwg_snd_dma_num == MWSND_DMA_MAX ||
        (mwg_snd_dma_num == 0 &&
         ((struct g2_dma_channel *)sdgG2DmaHandle)->busy == true)) {
        set_imask(mask);
        return false;
    }
    dma = &mwg_snd_dma[(mwg_snd_dma_first + mwg_snd_dma_num) % MWSND_DMA_MAX];
    dma->mblk = mblk;
    dma->src = src;
    dma->dst = dst;
    dma->size = size;
    sdMemBlkSetStat(mblk, SDE_MEMBLK_STAT_TRANSFER_PROGRESS);
    mwg_snd_dma_num++;
    if (mwg_snd_dma_num == 1) {
        mwlSndDmaStart();
    }
    set_imask(mask);
    return true;
}

/* wait for the end of mblk's copy, if the ring holds it. the result is TRUE
 * once the ring no longer holds it. it is FALSE, without a wait, where a
 * handler asks while the ring holds the copy. each pass, with every interrupt
 * masked, waits for the end bit, not SB_ADST's busy bit. it then runs the
 * channel's end handler, which clears the end bit. a pass must clear the end
 * its copy raised. otherwise the interrupt would run the end handler again
 * and end the next copy early */
static Bool mwlSndDmaWait(SDMEMBLK mblk)
{
    SDE_MEMBLK_STAT stat;
    int mask;

    /* a block in progress with the ring empty is in Shinobi's own DMA. SEGA's
     * code waits for that DMA's end as shipped. this test keeps short each
     * pass of mwRnaSetTransSw's masked wait for it */
    if (mwg_snd_dma_num == 0) {
        return true;
    }
    mask = get_imask();
    set_imask(IMASK_ALL);
    sdMemBlkGetStat(mblk, &stat);
    while (mwg_snd_dma_num > 0 && stat == SDE_MEMBLK_STAT_TRANSFER_PROGRESS) {
        if (syIntNestCount != 0) {
            set_imask(mask);
            return false;
        }
        while ((*G2_INT_STATUS & MWSND_INT_AICA_DMA_END) == 0) {
        }
        syG2DmaEndIntrruptHandler(sdgG2DmaHandle);
        sdMemBlkGetStat(mblk, &stat);
    }
    set_imask(mask);
    return true;
}
#endif

void mwSndInit(void)
{
    memset(mwg_snd_obj, 0, sizeof(mwg_snd_obj));
    mwSndBufInit();
}

static MwSnd *mwlSndAlloc(void)
{
    MwSnd *snd;
    int i;

    snd = 0;
    for (i = 0; i < MWSND_MAX_OBJ; i++) {
        if (mwg_snd_obj[i].used == 0) {
            snd = &mwg_snd_obj[i];
            break;
        }
    }
    return snd;
}

MwSnd *mwSndOpenPort(int nch)
{
    SDPSTM port;
    SDMEMBLK mblk;
    int bufno[2];
    MwSnd *snd;
    int err;
    int i;
    int j;
    Sint8 usech;
    int adr;
    int no;
    int *p;

    if (nch < 1 || nch > 2) {
        mwRnaErrCallFunc(TEXT_1);
        return 0;
    }
    snd = mwlSndAlloc();
    if (snd == 0) {
        mwRnaErrCallFunc(TEXT_2);
        return 0;
    }
    for (i = 0; i < nch; i++) {
        snd->buf[i] = mwSndBufCreate();
        if (snd->buf[i] == 0) {
            mwRnaErrCallFunc(TEXT_3);
            for (j = 0; j < i; j++) {
                mwSndBufDestroy(snd->buf[j]);
            }
            return 0;
        }
        bufno[i] = mwSndBufGetNo(snd->buf[i]);
    }
    /* @hack spelling: bufno read through p, a pointer stepped along it, which
     * SHC sets by copying bufno's address from R9 (MOV R9,R2 at 0x8c676286).
     * indexed as bufno[i], SHC adds R9 to 0 instead and 97 of 268 words
     * differ */
    /* @hack spelling: p set to &bufno[0], not to bufno, which keeps that copy.
     * set to bufno, SHC stores R9 itself, the function is 4 bytes shorter and
     * 182 of 268 words differ */
    for (i = 0, p = &bufno[0]; i < nch; i++) {
        /* @hack spelling: no assigned inside the subtraction, which has SHC
         * negate it and add 16 (NEG R5,R6 and ADD #16,R6 at 0x8c67629e and
         * 0x8c6762a0). assigned in a statement of its own, SHC subtracts it
         * from 16, the function is 4 bytes shorter and 182 of 268 words
         * differ */
        adr = MWSND_SND_MEM_SIZE -
              ((MWSND_NBANK - (no = *p++)) * MWSND_BANK_SIZE);
        err = sdSndMemAllocateBank(SDE_DATA_TYPE_PSTM_RING_BUF, no, adr,
                                   MWSND_RING_SIZE);
        if (err != 0) {
            mwRnaErrCallFunc(TEXT_4);
        }
    }
    for (i = 0; i < nch; i++) {
        err = sdMemBlkCreate(&mblk);
        if (err != 0) {
            mwRnaErrCallFunc(TEXT_5);
            for (j = 0; j < i; j++) {
                sdMemBlkDestroy(snd->mblk[j]);
                /* @bug logic: err is still sdMemBlkCreate's error, and
                 * nothing keeps sdMemBlkDestroy's result (TST R2,R2 at
                 * 0x8c6762f4 reads the error saved at 0x8c6762da). the test
                 * always holds, and each destroy reports TEXT_6, the message
                 * for a failed sdMemBlkDestroy */
                if (err != 0) {
                    mwRnaErrCallFunc(TEXT_6);
                }
            }
            for (j = 0; j < nch; j++) {
                mwSndBufDestroy(snd->buf[j]);
            }
            return 0;
        }
        snd->mblk[i] = mblk;
    }

    /* @hack spelling: usech set in the test, which sets the call's R5 in the
     * branch's slot (BF/S at 0x8c676344, MOV R12,R5 at 0x8c676346). set
     * before the test, SHC copies it first and 7 of 268 words differ */
    if (usech = nch, nch == 1) {
        err = sdPstmOpenPort(&port, usech, (Sint8)bufno[0]);
    } else {
        err = sdPstmOpenPort(&port, usech, (Sint8)bufno[0], (Sint8)bufno[1]);
    }
    if (err != 0) {
        mwRnaErrCallFunc(TEXT_7);
        for (j = 0; j < nch; j++) {
            sdMemBlkDestroy(snd->mblk[j]);
            /* @bug logic: err is still sdPstmOpenPort's error, as in the
             * loop above (TST R10,R10 at 0x8c6763c0). the test always holds,
             * and each destroy reports TEXT_6 */
            if (err != 0) {
                mwRnaErrCallFunc(TEXT_6);
            }
            /* @bug memory: the loop frees buf[i], where i is nch, not
             * buf[j] (MOV.L @(4,R15),R3 at 0x8c6763cc, which holds nch * 4).
             * with one channel it frees buf[1], which this open never set, and
             * with two, mblk[0], past buf's end. the port's own sound buffers
             * stay taken */
            mwSndBufDestroy(snd->buf[i]);
        }
        return 0;
    }
    snd->port = port;
    /* @hack spelling: both counts stored in one chained statement, which keeps
     * nch in R12 and mwRnaErrCallFunc's address in R13 (MOV R4,R12 at
     * 0x8c6761ce, MOV.L R12,@(8,R14) at 0x8c6763ea). as two statements, the
     * two trade and 36 of 268 words differ */
    snd->nch = snd->usech = nch;
    err = sdPstmResetPrm(snd->port);
    if (err != 0) {
        mwRnaErrCallFunc(TEXT_8);
    }
    err = sdPstmSetBaseVol(snd->port, 127);
    if (err != 0) {
        mwRnaErrCallFunc(TEXT_9);
    }
    snd->used = 1;
    return snd;
}

int mwSndClosePort(MwSnd *snd)
{
    int err;
    int i;

    if (snd == 0) {
        return SDE_ERR_HANDLE_ILLEGAL_VALUE;
    }
#ifdef NONMATCHING
    for (i = 0; i < snd->nch; i++) {
        if (!mwlSndDmaWait(snd->mblk[i])) {
            return SDE_ERR_MEMBLK_ALREADY_TRANSFER;
        }
    }
#endif
    for (i = 0; i < snd->nch; i++) {
        mwSndBufDestroy(snd->buf[i]);
        err = sdMemBlkDestroy(snd->mblk[i]);
        if (err != 0) {
            mwRnaErrCallFunc(TEXT_10);
        }
    }
    err = sdPstmResetPrm(snd->port);
    if (err != 0) {
        mwRnaErrCallFunc(TEXT_11);
    }
    err = sdPstmClosePort(snd->port);
    if (err != 0) {
        mwRnaErrCallFunc(TEXT_12);
    }
    snd->nch = 0;
    snd->usech = 0;
    memset(snd, 0, sizeof(*snd));
    snd->used = 0;
    return 0;
}

int mwSndSetHdInfo(MwSnd *snd)
{
    return 0;
}

int mwSndPlay(MwSnd *snd, int bps, Uint16 freq)
{
    int type;
    int err;

#ifdef NONMATCHING
    /* repairs the unset type the tag below names. for a bps no format has,
     * this side passes -1. sdPstmPlay rejects it as out of range, as it
     * rejects the rna's address the image passes */
    type = -1;
#endif
    /* NOLINTBEGIN(clang-diagnostic-sometimes-uninitialized) */
    if (bps == 16) {
        type = SDE_PCM_TYPE_16BIT_PCM;
    } else if (bps == 8) {
        type = SDE_PCM_TYPE_8BIT_PCM;
    } else if (bps == 4) {
        type = SDE_PCM_TYPE_4BIT_ADPCM;
    }
    /* NOLINTEND(clang-diagnostic-sometimes-uninitialized) */
    snd->bps = bps;
    /* @bug uninitialized: a bps other than 16, 8 and 4 sets no type (BF at
     * 0x8c6764f4). sdPstmPlayMw then gets R14 as the caller left it (MOV
     * R14,R5 at 0x8c676502). the one caller, mwRnaSetPlaySw, holds its rna
     * there (0x8c66f62a), so the pcm type is the rna's address */
    err = sdPstmPlayMw(snd->port, type, freq, 0);
    if (err != 0) {
        mwRnaErrCallFunc(TEXT_13);
    }
    return err;
}

int mwSndStop(MwSnd *snd)
{
    int err;

    err = sdPstmStop(snd->port);
    if (err != 0) {
        mwRnaErrCallFunc(TEXT_14);
    }
    return err;
}

int mwSndStopAll(void)
{
    int err;

    err = sdPstmStopAll();
    if (err != 0) {
        mwRnaErrCallFunc(TEXT_15);
    }
    return err;
}

int mwSndGetSfreq(MwSnd *snd, int *sfreq)
{
    *sfreq = 0;
    return 0;
}

int mwSndGetBitPerSmpl(MwSnd *snd, int *bps)
{
    *bps = 0;
    return 0;
}

int mwSndGetVol(MwSnd *snd, int *vol)
{
    SDS_PSTM_STAT stat;
    int err;

    err = sdPstmGetStat(snd->port, 0, &stat);
    /* @bug uninitialized: a failed sdPstmGetStat writes no part of stat, and
     * *vol is still computed from stat.m_Vol (MOV.B @(10,R15),R0 at
     * 0x8c676596) */
    *vol = stat.m_Vol * 2;
    return err;
}

int mwSndGetPan(MwSnd *snd, Sint8 ch, int *pan)
{
    SDS_PSTM_STAT stat;
    int err;

    err = sdPstmGetStat(snd->port, ch, &stat);
    /* @bug uninitialized: a failed sdPstmGetStat writes no part of stat, and
     * *pan is still computed from stat.m_Pan (MOV.B @(11,R15),R0 at
     * 0x8c6765bc) */
    *pan = stat.m_Pan * 15 / 127;
    return err;
}

int mwSndGetPlayPos(MwSnd *snd, Sint8 ch, int *pos)
{
    Uint32 flag = 1;
    Sint32 adr;
    int err;

    sdPstmGetFlg(snd->port, 0, &flag);
    if (flag & 1) {
        err = sdPstmGetCurAdr(snd->port, ch, &adr);
        if (err != 0) {
            mwRnaErrCallFunc(TEXT_16);
        }
        /* @bug uninitialized: a failed sdPstmGetCurAdr leaves adr unwritten,
         * and *pos is still computed from it (MOV.L @(4,R15),R1 at
         * 0x8c67667c) */
        *pos = adr * 8 / snd->bps;
    } else {
        *pos = 0;
        err = 0;
    }
    return err;
}

int mwSndSetVol(MwSnd *snd, int vol)
{
    int att;
    int err;

    if (vol >= 255) {
        att = -127;
    } else if (vol <= 0) {
        att = 127;
    } else {
        att = -(vol * 10 / 18);
        if (att < -127) {
            att = -127;
        }
    }
    err = sdPstmSetVol(snd->port, 0, att, 0);
    if (err != 0) {
        mwRnaErrCallFunc(TEXT_17);
        return err;
    }
    err = sdPstmSetVol(snd->port, 1, att, 0);
    if (err != 0) {
        mwRnaErrCallFunc(TEXT_17);
    }
    return err;
}

int mwSndSetDrctLev(MwSnd *snd, Sint8 lev)
{
    int err;

    err = sdPstmSetDrctLev(snd->port, lev);
    if (err != 0) {
        mwRnaErrCallFunc(TEXT_18);
    }
    return err;
}

int mwSndSetFx(MwSnd *snd, Sint8 ch, Sint8 lev)
{
    Sint8 level;
    int err;

    if (lev <= 0) {
        level = 0;
    } else if (lev >= 15) {
        level = 127;
    } else {
        /* @hack spelling: the doubling written as lev + lev, which keeps lev
         * in R14 for the sum (ADD R14,R1 at 0x8c676792). as 2 * lev, SHC
         * extends lev again and shifts it, and 43 of 47 words differ */
        level = (lev + lev - 15) * 127 / 15;
    }
    err = sdPstmSetFxCh(snd->port, ch, level);
    if (err != 0) {
        mwRnaErrCallFunc(TEXT_18);
    }
    return err;
}

int mwSndSetPan(MwSnd *snd, Sint8 ch, int pan)
{
    Sint8 p;
    int err;

    p = pan * 127 / 15;
    err = sdPstmSetPan(snd->port, ch, p, 0);
    if (err != 0) {
        mwRnaErrCallFunc(TEXT_19);
    }
    return err;
}

int mwSndSetPitch(MwSnd *snd, Sint16 semi, Sint16 fine)
{
    Sint32 cents;
    int err;

    if (semi <= -100) {
        /* @hack redundant: -100 written as 1.0F ? -100 : -100, which has
         * semi's clamp and fine's below each load -100 into R3 (MOV #-100,R3
         * at 0x8c676812 and 0x8c676830). a plain -100 has the two share R14,
         * and 41 of 52 words differ */
        semi = 1.0F ? -100 : -100; /* NOLINT(misc-redundant-expression) */
    } else if (semi >= 100) {
        /* @hack redundant: 100 written as 0.0F ? 100 : 100, which loads 100 by
         * MOV #100,R5 (0x8c67682c). a plain 100 is copied from R7, which
         * holds 100 for the test, and 1 of 52 words differs */
        semi = 0.0F ? 100 : 100; /* NOLINT(misc-redundant-expression) */
    } else {
        /* @hack redundant: an empty else, which puts the -100 of fine's clamp
         * in R3 and _divls's address in R2 (0x8c676830, 0x8c676848). without
         * it, the two trade and 4 of 52 words differ */
    }
    /* semi is within -100 to 100 here, so semi * 256 and the cents added
     * below fit a Sint16 */
    semi *= 256;
    if (fine <= -100) {
        cents = -256;
    } else if (fine >= 100) {
        cents = 256;
    } else {
        cents = fine * 256 / 100;
    }
    semi += cents;
    err = sdPstmSetPitch(snd->port, semi, 0);
    if (err != 0) {
        mwRnaErrCallFunc(TEXT_20);
    }
    return err;
}

int mwSndSetUseCh(MwSnd *snd, Sint8 usech)
{
    int err;

    snd->usech = usech; /* NOLINT(bugprone-signed-char-misuse) */
    err = sdPstmSetUseCh(snd->port, usech);
    if (err != 0) {
        mwRnaErrCallFunc(TEXT_21);
    }
    return err;
}

int mwSndSetFilter(MwSnd *snd, Sint16 coff, Sint16 q)
{
    return 0;
}

int mwSndFlushCmd(MwSnd *snd, Sint32 sw)
{
    int err;

    err = sdSysFlushHostCmd();
    if (err != 0) {
        mwRnaErrCallFunc(TEXT_22);
    }
    return err;
}

int mwSndSetTrnsMode(MwSnd *snd, Sint32 mode)
{
    return 0;
}

int mwSndTrns(MwSnd *snd, Sint8 ch, int dst, void *src, int size)
{
    int err;

#ifdef NONMATCHING
    if (!mwlSndDmaWait(snd->mblk[ch])) {
        return SDE_ERR_MEMBLK_ALREADY_TRANSFER;
    }
#endif
    if (size < MWSND_TRNS_MIN) {
        sdMemBlkSetTransferMode(SDE_MEMBLK_TRANSFER_MODE_CPU);
        mwRnaErrCallFunc(TEXT_23);
    }
    mwg_snd_tcnt[0] = syTmrGetCount();
    err = sdMemBlkSetPrm(snd->mblk[ch], src, size, SDD_MEMBLK_NO_FUNC, 0);
    if (err != 0) {
        mwRnaErrCallFunc(TEXT_24);
        return err;
    }
    mwg_snd_tcnt[1] = syTmrGetCount();
#ifdef NONMATCHING
    if (mwlSndDmaTake(snd->mblk[ch], dst, src, size)) {
        err = SDE_ERR_NOTHING;
    } else {
        err = sdMemBlkTransfer(snd->mblk[ch], dst);
    }
#else
    err = sdMemBlkTransfer(snd->mblk[ch], dst);
#endif
    if (err != 0) {
        mwRnaErrCallFunc(TEXT_25);
    }
    mwg_snd_tcnt[2] = syTmrGetCount();
    /* @hack redundant: a test of size that guards nothing, which keeps size in
     * R13 to the end for its CMP/GE R2,R13 (0x8c676990). without it, SHC
     * saves one register fewer, the function is 8 bytes shorter and 76 of 86
     * words differ */
    if (size < MWSND_TRNS_MIN) {
    }
    return err;
}

int mwSndGetMblkStat(MwSnd *snd, Sint8 ch, SDE_MEMBLK_STAT *stat)
{
    SDE_MEMBLK_STAT mstat;
    int err;

#ifdef NONMATCHING
    (void)mwlSndDmaWait(snd->mblk[ch]);
#endif
    err = sdMemBlkGetStat(snd->mblk[ch], &mstat);
    if (err != 0) {
        mwRnaErrCallFunc(TEXT_26);
    }
    /* @bug uninitialized: a failed sdMemBlkGetStat leaves mstat unwritten,
     * and it is still copied to *stat (MOV.L @R15,R2 at 0x8c6769da) */
    *stat = mstat;
    return err;
}

int mwSndGetBufInfo(MwSnd *snd, Sint8 ch, Sint32 *ptr, Sint32 *size)
{
    int no;
    int err;

    no = mwSndBufGetNo(snd->buf[ch]);
    err = sdSndMemGetBankStat(SDE_DATA_TYPE_PSTM_RING_BUF, no, ptr, size);
    if (err != 0) {
        mwRnaErrCallFunc(TEXT_27);
    }
    return err;
}

int mwSndGetFx(MwSnd *snd, int *ch, int *lev)
{
    *ch = 0;
    *lev = 0;
    return 0;
}

int mwSndGetFilter(MwSnd *snd, int *coff, int *q)
{
    *coff = 0;
    *q = 0;
    return 0;
}

/* mwSndGetInitPrm reads no argument and nothing calls it, so its parameters
 * are unknown. it returns 0 */
int mwSndGetInitPrm(void)
{
    return 0;
}

int mwSndGetHdPrm(void *obj, Sint32 arg1, Sint16 arg2)
{
    return 0;
}
