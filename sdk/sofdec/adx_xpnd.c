/* ADX expanders, each of which decodes the blocks an entry gives it in one
 * ADXPD_ExecHndl call */
#include <sg_xpt.h>
#include <sofdec.h>
#include <string.h>

#define ADXPD_MAX_OBJ 8
/* a new expander's filter: its cutoff, and the sample rate it assumes, in
 * hertz */
#define ADXPD_DEF_CUTOFF 500
#define ADXPD_DEF_SFREQ 44100

typedef struct adxpd_obj {
    Sint32 used;
    Sint32 no;
    Sint32 mode;
    Sint32 stat;
    Sint32 nblk; /* the blocks the last ADXPD_ExecHndl decoded */
    Sint32 nch;
    Sint8 *ibuf;
    Sint32 req_nblk; /* the blocks the entry gives */
    Sint16 *obufl;
    Sint16 *obufr;
    /* each channel's last two samples, newest first */
    Sint16 delay[ADXT_MAX_NCH][2];
    Sint16 k[2]; /* the filter's k0 and k1, from ADX_GetCoefficient */
} ADXPD_OBJ;

/* 1 once a stereo decode has returned an odd block count */
Sint32 adxpd_internal_error = 0;
ADXPD_OBJ adxpd_obj[ADXPD_MAX_OBJ];

void ADXPD_Init(void)
{
    memset(adxpd_obj, 0, sizeof(adxpd_obj));
}

ADXPD ADXPD_Create(void)
{
    Sint32 i;
    ADXPD xpd;

    for (i = 0; i < ADXPD_MAX_OBJ; i++) {
        if (adxpd_obj[i].used == 0) {
            break;
        }
    }
    if (i == ADXPD_MAX_OBJ) {
        return NULL;
    }
    xpd = &adxpd_obj[i];
    memset(xpd, 0, sizeof(ADXPD_OBJ));
    xpd->used = 1;
    xpd->no = i;
    xpd->mode = 0;
    xpd->stat = ADXPD_STAT_STOP;
    ADX_GetCoefficient(ADXPD_DEF_CUTOFF, ADXPD_DEF_SFREQ, &xpd->k[0],
                       &xpd->k[1]);
    memset(xpd->delay, 0, sizeof(xpd->delay));
    return xpd;
}

void ADXPD_SetCoef(ADXPD xpd, Sint32 sfreq, Sint32 cutoff)
{
    ADX_GetCoefficient(cutoff, sfreq, &xpd->k[0], &xpd->k[1]);
}

void ADXPD_Destroy(ADXPD xpd)
{
    if (xpd != NULL) {
        xpd->used = 0;
        memset(xpd, 0, sizeof(ADXPD_OBJ));
    }
}

void ADXPD_SetMode(ADXPD xpd, Sint32 mode)
{
    xpd->mode = mode;
}

Sint32 ADXPD_GetStat(ADXPD xpd)
{
    return xpd->stat;
}

Sint32 ADXPD_EntryMono(ADXPD xpd, Sint8 *ibuf, Sint32 nblk, Sint16 *obuf)
{
    if (xpd->stat == ADXPD_STAT_STOP) {
        xpd->nch = 1;
        xpd->ibuf = ibuf;
        xpd->req_nblk = nblk;
        xpd->obufl = obuf;
        xpd->obufr = NULL;
        return 1;
    }
    return 0;
}

Sint32 ADXPD_EntrySte(ADXPD xpd, Sint8 *ibuf, Sint32 nblk, Sint16 *obufl,
                      Sint16 *obufr)
{
    if (xpd->stat == ADXPD_STAT_STOP) {
        xpd->nch = 2;
        xpd->ibuf = ibuf;
        xpd->req_nblk = nblk;
        xpd->obufl = obufl;
        xpd->obufr = obufr;
        return 1;
    }
    return 0;
}

void ADXPD_Start(ADXPD xpd)
{
    if (xpd->stat == ADXPD_STAT_STOP) {
        xpd->nblk = 0;
        xpd->stat = ADXPD_STAT_PREP;
    }
}

void ADXPD_Stop(ADXPD xpd)
{
    xpd->stat = ADXPD_STAT_STOP;
    memset(xpd->delay, 0, sizeof(xpd->delay));
}

/* stop an expander that has decoded its blocks, so that it takes another
 * entry */
void ADXPD_Reset(ADXPD xpd)
{
    if (xpd->stat == ADXPD_STAT_END) {
        xpd->stat = ADXPD_STAT_STOP;
    }
}

Sint32 ADXPD_GetNumBlk(ADXPD xpd)
{
    return xpd->nblk;
}

void adxpd_error(void)
{
    adxpd_internal_error = 1;
}

void ADXPD_ExecHndl(ADXPD xpd)
{
    if (xpd->stat == ADXPD_STAT_PREP) {
        xpd->stat = ADXPD_STAT_EXEC;
    }
    if (xpd->stat == ADXPD_STAT_EXEC) {
        if (xpd->nch == 1) {
            xpd->nblk = ADX_DecodeMono2(xpd->ibuf, xpd->req_nblk, xpd->obufl,
                                        xpd->delay[0], xpd->k[0], xpd->k[1]);
        } else {
            xpd->nblk = ADX_DecodeSteFloat(xpd->ibuf, xpd->req_nblk, xpd->obufl,
                                           xpd->delay[0], xpd->obufr,
                                           xpd->delay[1], xpd->k[0], xpd->k[1]);
            /* the stereo decoder takes blocks in left and right pairs. it
             * returns an odd count when an end scale stops it between a
             * pair's blocks, or when req_nblk is odd */
            if (xpd->nblk % 2 == 1) {
                adxpd_error();
            }
        }
        xpd->stat = ADXPD_STAT_END;
    }
}

void ADXPD_ExecServer(void)
{
    Sint32 i;

    for (i = 0; i < ADXPD_MAX_OBJ; i++) {
        if (adxpd_obj[i].used == 1) {
            ADXPD_ExecHndl(&adxpd_obj[i]);
        }
    }
}
