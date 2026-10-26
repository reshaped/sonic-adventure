/* backup library interface over the VMS file system */
#include <machine.h>
#include <rfiber.h>
#include <sg_chain.h>
#include <sg_xpt.h>
#include <shinobi.h>
#include <vmsfs.h>

const char buBuild[] = "\nbu Ver 1.13 Build:Oct 28 1998 01:15:09\n"
                       "VMS File System for Application(dynamic work alloc) ";
const Uint32 buChLevel = 5;
static Uint32 library_version = 0x98;
Uint32 buInitializeFlag = 0;
SYCHAIN buChID;
/* masks of drives, bit n for drive n, that buExecServer keeps. the first two
 * hold the drives ready at its previous call and at its latest */
static Uint32 drives_last_ready;
static Uint32 drives_ready;
/* the drives with a mount, or a report of a removal, waiting for their
 * command to end */
static Uint32 drives_to_mount;
static Uint32 drives_to_remove;
/* buInit's work area, or NULL. with NULL, buExecServer connects an inserted
 * card without mounting it, and the caller mounts it with buMountDisk, which
 * gives the card's drive its work */
void *buWorkTop;

void buInterruptHandler(void *arg);
Sint32 buConnect(Sint32 drive);

/* buInit's handler on the Maple DMA end chain: run buExecServer */
void buInterruptHandler(void *arg)
{
    buExecServer();
}

Sint32 buInit(Sint32 capacity, Sint32 drives, void *work, BU_INITCALLBACK func)
{
    Sint32 result;
    Sint32 mask = get_imask();

    set_imask(IMASK_ALL);
    buWorkTop = work;
    drives_last_ready = 0;
    drives_ready = 0;
    drives_to_mount = 0;
    drives_to_remove = 0;
    rfInitSystem(0);
    vmsfs_c_init(drives);
    result = vmsfs_init(capacity, drives, work);
    buChID = syChainAddHandler(SYD_CHAIN_EVENT_IML4_MPDMAE, buInterruptHandler,
                               buChLevel, NULL);
    if (result == BUD_ERR_OK && func != NULL) {
        func();
    }
    buInitializeFlag = true;
    set_imask(mask);
    return result;
}

void buDisableServer(void)
{
    Sint32 mask = get_imask();

    set_imask(IMASK_ALL);
    syChainDeleteHandler(buChID);
    set_imask(mask);
}

Sint32 buExit(void)
{
    Sint32 drive;

    buInitializeFlag = false;
    for (drive = 0; drive < VMSFS_DRIVES; ++drive) {
        if (vmsfs_c_getstate(drive) != BUD_STAT_READY) {
            return BUD_ERR_BUSY;
        }
    }
    syChainDeleteHandler(buChID);
    vmsfs_c_exit();
    return vmsfs_exit();
}

void buExecServer(void)
{
    Sint32 drive;
    Uint32 idle;
    Uint32 changed;
    Sint32 bit;
    Sint32 (*mount)(Sint32 drive);

    if (!buInitializeFlag) {
        return;
    }
    if (buWorkTop != NULL) {
        mount = buMount;
    } else {
        mount = buConnect;
    }
    vmsfs_c_synch();
    drives_last_ready = drives_ready;
    drives_ready = vmsfs_isready();
    idle = 0;
    for (drive = 0, bit = 1; drive < VMSFS_DRIVES; ++drive, bit <<= 1) {
        idle |= buStat(drive) == BUD_STAT_READY ? bit : 0;
    }
    changed = (drives_last_ready ^ drives_ready) & drives_ready;
    for (drive = 0, bit = 1; drive < VMSFS_DRIVES; ++drive, bit <<= 1) {
        if (changed & bit) {
            if (idle & bit) {
                mount(drive);
                drives_to_mount &= ~bit;
            } else {
                drives_to_mount |= bit;
            }
        }
        if (idle & bit) {
            if (drives_to_mount & bit) {
                mount(drive);
                drives_to_mount &= ~bit;
            }
        }
    }
    changed = (drives_ready ^ drives_last_ready) & drives_last_ready;
    for (drive = 0, bit = 1; drive < VMSFS_DRIVES; ++drive, bit <<= 1) {
        if (changed & bit) {
            if (idle & bit) {
                vmsfs_c_execcallback(drive, BUD_OP_UNMOUNT, BUD_ERR_OK);
                drives_to_remove &= ~bit;
            } else {
                drives_to_remove |= bit;
            }
        }
        if (idle & bit) {
            if (drives_to_remove & bit) {
                vmsfs_c_execcallback(drive, BUD_OP_UNMOUNT, BUD_ERR_OK);
                drives_to_remove &= ~bit;
            }
        }
    }
}

void buSetCompleteCallback(BU_COMPLETECALLBACK func)
{
    vmsfs_c_setcompletecallback(func);
}

void buSetProgressCallback(BU_PROGRESSCALLBACK func)
{
    vmsfs_c_setprogresscallback(func);
}

void buSetCallbackParam(Uint32 param)
{
    vmsfs_c_setcallbackparam(param);
}

Uint32 buGetVersion(void)
{
    return library_version;
}

Sint32 buIsReady(Sint32 drive)
{
    return vmsfs_ismount(drive);
}

Sint32 buMount(Sint32 drive)
{
    if (vmsfs_c_getstate(drive)) {
        return BUD_ERR_BUSY;
    }
    vmsfs_c_setfunc(drive, BUD_OP_MOUNT);
    vmsfs_c_setparam(drive, drive, 0, 0, 0, 0, 0);
    return BUD_ERR_OK;
}

/* queue a connect, which reads the media information of drive's card
 * without mounting it */
Sint32 buConnect(Sint32 drive)
{
    if (vmsfs_c_getstate(drive)) {
        return BUD_ERR_BUSY;
    }
    vmsfs_c_setfunc(drive, BUD_OP_CONNECT);
    vmsfs_c_setparam(drive, drive, 0, 0, 0, 0, 0);
    return BUD_ERR_OK;
}

Sint32 buRemount(Sint32 drive)
{
    return buMount(drive);
}

Sint32 buUnmount(Sint32 drive)
{
    if (vmsfs_c_getstate(drive)) {
        return BUD_ERR_BUSY;
    }
    return vmsfs_unmount(drive);
}

Sint32 buIsFormat(Sint32 drive)
{
    return vmsfs_isformat(drive);
}

Sint32 buFormatDisk(Sint32 drive, const Uint8 *volume, Sint32 icon,
                    const BUS_TIME *time, Uint32 flag)
{
    if (vmsfs_c_getstate(drive)) {
        return BUD_ERR_BUSY;
    }
    vmsfs_c_setfunc(drive, BUD_OP_FORMATDISK);
    vmsfs_c_setparam(drive, drive, (Sint32)volume, icon, (Sint32)time, flag, 0);
    vmsfs_c_setvolume(drive, volume);
    vmsfs_c_settime(drive, time);
    return BUD_ERR_OK;
}

Sint32 buGetDiskInfo(Sint32 drive, BUS_DISKINFO *info)
{
    if (vmsfs_c_getstate(drive)) {
        return BUD_ERR_BUSY;
    }
    return vmsfs_getinfo(drive, info);
}

Sint32 buGetDiskFree(Sint32 drive, Sint32 type)
{
    if (vmsfs_c_getstate(drive)) {
        return BUD_ERR_BUSY;
    }
    return vmsfs_diskfree(drive, type);
}

Sint32 buFindFirstFile(Sint32 drive, char *fname)
{
    if (vmsfs_c_getstate(drive)) {
        return BUD_ERR_BUSY;
    }
    return vmsfs_findfirst(drive, fname);
}

Sint32 buFindNextFile(Sint32 drive, char *fname)
{
    if (vmsfs_c_getstate(drive)) {
        return BUD_ERR_BUSY;
    }
    return vmsfs_findnext(drive, fname);
}

Sint32 buFindFirstFileEx(Sint32 drive, char *fname, BUS_FILEINFO *finfo)
{
    if (vmsfs_c_getstate(drive)) {
        return BUD_ERR_BUSY;
    }
    return vmsfs_findfirst_ex(drive, fname, finfo);
}

Sint32 buFindNextFileEx(Sint32 drive, char *fname, BUS_FILEINFO *finfo)
{
    if (vmsfs_c_getstate(drive)) {
        return BUD_ERR_BUSY;
    }
    return vmsfs_findnext_ex(drive, fname, finfo);
}

Sint32 buFindExecFile(Sint32 drive, char *fname)
{
    if (vmsfs_c_getstate(drive)) {
        return BUD_ERR_BUSY;
    }
    return vmsfs_xfind(drive, fname);
}

Sint32 buOpenFile(Sint32 drive, const char *fname, BUFILE *file)
{
    if (vmsfs_c_getstate(drive)) {
        return BUD_ERR_BUSY;
    }
    return vmsfs_fopen(drive, fname, file);
}

Sint32 buCreateFile(Sint32 drive, const char *fname, BUS_TIME *time,
                    BUFILE *file)
{
    if (vmsfs_c_getstate(drive)) {
        return BUD_ERR_BUSY;
    }
    return vmsfs_fcreate(drive, fname, time, file);
}

Sint32 buCreateExecFile(Sint32 drive, const char *fname, BUS_TIME *time,
                        BUFILE *file)
{
    if (vmsfs_c_getstate(drive)) {
        return BUD_ERR_BUSY;
    }
    return vmsfs_xcreate(drive, fname, time, file);
}

Sint32 buSetFileParam(BUFILE file, Uint16 hdrofs, Uint8 copyflag)
{
    if (vmsfs_c_getstate(file->drive)) {
        return BUD_ERR_BUSY;
    }
    return vmsfs_fsetparam(file, hdrofs, copyflag);
}

Sint32 buCloseFile(BUFILE file)
{
    Sint32 drive = file->drive;

    if (vmsfs_c_getstate(drive)) {
        return BUD_ERR_BUSY;
    }
    vmsfs_c_setfunc(drive, BUD_OP_CLOSEFILE);
    vmsfs_c_setparam(drive, (Sint32)file, 1, 0, 0, 0, 0);
    return BUD_ERR_OK;
}

Sint32 buReadFile(BUFILE file, void *buf, Uint32 nblock)
{
    Sint32 drive = file->drive;

    if (vmsfs_c_getstate(drive)) {
        return BUD_ERR_BUSY;
    }
    vmsfs_c_setfunc(drive, BUD_OP_READFILE);
    vmsfs_c_setparam(drive, (Sint32)file, (Sint32)buf, nblock, 0, 0, 0);
    return BUD_ERR_OK;
}

Sint32 buWriteFile(BUFILE file, const void *buf, Uint32 nblock)
{
    Sint32 drive = file->drive;

    if (vmsfs_c_getstate(drive)) {
        return BUD_ERR_BUSY;
    }
    vmsfs_c_setfunc(drive, BUD_OP_WRITEFILE);
    vmsfs_c_setparam(drive, (Sint32)file, (Sint32)buf, nblock, 0, 0, 0);
    return BUD_ERR_OK;
}

Sint32 buDeleteFile(Sint32 drive, const char *fname)
{
    if (vmsfs_c_getstate(drive)) {
        return BUD_ERR_BUSY;
    }
    vmsfs_c_setfunc(drive, BUD_OP_DELETEFILE);
    vmsfs_c_setparam(drive, drive, (Sint32)fname, 0, 0, 0, 0);
    vmsfs_c_setfname(drive, fname, fname);
    return BUD_ERR_OK;
}

Sint32 buDeleteFileEx(Sint32 drive, const char **fname, Sint32 nfiles)
{
    if (vmsfs_c_getstate(drive)) {
        return BUD_ERR_BUSY;
    }
    vmsfs_c_setfunc(drive, BUD_OP_DELETEFILEEX);
    vmsfs_c_setparam(drive, drive, (Sint32)fname, nfiles, 0, 0, 0);
    return BUD_ERR_OK;
}

Sint32 buSetFileAttr(Sint32 drive, const char *fname, Uint16 hdrofs,
                     Uint8 copyflag)
{
    if (vmsfs_c_getstate(drive)) {
        return BUD_ERR_BUSY;
    }
    vmsfs_c_setfunc(drive, BUD_OP_SETFILEATTR);
    vmsfs_c_setparam(drive, drive, (Sint32)fname, hdrofs, copyflag, 0, 0);
    vmsfs_c_setfname(drive, fname, fname);
    return BUD_ERR_OK;
}

Sint32 buIsExistFile(Sint32 drive, const char *fname)
{
    if (vmsfs_c_getstate(drive)) {
        return BUD_ERR_BUSY;
    }
    return vmsfs_fisexist(drive, fname);
}

Sint32 buGetFileInfo(Sint32 drive, const char *fname, BUS_FILEINFO *finfo)
{
    if (vmsfs_c_getstate(drive)) {
        return BUD_ERR_BUSY;
    }
    return vmsfs_fgetinfo(drive, fname, finfo);
}

Sint32 buStat(Sint32 drive)
{
    if (vmsfs_c_getstate(drive)) {
        return BUD_STAT_BUSY;
    }
    return BUD_STAT_READY;
}

Sint32 buGetLastError(Sint32 drive)
{
    if (vmsfs_c_getstate(drive)) {
        return BUD_ERR_BUSY;
    }
    return buLastError[drive];
}

Sint32 buRenameFile(Sint32 drive, const char *from, const char *to)
{
    if (vmsfs_c_getstate(drive)) {
        return BUD_ERR_BUSY;
    }
    vmsfs_c_setfunc(drive, BUD_OP_RENAMEFILE);
    vmsfs_c_setparam(drive, drive, (Sint32)from, (Sint32)to, 0, 0, 0);
    vmsfs_c_setfname(drive, from, to);
    return BUD_ERR_OK;
}

Sint32 buGetFileSize(Sint32 drive, const char *fname)
{
    if (vmsfs_c_getstate(drive)) {
        return BUD_ERR_BUSY;
    }
    return vmsfs_fgetsize(drive, fname);
}

Sint32 buDefragDisk(Sint32 drive, void *work)
{
    if (vmsfs_c_getstate(drive)) {
        return BUD_ERR_BUSY;
    }
    vmsfs_c_setfunc(drive, BUD_OP_DEFRAGDISK);
    vmsfs_c_setparam(drive, drive, (Sint32)work, 0, 0, 0, 0);
    return BUD_ERR_OK;
}

Sint32 buLoadFile(Sint32 drive, const char *fname, void *buf, Uint32 nblock)
{
    if (vmsfs_c_getstate(drive)) {
        return BUD_ERR_BUSY;
    }
    vmsfs_c_setfunc(drive, BUD_OP_LOADFILE);
    vmsfs_c_setparam(drive, drive, (Sint32)fname, (Sint32)buf, nblock, 0, 0);
    vmsfs_c_setfname(drive, fname, fname);
    return BUD_ERR_OK;
}

Sint32 buSaveFile(Sint32 drive, const char *fname, const void *buf,
                  Uint32 nblock, BUS_TIME *time, Uint32 flag)
{
    if (vmsfs_c_getstate(drive)) {
        return BUD_ERR_BUSY;
    }
    vmsfs_c_setfunc(drive, BUD_OP_SAVEFILE);
    vmsfs_c_setparam(drive, drive, (Sint32)fname, (Sint32)buf, nblock,
                     (Sint32)time, flag);
    vmsfs_c_setfname(drive, fname, fname);
    vmsfs_c_settime(drive, time);
    return BUD_ERR_OK;
}

Sint32 buSaveExecFile(Sint32 drive, const char *fname, const void *buf,
                      Uint32 nblock, BUS_TIME *time, Uint32 flag)
{
    if (vmsfs_c_getstate(drive)) {
        return BUD_ERR_BUSY;
    }
    vmsfs_c_setfunc(drive, BUD_OP_SAVEEXECFILE);
    vmsfs_c_setparam(drive, drive, (Sint32)fname, (Sint32)buf, nblock,
                     (Sint32)time, flag);
    vmsfs_c_setfname(drive, fname, fname);
    vmsfs_c_settime(drive, time);
    return BUD_ERR_OK;
}

Sint32 buLoadFileEx(Sint32 drive, const char *fname, void *buf, Uint32 start,
                    Uint32 nblock)
{
    if (vmsfs_c_getstate(drive)) {
        return BUD_ERR_BUSY;
    }
    vmsfs_c_setfunc(drive, BUD_OP_LOADFILEEX);
    vmsfs_c_setparam(drive, drive, (Sint32)fname, (Sint32)buf, start, nblock,
                     0);
    vmsfs_c_setfname(drive, fname, fname);
    return BUD_ERR_OK;
}

Sint32 buRewriteExecFile(Sint32 drive, const char *fname, const void *buf,
                         Uint32 start, Uint32 nblock)
{
    if (vmsfs_c_getstate(drive)) {
        return BUD_ERR_BUSY;
    }
    vmsfs_c_setfunc(drive, BUD_OP_REWRITEEXECFILE);
    vmsfs_c_setparam(drive, drive, (Sint32)fname, (Sint32)buf, start, nblock,
                     0);
    vmsfs_c_setfname(drive, fname, fname);
    return BUD_ERR_OK;
}

/* queue a read of every block of drive's card into buf. buWriteDiskImage
 * queues the write of every block from buf */
Sint32 buReadDiskImage(Sint32 drive, void *buf)
{
    if (vmsfs_c_getstate(drive)) {
        return BUD_ERR_BUSY;
    }
    vmsfs_c_setfunc(drive, BUD_OP_READIMAGE);
    vmsfs_c_setparam(drive, drive, (Sint32)buf, 0, 0, 0, 0);
    return BUD_ERR_OK;
}

Sint32 buWriteDiskImage(Sint32 drive, const void *buf)
{
    if (vmsfs_c_getstate(drive)) {
        return BUD_ERR_BUSY;
    }
    vmsfs_c_setfunc(drive, BUD_OP_WRITEIMAGE);
    vmsfs_c_setparam(drive, drive, (Sint32)buf, 0, 0, 0, 0);
    return BUD_ERR_OK;
}

void buSetPrintFunc(void (*func)(const char *str))
{
    _vmsfs_dbg_out = func;
}

void buSetRetryCount(Sint32 count)
{
    vmsfs_setretry(count);
}

Sint32 buMountDisk(Sint32 drive, void *addr, Sint32 size)
{
    Sint32 result = vmsfs_setwork(drive, addr, size);

    if (result == BUD_ERR_OK) {
        buMount(drive);
    }
    /* @hack implicit: R0 still holds vmsfs_setwork's result where it
     * fails (MOV R0,R4 at 0x8c11b3da), and buMount's where SHC makes the
     * call the function's tail (BRA at 0x8c11b3e4); returned explicitly, 9
     * of 16 words differ */
    /* NOLINTNEXTLINE(clang-diagnostic-return-type) */
}

Sint32 buUnmountDisk(Sint32 drive)
{
    return buUnmount(drive);
}
