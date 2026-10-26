/* GD-ROM drive functions of the GD file system, through the boot ROM: the
 * GD_DCF table gdFsGetGdDcf returns */
#include <sg_xpt.h>
#include <shinobi.h>

#include "shinobi/gdfs.h"

/* the drive's commands, gdGdcReqCmd's cmd */
#define CMD_PIOREAD 16
#define CMD_DMAREAD 17
#define CMD_GETTOC2 19
#define CMD_PLAY 20
#define CMD_PLAY2 21
#define CMD_PAUSE 22
#define CMD_RELEASE 23
#define CMD_INIT 24
#define CMD_SEEK 27
#define CMD_DMAREAD_STREAM 28
#define CMD_GETSCD 34
#define CMD_GET_POS 36
#define CMD_GET_VERS 40

/* gdGdcGetCmdStat's request states */
#define GDC_STAT_FAILED (-1)
#define GDC_STAT_IDLE 0
#define GDC_STAT_BUSY 1
#define GDC_STAT_COMPLETE 2
#define GDC_STAT_STREAMING 3
#define GDC_STAT_ABORTED 4

/* the drive's error codes, in a failed request's stat.err */
#define GDC_ERR_NOERR 0
#define GDC_ERR_RECOVER 1
#define GDC_ERR_NOTREADY 2
#define GDC_ERR_MEDIA 3
#define GDC_ERR_HWARE 4
#define GDC_ERR_ILLREQ 5
#define GDC_ERR_UNITATTENT 6
#define GDC_ERR_PROTECT 7
#define GDC_ERR_ABORT 11
#define GDC_ERR_NOREADABLE 16

/* an address's top three bits select its area. AREA_MASK clears them, and
 * AREA_P2 sets those of P2, the area the CPU reads and writes uncached */
#define AREA_MASK 0x1fffffff
#define AREA_P2 0xa0000000
/* NIBBLE_SHIFT brings an address's top four bits to the bottom. there,
 * NIBBLE_AREA keeps the three that select its area, and NIBBLE_ALL keeps all
 * four. NIBBLE_P2 is AREA_P2's top four bits */
#define NIBBLE_SHIFT 28
#define NIBBLE_AREA 14
#define NIBBLE_ALL 15
#define NIBBLE_P2 (AREA_P2 >> NIBBLE_SHIFT)

static Bool gdc_init_system(void)
{
    gdGdcInitSystem();
    return TRUE;
}

/* request a read of nsct sectors from fad into buf: by DMA for
 * GDD_FS_TMODE_DMA, or by the CPU through P2 for GDD_FS_TMODE_CPU. once the
 * drive takes the request, the operand cache over buf is invalidated, so the
 * CPU reads the new data. when buf isn't 32-byte aligned, the cache over it
 * is purged instead. it returns the request number, 0 when the drive refuses
 * the read, or GDD_ERR_ILLTMODE for any other tmode */
static Sint32 gdc_read(Sint32 fad, Sint32 nsct, void *buf, Sint32 tmode)
{
    Sint32 param[4];
    Sint32 cmd;
    Sint32 gdchn;

    param[0] = fad;
    param[1] = nsct;
    param[3] = 0;
    if (tmode == GDD_FS_TMODE_DMA) {
        cmd = CMD_DMAREAD;
        param[2] = (Sint32)buf & AREA_MASK;
    } else if (tmode == GDD_FS_TMODE_CPU) {
        cmd = CMD_PIOREAD;
        param[2] = ((Sint32)buf & AREA_MASK) | AREA_P2;
    } else {
        return GDD_ERR_ILLTMODE;
    }
    gdchn = gdGdcReqCmd(cmd, param);
    /* a buf in P2 skips the cache operation: the cache holds none of P2 */
    if (gdchn != 0 &&
        (((Sint32)buf >> NIBBLE_SHIFT) & NIBBLE_AREA) != NIBBLE_P2) {
        if (((Sint32)buf & 31) == 0) {
            syCacheOCBI(buf, nsct * GDD_FS_SCTSIZE);
        } else {
            syCacheOCBP(buf, nsct * GDD_FS_SCTSIZE);
        }
    }
    return gdchn;
}

static Sint32 gdc_req_gd_read(Sint32 fad, Sint32 nsct)
{
    Sint32 param[2];

    param[0] = fad;
    param[1] = nsct;
    return gdGdcReqCmd(CMD_DMAREAD_STREAM, param);
}

static Sint32 gdc_trans(Sint32 gdchn, void *buf, Sint32 nbytes)
{
    Sint32 param[2];
    Sint32 result;

    param[0] = (Sint32)buf;
    param[1] = nbytes;
    result = gdGdcReqDmaTrans(gdchn, param);
    /* unlike gdc_read's, this test compares all four top bits with P2's, so
     * only a buf in P2 below 0xb0000000 skips the cache operation */
    if (result == 0 && nbytes != 0 &&
        (((Sint32)buf >> NIBBLE_SHIFT) & NIBBLE_ALL) != NIBBLE_P2) {
        if (((Sint32)buf & 31) == 0) {
            syCacheOCBI(buf, nbytes);
        } else {
            syCacheOCBP(buf, nbytes);
        }
    }
    return result;
}

static Sint32 gdc_get_cmd_stat(Sint32 gdchn, Sint32 *info)
{
    struct {
        Sint32 err;
        Sint32 ex_errcode;
        Sint32 trnsed;
        Sint32 wait;
    } stat;
    Sint32 result = gdGdcGetCmdStat(gdchn, &stat);

    switch (result) {
    case GDC_STAT_IDLE:
        result = GDD_STAT_IDLE;
        break;
    case GDC_STAT_BUSY:
        result = GDD_STAT_BUSY;
        info[REQ_TRNSED] = stat.trnsed;
        info[REQ_WAIT] = stat.wait;
        break;
    case GDC_STAT_COMPLETE:
        result = GDD_STAT_COMPLETE;
        info[REQ_TRNSED] = stat.trnsed;
        break;
    case GDC_STAT_STREAMING:
        result = GDD_STAT_CONTINUE;
        break;
    case GDC_STAT_ABORTED:
        result = GDD_STAT_BUSY;
        break;
    case GDC_STAT_FAILED:
        info[REQ_EX_ERRCODE] = stat.ex_errcode;
        result = GDD_STAT_ERR;
        switch (stat.err) {
        case GDC_ERR_NOERR:
            info[REQ_ERR] = GDD_ERR_NOERR;
            break;
        case GDC_ERR_RECOVER:
            info[REQ_ERR] = GDD_ERR_RECOVER;
            break;
        case GDC_ERR_NOTREADY:
            info[REQ_ERR] = GDD_ERR_NOTREADY;
            break;
        case GDC_ERR_MEDIA:
            info[REQ_ERR] = GDD_ERR_MEDIA;
            break;
        case GDC_ERR_HWARE:
            info[REQ_ERR] = GDD_ERR_HWARE;
            break;
        case GDC_ERR_ILLREQ:
            info[REQ_ERR] = GDD_ERR_ILLREQ;
            break;
        case GDC_ERR_UNITATTENT:
            info[REQ_ERR] = GDD_ERR_UNITATTENT;
            break;
        case GDC_ERR_PROTECT:
            info[REQ_ERR] = GDD_ERR_PROTECT;
            break;
        case GDC_ERR_ABORT:
            info[REQ_ERR] = GDD_ERR_ABORT;
            break;
        case GDC_ERR_NOREADABLE:
            info[REQ_ERR] = GDD_ERR_NOREADABLE;
            break;
        default:
            info[REQ_ERR] = GDD_ERR_UNDEF;
            break;
        }
        break;
    }
    return result;
}

static void gdc_read_abort(Sint32 gdchn)
{
    gdGdcReadAbort(gdchn);
}

static Sint32 gdc_seek(Sint32 fad)
{
    Sint32 param[2];

    param[0] = fad;
    param[1] = 0;
    return gdGdcReqCmd(CMD_SEEK, param);
}

static Sint32 gdc_init_drive(void)
{
    /* @hack spelling: the request's result through gdchn, which makes the
     * call a JSR (0x8c65e65c); returned straight, SHC jumps to it and 8 of 8
     * words differ */
    Sint32 gdchn = gdGdcReqCmd(CMD_INIT, NULL);

    return gdchn;
}

static Bool gdc_get_drv_stat(Sint32 *info)
{
    /* @hack spelling: the call's result through result, which SHC copies to
     * R4 in the branch's delay slot (MOV R0,R4 at 0x8c65e698); tested in
     * place, it isn't copied, and 10 of 14 words differ */
    Sint32 result = gdGdcGetDrvStat(info);

    if (result == 1) {
        return FALSE;
    }
    return TRUE;
}

static Sint32 gdc_check_trans(Sint32 gdchn, Sint32 *size)
{
    /* @hack spelling: the check's result through result, which makes the
     * call a JSR (0x8c65e6ac); returned straight, SHC jumps to it and 7 of 7
     * words differ */
    Sint32 result = gdGdcCheckDmaTrans(gdchn, size);

    return result;
}

static Sint32 gdc_da_play(Sint32 st_track, Sint32 end_track, Sint32 reptime)
{
    Sint32 param[4];

    param[0] = st_track;
    param[1] = end_track;
    param[2] = reptime;
    param[3] = 0;
    return gdGdcReqCmd(CMD_PLAY, param);
}

static Sint32 gdc_da_play_sct(Sint32 st_sct, Sint32 end_sct, Sint32 reptime)
{
    Sint32 param[4];

    param[0] = st_sct;
    param[1] = end_sct;
    param[2] = reptime;
    param[3] = 0;
    return gdGdcReqCmd(CMD_PLAY2, param);
}

static Sint32 gdc_da_pause(void)
{
    /* @hack spelling: the request's result through gdchn, which makes the
     * call a JSR (0x8c65e700); returned straight, SHC jumps to it and 8 of 8
     * words differ */
    Sint32 gdchn = gdGdcReqCmd(CMD_PAUSE, NULL);

    return gdchn;
}

static Sint32 gdc_da_release(Sint32 reptime)
{
    Sint32 param[2];

    param[0] = reptime;
    param[1] = 0;
    return gdGdcReqCmd(CMD_RELEASE, param);
}

static Sint32 gdc_get_toc(Sint32 type, void *buf)
{
    Sint32 param[2];

    param[0] = type;
    param[1] = (Sint32)buf;
    return gdGdcReqCmd(CMD_GETTOC2, param);
}

static Sint32 gdc_get_scd(Sint32 type, Sint32 size, void *buf)
{
    Sint32 param[4];

    param[0] = type;
    param[1] = size;
    param[2] = (Sint32)buf;
    param[3] = 0;
    return gdGdcReqCmd(CMD_GETSCD, param);
}

static Bool gdc_exec_server(void)
{
    gdGdcExecServer();
    return TRUE;
}

static void gdc_reset(void)
{
    gdGdcReset();
}

/* NOLINTBEGIN(readability-non-const-parameter) */
static Sint32 gdc_get_pos(Sint32 *stat, Sint32 *track, Sint32 *fad,
                          Sint32 *index)
/* NOLINTEND(readability-non-const-parameter) */
{
    Sint32 param[4];

    param[0] = (Sint32)stat;
    param[1] = (Sint32)track;
    param[2] = (Sint32)fad;
    param[3] = (Sint32)index;
    return gdGdcReqCmd(CMD_GET_POS, param);
}

/* NOLINTNEXTLINE(readability-non-const-parameter) */
static Sint32 gdc_get_ver(char *ver)
{
    Sint32 param[2];

    param[0] = (Sint32)ver;
    param[1] = 0;
    return gdGdcReqCmd(CMD_GET_VERS, param);
}

static const GD_DCF gdc_dcf = {
    gdc_init_system,  gdc_read,        gdc_req_gd_read, gdc_trans,
    gdc_get_cmd_stat, gdc_read_abort,  gdc_seek,        gdc_init_drive,
    gdc_get_drv_stat, gdc_check_trans, gdc_da_play,     gdc_da_play_sct,
    gdc_da_pause,     gdc_da_release,  gdc_get_toc,     gdc_get_scd,
    gdc_exec_server,  gdc_reset,       gdc_get_pos,     gdc_get_ver,
};

const GD_DCF *gdFsGetGdDcf(void)
{
    return &gdc_dcf;
}
