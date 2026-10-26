/* the ADX talk server's work on one stream: a step for each state, the loop's
 * two callbacks, and the error check after each step */
#include <sg_xpt.h>
#include <sj.h>
#include <sofdec.h>

#define ADXT_SCT_SIZE 2048
/* an end sector no disc stream reaches */
#define ADXT_NO_EOS 0x7fffffff

/* the decoder's trap, which runs once decoding reaches the loop's end. it drops
 * the lp_skiplen bytes that follow the loop's end in the input. it then sets
 * the decoder to decode the loop again and to trap at its end. a memory
 * stream's input starts again at the loop's start */
void adxt_trap_entry(void *obj)
{
    ADXT adxt = (ADXT)obj;
    ADXSJD sjd = adxt->sjd;
    SJ sji = adxt->sji;
    Sint32 lpspos;
    Sint32 lpsofst;
    Sint32 lpepos;
    SJCK ck;

    lpspos = ADXSJD_GetLpStartPos(sjd);
    lpsofst = ADXSJD_GetLpStartOfst(sjd);
    lpepos = ADXSJD_GetLpEndPos(sjd);
    ADXSJD_GetLpEndOfst(sjd);
    sji->vtbl->GetChunk(sji, SJ_LIN_DATA, adxt->lp_skiplen, &ck);
    if (ck.len < adxt->lp_skiplen) {
        ADXERR_CallErrFunc("E8101201: not enough data (adxt_trap_entry)\n");
    }
    sji->vtbl->PutChunk(sji, SJ_LIN_FREE, &ck);
    ADXSJD_SetTrapCnt(sjd, 0);
    ADXSJD_SetTrapNumSmpl(sjd, lpepos - lpspos);
    ADXSJD_SetTrapDtLen(sjd, lpsofst);
    ADXSJD_SetDecPos(sjd, lpspos);
    if (adxt->pmode == ADXT_PMODE_MEM) {
        sji->vtbl->Reset(sji);
        sji->vtbl->GetChunk(sji, SJ_LIN_DATA, lpsofst, &ck);
        sji->vtbl->PutChunk(sji, SJ_LIN_FREE, &ck);
    }
    adxt->lpcnt++;
}

/* a disc stream's end callback, which runs once its reads reach the end of the
 * sector the loop ends in. it seeks back to the sector that holds the loop's
 * start */
void adxt_eos_entry(void *obj)
{
    ADXT adxt = (ADXT)obj;
    ADXSTM stm = adxt->stm;
    ADXSJD sjd = adxt->sjd;
    Sint32 lpsofst;

    lpsofst = ADXSJD_GetLpStartOfst(sjd);
    ADXSTM_Seek(stm, lpsofst / ADXT_SCT_SIZE);
}

void adxt_set_outpan(ADXT adxt)
{
    Sint32 nch = ADXSJD_GetNumChan(adxt->sjd);

    if (nch == 1) {
        if (adxt->outpan[ADXT_CH_L] == ADXT_PAN_AUTO) {
            ADXRNA_SetOutPan(adxt->rna, ADXT_CH_L, ADXT_PAN_CENTER);
        } else {
            ADXRNA_SetOutPan(adxt->rna, ADXT_CH_L, adxt->outpan[ADXT_CH_L]);
        }
    } else {
        if (adxt->outpan[ADXT_CH_L] == ADXT_PAN_AUTO) {
            ADXRNA_SetOutPan(adxt->rna, ADXT_CH_L, ADXT_PAN_LEFT);
        } else {
            ADXRNA_SetOutPan(adxt->rna, ADXT_CH_L, adxt->outpan[ADXT_CH_L]);
        }
        if (adxt->outpan[ADXT_CH_R] == ADXT_PAN_AUTO) {
            ADXRNA_SetOutPan(adxt->rna, ADXT_CH_R, ADXT_PAN_RIGHT);
        } else {
            ADXRNA_SetOutPan(adxt->rna, ADXT_CH_R, adxt->outpan[ADXT_CH_R]);
        }
    }
}

/* set the stream up once the decoder has read its header. a step decodes at
 * most the samples that play in 3 server calls, or in 1.5 without a loop. the
 * limit rises to the next multiple of two blocks' samples above it. with a
 * loop, the decoder traps at the loop's end. a disc stream's reads then stop
 * at the end of the sector the loop ends in, and without a loop, at the
 * stream's end. the sound output then takes the stream's format */
void adxt_stat_decinfo(ADXT adxt)
{
    ADXSJD sjd = adxt->sjd;
    /* @hack spelling: the sector size through sctsize, a local, which loads
     * it into R12 once (MOV.W at 0x8c66e77c) for every use. with
     * ADXT_SCT_SIZE in each use, the function is 8 bytes shorter and 205 of
     * 211 words differ */
    Sint32 sctsize = ADXT_SCT_SIZE;
    Sint32 stat;
    Sint32 sfreq;
    Sint32 nloop;
    Sint32 blksmpl;
    Sint32 lpeofst;
    Sint32 nch;
    Sint32 total;
    /* @hack spelling: eossct, bps and loopend through locals, which keeps
     * lpeofst in R4, and sfreq, nch and total at @(4,R15), @R15 and @(8,R15)
     * (0x8c66e832, 0x8c66e8b2 to 0x8c66e8c2). with each call's result passed
     * straight on, lpeofst takes R11, sfreq and total trade slots and 6 of 211
     * words differ. with any one passed straight on, 6 to 8 differ */
    Sint32 eossct;
    Sint32 bps;
    Sint32 loopend;

    stat = ADXSJD_GetStat(sjd);
    if (stat != ADXSJD_STAT_DECODE) {
        return;
    }
    sfreq = ADXSJD_GetSfreq(sjd);
    nloop = ADXSJD_GetNumLoop(sjd);
    if (nloop > 0) {
        adxt->maxdecsmpl = sfreq / adxt->svrfreq * 3;
    } else {
        adxt->maxdecsmpl = sfreq / adxt->svrfreq * 3 / 2;
    }
    blksmpl = ADXSJD_GetBlkSmpl(sjd) * 2;
    adxt->maxdecsmpl = (adxt->maxdecsmpl + blksmpl) / blksmpl * blksmpl;
    ADXSJD_SetMaxDecSmpl(sjd, adxt->maxdecsmpl);
    if (nloop > 0) {
        if (adxt->pmode == ADXT_PMODE_MEM) {
            adxt->lp_skiplen = 0;
        } else {
            lpeofst = ADXSJD_GetLpEndOfst(sjd);
            /* the bytes from the loop's end to the end of its sector, which
             * the reads bring in and the trap drops */
            adxt->lp_skiplen = sctsize - (lpeofst % sctsize);
            adxt->lp_skiplen %= sctsize;
            eossct = (lpeofst + sctsize - 1) / sctsize;
            ADXSTM_SetEos(adxt->stm, eossct);
            ADXSTM_EntryEosFunc(adxt->stm, adxt_eos_entry, adxt);
        }
        loopend = ADXSJD_GetLpEndPos(sjd);
        ADXSJD_SetTrapNumSmpl(sjd, loopend);
        ADXSJD_SetTrapDtLen(sjd, 0);
        ADXSJD_SetTrapCnt(sjd, 0);
        ADXSJD_EntryTrapFunc(sjd, adxt_trap_entry, adxt);
    } else {
        if (adxt->stm != NULL) {
            ADXSTM_SetEos(adxt->stm, ADXT_NO_EOS);
        }
    }
    sfreq = ADXSJD_GetSfreq(sjd);
    nch = ADXSJD_GetNumChan(sjd);
    total = ADXSJD_GetTotalNumSmpl(sjd);
    bps = ADXSJD_GetOutBps(sjd);
    ADXRNA_SetBitPerSmpl(adxt->rna, bps);
    ADXRNA_SetSfreq(adxt->rna, sfreq);
    ADXRNA_SetNumChan(adxt->rna, nch);
    ADXRNA_SetTotalNumSmpl(adxt->rna, total);
    ADXRNA_SetOutVol(adxt->rna, adxt->outvol);
    adxt_set_outpan(adxt);
    if (ADXSJD_GetFormat(sjd) == ADXB_FORMAT_SPSD) {
        ADXRNA_SetStmHdInfo(adxt->rna, ADXSJD_GetSpsdInfo(sjd));
    }
    ADXRNA_SetTransSw(adxt->rna, 1);
    adxt->stat = ADXT_STAT_PREP;
}

/* start play, unless pstwait_flag leaves that to the caller, and set
 * pstready_flag. both happen on the first pass, since ADXRNA_GetNumRoom's 0
 * always passes the room test */
void adxt_stat_prep(ADXT adxt)
{
    ADXSJD sjd = adxt->sjd;
    ADXRNA rna = adxt->rna;
    Sint32 ndata;
    Sint32 nroom;

    ndata = ADXRNA_GetNumData(rna);
    nroom = ADXRNA_GetNumRoom(rna);
    /* @bug logic: the test means to wait until the sound output holds a
     * step's samples or has no room for another block. ADXRNA_GetNumRoom
     * returns 0 (0x8c66d83e), so the room test always holds (CMP/GT at
     * 0x8c66e95e) */
    if (ndata >= adxt->maxdecsmpl || nroom <= ADXSJD_GetBlkSmpl(sjd)) {
        if (adxt->pstwait_flag == 0) {
            ADXRNA_SetPlaySw(rna, 1);
            adxt->stat = ADXT_STAT_PLAYING;
        }
        adxt->pstready_flag = 1;
    }
}

/* end the stream's decoding once the decoder is done and no output joint
 * holds data */
void adxt_stat_playing(ADXT adxt)
{
    Sint32 i;
    Sint32 nch;

    if (ADXSJD_GetStat(adxt->sjd) == ADXSJD_STAT_DECEND) {
        nch = ADXSJD_GetNumChan(adxt->sjd);
        for (i = 0; i < nch; i++) {
            if (adxt->sjo[i]->vtbl->GetNumData(adxt->sjo[i], SJ_LIN_DATA) > 0) {
                break;
            }
        }
        if (i == nch) {
            ADXRNA_SetTransSw(adxt->rna, 0);
            adxt->stat = ADXT_STAT_DECEND;
        }
    }
}

void adxt_stat_decend(ADXT adxt)
{
    if (ADXRNA_GetNumData(adxt->rna) <= 0) {
        ADXRNA_SetPlaySw(adxt->rna, 0);
        adxt->stat = ADXT_STAT_PLAYEND;
    }
}

void adxt_stat_playend(ADXT adxt)
{
}

/* replay the stream: stop its output and decoding, empty the output joints,
 * and decode again from the input. a disc stream's input is emptied and read
 * again from its first sector. any other input goes on from where it stands */
void adxt_RcvrReplay(ADXT adxt)
{
    Sint32 i;

    ADXCRS_Lock();
    ADXRNA_SetTransSw(adxt->rna, 0);
    ADXRNA_SetPlaySw(adxt->rna, 0);
    ADXSJD_Stop(adxt->sjd);
    if (adxt->stm != NULL) {
        ADXSTM_Stop(adxt->stm);
        adxt->sji->vtbl->Reset(adxt->sji);
    }
    for (i = 0; i < adxt->maxnch; i++) {
        adxt->sjo[i]->vtbl->Reset(adxt->sjo[i]);
    }
    if (adxt->stm != NULL) {
        ADXSTM_Seek(adxt->stm, 0);
        ADXSTM_Start(adxt->stm);
    }
    adxt_start_sj(adxt, adxt->sji);
    ADXCRS_Unlock();
}

/* check the stream after a step, and recover from an error as autorcvr says.
 * after the recovery, any mode but ADXT_RMODE_NOACT clears the error */
void ADXT_ExecErrChk(ADXT adxt)
{
    /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
    Sint32 stat = adxt->stat;
    Sint32 pos;
    Sint32 ndata;

    /* while the stream plays, a decoded count that stays the same for more
     * than svrfreq checks, a second's worth, is ADXT_ERR_SNDBLK */
    if (stat == ADXT_STAT_PLAYING) {
        pos = ADXSJD_GetDecNumSmpl(adxt->sjd);
        if (adxt->edecpos == pos) {
            adxt->edeccnt++;
            if (adxt->edeccnt > adxt->svrfreq) {
                adxt->ercode = ADXT_ERR_SNDBLK;
            }
        } else {
            adxt->edeccnt = 0;
        }
        adxt->edecpos = pos;
        if (adxt->ercode != ADXT_ERR_OK) {
            /* ADXT_RMODE_REPLAY stops a stuck stream too, and doesn't replay
             * it */
            if (adxt->autorcvr == ADXT_RMODE_STOP ||
                adxt->autorcvr == ADXT_RMODE_REPLAY) {
                ADXT_Stop(adxt);
            }
            if (adxt->autorcvr != ADXT_RMODE_NOACT) {
                adxt->ercode = ADXT_ERR_OK;
                adxt->eshrtcnt = 0;
            }
        }
    } else {
        adxt->edeccnt = 0;
    }
    /* from the header's read through play, input under ADXT_MIN_BUFDATA bytes
     * for more than svrfreq checks is ADXT_ERR_SHRTBUF. before play, it takes
     * ADXT_EWAIT_NOTPLY times as many checks */
    if (stat >= ADXT_STAT_DECINFO && stat <= ADXT_STAT_PLAYING) {
        if (adxt->sji != NULL) {
            ndata = adxt->sji->vtbl->GetNumData(adxt->sji, SJ_LIN_DATA);
        } else {
            /* @hack spelling: 0 as stat * 0, which SHC takes from R13, where
             * it holds 0 for the stores above (MOV R13,R4 at 0x8c66ebce). as a
             * plain 0, SHC writes MOV #0,R4 and 1 of 170 words differ */
            ndata = adxt->stat * 0;
        }
        if (ndata < ADXT_MIN_BUFDATA) {
            adxt->eshrtcnt++;
            if (stat == ADXT_STAT_PLAYING) {
                if (adxt->eshrtcnt > adxt->svrfreq) {
                    adxt->ercode = ADXT_ERR_SHRTBUF;
                }
            } else {
                if (adxt->eshrtcnt > adxt->svrfreq * ADXT_EWAIT_NOTPLY) {
                    adxt->ercode = ADXT_ERR_SHRTBUF;
                }
            }
            if (adxt->ercode != ADXT_ERR_OK) {
                if (adxt->autorcvr == ADXT_RMODE_STOP) {
                    ADXT_Stop(adxt);
                } else if (adxt->autorcvr == ADXT_RMODE_REPLAY) {
                    adxt_RcvrReplay(adxt);
                }
                if (adxt->autorcvr != ADXT_RMODE_NOACT) {
                    adxt->ercode = ADXT_ERR_OK;
                    adxt->eshrtcnt = 0;
                }
            }
        } else {
            adxt->eshrtcnt = 0;
        }
    } else {
        adxt->eshrtcnt = 0;
    }
    /* a disc stream's read error takes the recovery autorcvr sets too */
    if (adxt->stm != NULL) {
        if (ADXSTM_GetStat(adxt->stm) == ADXSTM_STAT_ERROR) {
            if (adxt->autorcvr == ADXT_RMODE_STOP) {
                ADXT_Stop(adxt);
            } else if (adxt->autorcvr == ADXT_RMODE_REPLAY) {
                adxt_RcvrReplay(adxt);
            }
            if (adxt->autorcvr != ADXT_RMODE_NOACT) {
                adxt->ercode = ADXT_ERR_OK;
                adxt->eshrtcnt = 0;
            }
        }
    }
}

void ADXT_ExecHndl(ADXT adxt)
{
    if (adxt->stat == ADXT_STAT_PLAYING) {
        adxt_stat_playing(adxt);
    } else if (adxt->stat == ADXT_STAT_DECINFO) {
        adxt_stat_decinfo(adxt);
    } else if (adxt->stat == ADXT_STAT_PREP) {
        adxt_stat_prep(adxt);
    } else if (adxt->stat == ADXT_STAT_DECEND) {
        adxt_stat_decend(adxt);
    } else if (adxt->stat == ADXT_STAT_PLAYEND) {
        adxt_stat_playend(adxt);
    }
    ADXT_ExecErrChk(adxt);
}
