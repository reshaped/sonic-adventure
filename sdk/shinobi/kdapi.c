/* Kaede, the Maple bus driver's interface. kdVbInHandler sends a frame at
 * each vertical blank, and kdDmaEndHandler0 receives its responses when the
 * Maple DMA ends */
#include <kaede.h>
#include <machine.h>
#include <mpdrv.h>
#include <sg_chain.h>
#include <sg_xpt.h>
#include <string.h>

/* the system bus's normal interrupt status, error interrupt status and
 * level 4 normal interrupt mask registers. INT_STATUS and INT_LEVEL4_MASK
 * hold the two Maple bits, and INT_ERROR_STATUS the four Maple error bits.
 * writing 1 to a status bit clears it */
#define INT_STATUS (*(volatile Uint32 *)0xa05f6900)
#define INT_ERROR_STATUS (*(volatile Uint32 *)0xa05f6908)
#define INT_LEVEL4_MASK (*(volatile Uint32 *)0xa05f6920)
#define INT_MAPLE_DMA_END 0x1000UL
#define INT_MAPLE_VBLANK_OVER 0x2000UL
#define INT_MAPLE_ERRORS 0xf00
#define INT_MAPLE_ERROR(n) (0x800 >> (n))

/* the video sync generator's vertical blank and status registers.
 * SYNC_VBLANK's VBLANK_START field holds the line a vertical blank starts on,
 * and SYNC_STATUS's SCAN_LINE field the line the signal is on */
#define SYNC_VBLANK (*(volatile Uint32 *)0xa05f80dc)
#define SYNC_STATUS (*(volatile Uint32 *)0xa05f810c)
#define VBLANK_START 0x3ff
#define SCAN_LINE 0x3ff

/* the size kdInitSystem passes mpdrv_init for each buffer, which mpdrv_init
 * doesn't read */
#define BUFFER_SIZE 0xc000

/* the type of kdgErrFunc, which Kaede calls when it meets an error. where is
 * a KD_IN_ value, err a KD_ERR_ value, and vbin_count
 * kdgSystemInfo.vbin_count. detail is INT_ERROR_STATUS for KD_ERR_BUS, 0 for
 * KD_ERR_BLOCKED, and for the rest the result of the mpdrv_ call that
 * failed */
typedef void (*kd_err_func)(Sint32 where, Sint32 err, Sint32 vbin_count,
                            Sint32 detail);
enum { KD_IN_VBLANK, KD_IN_SEND, KD_IN_RECV };
#define KD_ERR_BLOCKED (-1)
#define KD_ERR_BUS(n) (-2 - (n))
#define KD_ERR_SEND_AGAIN (-6)
#define KD_ERR_SEND (-7)
#define KD_ERR_RECV (-8)

/* a callback slot. once device has answered the command with sequence number
 * seqno, kdRecvFrame clears func and calls it. func is NULL in a slot that
 * waits for nothing */
struct kd_callback {
    Sint32 device;
    Sint32 seqno;
    KD_CALLBACK func;
};

void kdVbInHandler(void *arg);
void kdDmaEndHandler0(void *arg);
void kdDmaEndHandler1(void *arg);
void kdVbOverHandler(void *arg);

char *kdgBuild = "\nkd Ver 1.23 Build:Nov 05 1998 18:35:15\n";
/* the chain priorities kdInitSystem gives kdVbInHandler, kdDmaEndHandler0,
 * kdDmaEndHandler1 and kdVbOverHandler, in that order */
const Uint32 kdgChLevel[4] = {56, 1, 64, 1};
static const char no_semaphore[] = "***FATAL ERROR:Cannot get semaphore!\n";
SYCHAIN kdgChID[4];
KDS_DEVICE kdgDevice[KDD_DEV_MAX];
KDS_SYSTEMINFO kdgSystemInfo;
kd_err_func kdgErrFunc;
/* two callback slots for each device. kdgCbTbl[2 * device + _mp->received]
 * holds the callback of a command queued while _mp->received had that value.
 * _mp->received is 0 while a frame is out, and 1 between frames */
struct kd_callback kdgCbTbl[2 * KDD_DEV_MAX];
/* the index, 0 or 1, of the kdgSystemInfo.send_line, recv_line and
 * vover_count entries that the frame on the bus fills. kdRecvFrame flips it
 * once it has received a frame */
Uint32 kdgSurface;
/* set while kdRecvFrame receives a frame, and while a receive waits for the
 * semaphore. kdVbInHandler sends no frame while it is set */
Uint32 kdgBlockFlag;

/* send a frame at each vertical blank. with kdgBlockFlag set, count the
 * blank as blocked and report KD_ERR_BLOCKED instead */
void kdVbInHandler(void *arg)
{
    kdgSystemInfo.vbin_count++;
    if (kdgBlockFlag) {
        kdgSystemInfo.blocked_count++;
        if (kdgErrFunc) {
            kdgErrFunc(KD_IN_VBLANK, KD_ERR_BLOCKED, kdgSystemInfo.vbin_count,
                       0);
        }
    } else {
        kdSendFrame();
    }
}

/* receive the frame's responses when the Maple DMA ends, with every
 * interrupt masked */
void kdDmaEndHandler0(void *arg)
{
    set_imask(IMASK_ALL);
    kdgSystemInfo.dmaend_count++;
    kdRecvFrame();
}

/* clear the Maple DMA end interrupt. kdInitSystem chains it to the same event
 * as kdDmaEndHandler0 */
void kdDmaEndHandler1(void *arg)
{
    INT_STATUS = INT_MAPLE_DMA_END;
}

void kdVbOverHandler(void *arg)
{
    kdgSystemInfo.total_vover_count++;
    kdgSystemInfo.vover_count[kdgSurface]++;
    INT_STATUS = INT_MAPLE_VBLANK_OVER;
}

/* copy the driver's device state to kdgDevice, for kdGetDevice */
static void publish_devices(void)
{
    Sint32 i;
    KDS_DEVICE *dev = kdgDevice;
    const struct maple_device *device = _mp->device;

    for (i = 0; i < KDD_DEV_MAX; i++, dev++, device++) {
        dev->on = device->on;
        dev->old = device->old;
        dev->connect = device->connect;
        dev->remove = device->remove;
        dev->recvdata = device->recvdata;
        dev->response = device->response;
        dev->dst_ap = device->dst_ap;
        dev->src_ap = device->src_ap;
        dev->datasize = device->datasize;
        dev->errcode = device->errcode;
    }
}

Sint32 kdInitSystem(void *sendbuf, void *recvbuf)
{
    Uint32 mask = get_imask();
    Sint32 err;

    set_imask(IMASK_ALL);
    kdgChID[0] = syChainAddHandler(SYD_CHAIN_EVENT_IML6_VBLANK, kdVbInHandler,
                                   kdgChLevel[0], NULL);
    kdgChID[1] = syChainAddHandler(SYD_CHAIN_EVENT_IML4_MPDMAE,
                                   kdDmaEndHandler0, kdgChLevel[1], NULL);
    kdgChID[2] = syChainAddHandler(SYD_CHAIN_EVENT_IML4_MPDMAE,
                                   kdDmaEndHandler1, kdgChLevel[2], NULL);
    kdgChID[3] = syChainAddHandler(SYD_CHAIN_EVENT_IML4_MPVOVR, kdVbOverHandler,
                                   kdgChLevel[3], NULL);
    kdgErrFunc = NULL;
    kdgSurface = 0;
    memset(&kdgSystemInfo, 0, sizeof(kdgSystemInfo));
    memset(kdgDevice, 0, sizeof(kdgDevice));
    memset(kdgCbTbl, 0, sizeof(kdgCbTbl));
    err = mpdrv_init(sendbuf, BUFFER_SIZE, recvbuf, BUFFER_SIZE);
    publish_devices();
    INT_LEVEL4_MASK |= INT_MAPLE_DMA_END | INT_MAPLE_VBLANK_OVER;
    set_imask(mask);
    return err;
}

/* send a frame. returns mpdrv_frame_send's result. after a Maple bus error,
 * it first counts and reports each error bit, clears the bits, and receives
 * the last frame's responses */
Sint32 kdSendFrame(void)
{
    Sint32 err;

    if (INT_ERROR_STATUS & INT_MAPLE_ERRORS) {
        if (INT_ERROR_STATUS & INT_MAPLE_ERROR(0)) {
            kdgSystemInfo.error_count[0]++;
            if (kdgErrFunc) {
                kdgErrFunc(KD_IN_SEND, KD_ERR_BUS(0), kdgSystemInfo.vbin_count,
                           INT_ERROR_STATUS);
            }
        }
        if (INT_ERROR_STATUS & INT_MAPLE_ERROR(1)) {
            kdgSystemInfo.error_count[1]++;
            if (kdgErrFunc) {
                kdgErrFunc(KD_IN_SEND, KD_ERR_BUS(1), kdgSystemInfo.vbin_count,
                           INT_ERROR_STATUS);
            }
        }
        if (INT_ERROR_STATUS & INT_MAPLE_ERROR(2)) {
            kdgSystemInfo.error_count[2]++;
            if (kdgErrFunc) {
                kdgErrFunc(KD_IN_SEND, KD_ERR_BUS(2), kdgSystemInfo.vbin_count,
                           INT_ERROR_STATUS);
            }
        }
        if (INT_ERROR_STATUS & INT_MAPLE_ERROR(3)) {
            kdgSystemInfo.error_count[3]++;
            if (kdgErrFunc) {
                kdgErrFunc(KD_IN_SEND, KD_ERR_BUS(3), kdgSystemInfo.vbin_count,
                           INT_ERROR_STATUS);
            }
        }
        INT_ERROR_STATUS = INT_MAPLE_ERRORS;
        kdRecvFrame();
        err = mpdrv_frame_send();
        if (err >= 0) {
            publish_devices();
        } else if (kdgErrFunc) {
            kdgErrFunc(KD_IN_SEND, KD_ERR_SEND_AGAIN, kdgSystemInfo.vbin_count,
                       err);
        }
    } else {
        err = mpdrv_frame_send();
        if (err >= 0) {
            kdgSystemInfo.vover_count[kdgSurface] = 0;
            kdgSystemInfo.send_line[kdgSurface] = SYNC_STATUS & SCAN_LINE;
            publish_devices();
            kdgSystemInfo.send_count++;
        } else {
            kdgSystemInfo.cannot_send_count++;
            if (kdgErrFunc) {
                kdgErrFunc(KD_IN_SEND, KD_ERR_SEND, kdgSystemInfo.vbin_count,
                           err);
            }
        }
    }
    return err;
}

/* receive the frame's responses, and call back each command they answer.
 * returns mpdrv_frame_recv_pre's result. without the semaphore, it sets
 * _mp->recv_pending, and the next command call receives the frame */
Sint32 kdRecvFrame(void)
{
    Sint32 err;
    Sint32 i;
    struct kd_callback *callback;
    KD_CALLBACK func;

    kdgBlockFlag = true;
    err = mpdrv_frame_recv_pre();
    if (err == -2) {
        _mp->recv_pending = true;
    } else {
        if (err >= 0) {
            mpdrv_frame_recv_main();
            kdgSystemInfo.recv_line[kdgSurface] =
                (SYNC_STATUS & SCAN_LINE) +
                (kdgSystemInfo.vover_count[kdgSurface] *
                 (SYNC_VBLANK & VBLANK_START));
            publish_devices();
            mpdrv_frame_recv_post();
            /* @hack spelling: i zeroed ahead of the for, which SHC does before
             * the BRA into the loop (MOV #0,R11 at 0x8c11cbce in
             * 1ST_READ.BIN); in the for's first clause, the zero takes the
             * BRA's slot, and 2 of 119 words differ in each image */
            i = 0;
            for (callback = kdgCbTbl; i < 2 * KDD_DEV_MAX; i++, callback++) {
                /* @hack spelling: the tests as two continues, which make SHC
                 * enter the loop by a BRA to its test (0x8c11cbd0 in
                 * 1ST_READ.BIN); as one test with &&, SHC enters the body
                 * directly, and 60 of 119 words differ in each image */
                if (!callback->func) {
                    continue;
                }
                if (!kdGetStatus(callback->device, callback->seqno)) {
                    continue;
                }
                func = callback->func;
                callback->func = NULL;
                func(callback->device, callback->seqno,
                     (KDS_DEVICE *)kdGetDevice(callback->device),
                     (KDS_DEVICEINFO *)kdGetDeviceInfo(callback->device));
            }
            _mp->recv_pending = false;
            kdgSurface ^= 1;
            kdgSystemInfo.recv_count++;
        } else if (kdgErrFunc) {
            kdgErrFunc(KD_IN_RECV, KD_ERR_RECV, kdgSystemInfo.vbin_count, err);
        }
        kdgBlockFlag = false;
    }
    return err;
}

void kdExitSystem(void)
{
    Uint32 mask = get_imask();

    set_imask(IMASK_ALL);
    INT_LEVEL4_MASK &= ~(INT_MAPLE_DMA_END | INT_MAPLE_VBLANK_OVER);
    syChainDeleteHandler(kdgChID[3]);
    syChainDeleteHandler(kdgChID[2]);
    syChainDeleteHandler(kdgChID[1]);
    syChainDeleteHandler(kdgChID[0]);
    mpdrv_exit();
    memset(kdgDevice, 0, sizeof(kdgDevice));
    set_imask(mask);
}

const KDS_DEVICE *kdGetDevice(Sint32 device)
{
    return &kdgDevice[device];
}

const KDS_DEVICEINFO *kdGetDeviceInfo(Sint32 device)
{
    /* @hack spelling: the info through a local, which has SHC call
     * mpdrv_get_deviceinfo (JSR @R3 at 0x8c11ccf8 in 1ST_READ.BIN); returned
     * directly, the call becomes a tail jump, the function runs 4 bytes
     * short, and 6 of 7 words differ in each image */
    const KDS_DEVICEINFO *info = mpdrv_get_deviceinfo(device);

    return info;
}

const KDS_PORT *kdGetPortStatus(Sint32 port)
{
    /* @hack spelling: the status through a local, which has SHC call
     * mpdrv_get_portstatus (JSR @R3 at 0x8c11cd06 in 1ST_READ.BIN); returned
     * directly, the call becomes a tail jump, the function runs 4 bytes
     * short, and 7 of 7 words differ in each image */
    const KDS_PORT *status = mpdrv_get_portstatus(port);

    return status;
}

Sint32 kdGetStatus(Sint32 device, Sint32 seq)
{
    return mpdrv_stat(device, seq);
}

/* each command call below queues its command. then, if kdRecvFrame put off
 * a receive for want of the semaphore, it receives that frame */
Sint32 kdSetDeviceRequest(Sint32 device)
{
    Sint32 seq = mpdrv_set_dreq(device);

    if (_mp->recv_pending) {
        kdRecvFrame();
    }
    return seq;
}

Sint32 kdSetAllStatusRequest(Sint32 device)
{
    Sint32 seq = mpdrv_set_allstatreq(device);

    if (_mp->recv_pending) {
        kdRecvFrame();
    }
    return seq;
}

Sint32 kdSetDeviceReset(Sint32 device)
{
    Sint32 seq = mpdrv_set_dreset(device);

    if (_mp->recv_pending) {
        kdRecvFrame();
    }
    return seq;
}

Sint32 kdSetDeviceKill(Sint32 device)
{
    Sint32 seq = mpdrv_set_dkill(device);

    if (_mp->recv_pending) {
        kdRecvFrame();
    }
    return seq;
}

Sint32 kdSetGetCondition(Sint32 device, Sint32 func)
{
    Sint32 seq = mpdrv_set_getcond(device, func);

    if (_mp->recv_pending) {
        kdRecvFrame();
    }
    return seq;
}

Sint32 kdSetGetMediaInfo(Sint32 device, Sint32 func, Sint32 pt)
{
    Sint32 seq = mpdrv_set_getminfo(device, func, pt);

    if (_mp->recv_pending) {
        kdRecvFrame();
    }
    return seq;
}

Sint32 kdSetBlockRead(Sint32 device, Sint32 func, Sint32 pt, Sint32 phase,
                      Sint32 block)
{
    Sint32 seq = mpdrv_set_bread(device, func, pt, phase, block);

    if (_mp->recv_pending) {
        kdRecvFrame();
    }
    return seq;
}

Sint32 kdSetBlockWrite(Sint32 device, Sint32 func, Sint32 pt, Sint32 phase,
                       Sint32 block, const void *src, Sint32 size)
{
    Sint32 seq = mpdrv_set_bwrite(device, func, pt, phase, block, src, size);

    if (_mp->recv_pending) {
        kdRecvFrame();
    }
    return seq;
}

Sint32 kdSetGetLastError(Sint32 device, Sint32 func, Sint32 pt, Sint32 phase,
                         Sint32 block)
{
    Sint32 seq = mpdrv_set_getlerr(device, func, pt, phase, block);

    if (_mp->recv_pending) {
        kdRecvFrame();
    }
    return seq;
}

Sint32 kdSetARControl(Sint32 device, const void *buf)
{
    Sint32 seq = mpdrv_set_arctrl(device, buf);

    if (_mp->recv_pending) {
        kdRecvFrame();
    }
    return seq;
}

Sint32 kdSetFT4Control(Sint32 device, const void *buf)
{
    Sint32 seq = mpdrv_set_ft4ctrl(device, buf);

    if (_mp->recv_pending) {
        kdRecvFrame();
    }
    return seq;
}

Sint32 kdSetSetCondition(Sint32 device, Sint32 func, const void *buf,
                         Sint32 size)
{
    Sint32 seq = mpdrv_set_setcond(device, func, buf, size);

    if (_mp->recv_pending) {
        kdRecvFrame();
    }
    return seq;
}

Sint32 kdSetTransmitAgain(Sint32 device)
{
    Sint32 seq = mpdrv_set_transagain(device);

    if (_mp->recv_pending) {
        kdRecvFrame();
    }
    return seq;
}

Sint32 kdSetResetPattern(Sint32 port)
{
    Sint32 seq = mpdrv_set_hreset(port);

    if (_mp->recv_pending) {
        kdRecvFrame();
    }
    return seq;
}

void kdGetVersion(Sint32 *major, Sint32 *minor)
{
    mpdrv_get_version(major, minor);
}

Sint32 kdIsChanged(Sint32 device)
{
    return mpdrv_ischanged(device);
}

void kdRecognizeChanged(Sint32 device)
{
    mpdrv_recognize(device);
}

const Uint8 *kdDevGetFD(const KDS_DEVICEINFO *dev, Uint32 func)
{
    Sint32 word = 0;
    Uint32 bit;
    Sint32 i;
    const Uint8 *fd = dev->fd[0];
    Uint32 function = dev->function;

    for (i = 0, bit = 0x80000000; i < 32; i++, bit >>= 1) {
        if (function & bit) {
            if (func == bit) {
                return fd;
            }
            word++;
            fd = dev->fd[word];
        }
    }
    return NULL;
}

Sint32 kdDevGetControllerInfo(KDS_CONTROLLERINFO *sinfo,
                              const KDS_DEVICEINFO *info)
{
    const Uint8 *fd = kdDevGetFD(info, KDD_FUNC_CONTROLLER);

    if (!fd) {
        return -1;
    }
    sinfo->button = ((Uint32)fd[0] << 24) | ((Uint32)fd[1] << 16) |
                    ((Uint32)fd[2] << 8) | fd[3];
    return 0;
}

Sint32 kdDevGetStorageInfo(KDS_STORAGEINFO *sinfo, const KDS_DEVICEINFO *info)
{
    const Uint8 *fd = kdDevGetFD(info, KDD_FUNC_STORAGE);

    if (!fd) {
        return -1;
    }
    sinfo->partition = fd[0] + 1;
    sinfo->blocksize = (fd[1] + 1) * 32;
    sinfo->writecount = (fd[2] >> 4) & 15;
    sinfo->readcount = fd[2] & 15;
    sinfo->removable = (fd[3] >> 7) & 1;
    return 0;
}

Sint32 kdDevGetSIPInfo(KDS_SIPINFO *sinfo, const KDS_DEVICEINFO *info)
{
    const Uint8 *fd = kdDevGetFD(info, KDD_FUNC_SOUNDINPUT);

    if (!fd) {
        return -1;
    }
    sinfo->function = ((Uint32)fd[0] << 24) | ((Uint32)fd[1] << 16) |
                      ((Uint32)fd[2] << 8) | fd[3];
    return 0;
}

Sint32 kdDevGetLCDInfo(KDS_LCDINFO *sinfo, const KDS_DEVICEINFO *info)
{
    const Uint8 *fd = kdDevGetFD(info, KDD_FUNC_LCD);

    if (!fd) {
        return -1;
    }
    sinfo->partition = fd[0] + 1;
    sinfo->blocksize = (fd[1] + 1) * 32;
    sinfo->writecount = (fd[2] >> 4) & 15;
    sinfo->direction = (fd[3] >> 6) & 3;
    sinfo->normally = (fd[3] >> 5) & 1;
    return 0;
}

Sint32 kdDevGetTimerInfo(KDS_TIMERINFO *sinfo, const KDS_DEVICEINFO *info)
{
    const Uint8 *fd = kdDevGetFD(info, KDD_FUNC_TIMER);

    if (!fd) {
        return -1;
    }
    sinfo->write_timer = fd[0];
    sinfo->read_timer = fd[1];
    sinfo->button = fd[2];
    sinfo->alarm = fd[3];
    return 0;
}

Sint32 kdDevGetGunInfo(KDS_GUNINFO *sinfo, const KDS_DEVICEINFO *info)
{
    const Uint8 *fd = kdDevGetFD(info, KDD_FUNC_ARGUN);

    if (!fd) {
        return -1;
    }
    sinfo->button = ((Uint32)fd[0] << 24) | ((Uint32)fd[1] << 16) |
                    ((Uint32)fd[2] << 8) | fd[3];
    return 0;
}

Sint32 kdDevGetVibrationInfo(KDS_VIBRATIONINFO *sinfo,
                             const KDS_DEVICEINFO *info)
{
    const Uint8 *fd = kdDevGetFD(info, KDD_FUNC_VIBRATION);

    if (!fd) {
        return -1;
    }
    sinfo->vn = fd[0];
    sinfo->se = fd[1];
    return 0;
}

/* set the driver mode of type to mode, and return 0. KDD_MODE_OPTIMIZE and an
 * unknown type set nothing and return -1. KDD_MODE_GUNMODE sets its mode, then
 * returns -1 */
Sint32 kdSetDriverMode(Sint32 type, Sint32 mode)
{
    switch (type) {
    case KDD_MODE_NOPCOUNT:
        mpdrv_nopcount(mode);
        break;
    case KDD_MODE_TIMEOUT:
        mpdrv_timeout(mode);
        break;
    case KDD_MODE_DREQCOUNT:
        mpdrv_dreqcount(mode);
        break;
    case KDD_MODE_SPEED:
        mpdrv_speed(mode);
        break;
    case KDD_MODE_DEBUG:
        mpdrv_debugmode(mode);
        break;
    case KDD_MODE_DEVENDIAN:
        mpdrv_swapmode(mode);
        break;
    case KDD_MODE_OPTIMIZE:
        return -1;
    case KDD_MODE_ROOTPLAY:
        mpdrv_rootplay(mode);
        break;
    case KDD_MODE_GUNMODE:
        mpdrv_gunmode(mode);
        /* fall through to the default's -1 */
    default:
        return -1;
    }
    return 0;
}

Sint32 kdGetDriverMode(Sint32 type)
{
    switch (type) {
    case KDD_MODE_NOPCOUNT:
        return mpdrv_get_nopcount();
    case KDD_MODE_TIMEOUT:
        return mpdrv_get_timeout();
    case KDD_MODE_DREQCOUNT:
        return mpdrv_get_dreqcount();
    case KDD_MODE_SPEED:
        return mpdrv_get_speed();
    case KDD_MODE_DEBUG:
        return mpdrv_get_debugmode();
    case KDD_MODE_DEVENDIAN:
        return mpdrv_get_swapmode();
    case KDD_MODE_OPTIMIZE:
        return -1;
    case KDD_MODE_ROOTPLAY:
        return mpdrv_get_rootplay();
    case KDD_MODE_GUNMODE:
        return mpdrv_get_gunmode();
    default:
        return -1;
    }
}

const KDS_SYSTEMINFO *kdGetSystemInfo(void)
{
    return &kdgSystemInfo;
}

const KDS_GUNMODEINFO *kdGetGunModeInfo(void)
{
    return mpdrv_get_gunmodeinfo();
}

Sint32 kdSetCallback(Sint32 device, KD_CALLBACK func, Sint32 seqno)
{
    struct kd_callback *callback = &kdgCbTbl[(device * 2) + _mp->received];

    callback->func = func;
    callback->seqno = seqno;
    callback->device = device;
    return 0;
}

/* while a call below holds the semaphore, no frame is sent or received, so
 * _mp->received still selects its command's slot when it registers the
 * callback */
Sint32 kdSetDeviceRequestEx(Sint32 device, KD_CALLBACK cbfunc)
{
    Sint32 seq;

    if (!tas(_mp_SemaphoreAddress)) {
        mpdrv_dpf(no_semaphore);
        return -2;
    }
    seq = mpdrv_set_dreq_ns(device);
    if (seq > 0) {
        kdSetCallback(device, cbfunc, seq);
    }
    *_mp_SemaphoreAddress = 0;
    if (_mp->recv_pending) {
        kdRecvFrame();
    }
    return seq;
}

Sint32 kdSetAllStatusRequestEx(Sint32 device, KD_CALLBACK cbfunc)
{
    Sint32 seq;

    if (!tas(_mp_SemaphoreAddress)) {
        mpdrv_dpf(no_semaphore);
        return -2;
    }
    seq = mpdrv_set_allstatreq_ns(device);
    if (seq > 0) {
        kdSetCallback(device, cbfunc, seq);
    }
    *_mp_SemaphoreAddress = 0;
    if (_mp->recv_pending) {
        kdRecvFrame();
    }
    return seq;
}

Sint32 kdSetDeviceResetEx(Sint32 device, KD_CALLBACK cbfunc)
{
    Sint32 seq;

    if (!tas(_mp_SemaphoreAddress)) {
        mpdrv_dpf(no_semaphore);
        return -2;
    }
    seq = mpdrv_set_dreset_ns(device);
    if (seq > 0) {
        kdSetCallback(device, cbfunc, seq);
    }
    *_mp_SemaphoreAddress = 0;
    if (_mp->recv_pending) {
        kdRecvFrame();
    }
    return seq;
}

Sint32 kdSetDeviceKillEx(Sint32 device, KD_CALLBACK cbfunc)
{
    Sint32 seq;

    if (!tas(_mp_SemaphoreAddress)) {
        mpdrv_dpf(no_semaphore);
        return -2;
    }
    seq = mpdrv_set_dkill_ns(device);
    if (seq > 0) {
        kdSetCallback(device, cbfunc, seq);
    }
    *_mp_SemaphoreAddress = 0;
    if (_mp->recv_pending) {
        kdRecvFrame();
    }
    return seq;
}

Sint32 kdSetGetConditionEx(Sint32 device, Sint32 func, KD_CALLBACK cbfunc)
{
    Sint32 seq;

    if (!tas(_mp_SemaphoreAddress)) {
        mpdrv_dpf(no_semaphore);
        return -2;
    }
    seq = mpdrv_set_getcond_ns(device, func);
    if (seq > 0) {
        kdSetCallback(device, cbfunc, seq);
    }
    *_mp_SemaphoreAddress = 0;
    if (_mp->recv_pending) {
        kdRecvFrame();
    }
    return seq;
}

Sint32 kdSetGetMediaInfoEx(Sint32 device, Sint32 func, Sint32 pt,
                           KD_CALLBACK cbfunc)
{
    Sint32 seq;

    if (!tas(_mp_SemaphoreAddress)) {
        mpdrv_dpf(no_semaphore);
        return -2;
    }
    seq = mpdrv_set_getminfo_ns(device, func, pt);
    if (seq > 0) {
        kdSetCallback(device, cbfunc, seq);
    }
    *_mp_SemaphoreAddress = 0;
    if (_mp->recv_pending) {
        kdRecvFrame();
    }
    return seq;
}

Sint32 kdSetBlockReadEx(Sint32 device, Sint32 func, Sint32 pt, Sint32 phase,
                        Sint32 block, KD_CALLBACK cbfunc)
{
    Sint32 seq;

    if (!tas(_mp_SemaphoreAddress)) {
        mpdrv_dpf(no_semaphore);
        return -2;
    }
    seq = mpdrv_set_bread_ns(device, func, pt, phase, block);
    if (seq > 0) {
        kdSetCallback(device, cbfunc, seq);
    }
    *_mp_SemaphoreAddress = 0;
    if (_mp->recv_pending) {
        kdRecvFrame();
    }
    return seq;
}

Sint32 kdSetBlockWriteEx(Sint32 device, Sint32 func, Sint32 pt, Sint32 phase,
                         Sint32 block, const void *src, Sint32 size,
                         KD_CALLBACK cbfunc)
{
    Sint32 seq;

    if (!tas(_mp_SemaphoreAddress)) {
        mpdrv_dpf(no_semaphore);
        return -2;
    }
    seq = mpdrv_set_bwrite_ns(device, func, pt, phase, block, src, size);
    if (seq > 0) {
        kdSetCallback(device, cbfunc, seq);
    }
    *_mp_SemaphoreAddress = 0;
    if (_mp->recv_pending) {
        kdRecvFrame();
    }
    return seq;
}

Sint32 kdSetGetLastErrorEx(Sint32 device, Sint32 func, Sint32 pt, Sint32 phase,
                           Sint32 block, KD_CALLBACK cbfunc)
{
    Sint32 seq;

    if (!tas(_mp_SemaphoreAddress)) {
        mpdrv_dpf(no_semaphore);
        return -2;
    }
    seq = mpdrv_set_getlerr_ns(device, func, pt, phase, block);
    if (seq > 0) {
        kdSetCallback(device, cbfunc, seq);
    }
    *_mp_SemaphoreAddress = 0;
    if (_mp->recv_pending) {
        kdRecvFrame();
    }
    return seq;
}

Sint32 kdSetARControlEx(Sint32 device, const void *buf, KD_CALLBACK cbfunc)
{
    Sint32 seq;

    if (!tas(_mp_SemaphoreAddress)) {
        mpdrv_dpf(no_semaphore);
        return -2;
    }
    seq = mpdrv_set_arctrl_ns(device, buf);
    if (seq > 0) {
        kdSetCallback(device, cbfunc, seq);
    }
    *_mp_SemaphoreAddress = 0;
    if (_mp->recv_pending) {
        kdRecvFrame();
    }
    return seq;
}

Sint32 kdSetFT4ControlEx(Sint32 device, const void *buf, KD_CALLBACK cbfunc)
{
    Sint32 seq;

    if (!tas(_mp_SemaphoreAddress)) {
        mpdrv_dpf(no_semaphore);
        return -2;
    }
    seq = mpdrv_set_ft4ctrl_ns(device, buf);
    if (seq > 0) {
        kdSetCallback(device, cbfunc, seq);
    }
    *_mp_SemaphoreAddress = 0;
    if (_mp->recv_pending) {
        kdRecvFrame();
    }
    return seq;
}

Sint32 kdSetSetConditionEx(Sint32 device, Sint32 func, const void *buf,
                           Sint32 size, KD_CALLBACK cbfunc)
{
    Sint32 seq;

    if (!tas(_mp_SemaphoreAddress)) {
        mpdrv_dpf(no_semaphore);
        return -2;
    }
    seq = mpdrv_set_setcond_ns(device, func, buf, size);
    if (seq > 0) {
        kdSetCallback(device, cbfunc, seq);
    }
    *_mp_SemaphoreAddress = 0;
    if (_mp->recv_pending) {
        kdRecvFrame();
    }
    return seq;
}
