/* the memory card file system's block I/O, for the port: make pc compiles
 * this in place of sdk/shinobi/vmsio.c, which talks to the cards over the
 * Maple bus. here each drive's card is a host file of its 256 blocks, a
 * dump of a 128 KB card as the console's tools write one, which the host's
 * card calls read and write a block at a time. the file system above,
 * vmsphys.c and the rest, is the console's.
 *
 * drive A1 holds a card, and the others none. a card the host has no file
 * for starts formatted and empty, as a new card comes: the game never
 * formats one */
#include <shinobi.h>
#include <sg_xpt.h>
#include <vmsfs.h>

#include "xbox/port/host.h"
#include "xbox/port/port.h"

#define BLOCK_SIZE 512
#define LAST_BLOCK 255
#define FAT_BLOCK 254
#define DIR_BLOCK 253
#define DIR_SIZE 13
#define USER_BLOCKS 200
#define SPARE_SIZE 31
#define EXEC_BLOCKS 128
#define FAT_FREE 0xfffc
#define FAT_LAST 0xfffa
/* the drives that hold a card: A1 alone */
#define CARD_DRIVES 1

Sint32 (*vmsio_sim_error)(Sint32 drive, Sint32 block, Uint8 *response, Sint32 *errcode) = NULL;

static void put_le(Uint8 *at, Uint16 value)
{
    at[0] = (Uint8)value;
    at[1] = (Uint8)(value >> 8);
}

static Uint8 bcd(int value)
{
    return (Uint8)(((value / 10) << 4) | (value % 10));
}

/* write a formatted, empty card to drive's file, block by block */
static void format_card(Sint32 drive)
{
    Uint8 block[BLOCK_SIZE];
    int date[7];
    int k;
    int b;

    for (b = 0; b <= LAST_BLOCK; b++) {
        for (k = 0; k < BLOCK_SIZE; k++) {
            block[k] = 0;
        }
        if (b == LAST_BLOCK) {
            for (k = 0; k < 16; k++) {
                block[k] = 0x55;
            }
            HOST->get_date(date);
            block[48] = bcd(date[0] / 100);
            block[49] = bcd(date[0] % 100);
            block[50] = bcd(date[1]);
            block[51] = bcd(date[2]);
            block[52] = bcd(date[3]);
            block[53] = bcd(date[4]);
            block[54] = bcd(date[5]);
            block[55] = bcd(date[6]);
            put_le(block + 64, LAST_BLOCK);
            put_le(block + 66, 0);
            put_le(block + 68, LAST_BLOCK);
            put_le(block + 70, FAT_BLOCK);
            put_le(block + 72, 1);
            put_le(block + 74, DIR_BLOCK);
            put_le(block + 76, DIR_SIZE);
            put_le(block + 80, USER_BLOCKS);
            put_le(block + 82, SPARE_SIZE);
            put_le(block + 84, 0);
            put_le(block + 86, EXEC_BLOCKS);
        } else if (b == FAT_BLOCK) {
            /* every block free but the directory's chain, from its first
             * block down, the FAT's and the system block's */
            for (k = 0; k <= LAST_BLOCK; k++) {
                Uint16 next = FAT_FREE;

                if (k == FAT_BLOCK || k == LAST_BLOCK || k == DIR_BLOCK - DIR_SIZE + 1) {
                    next = FAT_LAST;
                } else if (k > DIR_BLOCK - DIR_SIZE + 1 && k <= DIR_BLOCK) {
                    next = (Uint16)(k - 1);
                }
                put_le(block + k * 2, next);
            }
        }
        HOST->card_write(drive, (unsigned)b, block);
    }
}

void vmsio_init(void)
{
    Sint32 drive;

    vmsio_sim_error = NULL;
    for (drive = 0; drive < CARD_DRIVES; drive++) {
        if (!HOST->card_exists(drive)) {
            format_card(drive);
        }
    }
}

void vmsio_exit(void)
{
}

Sint32 vmsio_isready(Sint32 drive)
{
    return drive >= 0 && drive < CARD_DRIVES;
}

Sint32 vmsio_ismount(Sint32 drive)
{
    return vmsio_isready(drive);
}

/* a 128 KB card's layout, as its media information gives it. the port's
 * cards read and write a block in one phase */
Sint32 vmsio_get_media_info(Sint32 drive, struct vmsfs_media *media)
{
    if (!vmsio_isready(drive)) {
        vmsfb_sleep();
        return BUD_ERR_NO_DISK;
    }
    media->last_block = LAST_BLOCK;
    media->partition = 0;
    media->system_block = LAST_BLOCK;
    media->fat_block = FAT_BLOCK;
    media->fat_size = 1;
    media->dir_block = DIR_BLOCK;
    media->dir_size = DIR_SIZE;
    media->spare_block = USER_BLOCKS;
    media->spare_size = SPARE_SIZE;
    media->exec_block = 0;
    media->exec_size = EXEC_BLOCKS;
    media->icon = 0;
    media->unk_28 = 0;
    media->last_byte = BLOCK_SIZE - 1;
    media->read_phases = 1;
    media->write_phases = 1;
    media->read_bytes = BLOCK_SIZE;
    media->write_bytes = BLOCK_SIZE;
    return BUD_ERR_OK;
}

Sint32 vmsio_block_read(Sint32 drive, Sint32 block, void *buf)
{
    if (!vmsio_ismount(drive)) {
        vmsfb_sleep();
        return BUD_ERR_NO_DISK;
    }
    if (HOST->card_read(drive, (unsigned)block, buf) != 0) {
        return BUD_ERR_GENERIC;
    }
    return BUD_ERR_OK;
}

Sint32 vmsio_block_write(Sint32 drive, Sint32 block, const void *buf)
{
    if (!vmsio_ismount(drive)) {
        vmsfb_sleep();
        return BUD_ERR_NO_DISK;
    }
    if (HOST->card_write(drive, (unsigned)block, buf) != 0) {
        return BUD_ERR_WRITE_ERROR;
    }
    return BUD_ERR_OK;
}

/* 0 when the two blocks match, and -1 otherwise */
Sint32 vmsfs_memcmp(const void *buf1, const void *buf2)
{
    const Uint8 *a = buf1;
    const Uint8 *b = buf2;
    int k;

    for (k = 0; k < BLOCK_SIZE; k++) {
        if (a[k] != b[k]) {
            return -1;
        }
    }
    return 0;
}
