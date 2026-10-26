/* Shinobi libraries, as far as the sources use them */
#ifndef SHINOBI_H
#define SHINOBI_H
#include <machine.h>
#include <sg_xpt.h>

/* the cable in the video connector, as syCblCheckCable reports it */
typedef enum SYE_CBL_CABLE_TAG {
    SYE_CBL_CABLE_VGA = 0,
    SYE_CBL_CABLE_RESERVED = 1,
    SYE_CBL_CABLE_RGB = 2,
    SYE_CBL_CABLE_VBS = 3
} SYE_CBL_CABLE;
SYE_CBL_CABLE syCblCheckCable(void);
/* return to the boot ROM's menu */
void syBtExit(void);
/* have the boot ROM check the disc */
Sint32 syBtCheckDisc(void);
struct tag_SYS_BT_SYSTEMID {
    Sint32 nNo; /* the disc's number */
    Sint32 nAll; /* the disc count */
    Sint8 szProduct[16];
};
typedef struct tag_SYS_BT_SYSTEMID SYS_BT_SYSTEMID;
/* fill id from a disc header: the boot disc's for syBtGetBootSystemID, and
 * that of the disc in the drive now for syBtGetCurrentSystemID. id takes the
 * disc number and count from the header's "1/1" field, and the 10-byte product
 * number with a NUL after it. each returns 1 when the "1/1" field's numbers are
 * all digits, and 0 when they aren't or id is NULL */
Sint32 syBtGetBootSystemID(SYS_BT_SYSTEMID *id);
Sint32 syBtGetCurrentSystemID(SYS_BT_SYSTEMID *id);
/* store in *boot and *current the addresses of the boot ROM's copies of two
 * disc headers: the boot disc's, and that of the disc in the drive now */
void syBtGetSystemIDAddress(char **boot, char **current);

/* the Shinobi heap. syMallocInit starts it at the first 32-byte boundary after
 * heap, 32 bytes on when heap is itself aligned. the heap's size bytes then end
 * 1 to 32 bytes past heap + size */
void syMallocInit(void *heap, Uint32 size);
void syMallocFinish(void);
/* store the heap's free bytes in *wholeFreeSize, and its largest free block's
 * in *biggestFreeBlockSize, headers excluded */
void syMallocStat(Uint32 *wholeFreeSize, Uint32 *biggestFreeBlockSize);
/* malloc and free under the library's own names */
void *syMalloc(Uint32 size);
void syFree(void *ptr);

#define NJD_PORT_A0 0
#define NJD_PORT_B0 6
#define NJD_PORT_C0 12
#define NJD_PORT_D0 18

/* port numbers: each connector's controller, and its two expansion sockets */
#define PDD_PORT_A0 0
#define PDD_PORT_A1 1
#define PDD_PORT_A2 2
#define PDD_PORT_B0 6
#define PDD_PORT_B1 7
#define PDD_PORT_B2 8
#define PDD_PORT_C0 12
#define PDD_PORT_C1 13
#define PDD_PORT_C2 14
#define PDD_PORT_D0 18
#define PDD_PORT_D1 19
#define PDD_PORT_D2 20

/* button bits, in a peripheral's on, off, press and release masks */
#define PDD_DGT_TL (1 << 17)
#define PDD_DGT_TR (1 << 16)
#define PDD_DGT_TX (1 << 10)
#define PDD_DGT_TY (1 << 9)
#define PDD_DGT_TZ (1 << 8)
#define PDD_DGT_KR (1 << 7)
#define PDD_DGT_KL (1 << 6)
#define PDD_DGT_KD (1 << 5)
#define PDD_DGT_KU (1 << 4)
#define PDD_DGT_ST (1 << 3)
#define PDD_DGT_TA (1 << 2)
#define PDD_DGT_TB (1 << 1)
#define PDD_DGT_TC (1 << 0)

/* peripheral IDs: a standard controller, by its buttons and axes, and a
 * keyboard */
#define PDD_DEV_CONTROLLER 0x000f06fe
#define PDD_DEV_KEYBOARD 0x01020000

/* analog axis bits, in a peripheral's support */
#define PDD_DEV_SUPPORT_AY2 (1 << 21)
#define PDD_DEV_SUPPORT_AX2 (1 << 20)
#define PDD_DEV_SUPPORT_AY1 (1 << 19)
#define PDD_DEV_SUPPORT_AX1 (1 << 18)
#define PDD_DEV_SUPPORT_AL (1 << 17)
#define PDD_DEV_SUPPORT_AR (1 << 16)

/* device type bits, in PDS_PERIPHERALINFO's type */
#define PDD_DEVTYPE_CONTROLLER (1 << 0)
#define PDD_DEVTYPE_STORAGE (1 << 1)
#define PDD_DEVTYPE_LCD (1 << 2)
#define PDD_DEVTYPE_TIMER (1 << 3)
#define PDD_DEVTYPE_ARGUN (1 << 5)
#define PDD_DEVTYPE_KEYBOARD (1 << 6)

/* a port device's description of itself, the KDS_DEVICEINFO record with its
 * function bits as type */
typedef struct PDS_PERIPHERALINFO {
    Uint32 type;
    Uint32 reserved[3];
    Uint8 is_root;
    Uint8 area_code;
    Uint8 connector_dir[2];
    char product_name[32];
    char license[64];
    Uint16 stdby_pow;
    Uint16 max_pow;
} PDS_PERIPHERALINFO;

/* a port's state, as pdExecPeripheralServer last updated it. press and release
 * hold the buttons that went down and up since the update before, and old holds
 * that update's on mask */
typedef struct PDS_PERIPHERAL {
    Uint32 id;
    Uint32 support;
    Uint32 on;
    Uint32 off;
    Uint32 press;
    Uint32 release;
    Uint16 r, l;
    Sint16 x1, y1, x2, y2;
    char *name;
    void *extend;
    Uint32 old;
    struct PDS_PERIPHERALINFO *info;
} PDS_PERIPHERAL;
void pdExecPeripheralServer(void);

typedef PDS_PERIPHERAL NJS_PERIPHERAL;
const NJS_PERIPHERAL *njGetPeripheral(Uint32 port);
/* print a port's support and button masks in hex and its axes in decimal, one
 * labeled row each. the first row is at loc, an NJM_LOCATION, and the rest
 * follow below it */
void njPrintPeripheralInfo(Int loc, Uint32 port);

/* under active logic a pressed button's bit is set, and under negative logic
 * clear */
#define PDD_PLOGIC_ACTIVE 0
/* set the button logic and clear every port's state. then start Kaede with the
 * receive and send buffers, unless either is NULL */
void pdInitPeripheral(Sint32 logic, void *recvbuf, void *sendbuf);
void pdExitPeripheral(void);
const PDS_PERIPHERAL *pdGetPeripheral(Uint32 port);
const PDS_PERIPHERALINFO *pdGetPeripheralInfo(Uint32 port);

/* a port's error code, from its response in the last frame the peripheral
 * server read */
#define PDD_ERR_OK 0x00000000
#define PDD_ERR_RETRY 0xff0000fc
#define PDD_ERR_GENERIC 0xff000000
Sint32 pdGetPeripheralError(Uint32 port);

/* pdSetMode and pdGetMode set and read Kaede's KDD_MODE_DEVENDIAN. under
 * PDD_MODE_TYPE_A, the peripheral server rearranges a controller's support
 * bits */
#define PDD_MODE_TYPE_A 1
void pdSetMode(Sint32 mode);
Sint32 pdGetMode(void);

/* the CPU timer unit: timer 0 counts from syTmrInit, timer 1 raises a one-shot
 * interrupt, and timer 2 is the general-purpose timer. a SY_TMR_CLOCK selects
 * the peripheral clock divided by 4, 16, 64, 256 or 1024 */
#define SYD_TMR_CLOCK_P4 0
#define SYD_TMR_CLOCK_P16 1
#define SYD_TMR_CLOCK_P64 2
#define SYD_TMR_CLOCK_P256 3
#define SYD_TMR_CLOCK_P1024 4
#define SYD_TMR_ERR_OK 0
#define SYD_TMR_ERR_EXIST (-1)
typedef Uint16 SY_TMR_CLOCK;
void syTmrInit(void);
/* the count since syTmrInit, which rises as timer 0 counts down from
 * 0xffffffff */
Uint32 syTmrGetCount(void);
Uint32 syTmrDiffCount(Uint32 count1, Uint32 count2);
/* the microseconds in count, at 1.28 microseconds a count: 64 cycles of the
 * 50 MHz peripheral clock. only a result past 32 bits overflows */
Uint32 syTmrCountToMicro(Uint32 count);
Uint32 syTmrMicroToCount(Uint32 micro);
void syTmrGenStart(void);
void syTmrGenStop(void);
/* make func(arg) timer 2's underflow handler, and leave the timer running or
 * stopped as it was. with no func, start the timer, wait here for the
 * underflow and stop it */
void syTmrGenSetInt(void (*func)(void *), void *arg);
void syTmrGenCancelInt(void);
void syTmrGenSetClock(SY_TMR_CLOCK clock);
/* set timer 2's count so that syTmrGenGetCount reads count - 1, and leave the
 * timer running or stopped as it was */
void syTmrGenSetCount(Uint32 count);
/* timer 2's reading, as syTmrGenSetCount starts it, which rises by one each
 * time the timer counts down by one */
Uint32 syTmrGenGetCount(void);
Uint32 syTmrGenMicroToCount(Uint32 micro, SY_TMR_CLOCK clock);
Uint32 syTmrGenCountToMicro(Uint32 count, SY_TMR_CLOCK clock);
Uint32 syTmrGenDiffCount(Uint32 count1, Uint32 count2);
Sint32 syTmrInt1Set(Uint32 count, void (*func)(void *), void *arg);
void syTmrInt1Cancel(void);
/* start timer 1 counting down from count at clock, to call func(arg) at its
 * underflow, and return SYD_TMR_ERR_OK. with no func, wait here for the
 * underflow and stop the timer. the result is SYD_TMR_ERR_EXIST, with nothing
 * changed, while timer 1 runs */
Sint32 syTmrInt1SetEx(Uint32 count, void (*func)(void *), void *arg,
                      SY_TMR_CLOCK clock);

typedef struct _SYS_RTC_DATE {
    Uint16 year;
    Uint8 month;
    Uint8 day;
    Uint8 hour;
    Uint8 minute;
    Uint8 second;
    Uint8 dayofweek;
    Uint8 ageofmoon;
} SYS_RTC_DATE;
/* read the clock's count of seconds, chain syRtcExecServer to the vertical
 * blank to keep the count current, and return 0 */
Sint32 syRtcInit(void);
void syRtcFinish(void);
void syRtcExecServer(void);
/* store the count's date in *date, with its day of the week and the moon's age,
 * and return the server mode, as syRtcGetStat does. under RTC_STAT_DIRECT, the
 * count is read from the clock now, and otherwise it is the count the server
 * keeps */
Sint32 syRtcGetDate(SYS_RTC_DATE *date);
/* 0, or -1 when the clock doesn't read back the date, or a second past it */
Sint32 syRtcSetDate(const SYS_RTC_DATE *date);
/* set the server mode, the RTC_STAT_ bits that syRtcGetStat returns */
void syRtcSetServerMode(Sint32 mode);
Sint32 syRtcGetStat(void);
/* the server mode's bits. under RTC_STAT_DIRECT, syRtcGetDate reads the clock
 * itself, and syRtcExecServer skips syRtcGetAndCheckCount. under
 * RTC_STAT_FULL_READ, syRtcExecServer also reads the clock by
 * syRtcGetCountForce. under RTC_STAT_LOG, syRtcAicaGetCount logs both halves of
 * every reading */
#define RTC_STAT_DIRECT 1
#define RTC_STAT_FULL_READ 2
#define RTC_STAT_LOG 8
/* 1, 0 or -1 as date1 is later than, the same as or earlier than date2 */
Sint32 syRtcCompareDate(const SYS_RTC_DATE *date1, const SYS_RTC_DATE *date2);
/* convert between a date and a count of seconds since the start of 1950 */
void syRtcDateToCount(const SYS_RTC_DATE *date, Uint32 *count);
void syRtcCountToDate(Uint32 count, SYS_RTC_DATE *date);

/* fibers. each is a saved stack pointer and user data, in a table syFbrInit
 * takes. a free entry's sp is NULL, and a thread made a fiber has sp -1 until
 * it switches away. syFbrSwitchToFiber and the rest of the switches are
 * assembly, in sg_syf2.src */
struct sy_fiber {
    void *sp;
    void *data;
};
/* -1 for a NULL table or fewer than 2 entries */
Sint32 syFbrInit(struct sy_fiber *table, Sint32 count);
/* a fiber that starts at func(data) on size bytes at stack, or NULL */
struct sy_fiber *syFbrCreateFiber(void *stack, Uint32 size,
                                  void (*func)(void *), void *data);
/* make the calling thread the current fiber, or return NULL when a fiber is
 * current already or no entry is free */
struct sy_fiber *syFbrConvertThreadToFiber(void *data);
/* -1 for NULL or the current fiber */
Sint32 syFbrDeleteFiber(struct sy_fiber *fiber);
struct sy_fiber *syFbrGetCurrentFiber(void);
void *syFbrGetFiberData(void);
/* the stack the switches from an interrupt run on; -1 for NULL */
Sint32 syFbrSetIntStack(void *stack);
/* switch to fiber, float registers included. from an interrupt,
 * syFbrDelayedSwitchToFiber makes the switch once the interrupt returns */
void syFbrSwitchToFiber(struct sy_fiber *fiber);
void syFbrDelayedSwitchToFiber(struct sy_fiber *fiber);
struct sy_fiber *syFbrGetTblPtr(int index);
Sint32 syFbrGetMaxFiber(void);
void *syFbrGetSp(struct sy_fiber *fiber);
void *syFbrGetData(struct sy_fiber *fiber);
void syFbrSetSp(struct sy_fiber *fiber, void *sp);
void syFbrSetData(struct sy_fiber *fiber, void *data);
void *syFbrGet_fib_sp(int index);
void syFbrDumpTable(void);
/* set up a frame on size bytes at stack that starts func(data), and return
 * its sp */
void *syFbrCreateFiberMain(void *stack, Sint32 size, void (*func)(void *),
                           void *data);

/* purge the operand cache, set CCR to form and invalidate both caches. it
 * must run uncached, from P2 */
void syCacheInit_P2_(Uint32 form);
/* write back and invalidate every operand cache line. with CCR.ORA set, the
 * cache keeps entries 128 to 255 and 384 to 511 as RAM, and the purge reaches
 * only entries 0 to 127 and 256 to 383 */
void syCachePurgeAll(void);
/* invalidate the instruction cache. the function must run uncached, so
 * syCacheICI calls it through its P2 address */
void syCacheICI_P2_(void);
#define syCacheICI() ((void (*)(void))(0xa0000000 | (Uint32)syCacheICI_P2_))()
/* invalidate the operand cache; must run uncached, from P2 */
void syCacheOCI_P2_(void);
/* syCacheOCBP writes back and invalidates the operand cache lines over size
 * bytes at address, syCacheOCBI only invalidates them and syCacheOCWB only
 * writes them back */
/* @hack type: OCBP's and OCBI's size signed, for gdctl.c's calls; as Uint32,
 * 62 of the 108 words of gdc_read and gdc_trans, which call them, differ in
 * 1ST_READ.BIN, and 61 of 113 in LOADER.BIN */
void syCacheOCBP(void *address, Sint32 size);
void syCacheOCBI(void *address, Sint32 size);
void syCacheOCWB(void *address, Uint32 size);
/* prefetch the lines over size bytes at address; syCachePREF_s takes the
 * address line-aligned. size must be above 0: each call counts the lines down
 * and tests the count only after its first decrement, so a size that rounds to
 * 0 lines prefetches 2^32 of them */
void syCachePREF(void *address, Uint32 size);
void syCachePREF_s(void *address, Uint32 size);
/* store value at address, allocating its line without reading it */
void syCacheMOVCAL(void *address, Uint32 value);
/* the operand cache's RAM half holds eight 1 KB pages. syCacheAllocCacheRAM
 * hands out the next pages of them: it stores their start in *address and adds
 * pages to the count. a request that reaches the eighth page stores no address
 * and leaves the count. syCacheFreeCacheRAM never gives back the pages at
 * address. it lowers the count by one, and only for an address in the first
 * page. it does so only while the page at the count was never part of a
 * request. otherwise it changes nothing */
void syCacheAllocCacheRAM(void **address, Sint32 pages);
void syCacheFreeCacheRAM(void *address);

/* the CPU's exception and interrupt entry. syIntInit copies an exception stub
 * to VBR + 0x100 and VBR + 0x400, and an interrupt stub to VBR + 0x600. each
 * stub jumps to a master, which calls the handler USER_VECT holds for the
 * code */
/* syIntInit's modes. under SYD_INT_MODE_INTSTACK, the master runs exceptions
 * and interrupts on the area that ends at stack. the outermost one moves the
 * stack pointer to stack, and each one nested in it pushes below. that area
 * must stay valid while they can arrive. under any other mode they run on the
 * interrupted stack, and stack is ignored */
#define SYD_INT_MODE_LOCALSTACK 0x00000000
#define SYD_INT_MODE_INTSTACK 0x80000000
/* install the stubs and mode's master with every interrupt masked, and
 * invalidate the instruction cache. then restore the mask, make syIntBrkRteRte
 * the handler of every code from 0x40 to 0x800, and clear SR.BL */
void syIntInit(Uint32 mode, void *stack);
/* set the interrupt mask to level, 0 to 15, and clear SR.BL */
void syIntEnableInt(Uint32 level);
/* the handler syIntInit gives every code. it unwinds the master's frame,
 * back to the interrupted stack, and jumps to 0xac000010 */
void syIntBrkRteRte(void);
/* the word that holds code's handler, which the master calls with code in R4:
 * VBR + 0x200 + (code - 0x200) / 8. below 0x200, the word depends on code's
 * type. a signed code gives the word at VBR + 0x1c0 + code / 8. SHC shifts the
 * negative difference arithmetically, which C89 leaves to the
 * implementation. an unsigned code, such as syChainDeleteHandler passes
 * (SHLR2 at 0x8c65f49a in 1ST_READ.BIN), wraps, and its logical shift puts the
 * word 0x20000000 bytes higher. as (code >> 3) + 0x1c0, the macro would move
 * that word, and 49 of syIntInit's 191 words would differ */
#define USER_VECT(code)                                                        \
    (*(Uint32 *)((Uint32)get_vbr() + 0x200 + (((code) - 0x200) >> 3)))
/* the interrupt codes of timer 1's and timer 2's underflows */
#define SYD_INT_EVENT_TUNI1 0x420
#define SYD_INT_EVENT_TUNI2 0x440

/* the backup library: a memory card in each of eight drives, two for each
 * controller port. a call that takes time only starts its command, which
 * buExecServer runs */
typedef struct {
    Uint16 year;
    Uint8 month;
    Uint8 day;
    Uint8 hour;
    Uint8 minute;
    Uint8 second;
    Uint8 dayofweek;
} BUS_TIME;

typedef struct {
    Uint8 volume[32];
    Uint16 total_blocks;
    Uint16 total_user_blocks;
    Uint16 free_blocks;
    Uint16 free_user_blocks;
    Uint16 total_exe_blocks;
    Uint16 free_exe_blocks;
    Uint16 block_size;
    Uint16 icon_no;
    BUS_TIME time;
} BUS_DISKINFO;

typedef struct {
    Uint32 filesize;
    Uint16 blocks;
    Uint8 type;
    Uint8 copyflag;
    Uint16 headerofs;
    BUS_TIME time;
} BUS_FILEINFO;

typedef void (*BU_INITCALLBACK)(void);
typedef Sint32 (*BU_COMPLETECALLBACK)(Sint32 drive, Sint32 op, Sint32 status,
                                      Uint32 param);
typedef Sint32 (*BU_PROGRESSCALLBACK)(Sint32 drive, Sint32 op, Sint32 count,
                                      Sint32 max);

/* file types: a data file, or the card's executable file */
#define BUD_FILETYPE_NORMAL 0
#define BUD_FILETYPE_EXECUTABLE 1

/* drive states, from buStat, and command operations */
#define BUD_STAT_READY 0x00000000
#define BUD_STAT_BUSY 0xffffffff
#define BUD_OP_NOP 0x00000000
#define BUD_OP_MOUNT 0x00000001
#define BUD_OP_UNMOUNT 0x00000002
#define BUD_OP_FORMATDISK 0x00000003
#define BUD_OP_CLOSEFILE 0x00000004
#define BUD_OP_READFILE 0x00000005
#define BUD_OP_WRITEFILE 0x00000006
#define BUD_OP_DELETEFILE 0x00000007
#define BUD_OP_RENAMEFILE 0x00000008
#define BUD_OP_DEFRAGDISK 0x00000009
#define BUD_OP_SETFILEATTR 0x0000000a
#define BUD_OP_LOADFILE 0x0000000b
#define BUD_OP_SAVEFILE 0x0000000c
#define BUD_OP_SAVEEXECFILE 0x0000000d
#define BUD_OP_LOADFILEEX 0x0000000e
#define BUD_OP_READIMAGE 0x0000000f
#define BUD_OP_WRITEIMAGE 0x00000010
#define BUD_OP_DELETEFILEEX 0x00000011
#define BUD_OP_CONNECT 0x00000012
#define BUD_OP_REWRITEEXECFILE 0x00000014
#define BUD_ERR_OK 0x00000000
#define BUD_ERR_BUSY 0xffffffff
#define BUD_ERR_INVALID_PARAM 0xffffff00
/* errors in a card, a file or a backup file image */
#define BUD_ERR_ILLEGAL_DISK 0xffffff81
#define BUD_ERR_UNKNOWN_DISK 0xffffff83
#define BUD_ERR_NO_DISK 0xffffff01
#define BUD_ERR_UNFORMAT 0xffffff03
#define BUD_ERR_DISK_FULL 0xffffff04
#define BUD_ERR_FILE_NOT_FOUND 0xffffff05
#define BUD_ERR_FILE_EXIST 0xffffff06
#define BUD_ERR_CANNOT_OPEN 0xffffff07
#define BUD_ERR_CANNOT_CREATE 0xffffff08
#define BUD_ERR_EXECFILE_EXIST 0xffffff09
#define BUD_ERR_CANNOT_DELETE 0xffffff0a
#define BUD_ERR_ACCESS_DENIED 0xffffff0b
#define BUD_ERR_VERIFY 0xffffff10
#define BUD_ERR_WRITE_ERROR 0xffffff40
#define BUD_ERR_FILE_BROKEN 0xffffff41
#define BUD_ERR_BUPFILE_CRC 0xffffff20
#define BUD_ERR_BUPFILE_ILLEGAL 0xffffff21
#define BUD_ERR_GENERIC 0xffff0000
/* the callback return value that lets the operation continue */
#define BUD_CBRET_OK 0x00000000

/* the drives in port A's two sockets, and buInit's drive mask for all eight */
#define BUD_DRIVE_A1 0
#define BUD_DRIVE_A2 1
#define BUD_USE_DRIVE_ALL 0xff

/* buInit capacities, in blocks to cache, for cards of 128 KB to 1 MB: the
 * system block, the FAT blocks and the directory blocks. BUM_WORK_SIZE is the
 * work buInit needs: 512 bytes for each block and a gap of BUD_WORK_GAP bytes,
 * for each drive */
#define BUD_CAPACITY_128KB (1 + 1 + 13)
#define BUD_CAPACITY_256KB (1 + 2 + 30)
#define BUD_CAPACITY_512KB (1 + 4 + 60)
#define BUD_CAPACITY_1MB (1 + 8 + 113)
#define BUD_WORK_GAP 32
#define BUM_WORK_SIZE(capacity, drives)                                        \
    (((capacity) * (drives) * 512) + (BUD_WORK_GAP * (drives)))

/* save flags: verify the file by reading it back after the write, and the copy
 * flag value that forbids copying */
#define BUD_FLAG_VERIFY 0x80000000
#define BUD_FLAG_COPYDISABLE 0x000000ff

#define BUD_VISUALTYPE_NONE 0
#define BUD_VISUALTYPE_C 3
typedef struct {
    char vms_comment[18]; /* shown on the VMU */
    char btr_comment[34]; /* shown by the boot ROM */
    Uint8 game_name[16];
    void *icon_palette;
    void *icon_data;
    Uint16 icon_num;
    Uint16 icon_speed;
    void *visual_data;
    Uint16 visual_type;
    Uint16 reserved;
    void *save_data;
    Uint32 save_size;
} BUS_BACKUPFILEHEADER;

Sint32 buInit(Sint32 capacity, Sint32 drives, void *work, BU_INITCALLBACK func);
/* remove buInit's handler, clear the drives' commands and end the file system,
 * returning vmsfs_exit's result. the result is BUD_ERR_BUSY, with none of it
 * done, while a drive's command runs. either way buInitializeFlag is cleared,
 * so buExecServer runs no command after it */
Sint32 buExit(void);
/* the library's server, which buInterruptHandler runs at each Maple DMA end.
 * it gives each drive's command a turn. it then queues the mount of each card
 * inserted since its last call, and reports each card removed to the
 * completion callback as BUD_OP_UNMOUNT. a mount or a report waits while its
 * drive runs a command. with no buWorkTop, a card is connected, not mounted */
void buExecServer(void);
Uint32 buGetVersion(void);
void buSetCompleteCallback(BU_COMPLETECALLBACK func);
void buSetProgressCallback(BU_PROGRESSCALLBACK func);
void buSetCallbackParam(Uint32 param);
void buSetPrintFunc(void (*func)(const char *str));
void buSetRetryCount(Sint32 count);
Sint32 buIsReady(Sint32 drive);
Sint32 buStat(Sint32 drive);
Sint32 buGetLastError(Sint32 drive);
Sint32 buMount(Sint32 drive);
Sint32 buRemount(Sint32 drive);
Sint32 buUnmount(Sint32 drive);
/* give drive the size bytes at addr as its work, and queue its mount if
 * vmsfs_setwork succeeds. the result is vmsfs_setwork's when it fails, else
 * buMount's */
Sint32 buMountDisk(Sint32 drive, void *addr, Sint32 size);
Sint32 buUnmountDisk(Sint32 drive);
Sint32 buIsFormat(Sint32 drive);
Sint32 buFormatDisk(Sint32 drive, const Uint8 *volume, Sint32 icon,
                    const BUS_TIME *time, Uint32 flag);
Sint32 buGetDiskInfo(Sint32 drive, BUS_DISKINFO *info);
Sint32 buGetDiskFree(Sint32 drive, Sint32 type);
Sint32 buDefragDisk(Sint32 drive, void *work);
Sint32 buFindFirstFile(Sint32 drive, char *fname);
Sint32 buFindNextFile(Sint32 drive, char *fname);
Sint32 buFindFirstFileEx(Sint32 drive, char *fname, BUS_FILEINFO *finfo);
Sint32 buFindNextFileEx(Sint32 drive, char *fname, BUS_FILEINFO *finfo);
Sint32 buFindExecFile(Sint32 drive, char *fname);
Sint32 buIsExistFile(Sint32 drive, const char *fname);
Sint32 buGetFileInfo(Sint32 drive, const char *fname, BUS_FILEINFO *finfo);
Sint32 buGetFileSize(Sint32 drive, const char *fname);
Sint32 buLoadFile(Sint32 drive, const char *fname, void *buf, Uint32 nblock);
Sint32 buLoadFileEx(Sint32 drive, const char *fname, void *buf, Uint32 start,
                    Uint32 nblock);
Sint32 buSaveFile(Sint32 drive, const char *fname, const void *buf,
                  Uint32 nblock, BUS_TIME *time, Uint32 flag);
Sint32 buSaveExecFile(Sint32 drive, const char *fname, const void *buf,
                      Uint32 nblock, BUS_TIME *time, Uint32 flag);
Sint32 buRewriteExecFile(Sint32 drive, const char *fname, const void *buf,
                         Uint32 start, Uint32 nblock);
Sint32 buDeleteFile(Sint32 drive, const char *fname);
Sint32 buDeleteFileEx(Sint32 drive, const char **fname, Sint32 nfiles);
Sint32 buRenameFile(Sint32 drive, const char *from, const char *to);
Sint32 buSetFileAttr(Sint32 drive, const char *fname, Uint16 hdrofs,
                     Uint8 copyflag);
/* build a backup file image in buf from hdr, and return its size in blocks.
 * buf must hold the blocks that buCalcBackupFileSize counts for hdr's
 * icon_num, visual_type and save_size. zeros fill the rest of the last block.
 * the result is BUD_ERR_INVALID_PARAM, with buf unwritten, for an icon_num of 0
 * or above 3, a visual_type past BUD_VISUALTYPE_C, a null icon_palette or
 * icon_data, or a null visual_data with a picture */
Sint32 buMakeBackupFileImage(void *buf, const BUS_BACKUPFILEHEADER *hdr);
/* the blocks that inum icons, a picture of type vtype and size bytes of data
 * take, or BUD_ERR_INVALID_PARAM for an inum of 0 or above 3, or a vtype past
 * BUD_VISUALTYPE_C */
Sint32 buCalcBackupFileSize(Uint32 inum, Uint32 vtype, Uint32 size);
/* fill hdr from an image, with pointers into it, and return BUD_ERR_OK. buf
 * must hold the image up to its padding, since the CRC check reads all of it.
 * buf must also be writable despite its const: the check zeroes the image's
 * CRC, computes the CRC, then puts the stored one back. the result is
 * BUD_ERR_BUPFILE_ILLEGAL, with hdr as it was, for an icon_num of 0 or above 3
 * or a visual_type past BUD_VISUALTYPE_C. it is BUD_ERR_BUPFILE_CRC, with hdr
 * filled, when the image's CRC does not match */
Sint32 buAnalyzeBackupFileImage(BUS_BACKUPFILEHEADER *hdr, const void *buf);
/* the CRC-16 of buf[0..size): polynomial 0x1021, most significant bit first,
 * starting from 0. a byte past 0x7f sign-extends, and the bits it sets above
 * bit 15 never reach the result */
Uint16 buCalcCRC(const void *buf, Uint32 size);
/* remove the handler buInit adds to the Maple DMA end chain, which runs
 * buExecServer, so that the caller runs buExecServer itself */
void buDisableServer(void);

/* the boot ROM's console record. kbCfgIni makes it ready, and every caller
 * passes 0 for which. kbCfgGetId returns the record's address, and the console
 * settings start 16 bytes in */
void kbCfgIni(Sint32 which);
void *kbCfgGetId(void);

/* the boot ROM's flash memory services, on its partitions, 0 to 4.
 * syBtFlGetInfo gives a partition's flash address and size */
typedef struct SYS_BT_FL_BLK_INFO {
    Sint32 fmaddr;
    Sint32 size;
} SY_BT_FL_BLKINFO;
Sint32 syBtFlGetInfo(Sint32 part, SY_BT_FL_BLKINFO *info);
/* read size bytes at address into buffer; -1 when it fails */
Sint32 syBtFlRead(Sint32 address, void *buffer, Sint32 size);
/* write size bytes from buffer to address, and return the bytes written */
Sint32 syBtFlWrite(Sint32 address, void *buffer, Sint32 size);
/* erase the partition that holds address: 0 when it does, -1 when it fails */
Sint32 syBtFlDeleteSct(Sint32 address);

/* the flash memory manager, on one partition at a time. a partition holds
 * packets of FMD_DATA_BYTES data bytes, numbered from 0, written as a log. a
 * work area caches the partition whole. a packet never written reads as 0xff
 * bytes. a write the log has no room for erases the partition and writes back
 * the latest packet of each number */
#define FMD_DATA_BYTES 60
/* the manager's errors */
#define FMD_READ_FAILED (-1) /* a flash read failed */
#define FMD_WRITE_FAILED (-2) /* a flash write failed */
#define FMD_ERASE_FAILED (-3) /* erasing the partition failed */
#define FMD_BAD_CRC (-4) /* the packet read fails its CRC */
#define FMD_NO_PACKET (-5) /* no packet of that number */
#define FMD_WRONG_TYPE (-6) /* no such type, or an FMD_PROTECTED partition */
#define FMD_BAD_PARTITION (-7) /* a partition other than 1 to 4 */
#define FMD_BAD_HEADER (-8) /* the header's ID, partition or type is wrong */
#define FMD_REQUEST_PENDING (-9) /* a request is in progress */
#define FMD_NOT_OPEN (-11) /* no partition is open */
/* the types of partition; only a writable one is read and written */
#define FMD_WRITABLE 0
#define FMD_PROTECTED 1
/* fmMngExecServer's results. it returns FMD_STAT_RUNNING while a request is in
 * progress. once the request finishes, it returns FMD_STAT_COMPLETE or
 * FMD_STAT_ERR until the next request starts. it returns FMD_STAT_NONE before
 * any request since fmMngInit, and with no partition open */
#define FMD_STAT_NONE 0
#define FMD_STAT_COMPLETE 1
#define FMD_STAT_RUNNING 2
#define FMD_STAT_ERR 3
/* open part, 1 to 4, with no request pending, and cache it in work. it returns
 * 0 once the partition is cached. work must be aligned to 4 and hold the
 * partition's size. it must stay valid until fmMngFinish, or until a later
 * fmMngInit opens a partition. it returns FMD_BAD_PARTITION for a part outside
 * 1 to 4, and changes nothing then. any other part is open whatever the
 * result, so fmMngFormat can format a partition fmMngInit refused with
 * FMD_BAD_HEADER. it returns FMD_WRONG_TYPE for an FMD_PROTECTED partition,
 * and FMD_READ_FAILED when a flash read fails */
Sint32 fmMngInit(Sint32 part, void *work);
/* erase the open partition, write a header of type, and cache it again as
 * fmMngInit does. it returns FMD_WRONG_TYPE for no such type, before erasing,
 * and for FMD_PROTECTED, after */
Sint32 fmMngFormat(Sint32 type);
/* fmMngRead requests packet's FMD_DATA_BYTES bytes into buffer, and fmMngWrite
 * their write from buffer to packet, for fmMngExecServer to run. the server's
 * first call for the request copies the bytes, so buffer must stay valid until
 * then */
Sint32 fmMngRead(Sint32 packet, void *buffer);
Sint32 fmMngWrite(Sint32 packet, void *buffer);
/* run the request a call at a time: a read takes one call, a write one, or
 * three when it erases the partition */
Sint32 fmMngExecServer(void);
/* the result of the last request to finish since fmMngInit, or 0 before
 * any. it returns FMD_NOT_OPEN with no partition open */
Sint32 fmMngGetErr(void);
/* close the partition and drop its request, after which work need not stay
 * valid. a write dropped after its erase leaves the partition erased */
void fmMngFinish(void);
/* erase the open partition and write back the latest of each packet. it
 * returns FMD_WRONG_TYPE for a partition not FMD_WRITABLE, and leaves it
 * erased */
Sint32 fmMngRefresh(void);
/* the packets the log can take before a write erases the partition, or
 * FMD_NOT_OPEN with no partition open */
Sint32 fmMngGetFree(void);

/* the CRC-16 of size bytes at data, from 0xffff and returned inverted */
Uint32 fmcalccrc(Sint32 size, const Uint8 *data);

/* the console settings: 8 bytes at kbCfgGetId() + 16, kept in packet 5 of flash
 * partition 2 */
#define SYD_BT_CFG_OK 0
#define SYD_BT_CFG_ERR (-1)
/* syBtCfgSet's permits. SYD_BT_CFGINI reads the settings from the packet. each
 * other permit is a bit that names settings bytes to write to the packet. AT
 * names byte 7, SM byte 6, LG byte 5, GM byte 4 and TM bytes 0 to 3 */
#define SYD_BT_CFGINI 0x00
#define SYD_BT_CFGWRT_AT 0x01
#define SYD_BT_CFGWRT_SM 0x02
#define SYD_BT_CFGWRT_LG 0x04
#define SYD_BT_CFGWRT_GM 0x08
#define SYD_BT_CFGWRT_TM 0x10
/* the bytes of syBtCfgSet's work area */
#define SYD_BT_CFG_WORKSIZE 0x4000
/* under SYD_BT_CFGINI, copy the packet's first 8 bytes to the settings. under
 * any other permit, write the packet back with the bytes its bits name copied
 * in from the settings. the call opens the partition with fmMngInit on work,
 * and formats it first when fmMngInit returns FMD_BAD_HEADER. it leaves the
 * partition open, so work must stay valid until fmMngFinish. it returns
 * SYD_BT_CFG_OK, or SYD_BT_CFG_ERR when a flash call fails */
Sint32 syBtCfgSet(Sint32 permit, void *work);

/* the console's sound setting, stereo or mono, read and written through
 * syBtCfgSet with the work area given to syCfgInit */
#define SYD_CFG_STEREO 0x00
#define SYD_CFG_MONO 0x01
#define SYD_CFG_OK SYD_BT_CFG_OK
#define SYD_CFG_NG SYD_BT_CFG_ERR
Sint32 syCfgInit(void *work);
Sint32 syCfgExit(void);
Sint32 syCfgSetSoundMode(Sint32 mode);
Sint32 syCfgGetSoundMode(Sint32 *mode);

/* the sound library, and the error codes its calls return */
enum SDE_ERR {
    SDE_ERR_NOTHING,
    SDE_ERR_PRM_OVER_RANGE = 0x03000001,
    SDE_ERR_QUEUE_NO_ENOUGH = 0x05000101,
    /* a transfer of a block whose transfer is in progress */
    SDE_ERR_MEMBLK_ALREADY_TRANSFER = 0x06000001,
    SDE_ERR_HANDLE_NO_ENOUGH = 0x07000101,
    SDE_ERR_HANDLE_ILLEGAL_VALUE = 0x07000102,
    SDE_ERR_HANDLE_NULL = 0x07000103,
    SDE_ERR_PTR_NULL = 0x07000203,
    SDE_ERR_BANK_ILLEGAL_NUM = 0x08000003,
    SDE_ERR_DATA_ILLEGAL_TYPE = 0x09000001,
    SDE_ERR_DATA_ILLEGAL_VER = 0x09000002,
    SDE_ERR_HOST_CMD_BUF_NO_ENOUGH = 0x0a000001,
    SDD_ERR_SND_DRV_ILLEGAL_VER = 0x0b000002,
    SDE_ERR_SND_DRV_BUSY = 0x0b000003,
    SDE_ERR_NO_INIT = 0x0d000001,
    SDE_ERR_UNKNOW_NUM = 0x7fffffff
};
typedef enum SDE_ERR SDE_ERR;

#define SDD_CHAR32(a, b, c, d) ((a) | ((b) << 8) | ((c) << 16) | ((d) << 24))

enum SDE_DATA_TYPE {
    SDE_DATA_TYPE_SND_DRV = SDD_CHAR32('S', 'D', 'R', 'V'),
    SDE_DATA_TYPE_MULTI_UNIT = SDD_CHAR32('S', 'M', 'L', 'T'),
    SDE_DATA_TYPE_MIDI_SEQ_DATA = SDD_CHAR32('S', 'M', 'S', 'D'),
    SDE_DATA_TYPE_MIDI_SEQ_BANK = SDD_CHAR32('S', 'M', 'S', 'B'),
    SDE_DATA_TYPE_MIDI_PRG_BANK = SDD_CHAR32('S', 'M', 'P', 'B'),
    SDE_DATA_TYPE_SHOT_DATA = SDD_CHAR32('S', 'O', 'S', 'D'),
    SDE_DATA_TYPE_SHOT_BANK = SDD_CHAR32('S', 'O', 'S', 'B'),
    SDE_DATA_TYPE_PSTM_DATA = SDD_CHAR32('S', 'P', 'S', 'D'),
    SDE_DATA_TYPE_PSTM_RING_BUF = SDD_CHAR32('S', 'P', 'S', 'R'),
    SDE_DATA_TYPE_FX_OUT_BANK = SDD_CHAR32('S', 'F', 'O', 'B'),
    SDE_DATA_TYPE_FX_PRG_BANK = SDD_CHAR32('S', 'F', 'P', 'B'),
    SDE_DATA_TYPE_FX_PRG_WRK = SDD_CHAR32('S', 'F', 'P', 'W')
};
typedef enum SDE_DATA_TYPE SDE_DATA_TYPE;

enum SDE_SPACE {
    SDE_SPACE_MONO = SDD_CHAR32('M', 'O', 'N', 'O'),
    SDE_SPACE_STEREO = SDD_CHAR32('S', 'T', 'R', 'O')
};
typedef enum SDE_SPACE SDE_SPACE;

enum SDE_MEMBLK_STAT {
    SDE_MEMBLK_STAT_TRANSFER_REMAIN,
    SDE_MEMBLK_STAT_TRANSFER_FINISHED,
    SDE_MEMBLK_STAT_TRANSFER_PROGRESS
};
typedef enum SDE_MEMBLK_STAT SDE_MEMBLK_STAT;

enum SDE_MEMBLK_TRANSFER_MODE {
    SDE_MEMBLK_TRANSFER_MODE_CPU,
    SDE_MEMBLK_TRANSFER_MODE_DMA
};
typedef enum SDE_MEMBLK_TRANSFER_MODE SDE_MEMBLK_TRANSFER_MODE;

/* stream sample formats */
enum SDE_PCM_TYPE {
    SDE_PCM_TYPE_8BIT_PCM,
    SDE_PCM_TYPE_16BIT_PCM,
    SDE_PCM_TYPE_4BIT_ADPCM
};
typedef enum SDE_PCM_TYPE SDE_PCM_TYPE;

/* the port count of each kind, and a stream port's channel count */
#define SDD_MIDI_PORT_MAX 8
#define SDD_SHOT_PORT_MAX 8
#define SDD_PSTM_PORT_MAX 4
#define SDD_PSTM_PORT_CH_MAX 2
/* a stream port channel, or both */
#define SDD_PSTM_LEFT_CH 0
#define SDD_PSTM_RIGHT_CH 1
#define SDD_PSTM_ALL_CH (-1)

/* MIDI status bytes, without the channel */
#define SDD_MIDI_NOTE_OFF 0x80
#define SDD_MIDI_NOTE_ON 0x90
#define SDD_MIDI_MONO_PRES 0xa0
#define SDD_MIDI_CTL_CHG 0xb0
#define SDD_MIDI_PRG_CHG 0xc0
#define SDD_MIDI_PITCH_BEND 0xe0

typedef struct SDS_MIDI_MES {
    Uint8 m_Member[4];
} SDS_MIDI_MES;

struct SDS_VER {
    Sint8 m_MajorVer;
    Sint8 m_MinorVer;
    Sint8 m_DebugVer;
    Sint8 rsv;
};
typedef struct SDS_VER SDS_VER;

typedef struct SDS_PORT_REF *SDHANDLE;
typedef SDHANDLE *SDMIDI;
typedef SDHANDLE *SDPSTM;

struct SDS_MIDI_STAT {
    Sint8 m_HostCmdRsvNum;
    Sint8 m_HostCmdDelayTime;
    Sint8 m_Vol;
    Sint8 m_Pan;
    Sint8 m_FxLev;
    Sint8 m_DrctLev;
    Sint16 m_Pitch;
    Sint16 m_Speed;
    Uint32 m_TotalBeatTime;
    Sint32 m_CurAdr;
    Uint32 m_Err;
    Uint32 m_Flg;
};
typedef struct SDS_MIDI_STAT SDS_MIDI_STAT;

struct SDS_PSTM_STAT {
    Sint8 m_HostCmdRsvNum;
    Sint8 m_HostCmdDelayTime;
    Sint8 m_Vol;
    Sint8 m_Pan;
    Sint8 m_FxLev;
    Sint8 m_DrctLev;
    Sint16 m_Pitch;
    Sint16 m_Speed;
    Uint32 m_TotalSmpFrame;
    Sint32 m_CurAdr;
    Uint32 m_Err;
    Uint32 m_Flg;
};
typedef struct SDS_PSTM_STAT SDS_PSTM_STAT;

/* a memory block: data to be transferred to sound memory */
struct SDS_MEM_BLK {
    Uint32 m_Member[8];
};
typedef struct SDS_MEM_BLK *SDMEMBLK;
typedef void (*SD_MEMBLK_CALLBACK_FUNC)(void *first_arg);
#define SDD_MEMBLK_NO_FUNC ((SD_MEMBLK_CALLBACK_FUNC)0x00000000)
#define SDD_MEMBLK_SYNC_FUNC ((SD_MEMBLK_CALLBACK_FUNC)0xffffffff)

SDE_ERR sdDrvInit(SDMEMBLK handle);
/* read the running driver's version, and the sound memory addresses of its
 * tables, into the work */
SDE_ERR sdDrvGetInfo(void);
/* set the work up in the library's default buffers, and return
 * sdSysWrkInit's status. all three arguments are ignored */
SDE_ERR sdLibInit(void *wrk_ptr, Sint32 mem_blk_handle_max,
                  Sint32 second_host_cmd_max);
SDE_ERR sdSysServer(void);
SDE_ERR sdSysFlushHostCmd(void);
/* record an error and the function it came from */
void sdSysErrTrap(void *func, SDE_ERR err);
/* print through sdgPrintFunc, which sdDebugSetPrintFunc sets, while
 * sdgDebugMode is true */
int sdDebugPrintf(const char *format, ...);
int sdDebugPrint(const char *str);
int sdDebugSetPrintFunc(void (*func)(const char *str));
extern Bool sdgDebugMode;
extern void (*sdgPrintFunc)(const char *str);
extern Sint32 sdgExistSndDrv; /* whether a sound driver is running */
/* hold the AICA's ARM7 in reset, and release it */
SDE_ERR sdARM7ResetKeep(void);
SDE_ERR sdARM7ResetRel(void);
/* hold the ARM7 in reset and fill the 2 MB of sound memory with "SEGA" */
SDE_ERR sdSndMemClear(void);
SDE_ERR sdSndMemWriteUint8(Sint32 address, Uint8 value);
/* mark every port free, and number each kind's ports from 0 */
SDE_ERR sdModuleRefInit(void);
/* start the primary host command list in buffer, which sdgPrimaryBufPtr
 * keeps */
SDE_ERR sdPrimaryHostCmdInitList(void *buffer);
extern void *sdgPrimaryBufPtr;
/* send the second list's waiting commands on */
SDE_ERR sd2ndHostCmdSendList(void);
/* transfer the primary list to the sound driver if it holds any commands. the
 * work must be set up. sdPrimaryHostCmdFlushCallback runs once the list is in
 * sound memory. it signals the driver and empties the list, and doesn't read
 * first_arg */
SDE_ERR sdPrimaryHostCmdFlush(void);
void sdPrimaryHostCmdFlushCallback(void *first_arg);
/* sound memory access over the G2 bus, by offset into sound memory. an offset
 * past its 2 MB reads and writes nothing, and a byte is read or written through
 * the word that holds it. the test of the offset is signed, so a negative one
 * passes it and reaches below sound memory */
SDE_ERR sdSndMemReadSint8(Sint32 address, Sint8 *value);
SDE_ERR sdSndMemReadUint8(Sint32 address, Uint8 *value);
SDE_ERR sdSndMemReadSint32(Sint32 address, Sint32 *value);
SDE_ERR sdSndMemReadUint32(Sint32 address, Uint32 *value);
SDE_ERR sdSndMemWriteSint32(Sint32 address, Sint32 value);
SDE_ERR sdSndMemWriteUint32(Sint32 address, Uint32 value);
SDE_ERR sdSndSetSpace(const SDE_SPACE space);
SDE_ERR sdSndSetFxPrg(const Sint8 new_fx_prg_num, const Sint8 new_fx_out_num);
SDE_ERR sdSndSetFxOutPrm(const Sint8 out_num, const Sint8 out_pan,
                         const Sint8 out_lev);
SDE_ERR sdQsndSetPos(const Sint8 *pos);
SDE_ERR sdMultiUnitDownload(SDMEMBLK handle);
SDE_ERR sdBankDownload(SDMEMBLK handle, const SDE_DATA_TYPE bank_type,
                       const Sint8 bank_num);
/* store in *allocate_ptr and *allocate_size the sound memory offset and size
 * the work records for data_type's bank bank_num. each error is returned
 * without going to sdSysErrTrap. an unknown data_type sets *allocate_ptr to 0
 * and *allocate_size to -1 */
SDE_ERR sdSndMemGetBankStat(const SDE_DATA_TYPE data_type, const Sint8 bank_num,
                            Sint32 *allocate_ptr, Sint32 *allocate_size);
/* record allocate_size bytes at sound memory offset allocate_ptr as a bank's
 * place, in the work and in the driver's map. the MIDI sequence, MIDI program,
 * shot and stream ring buffer types have 16 banks each, and the three effect
 * types one. bank_num is taken as a Sint8. any other type returns
 * SDE_ERR_DATA_ILLEGAL_TYPE, and any other bank SDE_ERR_BANK_ILLEGAL_NUM */
SDE_ERR sdSndMemAllocateBank(const SDE_DATA_TYPE data_type, int bank_num,
                             Sint32 allocate_ptr, Sint32 allocate_size);
/* take the first unused of the work's 64 memory blocks, whatever the work
 * records as its block count. when all 64 are used, *handle is NULL and the
 * result is SDE_ERR_HANDLE_NO_ENOUGH */
SDE_ERR sdMemBlkCreate(SDMEMBLK *handle);
SDE_ERR sdMemBlkDestroy(SDMEMBLK handle);
SDE_ERR sdMemBlkSetPrm(SDMEMBLK handle, const void *src_blk_ptr,
                       const Sint32 src_blk_sz,
                       const SD_MEMBLK_CALLBACK_FUNC callback_func,
                       const void *callback_first_arg);
SDE_ERR sdMemBlkGetStat(SDMEMBLK handle, SDE_MEMBLK_STAT *stat);
SDE_ERR sdMemBlkTransfer(SDMEMBLK handle, Sint32 dst_ptr);
SDE_ERR sdMemBlkSetTransferMode(SDE_MEMBLK_TRANSFER_MODE transfer_mode);
SDE_ERR sdMemBlkQueGetPushNum(Sint32 *push_num);
/* memory block field setters, and sdMemBlkExecuteCallback, which calls a
 * block's callback once its data is in sound memory. only sdMemBlkSetCallback
 * and sdMemBlkExecuteCallback accept a NULL handle, for which they return
 * SDE_ERR_HANDLE_NULL */
SDE_ERR sdMemBlkSetStat(SDMEMBLK handle, SDE_MEMBLK_STAT stat);
SDE_ERR sdMemBlkSetAdr(SDMEMBLK handle, const void *src_blk_ptr);
SDE_ERR sdMemBlkSetSz(SDMEMBLK handle, Sint32 src_blk_sz);
SDE_ERR sdMemBlkSetCallback(SDMEMBLK handle,
                            SD_MEMBLK_CALLBACK_FUNC callback_func,
                            const void *callback_first_arg);
SDE_ERR sdMemBlkExecuteCallback(SDMEMBLK handle);
/* memory block transfer to sound memory: by the CPU, at once, or by DMA, with a
 * queue that holds blocks until the transfer in progress ends */
SDE_ERR sdMemBlkCpuTransfer(SDMEMBLK handle, Sint32 dst_ptr);
SDE_ERR sdMemBlkQuePush(SDMEMBLK handle, Sint32 dst_ptr);
SDE_ERR sdMemBlkQuePop(SDMEMBLK *handle, Sint32 *dst_ptr);
SDE_ERR sdMemBlkDmaTransfer(SDMEMBLK handle, Sint32 dst_ptr);
void sdMemBlkWaitSync(void);
void sdMemBlkG2DmaErrTrap(void);
/* end the transfer of the block given as first_arg, and start the next queued
 * block */
void sdMemBlkEndTransferHandller(void *first_arg);
/* copy size bytes from src_ptr to sound memory at dst_ptr by the CPU, then call
 * callback_func unless it is SDD_MEMBLK_NO_FUNC or SDD_MEMBLK_SYNC_FUNC.
 * src_ptr and size must be multiples of 4; otherwise it returns
 * SDE_ERR_UNKNOW_NUM */
SDE_ERR sdMoveWrkMemToSndMem(void *src_ptr, Sint32 dst_ptr, Sint32 size,
                             SD_MEMBLK_CALLBACK_FUNC callback_func,
                             void *callback_first_arg);
SDE_ERR sdMidiOpenPort(SDMIDI *handle);
SDE_ERR sdMidiPlay(SDMIDI handle, const Sint8 bank_num, const Sint8 data_num,
                   const Sint8 priority);
/* pack the MIDI message whose status byte is midi_mes into *midi_mes_ptr, for
 * sdMidiSendMes. the arguments after midi_mes are its data bytes: one for
 * SDD_MIDI_PRG_CHG and SDD_MIDI_MONO_PRES, and two for any other channel
 * message. a system message packs as four zeros */
SDE_ERR sdMidiSetMes(SDMIDI handle, SDS_MIDI_MES *midi_mes_ptr,
                     const Uint8 midi_mes, ...);
SDE_ERR sdMidiSetVol(SDMIDI handle, const Sint8 vol, const Sint32 fade_time);
SDE_ERR sdMidiStop(SDMIDI handle);
/* queue the four bytes sdMidiSetMes packed into *midi_mes_ptr as a host
 * command. priority is ignored */
SDE_ERR sdMidiSendMes(SDMIDI handle, const SDS_MIDI_MES *midi_mes_ptr,
                      const Sint8 priority);
SDE_ERR sdMidiGetCurAdr(SDMIDI handle, Sint32 *cur_adr);
SDE_ERR sdMidiGetStat(SDMIDI handle, SDS_MIDI_STAT *midi_stat);
SDE_ERR sdMidiGetTotalBeatTime(SDMIDI handle, Uint32 *cur_beat_time);
/* each reads one value of a MIDI port's state from the driver. sdMidiGetPitch
 * returns SDE_ERR_NOTHING, even when sdSysErrTrap has recorded an error */
SDE_ERR sdMidiGetDrctLev(SDMIDI handle, Sint8 *drct_lev);
SDE_ERR sdMidiGetErr(SDMIDI handle, Uint32 *err_num);
SDE_ERR sdMidiGetFlg(SDMIDI handle, Uint32 *flg);
SDE_ERR sdMidiGetFxLev(SDMIDI handle, Sint8 *fx_lev);
SDE_ERR sdMidiGetPan(SDMIDI handle, Sint8 *pan);
SDE_ERR sdMidiGetPitch(SDMIDI handle, Sint16 *pitch);
SDE_ERR sdMidiGetSpeed(SDMIDI handle, Sint16 *speed);
SDE_ERR sdMidiGetVol(SDMIDI handle, Sint8 *vol);
SDE_ERR sdPstmClosePort(SDPSTM handle);
/* the play position in bytes, from the driver's in samples: doubled for
 * 16-bit PCM and halved for 4-bit ADPCM */
SDE_ERR sdPstmGetCurAdr(SDPSTM handle, const Sint8 target_ch,
                        Sint32 *cur_smp_adr);
SDE_ERR sdPstmGetStat(SDPSTM handle, const Sint8 target_ch,
                      SDS_PSTM_STAT *pstm_stat);
SDE_ERR sdPstmGetTotalSmpFrame(SDPSTM handle, const Sint8 target_ch,
                               Uint32 *cur_smp_frame);
/* open a stream port of use_ch channels, 1 to SDD_PSTM_PORT_CH_MAX, whose
 * driver channels follow as int arguments, each 0 to 15 */
SDE_ERR sdPstmOpenPort(SDPSTM *handle, const Sint8 use_ch, ...);
/* start the port's stream at freq, a pitch word like those sdgFreqTable gives,
 * with priority 0 to 15 */
SDE_ERR sdPstmPlay(SDPSTM handle, const SDE_PCM_TYPE pcm_type,
                   const Uint16 freq, const Sint8 priority);
/* sdPstmPlay with freq in Hz, one of the rates sdgFreqTable lists, or
 * SDE_ERR_PRM_OVER_RANGE */
SDE_ERR sdPstmPlayMw(SDPSTM handle, const SDE_PCM_TYPE pcm_type,
                     const Uint16 freq, const Sint8 priority);
SDE_ERR sdPstmResetPrm(SDPSTM handle);
SDE_ERR sdPstmSetBaseVol(SDPSTM handle, const Sint8 base_vol);
SDE_ERR sdPstmSetDrctLev(SDPSTM handle, const Sint8 drct_lev);
SDE_ERR sdPstmSetFxCh(SDPSTM handle, const Sint8 fx_in_ch,
                      const Sint8 fx_base_lev);
/* store pan as target_ch's, and send it with fade_time. on a port of two
 * channels, the left channel's pan is halved into 0x80 to 0xff, and the right's
 * into 0x00 to 0x7f. a call for SDD_PSTM_ALL_CH, -1, stores pan in the entry
 * before the port's first pan. it then sends both channel bits, with the pans
 * and fade times the previous call left in the command */
SDE_ERR sdPstmSetPan(SDPSTM handle, const Sint8 target_ch, const Sint8 pan,
                     const Sint32 fade_time);
SDE_ERR sdPstmSetPitch(SDPSTM handle, const Sint16 pitch,
                       const Sint32 fade_time);
SDE_ERR sdPstmSetUseCh(SDPSTM handle, const Sint8 use_ch);
SDE_ERR sdPstmSetVol(SDPSTM handle, const Sint8 target_ch, const Sint8 vol,
                     const Sint32 fade_time);
SDE_ERR sdPstmStop(SDPSTM handle);
SDE_ERR sdPstmStopAll(void);
/* each reads one value of a stream port channel's state from the driver.
 * sdPstmGetPan reads the pan sdPstmSetPan gave the port back through
 * target_ch. on a port of two channels, each channel holds half the port's pan,
 * less 64 on the left and plus 64 on the right. sdPstmGetPan takes that offset
 * back off the channel's pan and doubles what is left, reading a right
 * channel's 0 as -1 */
SDE_ERR sdPstmGetDrctLev(SDPSTM handle, const Sint8 target_ch, Sint8 *drct_lev);
SDE_ERR sdPstmGetErr(SDPSTM handle, const Sint8 target_ch, Uint32 *err_num);
SDE_ERR sdPstmGetFlg(SDPSTM handle, const Sint8 target_ch, Uint32 *flg);
SDE_ERR sdPstmGetFxLev(SDPSTM handle, const Sint8 target_ch, Sint8 *fx_lev);
SDE_ERR sdPstmGetPan(SDPSTM handle, const Sint8 target_ch, Sint8 *pan);
SDE_ERR sdPstmGetPitch(SDPSTM handle, const Sint8 target_ch, Sint16 *pitch);
SDE_ERR sdPstmGetSpeed(SDPSTM handle, const Sint8 target_ch, Sint16 *speed);
SDE_ERR sdPstmGetVol(SDPSTM handle, const Sint8 target_ch, Sint8 *vol);

/* a GD-ROM file handle */
typedef struct GDS_FS_HANDLE *GDFS;
typedef struct GDS_FS_DIRREC_TBL *GDFS_DIRREC;
typedef void (*GDFS_ERRFUNC)(void *obj, Sint32 err);
typedef void (*GDFS_FUNC)(void *obj);
#define GDD_ERR_OK 0
#define GDD_ERR_INIT (-1)
#define GDD_ERR_RESET (-2)
#define GDD_ERR_LIBOV (-3)
#define GDD_ERR_MOUNT (-4)
#define GDD_ERR_DISC (-5)
#define GDD_ERR_NOTFOUND (-8)
#define GDD_ERR_NOHNDL (-9)
#define GDD_ERR_ILLHNDL (-10)
#define GDD_ERR_NOTDIR (-11)
#define GDD_ERR_DIROVER (-12)
#define GDD_ERR_BUSY (-13)
#define GDD_ERR_32ALIGN (-14)
#define GDD_ERR_SIZE (-15)
#define GDD_ERR_SEEK (-16)
#define GDD_ERR_OFS (-17)
#define GDD_ERR_ILLTMODE (-18)
#define GDD_ERR_READ (-19)
#define GDD_ERR_NOTREAD (-20)
#define GDD_ERR_TOUT (-21)
#define GDD_ERR_TRAYOPEND (-23)
#define GDD_ERR_SIZEOVER (-24)
#define GDD_ERR_FATAL (-25)
#define GDD_ERR_UNDEF (-26)
#define GDD_ERR_NOERR (-27)
#define GDD_ERR_RECOVER (-28)
#define GDD_ERR_NOTREADY (-29)
#define GDD_ERR_MEDIA (-30)
#define GDD_ERR_HWARE (-31)
#define GDD_ERR_ILLREQ (-32)
#define GDD_ERR_UNITATTENT (-33)
#define GDD_ERR_PROTECT (-34)
#define GDD_ERR_ABORT (-35)
#define GDD_ERR_NOREADABLE (-36)
#define GDD_ERR_CHECKBUSY (-37)
/* request states of a file handle, and drive states */
#define GDD_STAT_IDLE 0
#define GDD_STAT_COMPLETE 1
#define GDD_STAT_READ 2
#define GDD_STAT_SEEK 3
#define GDD_STAT_BUSY 4
#define GDD_STAT_ERR 5
#define GDD_STAT_FATAL 6
#define GDD_STAT_CONTINUE 7
#define GDD_STAT_UNDEF 8
#define GDD_DRVSTAT_CANTREAD (-1)
#define GDD_DRVSTAT_BUSY 0x00
#define GDD_DRVSTAT_OPEN 0x06
#define GDD_FS_SCTSIZE 2048
#define GDD_FS_FNAMESIZE (30 + 1) /* a file name's bytes, with its NUL */
/* the directory bit of a directory entry's flags */
#define GDD_FF_DIRECTORY 0x02
#define GDD_FS_TMODE_CPU 0
#define GDD_FS_TMODE_DMA 1
#define GDD_FS_TRANS_READY 0
#define GDD_FS_TRANS_BUSY 1
#define GDD_FS_TRANS_COMPLETE 2
/* the work size in bytes for max_open files, and the size of a directory record
 * of max_dirent entries */
#define GDFS_WORK_SIZE(max_open) (4340 + ((max_open) * 80))
#define GDFS_DIRREC_SIZE(max_dirent) (20 + ((max_dirent) * 44))

/* gdFsInitOvl opens the library, and gdFsFinish closes it. while it is closed,
 * each call that reaches the library through its interface returns 0, NULL or
 * FALSE, or does nothing. gdFsReinit returns GDD_ERR_INIT instead. those calls
 * are gdFsReinit, gdFsLoadDir, gdFsSetDir, gdFsGetDirInfo, gdFsOpen,
 * gdFsOpenRange, gdFsGetWorkHn, gdFsGetSysHn, gdFsGetToc, gdFsGetDrvStat,
 * gdFsReqDrvStat, gdFsIsTrayOpen, gdFsEntryErrFuncAll, gdFsExecServer and the
 * gdFsDa calls. while the library is open, gdFsFinish and each of those calls
 * but gdFsReinit need the work area gdFsInitOvl takes */

/* the library's own requests run through the system handle, which
 * gdFsGetSysHn returns. a call that makes one returns GDD_ERR_BUSY while the
 * system handle's request is running, or when the drive refuses the request.
 * a call that waits for its request returns gdFsWaitCompleteReq's result
 * (gdfs.h) */

/* the same as gdFsInitOvl */
Sint32 gdFsInit(Sint32 max_open, void *gdfs_work, Sint32 max_dirent,
                void *dirbuf);
/* open the library unless it is open, and start it in gdfs_work, with
 * max_open file handles and a current directory of max_dirent entries in
 * dirbuf. then start the drive, and read the root directory into the current
 * directory. gdfs_work must be aligned to 32 bytes. it returns GDD_ERR_OK once
 * the drive has started, and at once when it had started already. it returns
 * GDD_ERR_LIBOV when the library doesn't open, and GDD_ERR_32ALIGN for an
 * unaligned gdfs_work, before it takes the work area. any other result is from
 * the drive's start. it returns GDD_ERR_RESET, GDD_ERR_TRAYOPEND or the wait's
 * error when the drive fails to start. it returns GDD_ERR_DISC for a disc that
 * isn't a GD-ROM, has no ISO 9660 volume, or has an empty root. it returns
 * GDD_ERR_MOUNT when the volume or the root can't be read, and GDD_ERR_DIROVER
 * for a root with more entries than the current directory holds */
Sint32 gdFsInitOvl(Sint32 max_open, void *gdfs_work, Sint32 max_dirent,
                   void *dirbuf);
/* close every handle, then start the library in its work area and the drive
 * again, as gdFsInitOvl does. the global error callback stays, and in 1.30J
 * so does what gdFsIsTrayOpen has found. it returns GDD_ERR_INIT until
 * gdFsInitOvl has taken a work area, and otherwise the drive's start's
 * result */
Sint32 gdFsReinit(void);
/* remove the library's interrupt handlers, close every handle, and close the
 * library */
void gdFsFinish(void);
GDFS_DIRREC gdFsCreateDirhn(void *dirbuf, Sint32 max_dirent);
/* read a directory into gf_dirrec, or into the current directory when
 * gf_dirrec is NULL. a dirname that starts with a slash or backslash is a path
 * from the root, and any other is a name in the current directory. names match
 * whatever their case. it returns GDD_ERR_OK, and does so at once when
 * gf_dirrec was last read from that directory. it returns GDD_ERR_NOTFOUND for
 * no such directory, or a path table that can't be read. it returns
 * GDD_ERR_NOTDIR for a name that isn't a directory's. it returns the read's
 * error when a read fails, and GDD_ERR_DIROVER for a directory with more
 * entries than gf_dirrec holds. either leaves gf_dirrec marked unread */
Sint32 gdFsLoadDir(const char *dirname, GDFS_DIRREC gf_dirrec);
/* copy gf_dirrec into the current directory, as many of its entries as fit. it
 * returns GDD_ERR_DIROVER when some don't fit, and GDD_ERR_OK otherwise */
Sint32 gdFsSetDir(GDFS_DIRREC gf_dirrec);
Sint32 gdFsChangeDir(const char *dirname);
/* open fname from gf_dirrec, or from the current directory when gf_dirrec is
 * NULL, on the first closed file handle. fname matches whatever its case. it
 * returns the handle, and sets the global error to GDD_ERR_OK. it returns NULL
 * when no entry has that name, and sets the global error to GDD_ERR_NOTFOUND.
 * it returns NULL when every file handle is open, and sets the global error to
 * GDD_ERR_NOHNDL */
GDFS gdFsOpen(const char *fname, GDFS_DIRREC gf_dirrec);
/* open nsct sectors from frame address stsct as a file, as gdFsOpen opens an
 * entry. it returns NULL for a stsct below 45150, the high-density area's
 * first frame address, and sets the global error to GDD_ERR_ILLREQ */
GDFS gdFsOpenRange(Sint32 stsct, Sint32 nsct);
typedef struct GDS_FS_DIRINFO {
    Sint32 fad; /* the entry's first frame address */
    Sint32 fsize; /* its size in bytes */
    Uint8 flag; /* its GDD_FF_ bits */
    Uint8 pad[3];
} GDFS_DIRINFO;
/* copy the frame address, size and flags of name's entry in the current
 * directory into dirinfo. name matches whatever its case. it returns
 * GDD_ERR_OK, or GDD_ERR_NOTFOUND */
Sint32 gdFsGetDirInfo(const char *name, GDFS_DIRINFO *dirinfo);
Bool gdFsGetFileSize(GDFS gdfs, Sint32 *fsize);
Bool gdFsGetFileSctSize(GDFS gdfs, Sint32 *fsctsize);
Bool gdFsGetFileFad(GDFS gdfs, Sint32 *fad);
/* gdFsSeek origins: the file's start, its next sector to read, or its end */
#define GDD_SEEK_SET 0
#define GDD_SEEK_CUR 1
#define GDD_SEEK_END 2
Sint32 gdFsSeek(GDFS gdfs, Sint32 sctno, Sint32 type);
Sint32 gdFsTell(GDFS gdfs);
Sint32 gdFsCheckEof(GDFS gdfs, Bool *iseof);
/* read nsct sectors from the handle's next sector into buf, up to the file's
 * end, and wait for them. a buf aligned to 32 bytes is read in the handle's
 * transfer mode, and any other by the CPU. it returns a request's error, such
 * as GDD_ERR_ILLHNDL, at once, and otherwise the wait's result. 1.30J makes the
 * request again while another request runs or the drive refuses it, up to 20000
 * times. it returns GDD_ERR_TOUT when all are refused. 1.20J reads nothing
 * then, yet the wait can return GDD_ERR_OK */
Sint32 gdFsRead(GDFS gdfs, Sint32 nsct, void *buf);
Sint32 gdFsReqRd32(GDFS gdfs, Sint32 nsct, void *buf);
Sint32 gdFsGetStat(GDFS gdfs);
Sint32 gdFsGetErrStat(GDFS gdfs);
/* abort the handle's request and wait until it ends: a read, or in 1.30J a
 * read or a busy request. it returns GDD_ERR_OK at once, and marks the handle
 * idle, when the handle has no such request. it returns GDD_ERR_BUSY when that
 * request isn't the one running, and otherwise the wait's result. before the
 * abort, 1.30J waits for the drive's wait value to reach 1, and returns
 * GDD_ERR_TOUT when that takes more than 20 seconds */
Sint32 gdFsStopRd(GDFS gdfs);
Sint32 gdFsGetNumRd(GDFS gdfs);
/* seek the head to the handle's next sector, and wait for the seek. it returns
 * GDD_ERR_BUSY when a request is running or the drive refuses the seek, and
 * otherwise the wait's result */
Sint32 gdFsMovePickup(GDFS gdfs);
/* start a read of nsct sectors from the handle's next sector, up to the file's
 * end, whose sectors the drive holds until gdFsTrans32 moves them. it returns
 * the sectors requested, or GDD_ERR_ILLHNDL for a closed handle. it returns 0
 * while another request runs. it also returns 0 when nothing is left to read
 * or the drive refuses the read, and marks the handle's request complete */
Sint32 gdFsReqGdRd(GDFS gdfs, Sint32 nsct);
/* start moving nbytes of the sectors the drive holds for the gdFsReqGdRd read
 * into buf, by DMA. buf must be aligned to 32 bytes, or it returns
 * GDD_ERR_32ALIGN. nbytes must be a multiple of 32, or it returns
 * GDD_ERR_SIZE. it returns GDD_ERR_NOTREAD when the handle's request isn't a
 * gdFsReqGdRd read. it returns GDD_ERR_OK once the transfer starts. when the
 * drive refuses it, it returns GDD_ERR_SIZEOVER for the drive's -1, and
 * GDD_ERR_BUSY otherwise */
Sint32 gdFsTrans32(GDFS gdfs, Sint32 nbytes, void *buf);
Sint32 gdFsGetTransStat(GDFS gdfs);
GDFS gdFsGetWorkHn(void);
GDFS gdFsGetSysHn(void);
/* read the disc's table of contents of type into buf through the system
 * handle, and wait for it */
Sint32 gdFsGetToc(Sint32 type, void *buf);
void gdFsClose(GDFS gdfs);
/* the drive state, or GDD_DRVSTAT_CANTREAD while the drive is busy */
Sint32 gdFsGetDrvStat(void);
/* request a frame's Q channel subcode through the system handle, and return
 * GDD_ERR_OK without waiting. the request's data is never read */
Sint32 gdFsReqDrvStat(void);
/* in 1.30J, TRUE once the library has found the tray open, and FALSE
 * otherwise. the library finds the tray open when this call's gdFsGetDrvStat
 * returns GDD_DRVSTAT_OPEN, or when a request fails with GDD_ERR_TRAYOPEND or
 * GDD_ERR_UNITATTENT. a gdFsInitOvl that starts the library forgets it. when
 * gdFsGetDrvStat returns GDD_DRVSTAT_BUSY, the call also calls
 * gdFsReqDrvStat */
Bool gdFsIsTrayOpen(void);
void gdFsEntryRdEndFunc(GDFS gdfs, GDFS_FUNC func, void *obj);
void gdFsEntryTrEndFunc(GDFS gdfs, GDFS_FUNC func, void *obj);
void gdFsEntryErrFunc(GDFS gdfs, GDFS_ERRFUNC erfunc, void *obj);
void gdFsEntryErrFuncAll(GDFS_ERRFUNC erfunc, void *obj);
void gdFsExecServer(void);
/* CD-DA playback through the system handle. none waits for its request, and
 * each returns GDD_ERR_OK once the drive takes it. a drive of version 1.01 or
 * later keeps a play that gdFsDaPlay or gdFsDaPlaySct starts, for
 * gdFsDaRelease */

/* play tracks st_track to end_track, repeated reptime times. it returns
 * GDD_ERR_BUSY for a st_track below 3 */
Sint32 gdFsDaPlay(Sint32 st_track, Sint32 end_track, Sint32 reptime);
/* play frame addresses st_sct to end_sct, as gdFsDaPlay plays tracks. it
 * returns GDD_ERR_BUSY for a st_sct below 45150 */
Sint32 gdFsDaPlaySct(Sint32 st_sct, Sint32 end_sct, Sint32 reptime);
/* gdFsDaStop and gdFsDaPause each pause the play */
Sint32 gdFsDaStop(void);
Sint32 gdFsDaPause(void);
/* end the pause. a kept play that paused at the frame address where it started
 * plays again from its start, with its own repeat count, and stays kept. any
 * other play is released with reptime, and is no longer kept */
Sint32 gdFsDaRelease(Sint32 reptime);
/* the head position; min, sec and frame are the time within the track */
typedef struct GDS_FS_DAINFO {
    Sint32 track;
    Sint32 min;
    Sint32 sec;
    Sint32 frame;
    Sint32 fad;
} GDFS_DAINFO;
/* request a frame's whole subcode. once the request completes, the server
 * decodes the head position into dainfo, which must stay valid until then. a
 * subcode with no valid position fails the request with GDD_ERR_READ */
Sint32 gdFsDaGetInfo(GDFS_DAINFO *dainfo);

/* the boot ROM's GD-ROM drive calls (gdc_lib.src) */

/* issue command cmd with the parameters at param. it returns the request's
 * number, or 0 when the drive refuses the command */
Sint32 gdGdcReqCmd(Sint32 cmd, void *param);
/* the state of request gdchn, with the drive's report of it in stat */
Sint32 gdGdcGetCmdStat(Sint32 gdchn, void *stat);
/* run a step of the drive's server */
void gdGdcExecServer(void);
/* initialize the drive's calls */
void gdGdcInitSystem(void);
/* store the drive state and the disc type in info. it returns 1 while the
 * drive is busy */
Sint32 gdGdcGetDrvStat(Sint32 *info);
/* end a DMA from the drive, on its end interrupt. func, with obj, is the
 * transfer's end callback, or NULL */
void gdGdcG1DmaEnd(GDFS_FUNC func, void *obj);
/* start a DMA of what request gdchn has read: param holds the address and the
 * byte count. it returns 0 when the DMA starts */
Sint32 gdGdcReqDmaTrans(Sint32 gdchn, Sint32 *param);
/* the state of request gdchn's DMA, with a byte count in size */
Sint32 gdGdcCheckDmaTrans(Sint32 gdchn, Sint32 *size);
/* abort request gdchn */
void gdGdcReadAbort(Sint32 gdchn);
/* reset the drive's calls */
void gdGdcReset(void);

#endif
