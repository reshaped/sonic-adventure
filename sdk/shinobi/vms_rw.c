/* VMS file system card reads and writes, with retries. each call reports one
 * progress step of its drive's command. each failed try adds a step to
 * progress_max, the command's total */
#include <sg_xpt.h>
#include <shinobi.h>
#include <vmsfs.h>

/* the retries after a failed call; -1 retries until the call succeeds */
Sint32 _vmsfs_retryCount;

Sint32 vmsfs_setretry(Sint32 count)
{
    return _vmsfs_retryCount = count;
}

Sint32 vmsfs_mediainfo_retry(Sint32 drive, struct vmsfs_media *media)
{
    Sint32 left = _vmsfs_retryCount;
    Sint32 err;
    /* @hack spelling: the drive and its command through locals set after
     * the tries, which keeps the command's address at @R15 (MOV.L R2,@R15
     * at 0x8c11dd26); used in place, 19 of 76 words differ */
    struct vmsfs_drive *vms_drive;
    struct vmsfs_command *command;

    for (;;) {
        err = vmsio_get_media_info(drive, media);
        if (err == BUD_ERR_OK) {
            break;
        }
        /* @hack spelling: the sum spelled out, which keeps the drive's
         * address in R12 and vmsio_get_media_info's in R11 (0x8c11dcdc,
         * 0x8c11dce8); as ++, the two trade and 9 of 76 words differ */
        buVmsDrive[drive].progress_max = buVmsDrive[drive].progress_max + 1;
        if (left-- == 0 && _vmsfs_retryCount != -1) {
            break;
        }
    }
    vms_drive = &buVmsDrive[drive];
    command = &buVmsCmd[drive];
    vmsfs_c_progress(drive, command->op, vms_drive->progress++,
                     vms_drive->progress_max);
    switch (err) {
    case BUD_ERR_OK:
        return BUD_ERR_OK;
    case (Sint32)BUD_ERR_NO_DISK:
        return BUD_ERR_NO_DISK;
    default:
        return BUD_ERR_GENERIC;
    }
}

Sint32 vmsfs_read_retry(Sint32 drive, Sint32 block, void *buf)
{
    Sint32 left = _vmsfs_retryCount;
    Sint32 err;
    /* @hack spelling: the drive and its command through locals set after
     * the tries, which keeps the command's address at @R15 (MOV.L R2,@R15
     * at 0x8c11ddde); used in place, 19 of 78 words differ */
    struct vmsfs_drive *vms_drive;
    struct vmsfs_command *command;

    for (;;) {
        err = vmsio_block_read(drive, block, buf);
        if (err == BUD_ERR_OK) {
            break;
        }
        /* @hack spelling: the sum spelled out, which keeps the drive's
         * address in R12 (MOV.W at 0x8c11dd90); as ++, it takes R10, and 16
         * of 78 words differ */
        buVmsDrive[drive].progress_max = buVmsDrive[drive].progress_max + 1;
        if (left-- == 0 && _vmsfs_retryCount != -1) {
            break;
        }
    }
    vms_drive = &buVmsDrive[drive];
    command = &buVmsCmd[drive];
    vmsfs_c_progress(drive, command->op, vms_drive->progress++,
                     vms_drive->progress_max);
    switch (err) {
    case BUD_ERR_OK:
        return BUD_ERR_OK;
    case (Sint32)BUD_ERR_NO_DISK:
        return BUD_ERR_NO_DISK;
    default:
        return BUD_ERR_GENERIC;
    }
}

Sint32 vmsfs_write_retry(Sint32 drive, Sint32 block, const void *buf)
{
    Sint32 left = _vmsfs_retryCount;
    Sint32 err;
    /* @hack spelling: the drive and its command through locals set after
     * the tries, which keeps the command's address at @R15 (MOV.L R2,@R15
     * at 0x8c11de96); used in place, 19 of 84 words differ */
    struct vmsfs_drive *vms_drive;
    struct vmsfs_command *command;

    for (;;) {
        err = vmsio_block_write(drive, block, buf);
        if (err == BUD_ERR_OK) {
            break;
        }
        /* @hack spelling: the sum spelled out, which keeps the drive's
         * address in R12 (MOV.W at 0x8c11de48); as ++, it takes R10, and 16
         * of 84 words differ */
        buVmsDrive[drive].progress_max = buVmsDrive[drive].progress_max + 1;
        if (left-- == 0 && _vmsfs_retryCount != -1) {
            break;
        }
    }
    vms_drive = &buVmsDrive[drive];
    command = &buVmsCmd[drive];
    vmsfs_c_progress(drive, command->op, vms_drive->progress++,
                     vms_drive->progress_max);
    switch (err) {
    case BUD_ERR_OK:
        return BUD_ERR_OK;
    case (Sint32)BUD_ERR_NO_DISK:
        return BUD_ERR_NO_DISK;
    case (Sint32)BUD_ERR_WRITE_ERROR:
        return BUD_ERR_WRITE_ERROR;
    default:
        return BUD_ERR_GENERIC;
    }
}
