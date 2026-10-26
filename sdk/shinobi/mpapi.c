/* the Maple bus driver's start and stop, its frames' sending and receiving,
 * and its modes */
#include <kaede.h>
#include <machine.h>
#include <mpdrv.h>
#include <sg_xpt.h>
#include <shinobi.h>
#include <stdarg.h>
#include <stdio.h>

/* Maple bus registers, as word indices from _mp->reg. MAPLE_START reads 1
 * while a DMA runs, and a 1 written to it starts one */
enum {
    MAPLE_DMA_TABLE = 1,
    MAPLE_TRIGGER = 4,
    MAPLE_ENABLE = 5,
    MAPLE_START = 6,
    MAPLE_SYSTEM = 32,
    MAPLE_PROTECTION = 35
};

/* a MAPLE_PROTECTION value: the register's key, then the first and last
 * megabyte a DMA may reach, each as bits 20 to 26 of its address */
#define PROTECTION(key, first, last)                                           \
    (((Uint32)(key) << 16) | ((first) << 8) | (last))
/* the protection the driver sets: megabytes 0x40 to 0x4f, main memory from
 * 0x0c000000 to 0x0cffffff */
#define MAIN_MEMORY_ONLY PROTECTION(0x6155, 0x40, 0x4f)
/* the DMA table address mpdrv_exit sets on the idle bus, the start of the
 * last 64 KB of main memory */
#define IDLE_MEMORY 0x0cff0000

/* the boundary mpdrv_init rounds each buffer up to */
#define BUFFER_ALIGN 32
/* the base of the SH-4's P2 area, through which memory is reached uncached */
#define P2 0xa0000000

/* how long mpdrv_exit waits for a frame in transfer, and then for the bus
 * to stop, in CPU timer counts */
#define FRAME_WAIT 100000
#define STOP_WAIT 17000

static Sint32 make_gun_frame(void);
static Sint32 make_full_frame(void);
static Sint32 receive_frame(void);
static Sint32 receive_gun_frame(void);

/* each port's root device number, as _mp_RootDeviceNo holds it */
static const Sint32 mp_roots[MPDRV_PORTS] = {0, 6, 12, 18};
/* where mpdrv_dpf sends its text in debug mode; NULL sends it nowhere */
void (*_mpdrv_dbg_out)(const char *str) = NULL;
static char debug_text[256];

Sint32 mpdrv_init(void *sendbuf, Sint32 sendsize, void *recvbuf,
                  Sint32 recvsize)
{
    Sint32 port;

    _mp = &_mp_Context;
    _mp->sendbuf = (Uint32)sendbuf;
    _mp->recvbuf = (Uint32)recvbuf;
    if (_mp->sendbuf & (BUFFER_ALIGN - 1)) {
        _mp->sendbuf += BUFFER_ALIGN;
        _mp->sendbuf &= ~(BUFFER_ALIGN - 1);
    }
    if (_mp->recvbuf & (BUFFER_ALIGN - 1)) {
        _mp->recvbuf += BUFFER_ALIGN;
        _mp->recvbuf &= ~(BUFFER_ALIGN - 1);
    }
    _mp->sendbuf |= P2;
    _mp->recvbuf |= P2;
    mpdrv_init_globals();
    for (port = 0; port < MPDRV_PORTS; port++) {
        if (_mp->enable[_mp_RootDeviceNo[port]]) {
            mpdrv_set_dreq_ns(_mp_RootDeviceNo[port]);
        }
    }
    mpdrv_set_end_ns();
    _mp->received = true;
    _mp->reg[MAPLE_PROTECTION] = MAIN_MEMORY_ONLY;
    _mp->reg[MAPLE_TRIGGER] = 0;
    _mp->reg[MAPLE_SYSTEM] = _mp->timeout | _mp->speed;
    _mp->reg[MAPLE_ENABLE] = 1;
    return 0;
}

static Sint32 make_frame(void)
{
    if (_mp->gun_ports == 0) {
        return make_full_frame();
    }
    return make_gun_frame();
}

/* finish a gun mode frame for _mp->gun_port. the port's root device gets its
 * standing command, after a kill for the device in the port's first expansion
 * socket. it gets neither while it has a command queued, or for
 * MPDRV_CMD_NONE and MPDRV_CMD_ABSENT. the pattern that selects the port for
 * the gun's read comes last */
static Sint32 make_gun_frame(void)
{
    Sint32 root;
    Sint32 func;
    /* @hack redundant: device and info set to device 0's, which root's
     * replace. SHC computes these first addresses in R5 and R6 (0x8c1241fa to
     * 0x8c124202 in 1ST_READ.BIN) and adds root's offsets to them (ADD R3,R5 at
     * 0x8c124212, ADD R2,R6 at 0x8c12421e); declared bare, 20 of 111 words
     * differ in each image */
    struct maple_device *device = &_mp->device[0];
    KDS_DEVICEINFO *info = &_mp->info[0];

    root = mp_roots[_mp->gun_port];
    device = &_mp->device[root];
    info = &_mp->info[root];
    if (!device->queued) {
        switch (device->command) {
        case MPDRV_CMD_DREQ:
            mpdrv_set_dkill_ns(root + 1);
            mpdrv_set_dreq_ns(root);
            break;
        case MPDRV_CMD_ALLSTATREQ:
            mpdrv_set_dkill_ns(root + 1);
            mpdrv_set_allstatreq_ns(root);
            break;
        case MPDRV_CMD_DRESET:
            mpdrv_set_dkill_ns(root + 1);
            mpdrv_set_dreset_ns(root);
            break;
        case MPDRV_CMD_DKILL:
            mpdrv_set_dkill_ns(root + 1);
            mpdrv_set_dkill_ns(root);
            break;
        case MPDRV_CMD_GETCOND:
            func = info->function;
            if (func & KDD_FUNC_CONTROLLER) {
                func = KDD_FUNC_CONTROLLER;
            }
            mpdrv_set_dkill_ns(root + 1);
            mpdrv_set_getcond_ns(root, func);
            break;
        /* @hack redundant: case labels for the commands the frame sends nothing
         * for, which keep the image's compare of the command with each (CMP/EQ
         * #0,R0 at 0x8c124228, and CMP/EQ #10,R0 at 0x8c124242 to CMP/EQ R1,R0
         * at 0x8c124254, in 1ST_READ.BIN); without them, the function runs 36
         * bytes short and 79 of 111 words differ in each image */
        case MPDRV_CMD_NONE:
        case MPDRV_CMD_GETMINFO:
        case MPDRV_CMD_BREAD:
        case MPDRV_CMD_BWRITE:
        case MPDRV_CMD_GETLERR:
        case MPDRV_CMD_ABSENT:
            break;
        }
    }
    mpdrv_sdckbenable(_mp->gun_port);
    mpdrv_set_end_ns();
    return 0;
}

/* finish a frame outside gun mode. after the commands already queued, it adds
 * _mp->nops NOPs, then the standing command of each enabled device that has
 * no command queued. MPDRV_CMD_NONE and MPDRV_CMD_ABSENT send nothing */
static Sint32 make_full_frame(void)
{
    Sint32 i;
    Sint32 func;
    struct maple_device *device = &_mp->device[0];
    KDS_DEVICEINFO *info = &_mp->info[0];

    if (_mp->nops > 0) {
        mpdrv_insertnop(_mp->nops);
    }
    for (i = 0; i < KDD_DEV_MAX; i++, device++, info++) {
        if (!_mp->enable[i] || device->queued) {
            continue;
        }
        switch (device->command) {
        case MPDRV_CMD_DREQ:
            mpdrv_set_dreq_ns(i);
            break;
        case MPDRV_CMD_ALLSTATREQ:
            mpdrv_set_allstatreq_ns(i);
            break;
        case MPDRV_CMD_DRESET:
            mpdrv_set_dreset_ns(i);
            break;
        case MPDRV_CMD_DKILL:
            mpdrv_set_dkill_ns(i);
            break;
        case MPDRV_CMD_GETCOND:
            func = info->function;
            if (func & KDD_FUNC_CONTROLLER) {
                func = KDD_FUNC_CONTROLLER;
            }
            mpdrv_set_getcond_ns(i, func);
        /* @hack spelling: no break, so case MPDRV_CMD_GETCOND falls through to
         * the empty cases' break, which leaves R3 to call mpdrv_set_end_ns
         * (0x8c1243b0, 0x8c1243b2 in 1ST_READ.BIN); with a break of its own,
         * R2 does, and 2 of 109 words differ in each image */
        /* @hack redundant: case labels for the commands the frame sends nothing
         * for, which keep the image's compare of the command with each (CMP/EQ
         * #0,R0 at 0x8c12433c, and CMP/EQ #10,R0 at 0x8c124354 to CMP/EQ R1,R0
         * at 0x8c124366, in 1ST_READ.BIN); without them, the function runs 32
         * bytes short and 80 of 109 words differ in each image */
        case MPDRV_CMD_NONE:
        case MPDRV_CMD_GETMINFO:
        case MPDRV_CMD_BREAD:
        case MPDRV_CMD_BWRITE:
        case MPDRV_CMD_GETLERR:
        case MPDRV_CMD_ABSENT:
            break;
        }
    }
    mpdrv_set_end_ns();
    return 0;
}

/* finish the frame being built, start its DMA, and begin the next frame in
 * the other buffer. each enabled device's queued command becomes its sent
 * one. returns 0, or a failure mpdrv.h lists */
Sint32 mpdrv_frame_send(void)
{
    Sint32 i;
    struct maple_device *device;

    if (_mp->received != true) {
        return -3;
    }
    if (_mp->reg[MAPLE_START] & 1) {
        return -1;
    }
    if (!tas(_mp_SemaphoreAddress)) {
        mpdrv_dpf("***FATAL ERROR:Cannot get semaphore!\n");
        return -2;
    }
    _mp->gun_ports = _mp->gunmode;
    make_frame();
    _mp->last_send = _mp->send_frame[_mp->send_side];
    _mp->reg[MAPLE_DMA_TABLE] =
        (Sint32)mpdrv_v2p(_mp->send_frame[_mp->send_side]);
    _mp->reg[MAPLE_START] = 1;
    _mp->last_cmd_words = 0;
    _mp->send_side ^= 1;
    _mp->last_cmd = (Uint32 *)_mp->send_frame[_mp->send_side];
    _mp->recv_side ^= 1;
    _mp->send_size = 0;
    device = &_mp->device[0];
    for (i = 0; i < KDD_DEV_MAX; i++, device++) {
        if (_mp->enable[i]) {
            device->sent = device->queued;
            device->queued = 0;
            device->sent_seq = device->queued_seq;
            device->queued_seq = 0;
        }
    }
    _mp->received = false;
    *_mp_SemaphoreAddress = 0;
    return 0;
}

Sint32 mpdrv_frame_recv_pre(void)
{
    if (_mp->received) {
        return -3;
    }
    if (_mp->reg[MAPLE_START] & 1) {
        return -1;
    }
    if (!tas(_mp_SemaphoreAddress)) {
        mpdrv_dpf("***FATAL ERROR:Cannot get semaphore!\n");
        return -2;
    }
    return 0;
}

Sint32 mpdrv_frame_recv_post(void)
{
    _mp->received = true;
    *_mp_SemaphoreAddress = 0;
    return 0;
}

Sint32 mpdrv_frame_recv_main(void)
{
    if (_mp->gun_ports == 0) {
        return receive_frame();
    }
    return receive_gun_frame();
}

static Sint32 receive_frame(void)
{
    Sint32 port;
    Sint32 i;

    for (port = 0; port < MPDRV_PORTS; port++) {
        if (!_mp->enable[_mp_RootDeviceNo[port]]) {
            continue;
        }
        mpdrv_check_port(port);
        for (i = 0; i < PORT_DEVICES; i++) {
            if (_mp->enable[(port * PORT_DEVICES) + i]) {
                mpdrv_make_devicedata((port * PORT_DEVICES) + i, port);
            }
        }
    }
    return 0;
}

static Sint32 receive_gun_frame(void)
{
    Sint32 port;
    Sint32 i;

    for (port = 0; port < MPDRV_PORTS; port++) {
        if (!_mp->enable[_mp_RootDeviceNo[port]]) {
            continue;
        }
        if (_mp->gun_ports & (1 << port)) {
            mpdrv_check_port_gun(port);
        } else {
            mpdrv_check_port_dmy(port);
        }
        for (i = 0; i < PORT_DEVICES; i++) {
            if (_mp->enable[(port * PORT_DEVICES) + i]) {
                mpdrv_make_devicedata((port * PORT_DEVICES) + i, port);
            }
        }
    }
    _mp_GunModeInfo.enable = true;
    _mp_GunModeInfo.port = _mp->gun_ports;
    _mp_GunModeInfo.current_port = _mp->gun_port;
    return 0;
}

Sint32 mpdrv_exit(void)
{
    Uint32 start = syTmrGetCount();

    while (_mp->reg[MAPLE_START] & 1) {
        if ((Sint32)syTmrDiffCount(start, syTmrGetCount()) > FRAME_WAIT) {
            break;
        }
    }
    _mp->reg[MAPLE_START] = 0;
    _mp->reg[MAPLE_ENABLE] = 0;
    _mp->reg[MAPLE_TRIGGER] = 0;
    _mp->reg[MAPLE_PROTECTION] = MAIN_MEMORY_ONLY;
    _mp->reg[MAPLE_SYSTEM] = MAPLE_TIMEOUT(START_TIMEOUT);
    _mp->reg[MAPLE_DMA_TABLE] = IDLE_MEMORY;
    start = syTmrGetCount();
    while (_mp->reg[MAPLE_START] & 1) {
        if ((Sint32)syTmrDiffCount(start, syTmrGetCount()) > STOP_WAIT) {
            break;
        }
    }
    return 0;
}

void mpdrv_dpf(const char *format, ...)
{
    va_list args;

    if (_mp->debugmode) {
        va_start(args, format);
        vsprintf(debug_text, format, args);
        va_end(args);
        if (_mpdrv_dbg_out) {
            _mpdrv_dbg_out(debug_text);
        }
    }
}

const struct maple_device *mpdrv_get_device(Sint32 device)
{
    return &_mp->device[device];
}

const KDS_DEVICEINFO *mpdrv_get_deviceinfo(Sint32 device)
{
    return &_mp->info[device];
}

const KDS_PORT *mpdrv_get_portstatus(Sint32 port)
{
    return &_mp->port[port];
}

Sint32 mpdrv_stat(Sint32 device, Sint32 seq)
{
    const struct maple_device *dev = mpdrv_get_device(device);

    /* @hack spelling: the comparison's result returned through an if, which
     * leaves MOVT R0 ahead of the RTS and a NOP in its slot (0x8c124864 to
     * 0x8c124868 in 1ST_READ.BIN); returned directly, the MOVT takes the
     * slot, the function runs 4 bytes short, and 3 of 13 words differ in each
     * image */
    if (dev->done_seq == seq) {
        return true;
    }
    return false;
}

Sint32 mpdrv_ischanged(Sint32 device)
{
    const struct maple_device *dev = mpdrv_get_device(device);

    return dev->changed;
}

void mpdrv_recognize(Sint32 device)
{
    struct maple_device *dev = (struct maple_device *)mpdrv_get_device(device);

    dev->recognized = true;
}

/* the timeout is kept shifted into MAPLE_SYSTEM's field */
void mpdrv_timeout(Sint32 value)
{
    _mp->timeout = (Sint32)MAPLE_TIMEOUT(value);
}

Sint32 mpdrv_get_timeout(void)
{
    return (Uint16)(_mp->timeout >> 16);
}

void mpdrv_swapmode(Sint32 value)
{
    _mp->swapmode = value & 1;
}

Sint32 mpdrv_get_swapmode(void)
{
    return _mp->swapmode;
}

void mpdrv_nopcount(Sint32 value)
{
    _mp->nopcount = value;
}

Sint32 mpdrv_get_nopcount(void)
{
    return _mp->nopcount;
}

void mpdrv_speed(Sint32 value)
{
    _mp->speed = value;
}

Sint32 mpdrv_get_speed(void)
{
    return _mp->speed;
}

void mpdrv_get_version(Sint32 *major, Sint32 *minor)
{
    if (major) {
        *major = _mp_MajorVersion;
    }
    if (minor) {
        *minor = _mp_MinorVersion;
    }
}

void mpdrv_debugmode(Sint32 value)
{
    _mp->debugmode = value;
}

Sint32 mpdrv_get_debugmode(void)
{
    return _mp->debugmode;
}

void mpdrv_dreqcount(Sint32 value)
{
    _mp->dreqcount = value;
}

Sint32 mpdrv_get_dreqcount(void)
{
    return _mp->dreqcount;
}

void mpdrv_optimize(Sint32 value)
{
    _mp->optimize = value;
    switch (value) {
    case KDD_OPTIMIZE_NONE:
        _mp->nops = 0;
        break;
    case KDD_OPTIMIZE_1MS:
        _mp->nops = 208;
        break;
    case KDD_OPTIMIZE_2MS:
        _mp->nops = 192;
        break;
    case KDD_OPTIMIZE_4MS:
        _mp->nops = 160;
        break;
    case KDD_OPTIMIZE_8MS:
        _mp->nops = 96;
        break;
    }
}

void *mpdrv_get_lastsendaddr(void)
{
    return _mp->last_send;
}

void mpdrv_rootplay(Sint32 value)
{
    _mp->rootplay = value;
}

Sint32 mpdrv_get_rootplay(void)
{
    return _mp->rootplay;
}

/* value takes effect at the next frame's send, which copies gunmode to
 * gun_ports */
void mpdrv_gunmode(Sint32 value)
{
    _mp->gunmode = value;
}

Sint32 mpdrv_get_gunmode(void)
{
    return _mp->gun_ports;
}

const KDS_GUNMODEINFO *mpdrv_get_gunmodeinfo(void)
{
    return &_mp_GunModeInfo;
}
