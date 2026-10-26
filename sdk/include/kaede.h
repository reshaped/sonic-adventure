/* Kaede Maple bus driver, as far as the sources use it */
#ifndef KAEDE_H
#define KAEDE_H
#include <sg_xpt.h>

/* device numbers: six for each of the four ports, the port's root device
 * first */
#define KDD_DEV_MAX 24
#define KDD_DEV_A1 1
#define KDD_DEV_A2 2
#define KDD_DEV_A3 3
#define KDD_DEV_B1 7
#define KDD_DEV_B2 8
#define KDD_DEV_C1 13
#define KDD_DEV_C2 14
#define KDD_DEV_D1 19
#define KDD_DEV_D2 20

/* function bits, as in KDS_DEVICEINFO's function */
#define KDD_FUNC_CONTROLLER 0x00000001
#define KDD_FUNC_STORAGE 0x00000002
#define KDD_FUNC_LCD 0x00000004
#define KDD_FUNC_TIMER 0x00000008
#define KDD_FUNC_SOUNDINPUT 0x00000010
#define KDD_FUNC_ARGUN 0x00000020
#define KDD_FUNC_VIBRATION 0x00000100

/* response codes, as in KDS_DEVICE's response */
#define KDD_RES_DEVICESTATUS 0x05
#define KDD_RES_DEVICEALLSTATUS 0x06
#define KDD_RES_DEVICEREPLY 0x07
#define KDD_RES_DATATRANSFER 0x08
#define KDD_RES_NORESPONSE 0xff
#define KDD_RES_TRANSMITAGAIN 0xfc
#define KDD_RES_FILEERROR 0xfb

/* driver mode types, for kdSetDriverMode and kdGetDriverMode. for
 * KDD_MODE_OPTIMIZE and an unknown type, both return -1 and set nothing.
 * kdSetDriverMode returns 0 for the others, but -1 for KDD_MODE_GUNMODE,
 * which it sets all the same */
#define KDD_MODE_OPTIMIZE 0
#define KDD_MODE_NOPCOUNT 1
#define KDD_MODE_TIMEOUT 2
#define KDD_MODE_SPEED 3
#define KDD_MODE_DREQCOUNT 4
#define KDD_MODE_DEVENDIAN 5
#define KDD_MODE_DEBUG 6
#define KDD_MODE_ROOTPLAY 7
#define KDD_MODE_GUNMODE 8

/* optimize modes, which mpdrv_optimize takes. a frame outside gun mode gets
 * 96 NOPs under KDD_OPTIMIZE_8MS, 160 under 4MS, 192 under 2MS and 208 under
 * 1MS. they follow the commands queued into the frame, and precede the
 * standing commands mpdrv.h names. NONE adds none */
#define KDD_OPTIMIZE_NONE 0
#define KDD_OPTIMIZE_8MS 1
#define KDD_OPTIMIZE_4MS 2
#define KDD_OPTIMIZE_2MS 3
#define KDD_OPTIMIZE_1MS 4

typedef struct {
    Uint8 lmon;
    Uint8 lmold;
    Uint8 lmcon;
    Uint8 lmremv;
    Uint32 playcount;
} KDS_PORT;

/* a device's state as of the last frame received */
typedef struct {
    Uint8 on;
    Uint8 old;
    Uint8 connect;
    Uint8 remove;
    Uint8 response;
    Uint8 dst_ap;
    Uint8 src_ap;
    Uint8 datasize;
    void *recvdata;
    Sint32 errcode;
    Uint32 reserved[2];
} KDS_DEVICE;

/* a device's description of itself */
typedef struct {
    Uint32 function;
    /* a 4-byte word for each function it has, three at most, ordered from
     * the highest function bit */
    Uint8 fd[3][4];
    Uint8 is_root;
    Uint8 area_code;
    Uint8 connector_dir[2];
    char product_name[32];
    char license[64];
    Uint16 stdby_pow;
    Uint16 max_pow;
} KDS_DEVICEINFO;

typedef struct {
    Uint32 button;
} KDS_CONTROLLERINFO;

#define KDD_LCD_NORMALLYWHITE 0
#define KDD_LCD_NORMALLYBLACK 1
#define KDD_LCD_HV_00 0
#define KDD_LCD_HV_01 1
#define KDD_LCD_HV_10 2
#define KDD_LCD_HV_11 3
typedef struct {
    Uint16 partition;
    Uint16 blocksize; /* bytes in a block */
    Uint16 writecount; /* write accesses a block takes */
    Uint8 direction; /* the pixel order, a KDD_LCD_HV_ value */
    Uint8 normally; /* an unwritten pixel's shade, a KDD_LCD_NORMALLY value */
} KDS_LCDINFO;

typedef struct {
    Uint16 partition;
    Uint16 blocksize;
    Uint16 writecount;
    Uint16 readcount;
    Uint16 removable;
    Uint16 reserved;
} KDS_STORAGEINFO;

typedef struct {
    Uint8 write_timer;
    Uint8 read_timer;
    Uint8 button;
    Uint8 alarm;
} KDS_TIMERINFO;

typedef struct {
    Uint32 function;
} KDS_SIPINFO;

typedef struct {
    Uint32 button;
} KDS_GUNINFO;

typedef struct {
    Uint32 vn;
    Uint32 se;
} KDS_VIBRATIONINFO;

/* driver statistics. each [2] array holds the last two frames' entries,
 * filled in turn */
typedef struct {
    Uint32 send_count;
    Uint32 recv_count;
    Uint32 vbin_count; /* vertical blank interrupts */
    Uint32 dmaend_count; /* Maple DMA end interrupts */
    Uint32 cannot_send_count;
    Uint32 blocked_count; /* vertical blanks that found a receive under way */
    Uint32 total_vover_count; /* vertical blanks during a frame's transfer */
    Uint32 error_count[4];
    Uint16 send_line[2]; /* the scan line at the frame's send */
    /* the scan line at its receive, counted on past each vertical blank
     * during its transfer */
    Uint16 recv_line[2];
    Uint16 vover_count[2]; /* vertical blanks during its transfer */
} KDS_SYSTEMINFO;

/* light gun state, set as each gun mode frame is received */
typedef struct {
    Uint32 enable; /* TRUE once a gun mode frame is received */
    Uint32 port; /* the ports in gun mode */
    Uint32 current_port; /* the port whose gun the frame read */
} KDS_GUNMODEINFO;

Sint32 kdInitSystem(void *sendbuf, void *recvbuf);
void kdExitSystem(void);
const KDS_SYSTEMINFO *kdGetSystemInfo(void);

Sint32 kdSendFrame(void);
Sint32 kdRecvFrame(void);

Sint32 kdSetDriverMode(Sint32 type, Sint32 mode);
Sint32 kdGetDriverMode(Sint32 type);

const KDS_DEVICE *kdGetDevice(Sint32 device);
const KDS_DEVICEINFO *kdGetDeviceInfo(Sint32 device);
const KDS_PORT *kdGetPortStatus(Sint32 port);
/* TRUE once the frame that sent device's command seq is received, until the
 * frame that sends its next command is */
Sint32 kdGetStatus(Sint32 device, Sint32 seq);
/* whether a response showed device connected or removed. kdRecognizeChanged
 * clears it at the next response, which can set it again */
Sint32 kdIsChanged(Sint32 device);
void kdRecognizeChanged(Sint32 device);
const KDS_GUNMODEINFO *kdGetGunModeInfo(void);

/* commands, each queued for the next frame. a command call returns the
 * command's sequence number, -1 when device has a command queued, or -2 when
 * the semaphore is taken. kdSetResetPattern queues port's reset pattern and
 * returns 0. a kdRecvFrame that finds the semaphore taken leaves its frame
 * unreceived. the next of these calls then receives that frame, and the
 * frame's callbacks run. the size that kdSetBlockWrite, kdSetSetCondition and
 * their Ex forms take counts 4-byte words, since it reaches mpdrv_setcmd as
 * extra_count */
Sint32 kdSetDeviceRequest(Sint32 device);
Sint32 kdSetAllStatusRequest(Sint32 device);
Sint32 kdSetDeviceReset(Sint32 device);
Sint32 kdSetDeviceKill(Sint32 device);
Sint32 kdSetGetCondition(Sint32 device, Sint32 func);
Sint32 kdSetGetMediaInfo(Sint32 device, Sint32 func, Sint32 pt);
Sint32 kdSetBlockRead(Sint32 device, Sint32 func, Sint32 pt, Sint32 phase,
                      Sint32 block);
Sint32 kdSetBlockWrite(Sint32 device, Sint32 func, Sint32 pt, Sint32 phase,
                       Sint32 block, const void *src, Sint32 size);
Sint32 kdSetGetLastError(Sint32 device, Sint32 func, Sint32 pt, Sint32 phase,
                         Sint32 block);
Sint32 kdSetARControl(Sint32 device, const void *buf);
Sint32 kdSetFT4Control(Sint32 device, const void *buf);
Sint32 kdSetSetCondition(Sint32 device, Sint32 func, const void *buf,
                         Sint32 size);
Sint32 kdSetTransmitAgain(Sint32 device);
Sint32 kdSetResetPattern(Sint32 port);

/* the fd word of func, or NULL when the device lacks the function */
const Uint8 *kdDevGetFD(const KDS_DEVICEINFO *dev, Uint32 func);
/* each decodes its function's fd word and returns 0, or -1 when the device
 * lacks the function */
Sint32 kdDevGetControllerInfo(KDS_CONTROLLERINFO *sinfo,
                              const KDS_DEVICEINFO *info);
Sint32 kdDevGetStorageInfo(KDS_STORAGEINFO *sinfo, const KDS_DEVICEINFO *info);
Sint32 kdDevGetSIPInfo(KDS_SIPINFO *sinfo, const KDS_DEVICEINFO *info);
Sint32 kdDevGetLCDInfo(KDS_LCDINFO *sinfo, const KDS_DEVICEINFO *info);
Sint32 kdDevGetTimerInfo(KDS_TIMERINFO *sinfo, const KDS_DEVICEINFO *info);
Sint32 kdDevGetGunInfo(KDS_GUNINFO *sinfo, const KDS_DEVICEINFO *info);
Sint32 kdDevGetVibrationInfo(KDS_VIBRATIONINFO *sinfo,
                             const KDS_DEVICEINFO *info);

void kdGetVersion(Sint32 *major, Sint32 *minor);

/* commands with a callback, which kdRecvFrame calls once kdGetStatus holds
 * for the command. kdSetCallback registers func for device's command seqno
 * and returns 0. each of the others returns as its command above does, and
 * registers cbfunc for the command it queues. it holds the semaphore while it
 * does, or returns -2 when the semaphore is taken */
typedef void (*KD_CALLBACK)(Sint32 device, Sint32 seqno, KDS_DEVICE *dev,
                            KDS_DEVICEINFO *info);
Sint32 kdSetCallback(Sint32 device, KD_CALLBACK func, Sint32 seqno);
Sint32 kdSetDeviceRequestEx(Sint32 device, KD_CALLBACK cbfunc);
Sint32 kdSetAllStatusRequestEx(Sint32 device, KD_CALLBACK cbfunc);
Sint32 kdSetDeviceResetEx(Sint32 device, KD_CALLBACK cbfunc);
Sint32 kdSetDeviceKillEx(Sint32 device, KD_CALLBACK cbfunc);
Sint32 kdSetGetConditionEx(Sint32 device, Sint32 func, KD_CALLBACK cbfunc);
Sint32 kdSetGetMediaInfoEx(Sint32 device, Sint32 func, Sint32 pt,
                           KD_CALLBACK cbfunc);
Sint32 kdSetBlockReadEx(Sint32 device, Sint32 func, Sint32 pt, Sint32 phase,
                        Sint32 block, KD_CALLBACK cbfunc);
Sint32 kdSetBlockWriteEx(Sint32 device, Sint32 func, Sint32 pt, Sint32 phase,
                         Sint32 block, const void *src, Sint32 size,
                         KD_CALLBACK cbfunc);
Sint32 kdSetGetLastErrorEx(Sint32 device, Sint32 func, Sint32 pt, Sint32 phase,
                           Sint32 block, KD_CALLBACK cbfunc);
Sint32 kdSetARControlEx(Sint32 device, const void *buf, KD_CALLBACK cbfunc);
Sint32 kdSetFT4ControlEx(Sint32 device, const void *buf, KD_CALLBACK cbfunc);
Sint32 kdSetSetConditionEx(Sint32 device, Sint32 func, const void *buf,
                           Sint32 size, KD_CALLBACK cbfunc);

#endif
