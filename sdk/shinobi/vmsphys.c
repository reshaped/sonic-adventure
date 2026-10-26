/* VMS file system of the backup library
 *
 * a card's last block is its system block. files take the user blocks, the
 * blocks below the first spare block. each drive's work area caches its card's
 * system block, FAT and directory */
#include <sg_xpt.h>
#include <shinobi.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <vmsfs.h>

/* the most files open at once */
#define FILES 8

#define BLOCK_SIZE 512
/* each drive's share of _vms_verify_buf. a verified save reads its blocks back
 * one at a time, into the first BLOCK_SIZE bytes of its drive's share */
#define VERIFY_SIZE 2048

/* the number of the last block of a card of each capacity */
#define LAST_BLOCK_128KB 0xff
#define LAST_BLOCK_256KB 0x1ff
#define LAST_BLOCK_512KB 0x3ff
#define LAST_BLOCK_1MB 0x7ff

/* a block's FAT word is the next block of its chain, a file's or the
 * directory's, or one of these marks. the system block and each FAT block
 * are marked FAT_LAST */
#define FAT_FREE 0xfffc /* in no chain */
#define FAT_LAST 0xfffa /* the end of a chain */
#define FAT_BAD 0xffff /* a block whose write failed */

/* a file's first_block while it is open for writing and has no block
 * written */
#define NO_BLOCK 0xffff

/* how a file is open: by vmsfs_fopen, vmsfs_fcreate or vmsfs_xcreate */
enum { MODE_READ, MODE_WRITE, MODE_WRITE_EXEC };

/* a directory entry's type byte */
#define DIR_NONE 0x00
#define DIR_FILE 0x33
#define DIR_EXEC 0xcc

/* the system block's layout, in bytes. a formatted card's starts with
 * FORMATTED_SIZE bytes of FORMATTED. every field from SYS_LAST_BLOCK on but
 * SYS_ICON is a word in the card's byte order. SYS_ICON's first byte is the
 * icon's number, and its second is 0 */
#define FORMATTED 0x55
#define FORMATTED_SIZE 16
#define SYS_VOLUME 16
#define SYS_TIME 48
#define SYS_LAST_BLOCK 64
#define SYS_PARTITION 66
#define SYS_SYSTEM_BLOCK 68
#define SYS_FAT_BLOCK 70
#define SYS_FAT_SIZE 72
#define SYS_DIR_BLOCK 74
#define SYS_DIR_SIZE 76
#define SYS_ICON 78
#define SYS_SPARE_BLOCK 80
#define SYS_SPARE_SIZE 82
#define SYS_EXEC_BLOCK 84
#define SYS_EXEC_SIZE 86
#define VOLUME_SIZE 32

/* a directory entry's layout, in bytes */
#define DIR_COPYFLAG 1
#define DIR_FIRST_BLOCK 2
#define DIR_FNAME 4
#define DIR_TIME 16
#define DIR_BLOCKS 24
#define DIR_HDROFS 26
#define FNAME_SIZE 12
#define TIME_SIZE 8
#define DIR_ENTRY_SIZE 32
/* a directory block holds 1 << SLOT_BITS slots, BLOCK_SIZE / DIR_ENTRY_SIZE */
#define SLOT_BITS 4

/* a directory entry as the card holds it. type, copyflag, fname and time are
 * used as fields. its words are in the card's byte order, so only
 * _vm->get_word and set_word reach them, at their DIR_ offsets */
struct dir_entry {
    Uint8 type;
    Uint8 copyflag;
    Uint16 first_block;
    char fname[FNAME_SIZE];
    Uint8 time[TIME_SIZE];
    Uint16 blocks;
    Uint16 hdrofs;
    Uint8 unk_1c[4];
};

/* a directory entry as unpack_entry unpacks it: its words in the CPU's byte
 * order, and fname ended with a NUL */
struct file_entry {
    Sint32 unk_00;
    Sint32 type;
    Uint16 hdrofs;
    Uint8 copyflag;
    Sint32 first_block;
    /* 16 bytes, which put time at offset 32, where the image's code reads it.
     * FNAME_SIZE + 1 would put it at 29, and 4 of this file's words would
     * differ */
    char fname[16];
    Uint8 time[TIME_SIZE];
    Sint32 blocks;
};

extern Uint16 vmsfs_getword_big(void *buf, Sint32 offset);
extern Uint16 vmsfs_getword_little(void *buf, Sint32 offset);
extern void vmsfs_setword_big(void *buf, Sint32 offset, Uint16 value);
extern void vmsfs_setword_little(void *buf, Sint32 offset, Uint16 value);

static Sint32 fat_find_dir_or_spare(Sint32 drive, Uint16 mark);
static Sint32 fat_find_in(Sint32 drive, Uint16 mark, Sint32 first, Sint32 last);
static Sint32 delete_file(Sint32 drive, const char *fname);
static Sint32 first_used_block(Sint32 drive);
static Sint32 last_free_block(Sint32 drive, Sint32 lowest);
static Uint8 *entry_starting_at(Sint32 drive, Sint32 block);
static Sint32 check_links(Sint32 drive);
static Sint32 cache_dir(Sint32 drive);
static Sint32 rechain_dir(Sint32 drive);
static Sint32 flush_dir(Sint32 drive);
static Sint32 name_length(const char *fname);
static Sint32 name_equal(const char *a, const char *b);
static void unpack_entry(Uint8 *raw, struct file_entry *entry);
static Sint32 find_file(Sint32 drive, const char *fname,
                        struct file_entry *entry);
static Sint32 find_exec_file(Sint32 drive, struct file_entry *entry);
static Sint32 find_free_slot(Sint32 drive);
static Sint32 read_system(Sint32 drive);
static Sint32 write_system(Sint32 drive);
static Sint32 read_fat(Sint32 drive);
static Sint32 write_fat_blocks(Sint32 drive);
static Sint32 find_fat_room(Sint32 drive);
static Sint32 write_fat(Sint32 drive);
static Sint32 fat_find_user(Sint32 drive, Uint16 mark);
static Sint32 mark_starts_find_free(Sint32 drive);
static Sint32 unclaimed_block(Sint32 drive, Uint16 block);
static void fat_store(Sint32 drive, Sint32 block, Uint16 word);
static Uint16 fat_fetch(Sint32 drive, Sint32 block);
static Sint32 load_caches(Sint32 drive);
static Uint8 *dir_slot(Sint32 drive, Sint32 slot);
static void dir_touch(Sint32 drive, Sint32 slot);
static BUFILE alloc_file(Sint32 drive);
static void free_file(BUFILE file);
static Sint32 check_name(Sint32 drive, const char *fname);
static Sint32 check_chain(Sint32 drive, const char *fname);
static Uint8 to_bcd(Uint32 value);
static Uint32 from_bcd(Uint32 value);
static void pack_time(Uint8 *bcd, const BUS_TIME *time);
static void unpack_time(BUS_TIME *time, Uint8 *bcd);
static Sint32 admit_media(struct vmsfs_drive *drv);

void (*_vmsfs_dbg_out)(const char *str) = NULL;
/* the FAT blocks and the directory blocks a drive's work area holds, for a
 * card of each capacity */
Sint32 buCapsFats[CARD_SIZES] = {1, 2, 4, 8};
Sint32 buCapsDirs[CARD_SIZES] = {13, 30, 60, 113};
/* each operation's progress_max on a card of each capacity, by BUD_OP_ code.
 * the codes stop below BUD_OP_REWRITEEXECFILE, whose count is the blocks
 * vmsfs_rewrite_exec_file writes. vmsfs_format without erase takes a tenth of
 * its count. a save adds the blocks it writes to its count, twice when it
 * reads them back */
Sint32 buProgMaxTbl[CARD_SIZES][BUD_OP_REWRITEEXECFILE] = {
    {0, 17, 0, 285, 2, 0, 0, 2, 2, 0, 2, 0, 2, 2, 0, 256, 256, 2, 0, 0},
    {0, 30, 0, 570, 3, 0, 0, 3, 2, 0, 2, 0, 3, 3, 0, 512, 512, 2, 0, 0},
    {0, 57, 0, 1140, 2, 0, 0, 5, 2, 0, 2, 0, 5, 5, 0, 1024, 1024, 2, 0, 0},
    {0, 124, 0, 2295, 9, 0, 0, 9, 2, 0, 9, 0, 9, 9, 0, 2048, 2048, 9, 0, 0},
};

/* when set, vmsfs_diskfree doesn't count a free block that a file starts at */
const Sint32 buRealFree = 0;
/* when set, vmsfs_fsetattr sets a file's header offset */
const Sint32 buEnableHeaderOffset = 0;
/* day of the week conversions, indexed by the day's low 3 bits. the card
 * counts the days from Monday, and BUS_TIME from Sunday. a day of 7 converts
 * to 0xff */
static const Uint8 weekday_to_card[8] = {6, 0, 1, 2, 3, 4, 5, 0xff};
static const Uint8 weekday_from_card[8] = {1, 2, 3, 4, 5, 6, 0, 0xff};

struct vmsfs_context buVmsContext;
struct vmsfs_context *_vm;
struct vmsfs_drive buVmsDrive[VMSFS_DRIVES];
/* 1 when a drive's last load_caches failed to read its FAT or directory with
 * BUD_ERR_GENERIC, else 0. vmsfs_isformat then reports the card unformatted */
Sint32 buIllegalFormat[VMSFS_DRIVES];
/* where a save with BUD_FLAG_VERIFY reads its blocks back, VERIFY_SIZE bytes
 * for each drive */
Uint8 _vms_verify_buf[VMSFS_DRIVES * VERIFY_SIZE];
static struct vmsfs_media media_table[VMSFS_DRIVES];
static BUS_FILE files[FILES];
static Sint32 file_limit;
static Sint32 last_error;
static Sint32 drive_count;
/* a block of zeros for each drive, which vmsfs_format with erase writes to
 * every block below the system block */
static Uint8 blank_blocks[VMSFS_DRIVES * BLOCK_SIZE];
static char print_buf[512];
/* where vmsfs_connect reads block 0, to check that the card answers. nothing
 * reads the copy */
static Uint8 probe_block[BLOCK_SIZE];

/* the formatted message and its NUL must fit print_buf's 512 bytes, since
 * vsprintf doesn't check */
void vmsfs_dpf(const char *format, ...)
{
    va_list args;

    va_start(args, format);
    vsprintf(print_buf, format, args);
    va_end(args);
    if (_vmsfs_dbg_out != NULL) {
        _vmsfs_dbg_out(print_buf);
    }
}

void vmsfs_setdevicetype(Sint32 type)
{
    type &= 1;
    _vm->device_type = type;
    if (type == 1) {
        _vm->get_word = vmsfs_getword_big;
        _vm->set_word = vmsfs_setword_big;
    } else {
        _vm->get_word = vmsfs_getword_little;
        _vm->set_word = vmsfs_setword_little;
    }
}

Sint32 vmsfs_init(Sint32 capacity, Sint32 drives, void *work)
{
    Sint32 i;
    Sint32 bit;

    _vm = &buVmsContext;
    for (i = 0; i < VMSFS_DRIVES; i++) {
        buIllegalFormat[i] = 0;
    }
    memset(&buVmsContext, 0, sizeof(buVmsContext));
    memset(buVmsDrive, 0, sizeof(buVmsDrive));
    vmsfs_setdevicetype(0);
    drive_count = 0;
    for (i = 0, bit = 1; i < VMSFS_DRIVES; i++, bit <<= 1) {
        if (drives & bit) {
            drive_count++;
        }
    }
    if (work != NULL) {
        vmsfs_init_work_normal(capacity, drives, work);
        _vm->dynamic_work = 0;
    } else {
        vmsfs_init_work_new(capacity, drives, work);
        _vm->dynamic_work = 1;
    }
    file_limit = FILES;
    last_error = 0;
    vmsfs_setretry(10);
    memset(files, 0, file_limit * sizeof(BUS_FILE));
    vmsio_init();
    return BUD_ERR_OK;
}

Sint32 vmsfs_init_work_new(Sint32 capacity, Sint32 drives, Uint8 *work)
{
    switch (capacity) {
    case BUD_CAPACITY_128KB:
        _vm->capacity = CARD_128KB;
        break;
    case BUD_CAPACITY_256KB:
        _vm->capacity = CARD_256KB;
        break;
    case BUD_CAPACITY_512KB:
        _vm->capacity = CARD_512KB;
        break;
    case BUD_CAPACITY_1MB:
        _vm->capacity = CARD_1MB;
        break;
    default:
        return BUD_ERR_INVALID_PARAM;
    }
    return BUD_ERR_OK;
}

Sint32 vmsfs_init_work_normal(Sint32 capacity, Sint32 drives, Uint8 *work)
{
    Sint32 i;
    Sint32 bit;
    Sint32 fats;
    Sint32 dirs;
    Sint32 stride;
    Sint32 dir_offset;
    struct vmsfs_drive *drv = buVmsDrive;
    Uint8 *area;

    switch (capacity) {
    case BUD_CAPACITY_128KB:
        _vm->capacity = CARD_128KB;
        fats = buCapsFats[CARD_128KB];
        dirs = buCapsDirs[CARD_128KB];
        break;
    case BUD_CAPACITY_256KB:
        _vm->capacity = CARD_256KB;
        fats = buCapsFats[CARD_256KB];
        dirs = buCapsDirs[CARD_256KB];
        break;
    case BUD_CAPACITY_512KB:
        _vm->capacity = CARD_512KB;
        fats = buCapsFats[CARD_512KB];
        dirs = buCapsDirs[CARD_512KB];
        break;
    case BUD_CAPACITY_1MB:
        _vm->capacity = CARD_1MB;
        fats = buCapsFats[CARD_1MB];
        dirs = buCapsDirs[CARD_1MB];
        break;
    default:
        return BUD_ERR_INVALID_PARAM;
    }
    area = work;
    /* a BUD_CAPACITY_ code counts the blocks a drive's work area holds after
     * its BUD_WORK_GAP bytes */
    stride = (capacity * BLOCK_SIZE) + BUD_WORK_GAP;
    /* @hack spelling: i and bit set before the loop, which puts i's MOV #0,R9
     * after the stride's ADD #32,R4 (0x8c11e726). set in the loop's head with
     * dir_offset, 12 of 160 words differ */
    i = 0;
    bit = 1;
    for (dir_offset = (fats + 1) * BLOCK_SIZE; i < VMSFS_DRIVES;
         i++, bit <<= 1, drv++) {
        if (!(drives & bit)) {
            continue;
        }
        drv->has_work = 1;
        drv->number = i;
        drv->media = &media_table[i];
        drv->sys_cached = 0;
        drv->fat_cached = 0;
        drv->dir_cached = 0;
        drv->work = area;
        drv->sys = area + BUD_WORK_GAP;
        drv->fat = area + BUD_WORK_GAP + BLOCK_SIZE;
        drv->dir = area + dir_offset + BUD_WORK_GAP;
        drv->mounted = 0;
        drv->ready = 0;
        drv->sys_blocks = 1;
        drv->fat_blocks = fats;
        drv->dir_blocks = dirs;
        drv->gap = BUD_WORK_GAP;
        memset(drv->fat_unsaved, 0, sizeof(drv->fat_unsaved));
        memset(drv->dir_unsaved, 0, sizeof(drv->dir_unsaved));
        memset(drv->work, 'G', BUD_WORK_GAP);
        memset(drv->sys, 'S', BUD_WORK_GAP);
        memset(drv->fat, 'F', BUD_WORK_GAP);
        memset(drv->dir, 'D', BUD_WORK_GAP);
        area += stride;
    }
    return BUD_ERR_OK;
}

Sint32 vmsfs_resetwork(Sint32 drive)
{
    struct vmsfs_drive *drv = &buVmsDrive[drive];

    memset(drv, 0, sizeof(struct vmsfs_drive));
    drv->media = &media_table[drive];
    return BUD_ERR_OK;
}

Sint32 vmsfs_setwork(Sint32 drive, void *addr, Sint32 size)
{
    struct vmsfs_drive *drv;
    Sint32 fats;
    Sint32 dirs;
    Sint32 need;
    Sint32 capacity;

    if (_vm->dynamic_work == 0 || addr == NULL) {
        return BUD_ERR_INVALID_PARAM;
    }
    drv = &buVmsDrive[drive];
    /* @hack spelling: the capacity through a local, which SHC stores at @R15
     * (0x8c11e86a). switched on as drv->capacity, the function runs 6 bytes
     * short and 125 of 141 words differ */
    capacity = drv->capacity;
    switch (capacity) {
    case CARD_128KB:
        fats = buCapsFats[CARD_128KB];
        dirs = buCapsDirs[CARD_128KB];
        break;
    case CARD_256KB:
        fats = buCapsFats[CARD_256KB];
        dirs = buCapsDirs[CARD_256KB];
        break;
    case CARD_512KB:
        fats = buCapsFats[CARD_512KB];
        dirs = buCapsDirs[CARD_512KB];
        break;
    case CARD_1MB:
        fats = buCapsFats[CARD_1MB];
        dirs = buCapsDirs[CARD_1MB];
        break;
    default:
        return BUD_ERR_INVALID_PARAM;
    }
    need = ((fats + dirs + 1) * BLOCK_SIZE) + BUD_WORK_GAP;
    if (need > size) {
        return BUD_ERR_INVALID_PARAM;
    }
    drv->has_work = 1;
    drv->number = drive;
    drv->media = &media_table[drive];
    drv->sys_cached = 0;
    drv->fat_cached = 0;
    drv->dir_cached = 0;
    drv->work = addr;
    drv->sys = (Uint8 *)addr + BUD_WORK_GAP;
    drv->fat = (Uint8 *)addr + BUD_WORK_GAP + BLOCK_SIZE;
    drv->dir = (Uint8 *)addr + ((fats + 1) * BLOCK_SIZE) + BUD_WORK_GAP;
    drv->sys_blocks = 1;
    drv->fat_blocks = fats;
    drv->dir_blocks = dirs;
    drv->gap = BUD_WORK_GAP;
    memset(drv->fat_unsaved, 0, sizeof(drv->fat_unsaved));
    memset(drv->dir_unsaved, 0, sizeof(drv->dir_unsaved));
    memset(drv->work, 'G', BUD_WORK_GAP);
    memset(drv->sys, 'S', BUD_WORK_GAP);
    memset(drv->fat, 'F', BUD_WORK_GAP);
    memset(drv->dir, 'D', BUD_WORK_GAP);
    drv->media->user_blocks = drv->media->spare_block;
    drv->mounted = 1;
    drv->ready = 1;
    return BUD_ERR_OK;
}

Sint32 vmsfs_exit(void)
{
    memset(buVmsDrive, 0, sizeof(buVmsDrive));
    vmsio_exit();
    return BUD_ERR_OK;
}

Uint32 vmsfs_isready(void)
{
    Sint32 i;
    Uint32 bit;
    Uint32 mask;

    mask = 0;
    for (i = 0, bit = 1; i < VMSFS_DRIVES; i++, bit <<= 1) {
        if (vmsio_isready(i)) {
            mask |= bit;
        }
    }
    return mask;
}

Sint32 vmsfs_ismount(Sint32 drive)
{
    struct vmsfs_drive *drv = &buVmsDrive[drive];

    if (vmsio_ismount(drive) && drv->mounted) {
        return 1;
    }
    return 0;
}

Sint32 vmsfs_mount(Sint32 drive)
{
    struct vmsfs_drive *drv = &buVmsDrive[drive];
    struct vmsfs_command *cmd;

    if (_vm->dynamic_work == 1) {
        return vmsfs_mount2(drive);
    }
    if (!drv->has_work) {
        return BUD_ERR_NO_DISK;
    }
    drv->progress = 1;
    drv->progress_max = buProgMaxTbl[drv->capacity][BUD_OP_MOUNT];
    cmd = &buVmsCmd[drive];
    vmsfs_c_progress(drive, cmd->op, drv->progress++, drv->progress_max);
    /* @hack redundant: the else after the return, which loads admit_media's
     * address into R2 (0x8c11ea98). without it, R3 takes it and 4 of 108 words
     * differ */
    if (vmsfs_mediainfo_retry(drive, drv->media) < 0) {
        drv->ready = 0;
        drv->mounted = 0;
        return BUD_ERR_NO_DISK;
    } else if (admit_media(drv) < 0) {
        return BUD_ERR_UNKNOWN_DISK;
    }
    drv->media->user_blocks = drv->media->spare_block;
    drv->ready = 1;
    drv->mounted = 1;
    drv->sys_cached = 0;
    drv->fat_cached = 0;
    drv->dir_cached = 0;
    return load_caches(drive);
}

Sint32 vmsfs_connect(Sint32 drive)
{
    struct vmsfs_drive *drv = &buVmsDrive[drive];
    struct vmsfs_command *cmd;

    drv->media = &media_table[drive];
    drv->progress = 1;
    drv->progress_max = buProgMaxTbl[drv->capacity][BUD_OP_CONNECT];
    cmd = &buVmsCmd[drive];
    vmsfs_c_progress(drive, cmd->op, drv->progress++, drv->progress_max);
    /* @hack redundant: the else after the return, which loads admit_media's
     * address into R3 (0x8c11eb84). without it, R2 takes it and 6 of 113 words
     * differ */
    if (vmsfs_mediainfo_retry(drive, drv->media) < 0) {
        drv->ready = 0;
        drv->mounted = 0;
        return BUD_ERR_NO_DISK;
    } else if (admit_media(drv) < 0) {
        return BUD_ERR_UNKNOWN_DISK;
    }
    drv->media->user_blocks = drv->media->spare_block;
    if (vmsio_block_read(drive, 0, probe_block) < 0) {
        return BUD_ERR_NO_DISK;
    }
    drv->ready = 1;
    drv->sys_cached = 0;
    drv->fat_cached = 0;
    drv->dir_cached = 0;
    switch (drv->capacity) {
    case CARD_128KB:
        return BUD_CAPACITY_128KB;
    case CARD_256KB:
        return BUD_CAPACITY_256KB;
    case CARD_512KB:
        return BUD_CAPACITY_512KB;
    case CARD_1MB:
        return BUD_CAPACITY_1MB;
    }
    return BUD_ERR_UNKNOWN_DISK;
}

Sint32 vmsfs_mount2(Sint32 drive)
{
    struct vmsfs_drive *drv = &buVmsDrive[drive];
    struct vmsfs_command *cmd;

    if (!drv->has_work) {
        return BUD_ERR_NO_DISK;
    }
    drv->progress = 1;
    drv->progress_max = buProgMaxTbl[drv->capacity][BUD_OP_MOUNT];
    cmd = &buVmsCmd[drive];
    vmsfs_c_progress(drive, cmd->op, drv->progress++, drv->progress_max);
    return load_caches(drive);
}

Sint32 vmsfs_unmount(Sint32 drive)
{
    if (!buVmsDrive[drive].has_work) {
        return BUD_ERR_NO_DISK;
    }
    buVmsDrive[drive].ready = 0;
    buVmsDrive[drive].mounted = 0;
    buVmsDrive[drive].sys_cached = 0;
    buVmsDrive[drive].fat_cached = 0;
    buVmsDrive[drive].dir_cached = 0;
    buVmsDrive[drive].capacity = CARD_128KB;
    return BUD_ERR_OK;
}

Sint32 vmsfs_fgetinfo(Sint32 drive, const char *fname, BUS_FILEINFO *finfo)
{
    struct vmsfs_drive *drv = &buVmsDrive[drive];
    struct file_entry entry;
    Sint32 result;

    if (!buVmsDrive[drive].has_work) {
        return BUD_ERR_NO_DISK;
    }
    if (!vmsio_isready(drive)) {
        vmsfs_unmount(drive);
        return BUD_ERR_NO_DISK;
    }
    if (!vmsio_ismount(drive)) {
        return BUD_ERR_NO_DISK;
    }
    if (!buVmsDrive[drive].mounted) {
        return BUD_ERR_NO_DISK;
    }
    if (buIsFormat(drive) < 0) {
        return BUD_ERR_UNFORMAT;
    }
    result = find_file(drive, fname, &entry);
    if (result < 0) {
        return BUD_ERR_FILE_NOT_FOUND;
    }
    finfo->filesize = entry.blocks * (drv->media->last_byte + 1);
    finfo->blocks = entry.blocks;
    finfo->type = entry.type;
    finfo->copyflag = entry.copyflag;
    finfo->headerofs = entry.hdrofs;
    if (entry.type == DIR_EXEC) {
        finfo->type = BUD_FILETYPE_EXECUTABLE;
    } else {
        finfo->type = BUD_FILETYPE_NORMAL;
    }
    unpack_time(&finfo->time, entry.time);
    return BUD_ERR_OK;
}

Sint32 vmsfs_format(Sint32 drive, const Uint8 *volume, Sint32 icon,
                    const BUS_TIME *time, Sint32 erase)
{
    Sint32 i;
    Sint32 result;
    /* NOLINTNEXTLINE(clang-diagnostic-unused-but-set-variable) */
    Sint32 user_blocks;
    Sint32 dir_size;
    Sint32 dir_block;
    Sint32 fat_block;
    Uint8 *blank;
    const Uint8 *text;
    Uint16 mark;
    Uint8 bcd[TIME_SIZE];
    struct vmsfs_drive *drv = &buVmsDrive[drive];

    if (!drv->has_work) {
        return BUD_ERR_NO_DISK;
    }
    if (erase) {
        drv->progress = 1;
        drv->progress_max = buProgMaxTbl[drv->capacity][BUD_OP_FORMATDISK];
    } else {
        drv->progress = 1;
        drv->progress_max = buProgMaxTbl[drv->capacity][BUD_OP_FORMATDISK] / 10;
    }
    if (!vmsio_ismount(drive)) {
        return BUD_ERR_NO_DISK;
    }
    if (!buVmsDrive[drive].mounted) {
        return BUD_ERR_NO_DISK;
    }
    blank = blank_blocks + (drive * BLOCK_SIZE);
    memset(drv->sys, 0, BLOCK_SIZE);
    result = vmsfs_write_retry(drive, drv->media->system_block, drv->sys);
    if (result < 0) {
        /* @hack redundant: the else after the return, which loads the next two
         * calls' addresses into R3 (0x8c11ee96, 0x8c11eea4). without it, R2
         * takes them and 6 of 373 words differ */
        if (result == (Sint32)BUD_ERR_WRITE_ERROR) {
            return BUD_ERR_ILLEGAL_DISK;
        } else {
            return BUD_ERR_NO_DISK;
        }
    }
    /* @hack redundant: the else after the return, which loads admit_media's
     * address into R3 (0x8c11eea4). without it, R2 takes it and 4 of 373 words
     * differ */
    if (vmsfs_mediainfo_retry(drive, drv->media) < 0) {
        return BUD_ERR_NO_DISK;
    } else if (admit_media(drv) < 0) {
        return BUD_ERR_UNKNOWN_DISK;
    }
    drv->media->user_blocks = drv->media->spare_block;
    memset(drv->sys, 0, BLOCK_SIZE);
    memset(drv->sys + SYS_VOLUME, ' ', VOLUME_SIZE);
    text = volume;
    for (i = 0; i < VOLUME_SIZE; i++, text++) {
        drv->sys[i + SYS_VOLUME] = *text;
    }
    pack_time(bcd, time);
    for (i = 0; i < TIME_SIZE; i++) {
        drv->sys[i + SYS_TIME] = bcd[i];
    }
    fat_block = drv->media->fat_block;
    dir_block = drv->media->dir_block;
    _vm->set_word(drv->sys, SYS_PARTITION, drv->media->partition);
    drv->sys[SYS_ICON] = icon;
    drv->sys[SYS_ICON + 1] = 0;
    _vm->set_word(drv->sys, SYS_LAST_BLOCK, drv->media->last_block);
    _vm->set_word(drv->sys, SYS_DIR_BLOCK, dir_block);
    _vm->set_word(drv->sys, SYS_FAT_BLOCK, fat_block);
    _vm->set_word(drv->sys, SYS_DIR_SIZE, drv->media->dir_size);
    _vm->set_word(drv->sys, SYS_FAT_SIZE, drv->media->fat_size);
    _vm->set_word(drv->sys, SYS_SPARE_BLOCK, drv->media->spare_block);
    _vm->set_word(drv->sys, SYS_SPARE_SIZE, drv->media->spare_size);
    _vm->set_word(drv->sys, SYS_EXEC_BLOCK, drv->media->exec_block);
    _vm->set_word(drv->sys, SYS_EXEC_SIZE, drv->media->exec_size);
    _vm->set_word(drv->sys, SYS_SYSTEM_BLOCK, drv->media->last_block);
    memset(blank, 0, BLOCK_SIZE);
    for (i = 0; i < drv->media->last_block; i++) {
        if (erase) {
            result = vmsfs_write_retry(drive, i, blank);
        } else {
            result = BUD_ERR_OK;
        }
        switch (result) {
        case BUD_ERR_NO_DISK:
            return BUD_ERR_NO_DISK;
        case BUD_ERR_WRITE_ERROR:
            mark = FAT_BAD;
            break;
        default:
            mark = FAT_FREE;
            break;
        }
        fat_store(drive, i, mark);
    }
    /* @hack redundant: user_blocks, read and never used, which keeps
     * fat_block at @(8,R15) and dir_block at @(12,R15) (0x8c11ef0e,
     * 0x8c11ef16); without it, the two trade and 100 of 373 words differ */
    user_blocks = drv->media->user_blocks;
    dir_size = drv->media->dir_size;
    memset(drv->dir, 0, dir_size * BLOCK_SIZE);
    rechain_dir(drive);
    fat_store(drive, drv->media->system_block, FAT_LAST);
    for (i = 0; i < drv->media->fat_size; i++) {
        fat_store(drive, drv->media->fat_block + i, FAT_LAST);
    }
    /* this pass repeats the one above, and changes nothing */
    fat_store(drive, drv->media->system_block, FAT_LAST);
    for (i = 0; i < drv->media->fat_size; i++) {
        fat_store(drive, drv->media->fat_block + i, FAT_LAST);
    }
    write_fat(drive);
    /* the cache holds the format mark only while the system block is
     * written. load_caches then reads every cache back from the card */
    memset(drv->sys, FORMATTED, FORMATTED_SIZE);
    write_system(drive);
    memset(drv->sys, 0, FORMATTED_SIZE);
    drv->fat_cached = 0;
    drv->dir_cached = 0;
    drv->sys_cached = 0;
    return load_caches(drive);
}

/* the highest block whose FAT word is mark among the directory blocks, else
 * among the spare blocks, or -1. it takes the directory to be the dir_size
 * blocks right below the FAT, and the FAT the fat_size blocks right below the
 * system block */
static Sint32 fat_find_dir_or_spare(Sint32 drive, Uint16 mark)
{
    Sint32 first;
    Sint32 last;
    Sint32 found;
    struct vmsfs_drive *drv = &buVmsDrive[drive];

    first =
        drv->media->system_block - drv->media->fat_size - drv->media->dir_size;
    last = drv->media->system_block - drv->media->fat_size - 1;
    if ((found = fat_find_in(drive, mark, first, last)) > 0) {
        return found;
    }
    first = drv->media->spare_block;
    last = drv->media->spare_block + drv->media->spare_size - 1;
    if ((found = fat_find_in(drive, mark, first, last)) > 0) {
        return found;
    }
    return -1;
}

/* the highest block from first to last whose FAT word is mark, or -1 */
static Sint32 fat_find_in(Sint32 drive, Uint16 mark, Sint32 first, Sint32 last)
{
    struct vmsfs_drive *drv = &buVmsDrive[drive];
    Sint32 i;

    for (i = last; i >= first; i--) {
        if (_vm->get_word(drv->fat, i * 2) == mark) {
            return i;
        }
    }
    return -1;
}

Sint32 vmsfs_getinfo(Sint32 drive, BUS_DISKINFO *info)
{
    struct vmsfs_drive *drv = &buVmsDrive[drive];
    Sint32 i;
    /* NOLINTNEXTLINE(clang-diagnostic-unused-but-set-variable) */
    Sint32 j;
    Uint8 bcd[TIME_SIZE];

    if (!buVmsDrive[drive].has_work) {
        return BUD_ERR_NO_DISK;
    }
    if (!vmsio_isready(drive)) {
        vmsfs_unmount(drive);
        return BUD_ERR_NO_DISK;
    }
    if (!vmsio_ismount(drive)) {
        return BUD_ERR_NO_DISK;
    }
    if (!buVmsDrive[drive].mounted) {
        return BUD_ERR_NO_DISK;
    }
    if (buIsFormat(drive) < 0) {
        return BUD_ERR_UNFORMAT;
    }
    for (i = 0; i < VOLUME_SIZE; i++) {
        info->volume[i] = drv->sys[i + SYS_VOLUME];
    }
    /* @hack redundant: j, set to the volume's length and never read, which
     * keeps the volume's copy indexed by i, through @(R0,R3) (0x8c11f238);
     * without it, 71 of 127 words differ */
    j = i;
    for (i = 0; i < TIME_SIZE; i++) {
        bcd[i] = drv->sys[SYS_TIME + i];
    }
    unpack_time(&info->time, bcd);
    info->total_blocks = drv->media->last_block + 1;
    info->total_user_blocks = drv->media->user_blocks;
    info->free_user_blocks = vmsfs_diskfree(drive, BUD_FILETYPE_NORMAL);
    info->free_blocks = info->free_user_blocks;
    info->total_exe_blocks = drv->media->exec_size;
    info->free_exe_blocks = vmsfs_diskfree(drive, BUD_FILETYPE_EXECUTABLE);
    info->block_size = drv->media->last_byte + 1;
    info->icon_no = drv->sys[SYS_ICON];
    return BUD_ERR_OK;
}

Sint32 vmsfs_diskfree(Sint32 drive, Sint32 type)
{
    struct vmsfs_drive *drv = &buVmsDrive[drive];
    Sint32 i;
    Sint32 count;

    if (!buVmsDrive[drive].has_work) {
        return BUD_ERR_NO_DISK;
    }
    if (!vmsio_isready(drive)) {
        vmsfs_unmount(drive);
        return BUD_ERR_NO_DISK;
    }
    if (!vmsio_ismount(drive)) {
        return BUD_ERR_NO_DISK;
    }
    if (!buVmsDrive[drive].mounted) {
        return BUD_ERR_NO_DISK;
    }
    if (buIsFormat(drive) < 0) {
        return BUD_ERR_UNFORMAT;
    }
    count = 0;
    if (type == BUD_FILETYPE_EXECUTABLE) {
        for (i = 0; i < drv->media->exec_size; i++) {
            if (fat_fetch(drive, i) != FAT_FREE) {
                break;
            }
        }
        count = i;
    } else if (buRealFree) {
        for (i = 0; i < drv->media->user_blocks; i++) {
            if (fat_fetch(drive, i) == FAT_FREE &&
                unclaimed_block(drive, i) >= 0) {
                count++;
            }
        }
    } else {
        for (i = 0; i < drv->media->user_blocks; i++) {
            if (fat_fetch(drive, i) == FAT_FREE) {
                count++;
            }
        }
    }
    return count;
}

Sint32 vmsfs_sysdiskfree(Sint32 drive)
{
    struct vmsfs_drive *drv = &buVmsDrive[drive];
    Sint32 i;
    Sint32 count;

    if (!buVmsDrive[drive].has_work) {
        return BUD_ERR_NO_DISK;
    }
    if (!vmsio_isready(drive)) {
        vmsfs_unmount(drive);
        return BUD_ERR_NO_DISK;
    }
    if (!vmsio_ismount(drive)) {
        return BUD_ERR_NO_DISK;
    }
    if (!buVmsDrive[drive].mounted) {
        return BUD_ERR_NO_DISK;
    }
    if (buIsFormat(drive) < 0) {
        return BUD_ERR_UNFORMAT;
    }
    count = 0;
    for (i = 0; i < drv->media->fat_size * (drv->media->last_byte + 1) / 2;
         i++) {
        if (fat_fetch(drive, i) == FAT_FREE) {
            count++;
        }
    }
    return count;
}

Sint32 vmsfs_fopen(Sint32 drive, const char *fname, BUFILE *file)
{
    BUFILE new_file;
    struct file_entry entry;

    *file = NULL;
    if (!buVmsDrive[drive].has_work) {
        return BUD_ERR_NO_DISK;
    }
    if (!vmsio_isready(drive)) {
        vmsfs_unmount(drive);
        return BUD_ERR_NO_DISK;
    }
    if (!vmsio_ismount(drive)) {
        return BUD_ERR_NO_DISK;
    }
    if (!buVmsDrive[drive].mounted) {
        return BUD_ERR_NO_DISK;
    }
    if (buIsFormat(drive) < 0) {
        return BUD_ERR_UNFORMAT;
    }
    if (check_name(drive, fname) < 0) {
        return BUD_ERR_CANNOT_OPEN;
    }
    if (check_chain(drive, fname) < 0) {
        return BUD_ERR_FILE_BROKEN;
    }
    new_file = alloc_file(drive);
    if (new_file == NULL) {
        return BUD_ERR_CANNOT_OPEN;
    }
    last_error = find_file(drive, fname, &entry);
    if (last_error < 0) {
        free_file(new_file);
        return BUD_ERR_FILE_NOT_FOUND;
    }
    new_file->mode = MODE_READ;
    new_file->remaining = entry.blocks;
    new_file->first_block = entry.first_block;
    new_file->next_block = new_file->first_block;
    new_file->drive = drive;
    new_file->hdrofs = entry.hdrofs;
    new_file->copyflag = entry.copyflag;
    *file = new_file;
    return BUD_ERR_OK;
}

Sint32 vmsfs_xcreate(Sint32 drive, const char *fname, BUS_TIME *time,
                     BUFILE *file)
{
    struct vmsfs_drive *drv = &buVmsDrive[drive];
    BUFILE new_file;
    Sint32 slot;
    char *to;
    struct file_entry entry;

    *file = NULL;
    if (!buVmsDrive[drive].has_work) {
        return BUD_ERR_NO_DISK;
    }
    if (!vmsio_isready(drive)) {
        vmsfs_unmount(drive);
        return BUD_ERR_NO_DISK;
    }
    if (!vmsio_ismount(drive)) {
        return BUD_ERR_NO_DISK;
    }
    if (!buVmsDrive[drive].mounted) {
        return BUD_ERR_NO_DISK;
    }
    if (buIsFormat(drive) < 0) {
        return BUD_ERR_UNFORMAT;
    }
    if (drv->media->exec_size == 0 || check_name(drive, fname) < 0) {
        return BUD_ERR_CANNOT_CREATE;
    }
    if (find_file(drive, fname, &entry) >= 0) {
        return BUD_ERR_FILE_EXIST;
    }
    if (find_exec_file(drive, &entry) >= 0) {
        return BUD_ERR_EXECFILE_EXIST;
    }
    if (fat_fetch(drive, 0) != FAT_FREE) {
        return BUD_ERR_CANNOT_CREATE;
    }
    slot = find_free_slot(drive);
    if (slot < 0) {
        return BUD_ERR_CANNOT_CREATE;
    }
    dir_slot(drive, slot);
    new_file = alloc_file(drive);
    if (new_file == NULL) {
        return BUD_ERR_CANNOT_OPEN;
    }
    new_file->type = BUD_FILETYPE_EXECUTABLE;
    new_file->mode = MODE_WRITE_EXEC;
    new_file->slot = slot;
    new_file->first_block = NO_BLOCK;
    new_file->written = 0;
    new_file->drive = drive;
    new_file->hdrofs = 1;
    new_file->copyflag = 0;
    to = new_file->fname;
    while (*fname) {
        *to++ = *fname++;
    }
    *to = 0;
    pack_time(new_file->time, time);
    *file = new_file;
    return BUD_ERR_OK;
}

Sint32 vmsfs_fcreate(Sint32 drive, const char *fname, BUS_TIME *time,
                     BUFILE *file)
{
    BUFILE new_file;
    Sint32 slot;
    Sint32 result;
    char *to;
    struct file_entry entry;

    *file = NULL;
    if (!buVmsDrive[drive].has_work) {
        return BUD_ERR_NO_DISK;
    }
    if (!vmsio_isready(drive)) {
        vmsfs_unmount(drive);
        return BUD_ERR_NO_DISK;
    }
    if (!vmsio_ismount(drive)) {
        return BUD_ERR_NO_DISK;
    }
    if (!buVmsDrive[drive].mounted) {
        return BUD_ERR_NO_DISK;
    }
    if (buIsFormat(drive) < 0) {
        return BUD_ERR_UNFORMAT;
    }
    if (check_name(drive, fname) < 0) {
        return BUD_ERR_CANNOT_CREATE;
    }
    if (find_file(drive, fname, &entry) >= 0) {
        if ((result = delete_file(drive, fname)) < 0) {
            return result;
        }
    }
    slot = find_free_slot(drive);
    if (slot < 0) {
        return BUD_ERR_CANNOT_CREATE;
    }
    dir_slot(drive, slot);
    new_file = alloc_file(drive);
    if (new_file == NULL) {
        return BUD_ERR_CANNOT_OPEN;
    }
    new_file->type = BUD_FILETYPE_NORMAL;
    new_file->mode = MODE_WRITE;
    new_file->slot = slot;
    new_file->first_block = NO_BLOCK;
    new_file->written = 0;
    new_file->drive = drive;
    new_file->hdrofs = 0;
    new_file->copyflag = 0;
    to = new_file->fname;
    while (*fname) {
        *to++ = *fname++;
    }
    *to = 0;
    pack_time(new_file->time, time);
    *file = new_file;
    return BUD_ERR_OK;
}

Sint32 vmsfs_fsetparam(BUFILE file, Uint16 hdrofs, Uint8 copyflag)
{
    if (file == NULL) {
        return BUD_ERR_INVALID_PARAM;
    }
    if (!buVmsDrive[file->drive].has_work) {
        return BUD_ERR_NO_DISK;
    }
    if (!vmsio_isready(file->drive)) {
        vmsfs_unmount(file->drive);
        return BUD_ERR_NO_DISK;
    }
    if (!vmsio_ismount(file->drive)) {
        return BUD_ERR_NO_DISK;
    }
    if (!buVmsDrive[file->drive].mounted) {
        return BUD_ERR_NO_DISK;
    }
    if (buIsFormat(file->drive) < 0) {
        return BUD_ERR_UNFORMAT;
    }
    file->hdrofs = hdrofs;
    file->copyflag = copyflag;
    return BUD_ERR_OK;
}

Sint32 vmsfs_fclose(BUFILE file, Sint32 report)
{
    struct vmsfs_drive *drv;
    Sint32 drive;
    Sint32 result;
    Sint32 i;
    struct dir_entry *dir;
    Uint8 type;

    if (file == NULL) {
        return BUD_ERR_INVALID_PARAM;
    }
    result = BUD_ERR_OK;
    if (!buVmsDrive[file->drive].has_work) {
        goto done;
    }
    if (!vmsio_ismount(file->drive)) {
        goto done;
    }
    if (!buVmsDrive[file->drive].mounted) {
        goto done;
    }
    if (!vmsio_isready(file->drive)) {
        vmsfs_unmount(file->drive);
        goto done;
    }
    if (buIsFormat(file->drive) < 0) {
        goto done;
    }
    drive = file->drive;
    if (file->mode != MODE_READ) {
        if (report) {
            drv = &buVmsDrive[drive];
            drv->progress = 1;
            drv->progress_max = buProgMaxTbl[drv->capacity][BUD_OP_CLOSEFILE];
        }
        dir = (struct dir_entry *)dir_slot(drive, file->slot);
        if (file->type == BUD_FILETYPE_EXECUTABLE) {
            type = DIR_EXEC;
        } else {
            type = DIR_FILE;
        }
        dir->type = type;
        dir->copyflag = file->copyflag;
        _vm->set_word(dir, DIR_FIRST_BLOCK, file->first_block);
        _vm->set_word(dir, DIR_BLOCKS, file->written);
        _vm->set_word(dir, DIR_HDROFS, file->hdrofs);
        for (i = 0; i < FNAME_SIZE; i++) {
            dir->fname[i] = file->fname[i];
        }
        for (i = 0; i < TIME_SIZE; i++) {
            dir->time[i] = file->time[i];
        }
        dir_touch(drive, file->slot);
        if ((result = flush_dir(drive)) < 0) {
            goto done;
        }
        write_fat(drive);
    }
    result = BUD_ERR_OK;
done:
    free_file(file);
    return result;
}

Sint32 vmsfs_fwrite(BUFILE file, const void *buf, Sint32 nblock)
{
    Sint32 (*write_one)(BUFILE, const void *);
    struct vmsfs_drive *drv;
    const Uint8 *from;
    Sint32 i;
    Sint32 result;

    if (file == NULL) {
        return BUD_ERR_INVALID_PARAM;
    }
    if (!buVmsDrive[file->drive].has_work) {
        return BUD_ERR_NO_DISK;
    }
    switch (file->mode) {
    case MODE_WRITE:
        write_one = vmsfs_fwrite1;
        break;
    case MODE_WRITE_EXEC:
        write_one = vmsfs_xwrite1;
        break;
    default:
        return BUD_ERR_ACCESS_DENIED;
    }
    from = buf;
    drv = &buVmsDrive[file->drive];
    drv->progress = 1;
    drv->progress_max = nblock;
    for (i = 0; i < nblock; i++) {
        result = write_one(file, from);
        if (result < 0) {
            return result;
        }
        from += BLOCK_SIZE;
    }
    return BUD_ERR_OK;
}

Sint32 vmsfs_fwrite1(BUFILE file, const void *buf)
{
    Sint32 drive;
    Sint32 block;
    Sint32 result;

    if (!vmsio_isready(file->drive)) {
        vmsfs_unmount(file->drive);
        return BUD_ERR_NO_DISK;
    }
    if (!vmsio_ismount(file->drive)) {
        return BUD_ERR_NO_DISK;
    }
    if (!buVmsDrive[file->drive].mounted) {
        return BUD_ERR_NO_DISK;
    }
    if (buIsFormat(file->drive) < 0) {
        return BUD_ERR_UNFORMAT;
    }
    drive = file->drive;
    block = mark_starts_find_free(drive);
    if (block < 0) {
        return BUD_ERR_DISK_FULL;
    }
    result = vmsfs_write_retry(drive, block, buf);
    /* @hack redundant: the empty else, which loads fat_store's address into R3
     * for both calls below, and first_block and written into R2 (0x8c11fc38 to
     * 0x8c11fc3e); without it, the two registers trade and 9 of 118 words
     * differ */
    if (result < 0) {
        if (result == (Sint32)BUD_ERR_NO_DISK) {
            return result;
        } else {
            do {
                fat_store(drive, block, FAT_BAD);
                block = fat_find_user(drive, FAT_FREE);
                if (block < 0) {
                    return BUD_ERR_DISK_FULL;
                }
                result = vmsfs_write_retry(drive, block, buf);
            } while (result != BUD_ERR_OK);
        }
    } else {
    }
    fat_store(drive, block, FAT_LAST);
    /* @hack type: the casts to Sint16, which SHC computes once for both
     * stores, as EXTS.W R14,R10 at 0x8c11fc44; without them, the function runs
     * 2 bytes short and 31 of 118 words differ */
    if (file->first_block == NO_BLOCK) {
        file->first_block = (Sint16)block;
    } else {
        fat_store(drive, file->prev_block, block);
    }
    file->prev_block = (Sint16)block;
    file->written++;
    return BUD_ERR_OK;
}

Sint32 vmsfs_xwrite1(BUFILE file, const void *buf)
{
    struct vmsfs_drive *drv = &buVmsDrive[file->drive];
    Sint32 drive;
    Sint32 block;
    Sint32 result;

    if (!vmsio_isready(file->drive)) {
        vmsfs_unmount(file->drive);
        return BUD_ERR_NO_DISK;
    }
    if (!vmsio_ismount(file->drive)) {
        return BUD_ERR_NO_DISK;
    }
    if (!buVmsDrive[file->drive].mounted) {
        return BUD_ERR_NO_DISK;
    }
    if (buIsFormat(file->drive) < 0) {
        return BUD_ERR_UNFORMAT;
    }
    drive = file->drive;
    block = file->written;
    if (fat_fetch(drive, block) != FAT_FREE) {
        return BUD_ERR_DISK_FULL;
    }
    if (file->written >= drv->media->exec_size) {
        return BUD_ERR_DISK_FULL;
    }
    result = vmsfs_write_retry(drive, block, buf);
    if (result < 0) {
        return result;
    }
    fat_store(drive, block, FAT_LAST);
    /* @hack type: the casts to Sint16, which SHC computes once for both
     * stores, as EXTS.W R13,R11 at 0x8c11fd54; without them, the function runs
     * 6 bytes short and 97 of 108 words differ */
    if (file->first_block == NO_BLOCK) {
        file->first_block = (Sint16)block;
    } else {
        fat_store(drive, file->prev_block, block);
    }
    file->prev_block = (Sint16)block;
    file->written++;
    return BUD_ERR_OK;
}

Sint32 vmsfs_fread(BUFILE file, void *buf, Sint32 nblock)
{
    struct vmsfs_drive *drv;
    Uint8 *to;
    Sint32 i;
    Sint32 result;

    if (file == NULL) {
        return BUD_ERR_INVALID_PARAM;
    }
    if (!buVmsDrive[file->drive].has_work) {
        return BUD_ERR_NO_DISK;
    }
    to = buf;
    drv = &buVmsDrive[file->drive];
    drv->progress = 1;
    drv->progress_max = nblock;
    for (i = 0; i < nblock; i++) {
        result = vmsfs_fread1(file, to);
        if (result < 0) {
            return result;
        }
        to += BLOCK_SIZE;
    }
    return BUD_ERR_OK;
}

Sint32 vmsfs_fread1(BUFILE file, void *buf)
{
    Sint32 drive;
    Sint32 block;
    Sint32 result;

    if (!vmsio_isready(file->drive)) {
        vmsfs_unmount(file->drive);
        return BUD_ERR_NO_DISK;
    }
    if (!vmsio_ismount(file->drive)) {
        return BUD_ERR_NO_DISK;
    }
    if (!buVmsDrive[file->drive].mounted) {
        return BUD_ERR_NO_DISK;
    }
    if (buIsFormat(file->drive) < 0) {
        return BUD_ERR_UNFORMAT;
    }
    drive = file->drive;
    if (file->next_block == FAT_LAST) {
        if (file->remaining != 0) {
            return BUD_ERR_GENERIC;
        }
        return 0;
    }
    if (file->remaining == 0) {
        return 0;
    }
    if (file->next_block == FAT_FREE) {
        return BUD_ERR_FILE_BROKEN;
    }
    if (file->next_block == FAT_BAD) {
        return BUD_ERR_FILE_BROKEN;
    }
    block = file->next_block;
    result = vmsfs_read_retry(drive, block, buf);
    if (result < 0) {
        return result;
    }
    file->next_block = fat_fetch(drive, block);
    file->remaining--;
    return file->remaining;
}

Sint32 vmsfs_fisexist(Sint32 drive, const char *fname)
{
    struct file_entry entry;

    if (!buVmsDrive[drive].has_work) {
        return BUD_ERR_NO_DISK;
    }
    if (!vmsio_isready(drive)) {
        vmsfs_unmount(drive);
        return BUD_ERR_NO_DISK;
    }
    if (!vmsio_ismount(drive)) {
        return BUD_ERR_NO_DISK;
    }
    if (!buVmsDrive[drive].mounted) {
        return BUD_ERR_NO_DISK;
    }
    if (buIsFormat(drive) < 0) {
        return BUD_ERR_UNFORMAT;
    }
    if (find_file(drive, fname, &entry) >= 0) {
        return BUD_ERR_OK;
    }
    return BUD_ERR_FILE_NOT_FOUND;
}

Sint32 vmsfs_findfirst(Sint32 drive, char *fname)
{
    struct vmsfs_drive *drv = &buVmsDrive[drive];
    struct dir_entry *dir;
    Uint8 type;
    Sint32 found;
    Sint32 i;

    if (!drv->has_work) {
        return BUD_ERR_NO_DISK;
    }
    if (!vmsio_isready(drive)) {
        vmsfs_unmount(drive);
        return BUD_ERR_NO_DISK;
    }
    if (!vmsio_ismount(drive)) {
        return BUD_ERR_NO_DISK;
    }
    if (!buVmsDrive[drive].mounted) {
        return BUD_ERR_NO_DISK;
    }
    if (buIsFormat(drive) < 0) {
        return BUD_ERR_UNFORMAT;
    }
    drv->find_slot = 0;
    dir = (struct dir_entry *)dir_slot(drive, 0);
    found = 0;
    for (i = 0; i < drv->media->user_blocks; i++, dir++) {
        type = dir->type;
        if (type == DIR_FILE || type == DIR_EXEC) {
            found = 1;
            break;
        }
        drv->find_slot++;
    }
    if (!found) {
        return BUD_ERR_FILE_NOT_FOUND;
    }
    for (i = 0; i < FNAME_SIZE; i++) {
        fname[i] = dir->fname[i];
    }
    fname[FNAME_SIZE] = 0;
    drv->find_slot++;
    return BUD_ERR_OK;
}

Sint32 vmsfs_findnext(Sint32 drive, char *fname)
{
    struct vmsfs_drive *drv = &buVmsDrive[drive];
    struct dir_entry *dir;
    Uint8 type;
    Sint32 found;
    Sint32 i;

    if (!drv->has_work) {
        return BUD_ERR_NO_DISK;
    }
    if (!vmsio_isready(drive)) {
        vmsfs_unmount(drive);
        return BUD_ERR_NO_DISK;
    }
    if (!vmsio_ismount(drive)) {
        return BUD_ERR_NO_DISK;
    }
    if (!buVmsDrive[drive].mounted) {
        return BUD_ERR_NO_DISK;
    }
    if (buIsFormat(drive) < 0) {
        return BUD_ERR_UNFORMAT;
    }
    dir = (struct dir_entry *)dir_slot(drive, drv->find_slot);
    found = 0;
    for (i = drv->find_slot; i < drv->media->user_blocks; i++, dir++) {
        drv->find_slot++;
        type = dir->type;
        if (type == DIR_FILE || type == DIR_EXEC) {
            found = 1;
            break;
        }
    }
    if (!found) {
        return BUD_ERR_FILE_NOT_FOUND;
    }
    for (i = 0; i < FNAME_SIZE; i++) {
        fname[i] = dir->fname[i];
    }
    fname[FNAME_SIZE] = 0;
    return BUD_ERR_OK;
}

Sint32 vmsfs_findfirst_ex(Sint32 drive, char *fname, BUS_FILEINFO *finfo)
{
    struct vmsfs_drive *drv = &buVmsDrive[drive];
    struct dir_entry *dir;
    Uint8 type;
    Sint32 found;
    Sint32 i;
    struct file_entry entry;

    if (!drv->has_work) {
        return BUD_ERR_NO_DISK;
    }
    if (!vmsio_isready(drive)) {
        vmsfs_unmount(drive);
        return BUD_ERR_NO_DISK;
    }
    if (!vmsio_ismount(drive)) {
        return BUD_ERR_NO_DISK;
    }
    if (!buVmsDrive[drive].mounted) {
        return BUD_ERR_NO_DISK;
    }
    if (buIsFormat(drive) < 0) {
        return BUD_ERR_UNFORMAT;
    }
    drv->find_slot = 0;
    dir = (struct dir_entry *)dir_slot(drive, 0);
    found = 0;
    for (i = 0; i < drv->media->user_blocks; i++, dir++) {
        type = dir->type;
        if (type == DIR_FILE || type == DIR_EXEC) {
            found = 1;
            break;
        }
        drv->find_slot++;
    }
    if (!found) {
        return BUD_ERR_FILE_NOT_FOUND;
    }
    for (i = 0; i < FNAME_SIZE; i++) {
        fname[i] = dir->fname[i];
    }
    fname[FNAME_SIZE] = 0;
    drv->find_slot++;
    unpack_entry((Uint8 *)dir, &entry);
    finfo->filesize = entry.blocks * (drv->media->last_byte + 1);
    finfo->blocks = entry.blocks;
    finfo->type = entry.type;
    finfo->copyflag = entry.copyflag;
    finfo->headerofs = entry.hdrofs;
    if (entry.type == DIR_EXEC) {
        finfo->type = BUD_FILETYPE_EXECUTABLE;
    } else {
        finfo->type = BUD_FILETYPE_NORMAL;
    }
    unpack_time(&finfo->time, entry.time);
    return BUD_ERR_OK;
}

Sint32 vmsfs_findnext_ex(Sint32 drive, char *fname, BUS_FILEINFO *finfo)
{
    struct vmsfs_drive *drv = &buVmsDrive[drive];
    struct dir_entry *dir;
    Uint8 type;
    Sint32 found;
    Sint32 i;
    struct file_entry entry;

    if (!drv->has_work) {
        return BUD_ERR_NO_DISK;
    }
    if (!vmsio_isready(drive)) {
        vmsfs_unmount(drive);
        return BUD_ERR_NO_DISK;
    }
    if (!vmsio_ismount(drive)) {
        return BUD_ERR_NO_DISK;
    }
    if (!buVmsDrive[drive].mounted) {
        return BUD_ERR_NO_DISK;
    }
    if (buIsFormat(drive) < 0) {
        return BUD_ERR_UNFORMAT;
    }
    dir = (struct dir_entry *)dir_slot(drive, drv->find_slot);
    found = 0;
    for (i = drv->find_slot; i < drv->media->user_blocks; i++, dir++) {
        drv->find_slot++;
        type = dir->type;
        if (type == DIR_FILE || type == DIR_EXEC) {
            found = 1;
            break;
        }
    }
    if (!found) {
        return BUD_ERR_FILE_NOT_FOUND;
    }
    for (i = 0; i < FNAME_SIZE; i++) {
        fname[i] = dir->fname[i];
    }
    fname[FNAME_SIZE] = 0;
    unpack_entry((Uint8 *)dir, &entry);
    finfo->filesize = entry.blocks * (drv->media->last_byte + 1);
    finfo->blocks = entry.blocks;
    finfo->type = entry.type;
    finfo->copyflag = entry.copyflag;
    finfo->headerofs = entry.hdrofs;
    if (entry.type == DIR_EXEC) {
        finfo->type = BUD_FILETYPE_EXECUTABLE;
    } else {
        finfo->type = BUD_FILETYPE_NORMAL;
    }
    unpack_time(&finfo->time, entry.time);
    return BUD_ERR_OK;
}

/* free a file's directory entry, and its blocks when check_chain finds its
 * chain whole. only the caches change, and the caller writes them back */
static Sint32 delete_file(Sint32 drive, const char *fname)
{
    Sint32 slot;
    Uint8 *raw;
    Sint32 i;
    Sint32 block;
    Sint32 next;
    struct file_entry entry;

    slot = find_file(drive, fname, &entry);
    if (slot < 0) {
        return BUD_ERR_FILE_NOT_FOUND;
    }
    raw = dir_slot(drive, slot);
    if (check_chain(drive, fname) == BUD_ERR_OK) {
        block = entry.first_block;
        for (i = 0; i < entry.blocks; i++) {
            next = fat_fetch(drive, block);
            fat_store(drive, block, FAT_FREE);
            block = next;
        }
    }
    memset(raw, 0, DIR_ENTRY_SIZE);
    dir_touch(drive, slot);
    return BUD_ERR_OK;
}

Sint32 vmsfs_fdelete(Sint32 drive, const char *fname)
{
    struct vmsfs_drive *drv = &buVmsDrive[drive];
    Sint32 result;

    drv->progress = 1;
    drv->progress_max = buProgMaxTbl[drv->capacity][BUD_OP_DELETEFILE];
    if (!buVmsDrive[drive].has_work) {
        return BUD_ERR_NO_DISK;
    }
    if (!vmsio_isready(drive)) {
        vmsfs_unmount(drive);
        return BUD_ERR_NO_DISK;
    }
    if (!vmsio_ismount(drive)) {
        return BUD_ERR_NO_DISK;
    }
    if (!buVmsDrive[drive].mounted) {
        return BUD_ERR_NO_DISK;
    }
    if (buIsFormat(drive) < 0) {
        return BUD_ERR_UNFORMAT;
    }
    if (check_name(drive, fname) < 0) {
        return BUD_ERR_ACCESS_DENIED;
    }
    if ((result = delete_file(drive, fname)) < 0) {
        return result;
    }
    if ((result = flush_dir(drive)) < 0) {
        return result;
    }
    return write_fat(drive);
}

Sint32 vmsfs_fdelete_ex(Sint32 drive, const char **fnames, Sint32 nfiles)
{
    struct vmsfs_drive *drv = &buVmsDrive[drive];
    Sint32 result;
    Sint32 i;
    Sint32 missing;
    const char **each;

    drv->progress = 1;
    drv->progress_max = buProgMaxTbl[drv->capacity][BUD_OP_DELETEFILEEX];
    if (!buVmsDrive[drive].has_work) {
        return BUD_ERR_NO_DISK;
    }
    if (!vmsio_isready(drive)) {
        vmsfs_unmount(drive);
        return BUD_ERR_NO_DISK;
    }
    if (!vmsio_ismount(drive)) {
        return BUD_ERR_NO_DISK;
    }
    if (!buVmsDrive[drive].mounted) {
        return BUD_ERR_NO_DISK;
    }
    if (buIsFormat(drive) < 0) {
        return BUD_ERR_UNFORMAT;
    }
    for (i = 0, each = fnames; i < nfiles; i++, each++) {
        if (check_name(drive, *each) < 0) {
            return BUD_ERR_CANNOT_DELETE;
        }
    }
    missing = 0;
    for (i = 0; i < nfiles; i++) {
        if (delete_file(drive, fnames[i]) < 0) {
            missing = 1;
        }
    }
    if ((result = flush_dir(drive)) < 0) {
        return result;
    }
    if ((result = write_fat(drive)) < 0) {
        return result;
    }
    if (missing) {
        return BUD_ERR_CANNOT_DELETE;
    }
    return BUD_ERR_OK;
}

Sint32 vmsfs_frename(Sint32 drive, const char *from, const char *to)
{
    struct vmsfs_drive *drv = &buVmsDrive[drive];
    Sint32 slot;
    Sint32 i;
    struct dir_entry *dir;
    struct file_entry entry;

    drv->progress = 1;
    drv->progress_max = buProgMaxTbl[drv->capacity][BUD_OP_RENAMEFILE];
    if (!buVmsDrive[drive].has_work) {
        return BUD_ERR_NO_DISK;
    }
    if (!vmsio_isready(drive)) {
        vmsfs_unmount(drive);
        return BUD_ERR_NO_DISK;
    }
    if (!vmsio_ismount(drive)) {
        return BUD_ERR_NO_DISK;
    }
    if (!buVmsDrive[drive].mounted) {
        return BUD_ERR_NO_DISK;
    }
    if (buIsFormat(drive) < 0) {
        return BUD_ERR_UNFORMAT;
    }
    if (check_name(drive, from) < 0) {
        return BUD_ERR_ACCESS_DENIED;
    }
    slot = find_file(drive, from, &entry);
    if (slot < 0) {
        return BUD_ERR_FILE_NOT_FOUND;
    }
    if (find_file(drive, to, &entry) >= 0) {
        return BUD_ERR_FILE_EXIST;
    }
    dir = (struct dir_entry *)dir_slot(drive, slot);
    for (i = 0; i < FNAME_SIZE; i++) {
        dir->fname[i] = to[i];
    }
    dir_touch(drive, slot);
    return flush_dir(drive);
}

Sint32 vmsfs_fsettime(Sint32 drive, const char *fname, const BUS_TIME *time)
{
    struct vmsfs_drive *drv = &buVmsDrive[drive];
    Sint32 slot;
    Sint32 i;
    Uint8 *raw;
    Uint8 bcd[TIME_SIZE];
    struct file_entry entry;

    drv->progress = 1;
    drv->progress_max = buProgMaxTbl[drv->capacity][BUD_OP_SETFILEATTR];
    if (!buVmsDrive[drive].has_work) {
        return BUD_ERR_NO_DISK;
    }
    if (!vmsio_isready(drive)) {
        vmsfs_unmount(drive);
        return BUD_ERR_NO_DISK;
    }
    if (!vmsio_ismount(drive)) {
        return BUD_ERR_NO_DISK;
    }
    if (!buVmsDrive[drive].mounted) {
        return BUD_ERR_NO_DISK;
    }
    if (buIsFormat(drive) < 0) {
        return BUD_ERR_UNFORMAT;
    }
    if (check_name(drive, fname) < 0) {
        return BUD_ERR_ACCESS_DENIED;
    }
    slot = find_file(drive, fname, &entry);
    if (slot < 0) {
        return BUD_ERR_FILE_NOT_FOUND;
    }
    raw = dir_slot(drive, slot);
    pack_time(bcd, time);
    for (i = 0; i < TIME_SIZE; i++) {
        raw[DIR_TIME + i] = bcd[i];
    }
    dir_touch(drive, slot);
    return flush_dir(drive);
}

Sint32 vmsfs_fgettime(Sint32 drive, const char *fname, BUS_TIME *time)
{
    Sint32 slot;
    Sint32 i;
    Uint8 *raw;
    struct file_entry entry;
    Uint8 bcd[TIME_SIZE];

    if (!buVmsDrive[drive].has_work) {
        return BUD_ERR_NO_DISK;
    }
    if (!vmsio_isready(drive)) {
        vmsfs_unmount(drive);
        return BUD_ERR_NO_DISK;
    }
    if (!vmsio_ismount(drive)) {
        return BUD_ERR_NO_DISK;
    }
    if (!buVmsDrive[drive].mounted) {
        return BUD_ERR_NO_DISK;
    }
    if (buIsFormat(drive) < 0) {
        return BUD_ERR_UNFORMAT;
    }
    if (check_name(drive, fname) < 0) {
        return BUD_ERR_ACCESS_DENIED;
    }
    slot = find_file(drive, fname, &entry);
    if (slot < 0) {
        return BUD_ERR_FILE_NOT_FOUND;
    }
    raw = dir_slot(drive, slot);
    for (i = 0; i < TIME_SIZE; i++) {
        bcd[i] = raw[DIR_TIME + i];
    }
    unpack_time(time, bcd);
    return BUD_ERR_OK;
}

Sint32 vmsfs_fsetattr(Sint32 drive, const char *fname, Uint16 hdrofs,
                      Uint8 copyflag)
{
    struct vmsfs_drive *drv = &buVmsDrive[drive];
    Sint32 slot;
    Uint8 *raw;
    struct file_entry entry;

    drv->progress = 1;
    drv->progress_max = buProgMaxTbl[drv->capacity][BUD_OP_SETFILEATTR];
    if (!buVmsDrive[drive].has_work) {
        return BUD_ERR_NO_DISK;
    }
    if (!vmsio_isready(drive)) {
        vmsfs_unmount(drive);
        return BUD_ERR_NO_DISK;
    }
    if (!vmsio_ismount(drive)) {
        return BUD_ERR_NO_DISK;
    }
    if (!buVmsDrive[drive].mounted) {
        return BUD_ERR_NO_DISK;
    }
    if (buIsFormat(drive) < 0) {
        return BUD_ERR_UNFORMAT;
    }
    if (check_name(drive, fname) < 0) {
        return BUD_ERR_ACCESS_DENIED;
    }
    slot = find_file(drive, fname, &entry);
    if (slot < 0) {
        return BUD_ERR_FILE_NOT_FOUND;
    }
    raw = dir_slot(drive, slot);
    if (buEnableHeaderOffset) {
        _vm->set_word(raw, DIR_HDROFS, hdrofs);
    }
    raw[DIR_COPYFLAG] = copyflag;
    dir_touch(drive, slot);
    return flush_dir(drive);
}

Sint32 vmsfs_fgetsize(Sint32 drive, const char *fname)
{
    Sint32 result;
    struct file_entry entry;

    if (!buVmsDrive[drive].has_work) {
        return BUD_ERR_NO_DISK;
    }
    if (!vmsio_isready(drive)) {
        vmsfs_unmount(drive);
        return BUD_ERR_NO_DISK;
    }
    if (!vmsio_ismount(drive)) {
        return BUD_ERR_NO_DISK;
    }
    if (!buVmsDrive[drive].mounted) {
        return BUD_ERR_NO_DISK;
    }
    if (buIsFormat(drive) < 0) {
        return BUD_ERR_UNFORMAT;
    }
    if (check_name(drive, fname) < 0) {
        return BUD_ERR_ACCESS_DENIED;
    }
    result = find_file(drive, fname, &entry);
    if (result < 0) {
        return BUD_ERR_FILE_NOT_FOUND;
    }
    return entry.blocks;
}

Sint32 vmsfs_xfind(Sint32 drive, char *fname)
{
    Sint32 slot;
    Sint32 i;
    struct file_entry entry;

    if (!buVmsDrive[drive].has_work) {
        return BUD_ERR_NO_DISK;
    }
    if (!vmsio_isready(drive)) {
        vmsfs_unmount(drive);
        return BUD_ERR_NO_DISK;
    }
    if (!vmsio_ismount(drive)) {
        return BUD_ERR_NO_DISK;
    }
    if (!buVmsDrive[drive].mounted) {
        return BUD_ERR_NO_DISK;
    }
    if (buIsFormat(drive) < 0) {
        return BUD_ERR_UNFORMAT;
    }
    slot = find_exec_file(drive, &entry);
    if (slot < 0) {
        return BUD_ERR_FILE_NOT_FOUND;
    }
    for (i = 0; i < FNAME_SIZE; i++) {
        fname[i] = entry.fname[i];
    }
    fname[FNAME_SIZE] = 0;
    return BUD_ERR_OK;
}

Sint32 vmsfs_isformat(Sint32 drive)
{
    struct vmsfs_drive *drv = &buVmsDrive[drive];
    Sint32 i;

    if (!drv->has_work) {
        return BUD_ERR_NO_DISK;
    }
    if (!vmsio_isready(drive)) {
        vmsfs_unmount(drive);
        return BUD_ERR_NO_DISK;
    }
    if (!vmsio_ismount(drive)) {
        return BUD_ERR_NO_DISK;
    }
    if (!buVmsDrive[drive].mounted) {
        return BUD_ERR_NO_DISK;
    }
    for (i = 0; i < FORMATTED_SIZE; i++) {
        if (drv->sys[i] != FORMATTED) {
            drv->fat_cached = 0;
            drv->dir_cached = 0;
            return BUD_ERR_UNFORMAT;
        }
    }
    if (buIllegalFormat[drive]) {
        return BUD_ERR_UNFORMAT;
    }
    return BUD_ERR_OK;
}

Sint32 vmsfs_getvolume(Sint32 drive, Uint8 *volume)
{
    struct vmsfs_drive *drv = &buVmsDrive[drive];
    Sint32 i;

    if (!buVmsDrive[drive].has_work) {
        return BUD_ERR_NO_DISK;
    }
    if (!vmsio_isready(drive)) {
        vmsfs_unmount(drive);
        return BUD_ERR_NO_DISK;
    }
    if (!vmsio_ismount(drive)) {
        return BUD_ERR_NO_DISK;
    }
    if (!buVmsDrive[drive].mounted) {
        return BUD_ERR_NO_DISK;
    }
    if (buIsFormat(drive) < 0) {
        return BUD_ERR_UNFORMAT;
    }
    for (i = 0; i < VOLUME_SIZE; i++) {
        *volume++ = drv->sys[i + SYS_VOLUME];
    }
    return BUD_ERR_OK;
}

Sint32 vmsfs_setvolume(Sint32 drive, const Uint8 *volume)
{
    struct vmsfs_drive *drv = &buVmsDrive[drive];
    Sint32 i;

    if (!buVmsDrive[drive].has_work) {
        return BUD_ERR_NO_DISK;
    }
    if (!vmsio_isready(drive)) {
        vmsfs_unmount(drive);
        return BUD_ERR_NO_DISK;
    }
    if (!vmsio_ismount(drive)) {
        return BUD_ERR_NO_DISK;
    }
    if (!buVmsDrive[drive].mounted) {
        return BUD_ERR_NO_DISK;
    }
    if (buIsFormat(drive) < 0) {
        return BUD_ERR_UNFORMAT;
    }
    for (i = 0; i < VOLUME_SIZE; i++) {
        drv->sys[i + SYS_VOLUME] = 0;
    }
    for (i = 0; i < VOLUME_SIZE; i++) {
        drv->sys[i + SYS_VOLUME] = *volume++;
    }
    return write_system(drive);
}

Sint32 vmsfs_stat(void)
{
    return 0;
}

/* the lowest user block that is neither free nor bad, or -1 */
static Sint32 first_used_block(Sint32 drive)
{
    struct vmsfs_drive *drv = &buVmsDrive[drive];
    Sint32 i;
    Sint32 link;

    for (i = 0; i < drv->media->user_blocks; i++) {
        link = _vm->get_word(drv->fat, i * 2);
        switch (link) {
        case FAT_FREE:
        case FAT_BAD:
            break;
        default:
            return i;
        }
    }
    return -1;
}

/* the highest free user block at or above lowest, or -1 */
static Sint32 last_free_block(Sint32 drive, Sint32 lowest)
{
    struct vmsfs_drive *drv = &buVmsDrive[drive];
    Sint32 i;

    for (i = drv->media->user_blocks - 1; i >= lowest; i--) {
        if (_vm->get_word(drv->fat, i * 2) == FAT_FREE) {
            return i;
        }
    }
    return -1;
}

/* the directory entry of the file that starts at block, or NULL */
static Uint8 *entry_starting_at(Sint32 drive, Sint32 block)
{
    struct vmsfs_drive *drv = &buVmsDrive[drive];
    Uint8 *raw;
    Uint8 type;
    Sint32 i;

    raw = drv->dir;
    for (i = 0; i < drv->media->user_blocks; i++, raw += DIR_ENTRY_SIZE) {
        type = *raw;
        if (type != DIR_NONE) {
            if (_vm->get_word(raw, DIR_FIRST_BLOCK) == block) {
                return raw;
            }
        }
    }
    return NULL;
}

/* 0 when every link in the FAT stays in its area, and -1 when one doesn't. a
 * user block's link must be a user block, and any other block's a block on
 * the card */
static Sint32 check_links(Sint32 drive)
{
    struct vmsfs_drive *drv = &buVmsDrive[drive];
    Sint32 i;
    Sint32 link;

    for (i = 0; i < drv->media->last_block + 1; i++) {
        link = fat_fetch(drive, i);
        switch (link) {
        case FAT_LAST:
        case FAT_FREE:
        case FAT_BAD:
            break;
        default:
            if (i < drv->media->user_blocks) {
                if (link >= drv->media->user_blocks) {
                    return -1;
                }
            } else {
                if (link >= drv->media->last_block + 1) {
                    return -1;
                }
            }
            break;
        }
    }
    return 0;
}

Sint32 vmsfs_xdefrag(Sint32 drive, void *buf)
{
    struct vmsfs_drive *drv = &buVmsDrive[drive];
    Sint32 from;
    Sint32 to;
    Sint32 prev;
    Sint32 result;
    Sint32 i;
    Uint8 *raw;
    char fname[FNAME_SIZE + 1];

    if (!buVmsDrive[drive].has_work) {
        return BUD_ERR_NO_DISK;
    }
    if (!vmsio_isready(drive)) {
        vmsfs_unmount(drive);
        return BUD_ERR_NO_DISK;
    }
    if (!vmsio_ismount(drive)) {
        return BUD_ERR_NO_DISK;
    }
    if (!buVmsDrive[drive].mounted) {
        return BUD_ERR_NO_DISK;
    }
    if (buIsFormat(drive) < 0) {
        return BUD_ERR_UNFORMAT;
    }
    if (drv->media->exec_size == 0) {
        return BUD_ERR_ACCESS_DENIED;
    }
    result = vmsfs_xfind(drive, fname);
    if (result == BUD_ERR_OK) {
        return BUD_ERR_ACCESS_DENIED;
    }
    if (check_links(drive) < 0) {
        return BUD_ERR_ACCESS_DENIED;
    }
    for (;;) {
        from = first_used_block(drive);
        if (from < 0) {
            break;
        }
        to = last_free_block(drive, from + 1);
        if (to < 0) {
            break;
        }
        result = vmsfs_read_retry(drive, from, buf);
        if (result < 0) {
            return result;
        }
        result = vmsfs_write_retry(drive, to, buf);
        if (result < 0) {
            return result;
        }
        prev = fat_find_user(drive, from);
        if (prev > 0) {
            fat_store(drive, prev, to);
        } else {
            raw = entry_starting_at(drive, from);
            _vm->set_word(raw, DIR_FIRST_BLOCK, to);
        }
        fat_store(drive, to, fat_fetch(drive, from));
        fat_store(drive, from, FAT_FREE);
    }
    for (i = 0; i < (Sint32)sizeof(drv->dir_unsaved); i++) {
        drv->dir_unsaved[i] = 1;
    }
    if ((result = flush_dir(drive)) >= 0) {
        result = write_fat(drive);
    }
    return result;
}

/* read the directory's blocks into the cache, from dir_block along their
 * chain, dir_size at most */
static Sint32 cache_dir(Sint32 drive)
{
    struct vmsfs_drive *drv = &buVmsDrive[drive];
    Uint16 block;
    Uint16 next;
    Sint32 count;
    Sint32 i;
    Sint32 result;

    count = drv->media->dir_size;
    block = drv->media->dir_block;
    for (i = 0; i < count; i++) {
        result = vmsfs_read_retry(drive, block, drv->dir + (i * BLOCK_SIZE));
        if (result < 0) {
            return result;
        }
        next = fat_fetch(drive, block);
        if (next == FAT_LAST) {
            break;
        }
        block = next;
    }
    return BUD_ERR_OK;
}

/* write the cached directory to the card as a new chain from dir_block. it
 * first marks the old chain FAT_FREE, from dir_block along its links, dir_size
 * blocks at most. each block of the new chain after the first is the highest
 * free block fat_find_dir_or_spare finds. a block whose write fails other than
 * with BUD_ERR_NO_DISK is marked FAT_BAD, and the next block
 * fat_find_dir_or_spare finds replaces it. when the first block moves, the
 * system block records it, and the FAT and system block are written, their
 * results ignored. a directory block's BUD_ERR_NO_DISK returns at once, and
 * the result is otherwise BUD_ERR_OK. nothing tests whether
 * fat_find_dir_or_spare finds a block */
static Sint32 rechain_dir(Sint32 drive)
{
    struct vmsfs_drive *drv = &buVmsDrive[drive];
    Uint16 block;
    Uint16 next;
    Uint16 first;
    Sint32 count;
    Sint32 last;
    Sint32 i;
    Sint32 result;

    first = drv->media->dir_block;
    count = drv->media->dir_size;
    block = first;
    for (i = 0; i < count; i++) {
        next = fat_fetch(drive, block);
        fat_store(drive, block, FAT_FREE);
        if (next == FAT_LAST) {
            break;
        }
        if (next == FAT_FREE) {
            break;
        }
        block = next;
    }
    block = first;
    for (i = 0, last = count - 1; i < count; i++) {
        for (;;) {
            result =
                vmsfs_write_retry(drive, block, drv->dir + (i * BLOCK_SIZE));
            if (result == BUD_ERR_OK) {
                break;
            }
            if (result == (Sint32)BUD_ERR_NO_DISK) {
                return result;
            }
            fat_store(drive, block, FAT_BAD);
            /* @bug logic: with no free block left, the -1 this returns
             * becomes block 0xffff, which the loop writes next (MOV R0,R12 at
             * 0x8c1212e0, EXTU.W R12,R5 at 0x8c1212b2) */
            block = fat_find_dir_or_spare(drive, FAT_FREE);
        }
        if (i == 0) {
            first = block;
        }
        fat_store(drive, block, FAT_LAST);
        if (i != last) {
            /* @bug logic: with no free block left, the -1 this returns
             * becomes 0xffff. fat_store links block to it, FAT_BAD's value,
             * and the loop writes block 0xffff next (MOV.W R0,@R15 at
             * 0x8c121300, BSR fat_store at 0x8c121306) */
            next = fat_find_dir_or_spare(drive, FAT_FREE);
            fat_store(drive, block, next);
            block = next;
        }
    }
    if (first != drv->media->dir_block) {
        _vm->set_word(drv->sys, SYS_DIR_BLOCK, first);
        write_fat(drive);
        write_system(drive);
    }
    return BUD_ERR_OK;
}

/* write the directory blocks dir_unsaved marks, and clear their marks. a
 * block whose write fails other than with BUD_ERR_NO_DISK is marked FAT_BAD,
 * and the highest free block fat_find_dir_or_spare finds takes its place in
 * the chain. after a move, the FAT and then the system block are written, and
 * the first that fails gives the result. a directory block's BUD_ERR_NO_DISK
 * returns at once, and so does BUD_ERR_DISK_FULL when no free block is left.
 * neither writes the FAT or the system block */
static Sint32 flush_dir(Sint32 drive)
{
    Sint32 block;
    Sint32 next;
    Sint32 prev;
    Sint32 spare;
    Uint16 first;
    Sint32 i;
    Sint32 result;
    Sint32 count;
    Sint32 moved;
    struct vmsfs_drive *drv = &buVmsDrive[drive];

    first = drv->media->dir_block;
    count = drv->media->dir_size;
    block = first;
    moved = 0;
    for (i = 0; i < count; i++) {
        if (drv->dir_unsaved[i]) {
            while (1) {
                result = vmsfs_write_retry(drive, block,
                                           drv->dir + (i * BLOCK_SIZE));
                if (result == BUD_ERR_OK) {
                    next = fat_fetch(drive, block);
                    break;
                }
                if (result == (Sint32)BUD_ERR_NO_DISK) {
                    return result;
                }
                moved = 1;
                next = fat_fetch(drive, block);
                fat_store(drive, block, FAT_BAD);
                spare = fat_find_dir_or_spare(drive, FAT_FREE);
                if (spare < 0) {
                    return BUD_ERR_DISK_FULL;
                }
                fat_store(drive, spare, next);
                if (i == 0) {
                    _vm->set_word(drv->sys, SYS_DIR_BLOCK, spare);
                } else {
                    fat_store(drive, prev, spare);
                }
                /* @bug logic: prev takes the block whose write failed.
                 * when the replacement fails too, the next replacement is
                 * linked from that block, and the block before it stays linked
                 * to the first replacement, now FAT_BAD (MOV R14,R8 at
                 * 0x8c121428, the store at 0x8c121424) */
                prev = block;
                block = spare;
            }
            prev = block;
            block = next;
            drv->dir_unsaved[i] = 0;
        } else {
            next = fat_fetch(drive, block);
            prev = block;
            block = next;
        }
    }
    if (moved) {
        result = write_fat(drive);
        if (result < 0) {
            return result;
        }
        result = write_system(drive);
        if (result < 0) {
            return result;
        }
    }
    return BUD_ERR_OK;
}

static Sint32 name_length(const char *fname)
{
    Sint32 length;
    Sint32 i;

    length = 0;
    for (i = 0; i < FNAME_SIZE; i++, fname++) {
        if (*fname == 0) {
            break;
        }
        length++;
    }
    return length;
}

static Sint32 name_equal(const char *a, const char *b)
{
    Sint32 length;
    Sint32 other;
    Sint32 i;

    length = name_length(a);
    other = name_length(b);
    if (length != other) {
        return 0;
    }
    for (i = 0; i < length; i++) {
        if (a[i] != b[i]) {
            return 0;
        }
    }
    return 1;
}

static void unpack_entry(Uint8 *raw, struct file_entry *entry)
{
    Sint32 i;

    entry->type = raw[0];
    /* @hack spelling: both copies read each byte as *(raw + i + offset), which
     * keeps raw + i at @R15 and offsets it for each field (0x8c12150e). as
     * raw[offset + i], the function runs 8 bytes short and 59 of 68 words
     * differ */
    for (i = 0; i < FNAME_SIZE; i++) {
        entry->fname[i] = *(raw + i + DIR_FNAME);
    }
    entry->fname[FNAME_SIZE] = 0;
    for (i = 0; i < TIME_SIZE; i++) {
        entry->time[i] = *(raw + i + DIR_TIME);
    }
    entry->first_block = _vm->get_word(raw, DIR_FIRST_BLOCK);
    entry->blocks = _vm->get_word(raw, DIR_BLOCKS);
    entry->copyflag = raw[DIR_COPYFLAG];
    entry->hdrofs = _vm->get_word(raw, DIR_HDROFS);
}

/* the directory slot of the file named fname, or -1. a slot found also has
 * its entry unpacked into entry */
static Sint32 find_file(Sint32 drive, const char *fname,
                        struct file_entry *entry)
{
    struct vmsfs_drive *drv = &buVmsDrive[drive];
    Uint8 *raw;
    Uint8 type;
    Sint32 i;

    raw = drv->dir;
    for (i = 0; i < drv->media->user_blocks; i++, raw += DIR_ENTRY_SIZE) {
        type = *raw;
        switch (type) {
        case DIR_NONE:
            break;
        case DIR_FILE:
        case DIR_EXEC:
            if (name_equal(fname, (char *)raw + DIR_FNAME)) {
                unpack_entry(raw, entry);
                return i;
            }
            break;
        }
    }
    return -1;
}

static Sint32 find_exec_file(Sint32 drive, struct file_entry *entry)
{
    struct vmsfs_drive *drv = &buVmsDrive[drive];
    Uint8 *raw;
    Uint8 type;
    Sint32 i;

    raw = drv->dir;
    for (i = 0; i < drv->media->user_blocks; i++, raw += DIR_ENTRY_SIZE) {
        type = *raw;
        if (type == DIR_EXEC) {
            unpack_entry(raw, entry);
            return i;
        }
    }
    return -1;
}

static Sint32 find_free_slot(Sint32 drive)
{
    struct vmsfs_drive *drv = &buVmsDrive[drive];
    Uint8 *raw;
    Uint8 type;
    Sint32 i;

    raw = drv->dir;
    for (i = 0; i < drv->media->user_blocks; i++, raw += DIR_ENTRY_SIZE) {
        type = *raw;
        if (type == DIR_NONE) {
            return i;
        }
    }
    return -1;
}

static Sint32 read_system(Sint32 drive)
{
    struct vmsfs_drive *drv = &buVmsDrive[drive];

    return vmsfs_read_retry(drive, drv->media->system_block, drv->sys);
}

static Sint32 write_system(Sint32 drive)
{
    struct vmsfs_drive *drv = &buVmsDrive[drive];

    return vmsfs_write_retry(drive, drv->media->system_block, drv->sys);
}

static Sint32 read_fat(Sint32 drive)
{
    struct vmsfs_drive *drv = &buVmsDrive[drive];
    Sint32 count;
    Sint32 i;
    Sint32 result;

    count = drv->media->fat_size;
    for (i = 0; i < count; i++) {
        result = vmsfs_read_retry(drive, drv->media->fat_block + i,
                                  drv->fat + (i * BLOCK_SIZE));
        if (result < 0) {
            return result;
        }
    }
    return BUD_ERR_OK;
}

static Sint32 write_fat_blocks(Sint32 drive)
{
    struct vmsfs_drive *drv = &buVmsDrive[drive];
    Sint32 count;
    Sint32 i;
    Sint32 result;

    count = drv->media->fat_size;
    for (i = 0; i < count; i++) {
        result = vmsfs_write_retry(drive, drv->media->fat_block + i,
                                   drv->fat + (i * BLOCK_SIZE));
        if (result < 0) {
            fat_store(drive, drv->media->fat_block + i, FAT_BAD);
            return result;
        }
        fat_store(drive, drv->media->fat_block + i, FAT_LAST);
    }
    return BUD_ERR_OK;
}

/* the first block of the highest run of free spare blocks long enough for the
 * FAT, or -1 */
static Sint32 find_fat_room(Sint32 drive)
{
    struct vmsfs_drive *drv = &buVmsDrive[drive];
    Sint32 low;
    Sint32 high;
    Sint32 count;
    Sint32 top;
    Sint32 i;

    low = drv->media->spare_block;
    high = drv->media->spare_block + drv->media->spare_size - 1;
    count = drv->media->fat_size;
    for (;;) {
        top = fat_find_in(drive, FAT_FREE, low, high);
        if (top < 0) {
            return -1;
        }
        for (i = 0; i < count; i++) {
            if (fat_fetch(drive, top - i) != FAT_FREE) {
                high = top - i - 1;
                break;
            }
        }
        if (i == count) {
            return top - count + 1;
        }
    }
}

/* write the FAT's blocks. a FAT block whose write fails is marked FAT_BAD.
 * after BUD_ERR_WRITE_ERROR, the FAT moves to the spare blocks find_fat_room
 * finds, and the write starts over there. BUD_ERR_WRITE_ERROR when
 * find_fat_room finds none, and BUD_ERR_NO_DISK when a block's write fails
 * with it. a block's BUD_ERR_GENERIC ends the call as BUD_ERR_OK, with the
 * FAT's later blocks unwritten. when the call returns BUD_ERR_OK after a move,
 * it writes the system block, which records the move, and ignores that write's
 * result */
static Sint32 write_fat(Sint32 drive)
{
    Sint32 i;
    Sint32 result;
    Sint32 block;
    Sint32 moved;
    struct vmsfs_drive *drv = &buVmsDrive[drive];

    moved = 0;
    for (;;) {
        result = write_fat_blocks(drive);
        switch (result) {
        case BUD_ERR_NO_DISK:
            return BUD_ERR_NO_DISK;
        case BUD_ERR_WRITE_ERROR:
            block = find_fat_room(drive);
            if (block < 0) {
                return BUD_ERR_WRITE_ERROR;
            }
            drv->media->fat_block = block;
            _vm->set_word(drv->sys, SYS_FAT_BLOCK, block);
            for (i = 0; i < drv->media->fat_size; i++) {
                fat_store(drive, drv->media->fat_block + i, FAT_LAST);
            }
            moved = 1;
            break;
        /* @hack redundant: case BUD_ERR_OK, which shares the default's code,
         * keeps SHC's CMP/EQ #0,R0 at 0x8c121842; without it, the function runs
         * 4 bytes short and 58 of 75 words differ */
        case BUD_ERR_OK:
        default:
            if (moved) {
                write_system(drive);
            }
            return BUD_ERR_OK;
        }
    }
}

/* the highest user block whose FAT word is mark, or -1 */
static Sint32 fat_find_user(Sint32 drive, Uint16 mark)
{
    struct vmsfs_drive *drv = &buVmsDrive[drive];
    Sint32 count;
    Sint32 i;

    count = drv->media->user_blocks;
    for (i = count - 1; i >= 0; i--) {
        if (_vm->get_word(drv->fat, i * 2) == mark) {
            return i;
        }
    }
    return -1;
}

/* the highest free user block that no directory entry starts at, or -1. each
 * free block above it that an entry starts at is first marked FAT_LAST */
static Sint32 mark_starts_find_free(Sint32 drive)
{
    Sint32 block;

    for (;;) {
        block = fat_find_user(drive, FAT_FREE);
        if (block < 0) {
            return -1;
        }
        if (unclaimed_block(drive, block) >= 0) {
            break;
        }
        fat_store(drive, block, FAT_LAST);
    }
    return block;
}

/* block, or -1 when a file's directory entry starts at it */
static Sint32 unclaimed_block(Sint32 drive, Uint16 block)
{
    struct vmsfs_drive *drv = &buVmsDrive[drive];
    Uint8 *raw;
    Sint32 first;
    Sint32 i;

    for (i = 0; i < drv->media->user_blocks; i++) {
        raw = dir_slot(drive, i);
        if (*raw == DIR_FILE || *raw == DIR_EXEC) {
            first = _vm->get_word(raw, DIR_FIRST_BLOCK);
            if (block == first) {
                return -1;
            }
        }
    }
    return block;
}

/* set block's FAT word. a block past the card's last is ignored */
static void fat_store(Sint32 drive, Sint32 block, Uint16 word)
{
    struct vmsfs_drive *drv = &buVmsDrive[drive];

    if (block > drv->media->last_block) {
        return;
    }
    _vm->set_word(drv->fat, block * 2, word);
}

static Uint16 fat_fetch(Sint32 drive, Sint32 block)
{
    struct vmsfs_drive *drv = &buVmsDrive[drive];

    return _vm->get_word(drv->fat, block * 2);
}

/* read whichever of the system block, FAT and directory isn't cached. on an
 * unformatted card it stops after the system block. a FAT or directory read
 * that fails with BUD_ERR_GENERIC sets buIllegalFormat, and the call returns
 * BUD_ERR_OK */
static Sint32 load_caches(Sint32 drive)
{
    Sint32 result;
    struct vmsfs_drive *drv = &buVmsDrive[drive];

    buIllegalFormat[drive] = 0;
    if (!drv->sys_cached) {
        if ((result = read_system(drive)) < 0) {
            return result;
        }
    }
    if (vmsfs_isformat(drive) < 0) {
        return BUD_ERR_OK;
    }
    if (!drv->fat_cached) {
        if ((result = read_fat(drive)) < 0) {
            if (result == (Sint32)BUD_ERR_GENERIC) {
                buIllegalFormat[drive] = 1;
                return BUD_ERR_OK;
            }
            return result;
        }
    }
    if (!drv->dir_cached) {
        if ((result = cache_dir(drive)) < 0) {
            if (result == (Sint32)BUD_ERR_GENERIC) {
                buIllegalFormat[drive] = 1;
                return BUD_ERR_OK;
            }
            return result;
        }
    }
    drv->sys_cached = 1;
    drv->fat_cached = 1;
    drv->dir_cached = 1;
    return BUD_ERR_OK;
}

static Uint8 *dir_slot(Sint32 drive, Sint32 slot)
{
    struct vmsfs_drive *drv = &buVmsDrive[drive];

    return drv->dir + (slot * DIR_ENTRY_SIZE);
}

/* mark slot's directory block for flush_dir to write */
static void dir_touch(Sint32 drive, Sint32 slot)
{
    struct vmsfs_drive *drv = &buVmsDrive[drive];

    drv->dir_unsaved[slot >> SLOT_BITS] = 1;
}

static BUFILE alloc_file(Sint32 drive)
{
    BUFILE file;
    Sint32 i;

    file = files;
    for (i = 0; i < file_limit; i++, file++) {
        if (!file->open) {
            file->open = 1;
            file->index = i;
            return file;
        }
    }
    last_error = BUD_ERR_CANNOT_OPEN;
    return NULL;
}

static void free_file(BUFILE file)
{
    memset(file, 0, sizeof(BUS_FILE));
}

/* 0 for every name, so no caller's refusal of a name ever runs */
static Sint32 check_name(Sint32 drive, const char *fname)
{
    return 0;
}

/* BUD_ERR_OK when the file's chain stays in the user blocks and ends at its
 * length. BUD_ERR_FILE_BROKEN when it doesn't, and BUD_ERR_FILE_NOT_FOUND when
 * no file has the name */
static Sint32 check_chain(Sint32 drive, const char *fname)
{
    struct vmsfs_drive *drv = &buVmsDrive[drive];
    Sint32 slot;
    Sint32 block;
    Sint32 i;
    Sint32 next;
    struct file_entry entry;

    slot = find_file(drive, fname, &entry);
    if (slot < 0) {
        return BUD_ERR_FILE_NOT_FOUND;
    }
    dir_slot(drive, slot);
    block = entry.first_block;
    if (block >= drv->media->user_blocks) {
        return BUD_ERR_FILE_BROKEN;
    }
    for (i = 0; i < entry.blocks; i++) {
        next = fat_fetch(drive, block);
        if (i == entry.blocks - 1) {
            if (next != FAT_LAST) {
                return BUD_ERR_FILE_BROKEN;
            }
        } else {
            if (next == block) {
                return BUD_ERR_FILE_BROKEN;
            }
            if (next == FAT_LAST) {
                return BUD_ERR_FILE_BROKEN;
            }
            if (next == FAT_FREE) {
                return BUD_ERR_FILE_BROKEN;
            }
            if (next == FAT_BAD) {
                return BUD_ERR_FILE_BROKEN;
            }
            if (next >= drv->media->user_blocks) {
                return BUD_ERR_FILE_BROKEN;
            }
        }
        block = next;
    }
    return BUD_ERR_OK;
}

static Uint8 to_bcd(Uint32 value)
{
    Uint32 tens;
    Uint32 ones;

    tens = value / 10;
    ones = value % 10;
    return (tens * 16) + ones;
}

static Uint32 from_bcd(Uint32 value)
{
    Uint32 tens;
    Uint32 ones;

    tens = value >> 4;
    ones = value & 0x0f;
    return (tens * 10) + ones;
}

/* pack a time as the card holds it: seven BCD bytes, from the century to the
 * second, then the day of the week */
static void pack_time(Uint8 *bcd, const BUS_TIME *time)
{
    *bcd++ = to_bcd(time->year / 100);
    *bcd++ = to_bcd(time->year % 100);
    *bcd++ = to_bcd(time->month);
    *bcd++ = to_bcd(time->day);
    *bcd++ = to_bcd(time->hour);
    *bcd++ = to_bcd(time->minute);
    *bcd++ = to_bcd(time->second);
    *bcd = weekday_to_card[time->dayofweek & 7];
}

static void unpack_time(BUS_TIME *time, Uint8 *bcd)
{
    time->year = (from_bcd(bcd[0]) * 100) + from_bcd(bcd[1]);
    time->month = from_bcd(bcd[2]);
    time->day = from_bcd(bcd[3]);
    time->hour = from_bcd(bcd[4]);
    time->minute = from_bcd(bcd[5]);
    time->second = from_bcd(bcd[6]);
    time->dayofweek = weekday_from_card[bcd[7] & 7];
}

/* 0 when the file system takes the card's layout, and -1 when it doesn't. the
 * last block must be the system block, and one of the four LAST_BLOCK_
 * numbers. the FAT and directory must start below it, and neither the
 * executable area's start nor its size may pass it. with buInit's work area,
 * the drive's FAT and directory blocks must hold the card's. it sets
 * drv->capacity only when it returns 0 */
static Sint32 admit_media(struct vmsfs_drive *drv)
{
    struct vmsfs_media *media;

    media = drv->media;
    if (media->last_block < 1 || media->last_block > LAST_BLOCK_1MB) {
        return -1;
    }
    if (media->last_block != media->system_block) {
        return -1;
    }
    if (media->fat_block < 0 || media->fat_block >= media->last_block) {
        return -1;
    }
    if (media->dir_block < 0 || media->dir_block >= media->last_block) {
        return -1;
    }
    if (media->exec_block < 0 || media->exec_block > media->last_block) {
        return -1;
    }
    if (media->exec_size < 0 || media->exec_size > media->last_block) {
        return -1;
    }
    if (_vm->dynamic_work == 0) {
        if (drv->fat_blocks < media->fat_size) {
            return -1;
        }
        if (drv->dir_blocks < media->dir_size) {
            return -1;
        }
    }
    switch (media->last_block) {
    case LAST_BLOCK_128KB:
        drv->capacity = CARD_128KB;
        break;
    case LAST_BLOCK_256KB:
        drv->capacity = CARD_256KB;
        break;
    case LAST_BLOCK_512KB:
        drv->capacity = CARD_512KB;
        break;
    case LAST_BLOCK_1MB:
        drv->capacity = CARD_1MB;
        break;
    default:
        return -1;
    }
    return 0;
}

Sint32 vmsfs_read_disk_image(Sint32 drive, void *buf)
{
    Sint32 i;
    Sint32 result;
    Uint8 *to;
    struct vmsfs_drive *drv = &buVmsDrive[drive];
    /* @hack spelling: a second pointer to the drive, for its progress, which
     * keeps drive in R10 and BLOCK_SIZE in R11 (0x8c121dda, 0x8c121dec).
     * through drv, the two trade and 8 of 56 words differ */
    struct vmsfs_drive *alias;

    to = buf;
    alias = &buVmsDrive[drive];
    alias->progress = 1;
    alias->progress_max = buProgMaxTbl[alias->capacity][BUD_OP_READIMAGE];
    for (i = 0; i < drv->media->last_block + 1; i++) {
        result = vmsfs_read_retry(drive, i, to);
        if (result < 0) {
            return result;
        }
        to += BLOCK_SIZE;
    }
    return BUD_ERR_OK;
}

Sint32 vmsfs_write_disk_image(Sint32 drive, const void *buf)
{
    const Uint8 *from;
    struct vmsfs_drive *drv;
    Sint32 i;
    Sint32 result;

    from = buf;
    drv = &buVmsDrive[drive];
    drv->progress = 1;
    drv->progress_max = buProgMaxTbl[drv->capacity][BUD_OP_WRITEIMAGE];
    for (i = 0; i < drv->media->last_block + 1; from += BLOCK_SIZE, i++) {
        result = vmsfs_write_retry(drive, i, from);
        if (result < 0) {
            return result;
        }
    }
    return BUD_ERR_OK;
}

Sint32 vmsfs_load_file(Sint32 drive, const char *fname, void *buf,
                       Uint32 nblock)
{
    BUFILE file;
    struct vmsfs_drive *drv;
    Uint8 *to;
    Uint32 i;
    Sint32 size;
    Sint32 result;

    if (nblock == 0) {
        size = vmsfs_fgetsize(drive, fname);
        if (size < 0) {
            return size;
        }
        nblock = size;
    }
    result = vmsfs_fopen(drive, fname, &file);
    if (result < 0) {
        return result;
    }
    to = buf;
    drv = &buVmsDrive[drive];
    drv->progress = 1;
    drv->progress_max = nblock;
    for (i = 0; i < nblock; i++, to += BLOCK_SIZE) {
        result = vmsfs_fread1(file, to);
        if (result < 0) {
            goto done;
        }
    }
    result = BUD_ERR_OK;
done:
    vmsfs_fclose(file, 0);
    return result;
}

Sint32 vmsfs_save_file(Sint32 drive, const char *fname, const void *buf,
                       Uint32 nblock, BUS_TIME *time, Sint32 flag)
{
    BUFILE file;
    struct vmsfs_drive *drv;
    const Uint8 *from;
    Uint32 i;
    Sint32 room;
    Sint32 size;
    Sint32 result;

    room = vmsfs_diskfree(drive, BUD_FILETYPE_NORMAL);
    size = vmsfs_fgetsize(drive, fname);
    if (size > 0) {
        room += size;
    }
    if (room < 0) {
        return room;
    }
    if ((Uint32)room < nblock) {
        return BUD_ERR_DISK_FULL;
    }
    /* @hack spelling: the flag's test through a local, which SHC takes as AND
     * R10,R4 and TST R4,R4 (0x8c121fae). tested in place, as TST, the function
     * runs 4 bytes short and 81 of 110 words differ */
    result = flag & BUD_FLAG_VERIFY;
    if (result) {
        return vmsfs_save_file_verify(drive, fname, buf, nblock, time, flag);
    }
    result = vmsfs_fcreate(drive, fname, time, &file);
    if (result < 0) {
        return result;
    }
    file->copyflag = flag;
    from = buf;
    drv = &buVmsDrive[drive];
    drv->progress = 1;
    drv->progress_max = buProgMaxTbl[drv->capacity][BUD_OP_SAVEFILE] + nblock;
    for (i = 0; i < nblock; i++, from += BLOCK_SIZE) {
        result = vmsfs_fwrite1(file, from);
        if (result < 0) {
            goto done;
        }
    }
    result = BUD_ERR_OK;
done:
    vmsfs_fclose(file, 0);
    return result;
}

Sint32 vmsfs_save_file_verify(Sint32 drive, const char *fname, const void *buf,
                              Uint32 nblock, BUS_TIME *time, Sint32 flag)
{
    BUFILE file;
    struct vmsfs_drive *drv;
    const Uint8 *from;
    Uint32 i;
    Sint32 result;

    result = vmsfs_fcreate(drive, fname, time, &file);
    if (result < 0) {
        return result;
    }
    file->copyflag = flag;
    from = buf;
    drv = &buVmsDrive[drive];
    drv->progress = 1;
    drv->progress_max =
        (nblock * 2) + buProgMaxTbl[drv->capacity][BUD_OP_SAVEFILE];
    for (i = 0; i < nblock; i++, from += BLOCK_SIZE) {
        result = vmsfs_fwrite1(file, from);
        if (result < 0) {
            goto done;
        }
    }
    vmsfs_fclose(file, 0);
    result = vmsfs_fopen(drive, fname, &file);
    if (result < 0) {
        return result;
    }
    from = buf;
    for (i = 0; i < nblock; i++, from += BLOCK_SIZE) {
        result = vmsfs_fread1(file, &_vms_verify_buf[drive * VERIFY_SIZE]);
        if (result < 0) {
            goto done;
        }
        if (vmsfs_memcmp(&_vms_verify_buf[drive * VERIFY_SIZE], from)) {
            result = BUD_ERR_VERIFY;
            goto done;
        }
    }
    result = BUD_ERR_OK;
done:
    vmsfs_fclose(file, 0);
    return result;
}

Sint32 vmsfs_save_exec_file(Sint32 drive, const char *fname, const void *buf,
                            Uint32 nblock, BUS_TIME *time, Sint32 flag)
{
    BUFILE file;
    struct vmsfs_drive *drv;
    const Uint8 *from;
    Uint32 i;
    Sint32 room;
    Sint32 result;

    room = vmsfs_diskfree(drive, BUD_FILETYPE_EXECUTABLE);
    if (room < 0) {
        return room;
    }
    if ((Uint32)room < nblock) {
        return BUD_ERR_DISK_FULL;
    }
    /* @hack spelling: the flag's test through a local, which SHC takes as AND
     * R11,R4 and TST R4,R4 (0x8c1221e0). tested in place, as TST, the function
     * runs 2 bytes short and 70 of 104 words differ */
    result = flag & BUD_FLAG_VERIFY;
    if (result) {
        return vmsfs_save_exec_file_verify(drive, fname, buf, nblock, time,
                                           flag);
    }
    result = vmsfs_xcreate(drive, fname, time, &file);
    if (result < 0) {
        return result;
    }
    file->copyflag = flag;
    from = buf;
    drv = &buVmsDrive[drive];
    drv->progress = 1;
    drv->progress_max = buProgMaxTbl[drv->capacity][BUD_OP_SAVEFILE] + nblock;
    for (i = 0; i < nblock; i++, from += BLOCK_SIZE) {
        result = vmsfs_xwrite1(file, from);
        if (result < 0) {
            goto done;
        }
    }
    result = BUD_ERR_OK;
done:
    vmsfs_fclose(file, 0);
    return result;
}

Sint32 vmsfs_save_exec_file_verify(Sint32 drive, const char *fname,
                                   const void *buf, Uint32 nblock,
                                   BUS_TIME *time, Sint32 flag)
{
    BUFILE file;
    struct vmsfs_drive *drv;
    const Uint8 *from;
    Uint32 i;
    Sint32 result;

    result = vmsfs_xcreate(drive, fname, time, &file);
    if (result < 0) {
        return result;
    }
    file->copyflag = flag;
    from = buf;
    drv = &buVmsDrive[drive];
    drv->progress = 1;
    drv->progress_max =
        (nblock * 2) + buProgMaxTbl[drv->capacity][BUD_OP_SAVEFILE];
    for (i = 0; i < nblock; i++, from += BLOCK_SIZE) {
        result = vmsfs_xwrite1(file, from);
        if (result < 0) {
            goto done;
        }
    }
    vmsfs_fclose(file, 0);
    result = vmsfs_fopen(drive, fname, &file);
    if (result < 0) {
        return result;
    }
    from = buf;
    for (i = 0; i < nblock; i++, from += BLOCK_SIZE) {
        result = vmsfs_fread1(file, &_vms_verify_buf[drive * VERIFY_SIZE]);
        if (result < 0) {
            goto done;
        }
        if (vmsfs_memcmp(&_vms_verify_buf[drive * VERIFY_SIZE], from)) {
            result = BUD_ERR_VERIFY;
            goto done;
        }
    }
    result = BUD_ERR_OK;
done:
    vmsfs_fclose(file, 0);
    return result;
}

/* step block times along file's chain from its first block, whatever origin
 * says, and return remaining, the blocks left. the seek resets the next block
 * to the first, but not remaining, which each step counts down. after earlier
 * reads it therefore returns too few. when a step finds the chain ended and
 * remaining 0, the seek returns 0. when a step finds either alone, it returns
 * BUD_ERR_GENERIC */
Sint32 vmsfs_fseek(BUFILE file, Uint32 block, Sint32 origin)
{
    Sint32 drive;
    Uint32 i;

    drive = file->drive;
    file->next_block = file->first_block;
    for (i = 0; i < block; i++) {
        if (file->next_block == FAT_LAST) {
            if (file->remaining != 0) {
                return BUD_ERR_GENERIC;
            }
            return 0;
        }
        if (file->remaining == 0) {
            return BUD_ERR_GENERIC;
        }
        file->next_block = fat_fetch(drive, file->next_block);
        file->remaining--;
    }
    return file->remaining;
}

Sint32 vmsfs_load_file_ex(Sint32 drive, const char *fname, void *buf,
                          Uint32 start, Uint32 nblock)
{
    BUFILE file;
    struct vmsfs_drive *drv;
    Uint8 *to;
    Uint32 i;
    Sint32 size;
    Sint32 result;

    if (nblock == 0) {
        size = vmsfs_fgetsize(drive, fname);
        if (size < 0) {
            return size;
        }
        nblock = size - start;
    }
    result = vmsfs_fopen(drive, fname, &file);
    if (result < 0) {
        return result;
    }
    to = buf;
    drv = &buVmsDrive[drive];
    drv->progress = 1;
    drv->progress_max = nblock;
    vmsfs_fseek(file, start, 0);
    for (i = 0; i < nblock; i++, to += BLOCK_SIZE) {
        result = vmsfs_fread1(file, to);
        if (result < 0) {
            goto done;
        }
    }
    result = BUD_ERR_OK;
done:
    vmsfs_fclose(file, 0);
    return result;
}

Sint32 vmsfs_scanfile(Sint32 drive, const char *fname, Uint16 *chain)
{
    Sint32 slot;
    Sint32 i;
    Sint32 block;
    Sint32 next;
    Sint32 result;
    struct file_entry entry;

    slot = find_file(drive, fname, &entry);
    if (slot < 0) {
        return BUD_ERR_FILE_NOT_FOUND;
    }
    dir_slot(drive, slot);
    block = entry.first_block;
    *chain++ = block;
    /* @bug memory: when the chain reaches a mark before the file's
     * length, the next step takes the mark as a block, and reads a FAT word up
     * to 0x1fffe bytes past drv->fat. a first_block past the FAT does the same
     * (BSR fat_fetch at 0x8c122516, SHLL R5 at 0x8c121a0e) */
    for (i = 0; i < entry.blocks; i++) {
        next = fat_fetch(drive, block);
        block = next;
        *chain++ = block;
    }
    if ((result = check_chain(drive, fname)) < 0) {
        return result;
    }
    return slot;
}

Sint32 vmsfs_rewrite_exec_file(Sint32 drive, const char *fname, const void *buf,
                               Uint32 start, Uint32 nblock)
{
    Uint32 i;
    Sint32 result;
    const Uint8 *from;
    struct vmsfs_drive *drv;
    BUS_FILEINFO finfo;

    /* @hack redundant: a first store to drv, which the second repeats. it
     * keeps drive in R12 and sizeof(struct vmsfs_drive), 0x148, in R13
     * (0x8c122556, 0x8c122558); without it, 44 of 89 words differ */
    drv = &buVmsDrive[drive];
    from = buf;
    drv = &buVmsDrive[drive];
    drv->progress = 1;
    drv->progress_max = nblock;
    result = vmsfs_fgetinfo(drive, fname, &finfo);
    if (result < 0) {
        return result;
    }
    if (finfo.type != BUD_FILETYPE_EXECUTABLE) {
        return BUD_ERR_ACCESS_DENIED;
    }
    if (start >= finfo.blocks) {
        return BUD_ERR_INVALID_PARAM;
    }
    if (nblock > finfo.blocks) {
        return BUD_ERR_INVALID_PARAM;
    }
    if (start + nblock > finfo.blocks) {
        return BUD_ERR_INVALID_PARAM;
    }
    if ((result = check_chain(drive, fname)) < 0) {
        return result;
    }
    for (i = 0; i < nblock; i++) {
        result = vmsfs_write_retry(drive, start + i, from);
        if (result < 0) {
            return result;
        }
        from += BLOCK_SIZE;
    }
    return BUD_ERR_OK;
}
