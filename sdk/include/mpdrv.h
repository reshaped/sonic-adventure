/* Maple bus driver under Kaede, as far as the sources use it */
#ifndef MPDRV_H
#define MPDRV_H
#include <kaede.h>
#include <sg_xpt.h>

#define MPDRV_PORTS 4
/* how many device numbers each port takes. its root device takes the first */
#define PORT_DEVICES (KDD_DEV_MAX / MPDRV_PORTS)

/* standing commands. a frame outside gun mode sends each enabled device its
 * standing command, unless another is queued for it. MPDRV_CMD_NONE sends
 * nothing */
#define MPDRV_CMD_NONE 0
#define MPDRV_CMD_DREQ 1
#define MPDRV_CMD_ALLSTATREQ 2
#define MPDRV_CMD_DRESET 3
#define MPDRV_CMD_DKILL 4
#define MPDRV_CMD_GETCOND 9
/* the standing command of an expansion device the driver takes as absent:
 * its socket empty, the device removed or unanswered past dreqcount, or any
 * port in gun mode. the frames send it nothing */
#define MPDRV_CMD_ABSENT 0xff
/* commands that only their own mpdrv_set_ calls queue, never standing */
#define MPDRV_CMD_GETMINFO 10
#define MPDRV_CMD_BREAD 11
#define MPDRV_CMD_BWRITE 12
#define MPDRV_CMD_GETLERR 13
#define MPDRV_CMD_SETCOND 14
#define MPDRV_CMD_FT4CTRL 15
#define MPDRV_CMD_ARCTRL 16
#define MPDRV_CMD_TRANSAGAIN 0xfc

/* a device as the driver keeps it. Kaede copies its state and response to the
 * KDS_DEVICE that kdGetDevice returns */
struct maple_device {
    Uint8 on;
    Uint8 old;
    Uint8 connect;
    Uint8 remove;
    Uint8 command; /* the standing command */
    Uint8 old_command; /* command before the last response updated it */
    Uint8 queued; /* the command queued for the next frame */
    Uint8 sent; /* the command sent in the last frame */
    Uint8 response;
    Uint8 dst_ap;
    Uint8 src_ap;
    Uint8 datasize;
    Uint8 changed; /* a response showed it connected or removed */
    /* set by mpdrv_recognize. the next response clears it and changed */
    Uint8 recognized;
    Uint8 unk_0e[2];
    void *recvdata;
    Sint32 errcode;
    /* the sequence numbers of the command queued, the command sent, and the
     * last command whose frame was received */
    Sint32 queued_seq;
    Sint32 sent_seq;
    Sint32 done_seq;
    /* the count of an expansion device's connection and of each device
     * request since then that brought no status. past dreqcount the device
     * goes MPDRV_CMD_ABSENT and the count clears, as it does at removal */
    Uint32 retries;
    Uint8 unk_28[4];
};

/* driver state. a four-character tag starts each part: 'PORT', 'DEV ', 'INFO',
 * 'ENBL', 'STAT', 'RADR', 'TOUT', 'DRQC', 'FUNC' and 'END ' */
struct mp_context {
    Uint32 tag_port;
    KDS_PORT port[MPDRV_PORTS];
    Uint32 tag_dev;
    struct maple_device device[KDD_DEV_MAX];
    Uint32 tag_info;
    KDS_DEVICEINFO info[KDD_DEV_MAX];
    Uint32 tag_enbl;
    /* the devices the driver includes in its frames: 'R' for a port's root
     * device and 'E' for its first two expansion devices, or 0 */
    char enable[KDD_DEV_MAX];
    Uint32 tag_stat;
    Sint32 gunmode; /* the port bits requested for gun mode */
    Uint32 gun_ports; /* the bits in effect, copied from gunmode at each send */
    Sint32 gun_port; /* the port whose gun a gun mode frame reads */
    /* false from a frame's send until mpdrv_frame_recv_post */
    Sint32 received;
    /* the send and receive buffers passed to kdInitSystem, rounded up to 32
     * bytes, as addresses in P2, the uncached area */
    Uint32 sendbuf;
    Uint32 recvbuf;
    /* the last command in the frame being built, last_cmd_words long; the
     * next is written after it */
    Uint32 *last_cmd;
    Uint8 unk_fc8[4];
    Uint8 *send_end; /* the end of the two frame buffers */
    Sint32 last_cmd_words;
    Sint32 send_size; /* the bytes of the frame being built */
    /* the next command's sequence number, which counts from 1 to 0x7fffffff
     * and wraps to 1 */
    Sint32 next_seq;
    Uint32 tag_radr;
    /* where each device's response lands, 1 KB of recvbuf for each. a
     * command queued while recv_side is side takes recv_data[side]. side 0's
     * lie in recvbuf's first 24 KB, and side 1's in the next */
    Uint32 *recv_data[2][KDD_DEV_MAX];
    /* two frame buffers: one is built while the other is sent */
    Uint8 *send_frame[2];
    void *last_send;
    Sint32 recv_side;
    Sint32 send_side;
    Uint32 tag_tout;
    Sint32 timeout;
    Sint32 swapmode;
    Sint32 debugmode;
    Sint32 nopcount;
    Sint32 speed;
    Uint32 tag_drqc;
    Uint32 dreqcount;
    /* the frames a port's root device can leave unanswered, in its
     * playcount, before the port reads as empty */
    Uint32 rootplay;
    Sint32 optimize;
    /* the count of NOPs a frame outside gun mode gets after the commands
     * queued into it, before its standing commands. mpdrv_optimize sets
     * the count */
    Sint32 nops;
    /* set when a receive could not take the semaphore; Kaede receives the frame
     * after its next command */
    Sint32 recv_pending;
    Uint32 unk_10e4;
    Uint32 tag_func;
    volatile Sint32 *reg; /* the Maple bus registers */
    Uint32 tag_end;
};
/* the Maple system register's timeout field, in bits 16 and up, as
 * mp_context.timeout keeps it */
#define MAPLE_TIMEOUT(count) ((Uint32)(count) << 16)
/* the timeout count the bus starts with. syHwInit sets it, mpdrv_init_globals
 * starts the driver with it, and mpdrv_exit leaves it on the idle bus */
#define START_TIMEOUT 50000

extern struct mp_context _mp_Context;
extern struct mp_context *_mp;
extern KDS_GUNMODEINFO _mp_GunModeInfo;
/* each port's root device number */
extern const Sint32 _mp_RootDeviceNo[MPDRV_PORTS];
/* each device's Maple address */
extern const Uint8 _mp_Per2APTable[KDD_DEV_MAX];
/* a Maple address: the port in bits 6 and 7, then ADDRESS_ROOT for a root
 * device, or an expansion device's socket bit in ADDRESS_SOCKETS. the host's
 * address on a port is the port's bits alone */
#define ADDRESS_PORT 0xc0
#define ADDRESS_ROOT 0x20
#define ADDRESS_SOCKETS 0x1f
extern const Sint32 _mp_MajorVersion;
extern const Sint32 _mp_MinorVersion;
/* the semaphore byte's address. tas takes the byte, and 0 frees it. only its
 * holder builds, sends or receives a frame */
extern char *_mp_SemaphoreAddress;

/* start the driver on the buffers at sendbuf and recvbuf, each address first
 * rounded up to a multiple of 32, and return 0. sendsize and recvsize go
 * unread. the first frame holds a device request for each enabled port's root
 * device */
Sint32 mpdrv_init(void *sendbuf, Sint32 sendsize, void *recvbuf,
                  Sint32 recvsize);
void mpdrv_init_globals(void);
/* stop the bus, and return 0. mpdrv_exit first waits up to 100000 counts of
 * syTmrGetCount for a frame in transfer. it then leaves the bus disabled and
 * idle, and waits up to 17000 counts for the bus to stop */
Sint32 mpdrv_exit(void);
/* in debug mode, format the text as vsprintf does and pass it to
 * _mpdrv_dbg_out, unless that is NULL. the formatted text must fit 256 bytes,
 * its NUL included */
void mpdrv_dpf(const char *format, ...);
/* addr as a physical address, its top four bits cleared, for the Maple DMA;
 * mpdrv_p2v returns addr as it is */
void *mpdrv_v2p(void *addr);
void *mpdrv_p2v(void *addr);
/* reverse the byte order of each of count words at words, in place. count
 * must be 1 or more. returns 0 */
Sint32 mpdrv_swaplong(Uint32 *words, Sint32 count);

/* mpdrv_frame_send sends the queued commands as a frame, starts the next
 * frame in the other buffer, and returns 0. mpdrv_frame_recv_pre takes the
 * semaphore to receive the frame sent, and returns 0. mpdrv_frame_recv_main
 * then reads the responses, and mpdrv_frame_recv_post marks the frame
 * received and frees the semaphore, which stays taken until then.
 * mpdrv_frame_send returns -3 until the frame it last sent is received, and
 * mpdrv_frame_recv_pre returns -3 once it is. both return -1 while the bus is
 * busy, and -2 when the semaphore is taken */
Sint32 mpdrv_frame_send(void);
Sint32 mpdrv_frame_recv_pre(void);
Sint32 mpdrv_frame_recv_main(void);
Sint32 mpdrv_frame_recv_post(void);
void *mpdrv_get_lastsendaddr(void);
/* read a port's presence bits from its root device's response.
 * mpdrv_check_port reads a normal frame's. in a gun mode frame,
 * mpdrv_check_port_gun reads a gun port's, and mpdrv_check_port_dmy marks
 * any other port empty. mpdrv_check_port and mpdrv_check_port_gun return -1
 * and mark the port empty once its root device has left more than rootplay
 * frames unanswered, and return 0 otherwise. mpdrv_make_devicedata then
 * reads each enabled device's response */
Sint32 mpdrv_check_port(Sint32 port);
Sint32 mpdrv_check_port_gun(Sint32 port);
Sint32 mpdrv_check_port_dmy(Sint32 port);
Sint32 mpdrv_make_devicedata(Sint32 device, Sint32 port);

void mpdrv_timeout(Sint32 value);
void mpdrv_swapmode(Sint32 value);
void mpdrv_nopcount(Sint32 value);
void mpdrv_speed(Sint32 value);
void mpdrv_debugmode(Sint32 value);
void mpdrv_dreqcount(Sint32 value);
void mpdrv_optimize(Sint32 value);
void mpdrv_rootplay(Sint32 value);
void mpdrv_gunmode(Sint32 value);
Sint32 mpdrv_get_timeout(void);
Sint32 mpdrv_get_swapmode(void);
Sint32 mpdrv_get_nopcount(void);
Sint32 mpdrv_get_speed(void);
Sint32 mpdrv_get_debugmode(void);
Sint32 mpdrv_get_dreqcount(void);
Sint32 mpdrv_get_rootplay(void);
/* gun_ports, the ports in gun mode for the frame out, not gunmode */
Sint32 mpdrv_get_gunmode(void);

const struct maple_device *mpdrv_get_device(Sint32 device);
const KDS_DEVICEINFO *mpdrv_get_deviceinfo(Sint32 device);
const KDS_PORT *mpdrv_get_portstatus(Sint32 port);
Sint32 mpdrv_stat(Sint32 device, Sint32 seq);
Sint32 mpdrv_ischanged(Sint32 device);
void mpdrv_recognize(Sint32 device);
const KDS_GUNMODEINFO *mpdrv_get_gunmodeinfo(void);
void mpdrv_get_version(Sint32 *major, Sint32 *minor);

/* queue command for device in the frame being built, and return its sequence
 * number, or -1 when the device has a command queued. device must be below
 * KDD_DEV_MAX, the caller must hold the semaphore, and the frame must have
 * room for the command and its NOPs.
 *
 * the command's first word holds the device's port and the length, and its
 * second the physical address of the device's recv_data. its third, the
 * command word, holds the command, both Maple addresses and the length.
 * count words from data and extra_count from extra follow. while
 * _mp->swapmode is 0, each word of data is byte-reversed. while it is 1,
 * extra is read as bytes, the most significant first. while _mp->nopcount is
 * above 0, a command to the same port as the command, pattern or NOP before it
 * goes after nopcount NOPs. a NOP counts as port A */
Sint32 mpdrv_setcmd(Sint32 device, Uint8 command, const void *data,
                    Sint32 count, const void *extra, Sint32 extra_count);

/* commands, each queued for the next frame through mpdrv_setcmd, whose result
 * each returns. each takes the semaphore around it, or returns -2 when the
 * semaphore is taken; an _ns variant leaves the semaphore to its caller */
Sint32 mpdrv_set_dreq(Sint32 device);
Sint32 mpdrv_set_allstatreq(Sint32 device);
Sint32 mpdrv_set_dreset(Sint32 device);
Sint32 mpdrv_set_dkill(Sint32 device);
Sint32 mpdrv_set_getcond(Sint32 device, Sint32 func);
Sint32 mpdrv_set_getminfo(Sint32 device, Sint32 func, Sint32 pt);
Sint32 mpdrv_set_bread(Sint32 device, Sint32 func, Sint32 pt, Sint32 phase,
                       Sint32 block);
Sint32 mpdrv_set_bwrite(Sint32 device, Sint32 func, Sint32 pt, Sint32 phase,
                        Sint32 block, const void *src, Sint32 size);
Sint32 mpdrv_set_getlerr(Sint32 device, Sint32 func, Sint32 pt, Sint32 phase,
                         Sint32 block);
Sint32 mpdrv_set_arctrl(Sint32 device, const void *buf);
Sint32 mpdrv_set_ft4ctrl(Sint32 device, const void *buf);
Sint32 mpdrv_set_setcond(Sint32 device, Sint32 func, const void *buf,
                         Sint32 size);
Sint32 mpdrv_set_transagain(Sint32 device);
Sint32 mpdrv_set_dreq_ns(Sint32 device);
Sint32 mpdrv_set_allstatreq_ns(Sint32 device);
Sint32 mpdrv_set_dreset_ns(Sint32 device);
Sint32 mpdrv_set_dkill_ns(Sint32 device);
Sint32 mpdrv_set_getcond_ns(Sint32 device, Sint32 func);
Sint32 mpdrv_set_getminfo_ns(Sint32 device, Sint32 func, Sint32 pt);
Sint32 mpdrv_set_bread_ns(Sint32 device, Sint32 func, Sint32 pt, Sint32 phase,
                          Sint32 block);
Sint32 mpdrv_set_bwrite_ns(Sint32 device, Sint32 func, Sint32 pt, Sint32 phase,
                           Sint32 block, const void *src, Sint32 size);
Sint32 mpdrv_set_getlerr_ns(Sint32 device, Sint32 func, Sint32 pt, Sint32 phase,
                            Sint32 block);
Sint32 mpdrv_set_arctrl_ns(Sint32 device, const void *buf);
Sint32 mpdrv_set_ft4ctrl_ns(Sint32 device, const void *buf);
Sint32 mpdrv_set_setcond_ns(Sint32 device, Sint32 func, const void *buf,
                            Sint32 size);
Sint32 mpdrv_set_transagain_ns(Sint32 device);
/* these write the frame being built and return 0. mpdrv_insertnop adds count
 * NOPs. mpdrv_set_end_ns marks the frame's last command as its last, and
 * mpdrv_set_end does so under the semaphore, or returns -2 when it is taken.
 * mpdrv_hreset adds port's reset pattern, and mpdrv_set_hreset and
 * mpdrv_set_hreset_ns only call it, without the semaphore. mpdrv_sdckbenable
 * adds the pattern that selects the port a gun mode frame reads, and
 * mpdrv_sdckbdisable the one that ends gun mode */
Sint32 mpdrv_insertnop(Sint32 count);
Sint32 mpdrv_set_end(void);
Sint32 mpdrv_set_end_ns(void);
Sint32 mpdrv_hreset(Sint32 port);
Sint32 mpdrv_set_hreset(Sint32 port);
Sint32 mpdrv_set_hreset_ns(Sint32 port);
Sint32 mpdrv_sdckbenable(Sint32 port);
Sint32 mpdrv_sdckbdisable(Sint32 port);

#endif
