/* vibration pack control through Kaede */
#include <kaede.h>
#include <sg_pdvib.h>
#include <sg_xpt.h>
#include <shinobi.h>

#define PORTS 24

/* a response to a media info request: the header and function words, then
 * the unit's description */
struct vib_frame {
    Uint32 header[2];
    Uint8 unit[4];
};

/* a port's vibration state */
struct vib_port {
    Sint32 busy; /* a unit enumeration is under way */
    PDS_VIBUNITINFO *unit_info;
    Uint32 param; /* passed to the callbacks */
    PD_VIBCALLBACK info_func;
    PD_VIBENUMUNITCALLBACK unit_func;
    Uint32 unit; /* the unit the enumeration has reached, from 1 */
    PDS_VIBINFO info;
};

Sint32 pdVibSetUnitInfo(Sint32 device, Sint32 seqno, const KDS_DEVICE *dev,
                        const KDS_DEVICEINFO *info, PDS_VIBUNITINFO *unit_info,
                        Uint32 *unit);
void pdVibGetUnitInfoCallback(Sint32 device, Sint32 seqno, KDS_DEVICE *dev,
                              KDS_DEVICEINFO *info);
void pdVibEnumUnitCallback(Sint32 device, Sint32 seqno, KDS_DEVICE *dev,
                           KDS_DEVICEINFO *info);

const char pdgVibBuild[] = "\npdVib Ver 0.30 Build:Nov 05 1998 19:00:38\n";

/* the port of the controller on each port's connector */
static const Uint8 vib_roots[PORTS] = {0,  0,  0,  0,  0,  0,  6,  6,
                                       6,  6,  6,  6,  12, 12, 12, 12,
                                       12, 12, 18, 18, 18, 18, 18, 18};

/* pdVibGetDirection's results, indexed by 4 times the connector direction of
 * the controller's socket, plus the connector direction of the pack in it */
static const Uint8 vib_directions[4 * 4] = {
    PDD_VIB_DIRECTION_FLIP,  PDD_VIB_DIRECTION_NORMAL, PDD_VIB_DIRECTION_LEFT,
    PDD_VIB_DIRECTION_RIGHT, PDD_VIB_DIRECTION_NORMAL, PDD_VIB_DIRECTION_FLIP,
    PDD_VIB_DIRECTION_RIGHT, PDD_VIB_DIRECTION_LEFT,   PDD_VIB_DIRECTION_RIGHT,
    PDD_VIB_DIRECTION_LEFT,  PDD_VIB_DIRECTION_FLIP,   PDD_VIB_DIRECTION_NORMAL,
    PDD_VIB_DIRECTION_LEFT,  PDD_VIB_DIRECTION_RIGHT,  PDD_VIB_DIRECTION_NORMAL,
    PDD_VIB_DIRECTION_FLIP};

struct vib_port pdgVib[PORTS];

Sint32 pdVibIsReady(Uint32 port)
{
    const KDS_DEVICEINFO *info;

    info = kdGetDeviceInfo(port);
    return (info->function & KDD_FUNC_VIBRATION) ? 1 : 0;
}

Sint32 pdVibSetStopTime(Uint32 port, Uint8 *unit, Uint8 *time, Uint32 num)
{
    Uint32 block[5];
    Uint8 *byte;
    Uint32 units;
    Uint32 i;
    Uint32 words;
    Sint32 err;

    if (!pdVibIsReady(port)) {
        return PDD_VIBERR_NO_VIBRATOR;
    }
    block[0] = block[1] = block[2] = block[3] = block[4] = 0;
    byte = (Uint8 *)block;
    units = 0;
    for (i = 0; i < num; i++) {
        units |= 1 << *unit++;
    }
    *byte++ = (Uint16)units >> 8;
    *byte++ = units;
    for (i = 0; i < num; i++) {
        *byte++ = *time++;
    }
    /* the mask's two bytes and the times, in whole words */
    /* @hack spelling: the rounding in a statement of its own, which has SHC
     * copy num from R13 to R4 and round the copy (MOV R13,R4 at 0x8c12280a, ADD
     * #5,R4 at 0x8c12280e in 1ST_READ.BIN); as one expression, SHC rounds num
     * in R13 itself, and 24 of 88 words differ */
    words = num + 2;
    words = (words + 3) / 4;
    /* @hack spelling: the result through err, which SHC moves to R4 to test
     * it (MOV R0,R4 at 0x8c122826 in 1ST_READ.BIN); tested directly, 14 of 88
     * words differ */
    err = kdSetBlockWrite(port, KDD_FUNC_VIBRATION, 0, 0, 0, block, words);
    if (err < 0) {
        return PDD_VIBERR_BUSY;
    }
    return PDD_VIBERR_OK;
}

/* send each param as a word, in condition, which holds 16 words. the word's
 * bytes are the unit with the continuous flag, the power with the exhalation
 * and convergence flags, the frequency, and the increment. a positive power
 * goes in bits 4 to 6, and a negative one's magnitude in bits 0 to 2. the
 * increment is 0 unless either of those flags is set */
Sint32 pdVibStart(Uint32 port, const PDS_VIBPARAM *param, Uint32 numunits)
{
    Uint8 condition[64];
    Uint8 *byte;
    Uint32 i;
    Uint8 power;
    Uint8 flag;
    Sint32 err;

    if (!pdVibIsReady(port)) {
        return PDD_VIBERR_NO_VIBRATOR;
    }
    byte = condition;
    for (i = 0; i < numunits; i++, param++) {
        /* @hack spelling: flag, a copy of param->flag, which SHC reads once
         * into R6 (0x8c122868 in 1ST_READ.BIN); with param->flag at each use,
         * the function runs 4 bytes short and 83 of 88 words differ */
        flag = param->flag;
        *byte++ = (param->unit << 4) | (flag & PDD_VIB_FLAG_CONTINUOUS);
        if (param->power > 0) {
            power = (param->power << 4) & 0x70;
        } else if (param->power < 0) {
            power = -param->power & 7;
        } else {
            power = 0;
        }
        power |= flag & (PDD_VIB_FLAG_EXHALATION | PDD_VIB_FLAG_CONVERGENCE);
        *byte++ = power;
        *byte++ = param->freq;
        *byte++ = (flag & (PDD_VIB_FLAG_EXHALATION | PDD_VIB_FLAG_CONVERGENCE))
                      ? param->inc
                      : 0;
    }
    /* @hack spelling: the result through err, which SHC moves to R4 to test
     * it (MOV R0,R4 at 0x8c1228e2 in 1ST_READ.BIN); tested directly, 15 of 88
     * words differ */
    err = kdSetSetCondition(port, KDD_FUNC_VIBRATION, condition, numunits);
    if (err < 0) {
        return PDD_VIBERR_BUSY;
    }
    return PDD_VIBERR_OK;
}

Sint32 pdVibGetInfo(Uint32 port, PDS_VIBINFO *info)
{
    KDS_VIBRATIONINFO vib;
    Sint32 err;

    /* @hack spelling: the result through err, which SHC moves to R4 to test
     * it (MOV R0,R4 at 0x8c122912 in 1ST_READ.BIN); tested directly, 18 of 30
     * words differ */
    err = kdDevGetVibrationInfo(&vib, kdGetDeviceInfo(port));
    if (err < 0) {
        return PDD_VIBERR_NO_VIBRATOR;
    }
    info->units = vib.vn;
    info->se_units = vib.se;
    return PDD_VIBERR_OK;
}

/* fill unit_info, and *unit unless unit is NULL, from the pack's response
 * to a media info request. returns PDD_VIBERR_NO_VIBRATOR when the device
 * has no vibration function or its response isn't KDD_RES_DATATRANSFER */
Sint32 pdVibSetUnitInfo(Sint32 device, Sint32 seqno, const KDS_DEVICE *dev,
                        const KDS_DEVICEINFO *info, PDS_VIBUNITINFO *unit_info,
                        Uint32 *unit)
{
    const Uint8 *data;
    Uint8 can;
    Sint32 err;

    if (!(info->function & KDD_FUNC_VIBRATION) ||
        dev->response != KDD_RES_DATATRANSFER) {
        err = PDD_VIBERR_NO_VIBRATOR;
    } else {
        data = ((const struct vib_frame *)dev->recvdata)->unit;
        if (unit) {
            *unit = (data[0] >> 4) & 15;
        }
        can = data[1];
        unit_info->position = (data[0] >> 2) & 3;
        unit_info->axis = data[0] & 3;
        unit_info->pow_enable = (can & 0x80) ? 1 : 0;
        unit_info->cont_enable = (can & 0x40) ? 1 : 0;
        unit_info->dir_enable = (can & 0x20) ? 1 : 0;
        unit_info->wave_enable = (can & 0x10) ? 1 : 0;
        switch (can & 15) {
        case 0:
            unit_info->min_freq = data[2];
            unit_info->max_freq = data[3];
            break;
        case 1:
            unit_info->min_freq = data[2];
            unit_info->max_freq = data[2];
            break;
        case 15:
            unit_info->min_freq = 0;
            unit_info->max_freq = 0;
            break;
        default:
            unit_info->min_freq = 0xff;
            unit_info->max_freq = 0xff;
            break;
        }
        err = PDD_VIBERR_OK;
    }
    return err;
}

void pdVibGetUnitInfoCallback(Sint32 device, Sint32 seqno, KDS_DEVICE *dev,
                              KDS_DEVICEINFO *info)
{
    struct vib_port *vib;
    Sint32 err;

    vib = &pdgVib[device];
    if (vib->info_func) {
        err = pdVibSetUnitInfo(device, seqno, dev, info, vib->unit_info, NULL);
        vib->info_func(err, vib->param);
    }
}

/* report the unit that responded to unit_func. when that succeeded and the pack
 * has units past it, request the next unit's media info */
void pdVibEnumUnitCallback(Sint32 device, Sint32 seqno, KDS_DEVICE *dev,
                           KDS_DEVICEINFO *info)
{
    struct vib_port *vib = &pdgVib[device];
    Uint32 unit;
    PDS_VIBUNITINFO unit_info;
    Sint32 err;

    vib->busy = false;
    /* NOLINTNEXTLINE(clang-diagnostic-sometimes-uninitialized) */
    if (vib->unit_func) {
        err = pdVibSetUnitInfo(device, seqno, dev, info, &unit_info, &unit);
        vib->unit_func(unit, err, &unit_info, vib->param);
    }
    /* @bug uninitialized: with no unit_func, nothing writes err, and the test
     * reads R12 as the caller left it (TST R12,R12 at 0x8c122a8a in
     * 1ST_READ.BIN) */
    if (err == PDD_VIBERR_OK && vib->info.units > vib->unit) {
        vib->unit++;
        kdSetGetMediaInfoEx(device, KDD_FUNC_VIBRATION, vib->unit,
                            pdVibEnumUnitCallback);
    }
}

Sint32 pdVibEnumerateUnit(Uint32 port, PD_VIBENUMUNITCALLBACK func,
                          Uint32 param)
{
    struct vib_port *vib = &pdgVib[port];
    Sint32 err;

    if (vib->busy) {
        err = PDD_VIBERR_BUSY;
    } else {
        vib->busy = true;
        vib->unit_info = NULL;
        vib->param = param;
        vib->info_func = NULL;
        vib->unit_func = func;
        vib->unit = 1;
        err = pdVibGetInfo(port, &vib->info);
        if (err >= 0) {
            err = kdSetGetMediaInfoEx(port, KDD_FUNC_VIBRATION, 1,
                                      pdVibEnumUnitCallback);
            if (err < 0) {
                err = PDD_VIBERR_BUSY;
            } else {
                err = PDD_VIBERR_OK;
            }
        }
    }
    return err;
}

Sint32 pdVibGetDirection(Uint32 port)
{
    const KDS_DEVICEINFO *root;
    const KDS_DEVICEINFO *info;
    Sint32 socket;

    root = kdGetDeviceInfo(vib_roots[port]);
    info = kdGetDeviceInfo(port);
    /* a controller's second socket has its own connector direction */
    switch (port) {
    case PDD_PORT_A2:
    case PDD_PORT_B2:
    case PDD_PORT_C2:
    case PDD_PORT_D2:
        socket = 1;
        break;
    default:
        socket = 0;
        break;
    }
    return vib_directions[(4 * root->connector_dir[socket]) +
                          info->connector_dir[0]];
}
