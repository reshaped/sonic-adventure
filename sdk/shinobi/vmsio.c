/* memory card block access through Kaede */
#include <kaede.h>
#include <sg_xpt.h>
#include <shinobi.h>
#include <vmsfs.h>

#define BLOCK_SIZE 512
/* where a response's data starts: past the frame's 8-byte header */
#define FRAME_HEADER 8
/* where the bytes start in a block read's response: past the header and one
 * more word */
#define READ_DATA (FRAME_HEADER + 4)
/* every card's executable area, in blocks */
#define EXEC_BLOCKS 128

extern void vmsfs_memcpyl(void *dst, const void *src, Sint32 size);

/* each drive's Kaede device, the two expansion sockets of each of ports A
 * to D. a ninth entry, KDD_DEV_A3, lies past the last drive */
static const Sint32 drive_devices[VMSFS_DRIVES + 1] = {
    KDD_DEV_A1, KDD_DEV_A2, KDD_DEV_B1, KDD_DEV_B2, KDD_DEV_C1,
    KDD_DEV_C2, KDD_DEV_D1, KDD_DEV_D2, KDD_DEV_A3};
/* a hook that can stand in for the card's answer to a write. vmsio_init
 * clears it, and nothing else sets it. while it is set, vmsio_block_write
 * calls it with the drive and block once the write's result arrives. a
 * nonzero return makes the write take *response as the card's response.
 * nothing reads *errcode */
Sint32 (*vmsio_sim_error)(Sint32 drive, Sint32 block, Uint8 *response,
                          Sint32 *errcode) = NULL;
/* zeroed by vmsio_init, and read nowhere */
static Sint32 dat_8c7dd47c;

void vmsio_init(void)
{
    dat_8c7dd47c = 0;
    vmsio_sim_error = NULL;
}

void vmsio_exit(void)
{
}

Sint32 vmsio_get_media_info(Sint32 drive, struct vmsfs_media *media)
{
    Sint32 device = drive_devices[drive];
    const KDS_DEVICE *dev = kdGetDevice(device);
    const KDS_DEVICEINFO *info = kdGetDeviceInfo(device);
    Sint32 seq;
    Uint8 *data;
    KDS_STORAGEINFO storage;
    const KDS_DEVICE *answer;

    if (dev->on && (info->function & KDD_FUNC_STORAGE)) {
        kdRecognizeChanged(device);
    again:
        for (;;) {
            kdGetDevice(device);
            seq = kdSetGetMediaInfo(device, KDD_FUNC_STORAGE, 0);
            if (seq >= 0) {
                break;
            }
            vmsfb_sleep();
        }
        for (;;) {
            if (kdGetStatus(device, seq)) {
                break;
            }
            vmsfb_sleep();
        }
        answer = kdGetDevice(device);
        if (kdIsChanged(device)) {
            goto gone;
        }
        data = (Uint8 *)answer->recvdata + FRAME_HEADER;
        switch (answer->response) {
        case KDD_RES_TRANSMITAGAIN:
            goto again;
        case KDD_RES_DATATRANSFER:
            media->last_block = _vm->get_word(data, 0);
            media->partition = _vm->get_word(data, 2);
            media->system_block = _vm->get_word(data, 4);
            media->fat_block = _vm->get_word(data, 6);
            media->fat_size = _vm->get_word(data, 8);
            media->dir_block = _vm->get_word(data, 10);
            media->dir_size = _vm->get_word(data, 12);
            media->spare_block = _vm->get_word(data, 16);
            media->spare_size = _vm->get_word(data, 18);
            media->exec_block = _vm->get_word(data, 20);
            media->exec_size = EXEC_BLOCKS;
            media->icon = data[14];
            media->unk_28 = data[15];
            media->last_byte = BLOCK_SIZE - 1;
            if (kdDevGetStorageInfo(&storage, kdGetDeviceInfo(device)) < 0) {
                goto gone;
            }
#ifdef GCC
            /* repairs the division by 0 phases: a card with none is no disk */
            if (storage.readcount == 0 || storage.writecount == 0) {
                goto gone;
            }
#endif
            media->read_phases = storage.readcount;
            media->write_phases = storage.writecount;
            media->read_bytes = BLOCK_SIZE / storage.readcount;
            media->write_bytes = BLOCK_SIZE / storage.writecount;
            return BUD_ERR_OK;
        default:
            return BUD_ERR_NO_DISK;
        }
    }
gone:
    vmsfb_sleep();
    return BUD_ERR_NO_DISK;
}

Sint32 vmsio_block_read(Sint32 drive, Sint32 block, void *buf)
{
    struct vmsfs_drive *vms_drive = &buVmsDrive[drive];
    Uint8 *dst;
    Sint32 device;
    Sint32 phase;
    Sint32 count;
    Sint32 seq;
    const KDS_DEVICE *dev;
    Uint8 *data;

    if (!vmsio_ismount(drive)) {
        vmsfb_sleep();
        return BUD_ERR_NO_DISK;
    }
    device = drive_devices[drive];
    kdGetDevice(device);
    dst = buf;
    /* @hack spelling: the phase counted twice, in count and phase, which
     * keeps count at @(4,R15) and phase in R8 (0x8c125fac, 0x8c125fb0); with
     * phase alone, SHC reserves 8 bytes of stack, not 12, and 78 of 128 words
     * differ */
    for (count = 0, phase = 0; count < vms_drive->media->read_phases;
         count++, phase++) {
    again:
        for (;;) {
            if (kdIsChanged(device)) {
                return BUD_ERR_NO_DISK;
            }
            seq = kdSetBlockRead(device, KDD_FUNC_STORAGE, 0, phase, block);
            if (seq >= 0) {
                break;
            }
            vmsfb_sleep();
        }
        for (;;) {
            if (kdGetStatus(device, seq)) {
                break;
            }
            vmsfb_sleep();
        }
        dev = kdGetDevice(device);
        if (kdIsChanged(device)) {
            return BUD_ERR_NO_DISK;
        }
        data = dev->recvdata;
        switch (dev->response) {
        case KDD_RES_DATATRANSFER:
            break;
        case KDD_RES_FILEERROR:
            return BUD_ERR_GENERIC;
        case KDD_RES_TRANSMITAGAIN:
            goto again;
        default:
            return BUD_ERR_NO_DISK;
        }
        vmsfs_memcpyl(dst, data + READ_DATA, vms_drive->media->read_bytes);
        dst += vms_drive->media->read_bytes;
    }
    return BUD_ERR_OK;
}

Sint32 vmsio_block_write(Sint32 drive, Sint32 block, const void *buf)
{
    const Uint8 *src;
    struct vmsfs_drive *vms_drive = &buVmsDrive[drive];
    Sint32 device;
    Sint32 phase;
    Sint32 count;
    Sint32 seq;
    Sint32 size;
    const KDS_DEVICE *dev;
    const KDS_DEVICE *answer;
    /* @hack redundant: data, set to dev->recvdata and never read, which
     * keeps kdGetStatus's address in R9 and size at @(4,R15) (0x8c1260a8,
     * 0x8c126110); without it, 169 of 206 words differ, and the function
     * runs 4 bytes long */
    /* NOLINTNEXTLINE(clang-diagnostic-unused-but-set-variable) */
    Uint8 *data;
    Uint8 response;
    Sint32 errcode;

    if (!vmsio_ismount(drive)) {
        vmsfb_sleep();
        return BUD_ERR_NO_DISK;
    }
    device = drive_devices[drive];
    kdGetDevice(device);
    src = buf;
    size = vms_drive->media->write_bytes;
    /* @hack spelling: the phase counted twice, in count and phase, which
     * keeps count at @(20,R15) and phase in R10 (0x8c126114, 0x8c126112);
     * with phase alone, SHC reserves 28 bytes of stack, not 32, and 169 of 206
     * words differ */
    for (count = 0, phase = 0; count < vms_drive->media->write_phases;
         count++, phase++) {
    again:
        for (;;) {
            if (kdIsChanged(device)) {
                return BUD_ERR_NO_DISK;
            }
            seq = kdSetBlockWrite(device, KDD_FUNC_STORAGE, 0, phase, block,
                                  src, size / 4);
            if (seq >= 0) {
                break;
            }
            vmsfb_sleep();
        }
        for (;;) {
            if (kdGetStatus(device, seq)) {
                break;
            }
            vmsfb_sleep();
        }
        dev = kdGetDevice(device);
        if (kdIsChanged(device)) {
            return BUD_ERR_NO_DISK;
        }
        data = dev->recvdata;
        switch (dev->response) {
        case KDD_RES_DEVICEREPLY:
            break;
        case KDD_RES_TRANSMITAGAIN:
            goto again;
        default:
            return BUD_ERR_NO_DISK;
        }
        src += size;
    }
last_error:
    for (;;) {
        if (kdIsChanged(device)) {
            return BUD_ERR_NO_DISK;
        }
        seq = kdSetGetLastError(device, KDD_FUNC_STORAGE, 0, phase, block);
        if (seq >= 0) {
            break;
        }
        vmsfb_sleep();
    }
    for (;;) {
        if (kdGetStatus(device, seq)) {
            break;
        }
        vmsfb_sleep();
    }
    answer = kdGetDevice(device);
    if (kdIsChanged(device)) {
        return BUD_ERR_NO_DISK;
    }
    /* @hack spelling: the hook's test nested, with the card's answer copied
     * in both arms, which loads vmsio_sim_error once and keeps it at
     * @(4,R15) for the call (MOV.L R3,@(4,R15) at 0x8c126216); as one test
     * with ||, SHC loads the hook again, the function runs 4 bytes long, and
     * 165 of 206 words differ */
    if (vmsio_sim_error) {
        if (!vmsio_sim_error(drive, block, &response, &errcode)) {
            response = answer->response;
            errcode = answer->errcode;
        }
    } else {
        response = answer->response;
        errcode = answer->errcode;
    }
    switch (response) {
    case KDD_RES_DEVICEREPLY:
        return BUD_ERR_OK;
    case KDD_RES_FILEERROR:
        return BUD_ERR_WRITE_ERROR;
    case KDD_RES_TRANSMITAGAIN:
        goto last_error;
    default:
        return BUD_ERR_NO_DISK;
    }
}

Sint32 vmsfs_memcmp(const void *buf1, const void *buf2)
{
    int i;
    int differ = 0;

    for (i = 0; i < BLOCK_SIZE;) {
        if (*(const Uint8 *)buf1 != *(const Uint8 *)buf2) {
            differ = -1;
        }
        /* @hack spelling: i = i + 1, which keeps i counting up to
         * BLOCK_SIZE in R6 (CMP/GE R0,R6 at 0x8c126284); as i++, SHC counts
         * R6 down from 512 instead, and 15 of 15 words differ */
        i = i + 1;
        buf1 = (const Uint8 *)buf1 + 1;
        buf2 = (const Uint8 *)buf2 + 1;
    }
    return differ;
}

/* whether the drive holds a storage device, recognized or not */
Sint32 vmsio_isready(Sint32 drive)
{
    Sint32 device = drive_devices[drive];
    const KDS_DEVICE *dev = kdGetDevice(device);
    const KDS_DEVICEINFO *info = kdGetDeviceInfo(device);

    if (!dev->on || !(info->function & KDD_FUNC_STORAGE)) {
        return false;
    }
    return true;
}

/* whether the drive still holds the storage device vmsio_get_media_info
 * last recognized there */
Sint32 vmsio_ismount(Sint32 drive)
{
    Sint32 device = drive_devices[drive];
    const KDS_DEVICE *dev = kdGetDevice(device);
    const KDS_DEVICEINFO *info = kdGetDeviceInfo(device);

    if (!dev->on || kdIsChanged(device) ||
        !(info->function & KDD_FUNC_STORAGE)) {
        return false;
    }
    return true;
}
