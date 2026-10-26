/* disc system ID, from the boot ROM's copies of the disc headers */
#include <ctype.h>
#include <sg_xpt.h>
#include <shinobi.h>
#include <string.h>

/* the boot ROM's copies of the header of the disc the console booted from,
 * and of the disc in the drive now */
#define BOOT_HEADER ((char *)0x8c008000)
#define CURRENT_HEADER ((char *)0x8c008100)
/* where a header holds the disc number and count, as in "1/1", and the
 * product number, PRODUCT_SIZE bytes */
#define HEADER_DISC 43
#define HEADER_PRODUCT 64
#define PRODUCT_SIZE 10
/* the most digits each number of "1/1" takes */
#define DISC_DIGITS 2

static Sint32 read_boot_identity(char *header, SYS_BT_SYSTEMID *id);
static Sint32 read_disc_numbers(const char *disc, Sint32 *all, Sint32 *no);

Sint32 syBtGetBootSystemID(SYS_BT_SYSTEMID *id)
{
    /* @hack spelling: result through a local, which SHC stores at @R15 in an
     * 8-byte frame (MOV.L R0,@R15 at 0x8c65f892); returned directly, the
     * frame is 4 bytes, the function is 2 bytes shorter and 8 of 12 words
     * differ */
    Sint32 result;

    result = read_boot_identity(BOOT_HEADER, id);
    return result;
}

Sint32 syBtGetCurrentSystemID(SYS_BT_SYSTEMID *id)
{
    /* @hack spelling: result through a local, which SHC stores at @R15 in an
     * 8-byte frame (MOV.L R0,@R15 at 0x8c65f8b2); returned directly, the
     * frame is 4 bytes, the function is 2 bytes shorter and 8 of 12 words
     * differ */
    Sint32 result;

    result = read_boot_identity(CURRENT_HEADER, id);
    return result;
}

/* fill id from the disc header at header. 1 when the disc field's numbers
 * are all digits, and 0 when they aren't or id is NULL */
static Sint32 read_boot_identity(char *header, SYS_BT_SYSTEMID *id)
{
    Sint32 result;
    Sint32 no;
    Sint32 all;
    /* @hack redundant: a local never used, which takes the frame word at
     * @(8,R15) and makes the frame 32 bytes (ADD #-32,R15 at 0x8c65f8ce);
     * without it, the frame is 28 bytes and 18 of 48 words differ */
    Uint32 unused;
    char *disc;
    char *product;

    if (id == NULL) {
        return 0;
    }
    disc = header + HEADER_DISC;
    product = header + HEADER_PRODUCT;
    result = read_disc_numbers(disc, &all, &no);
    id->nAll = all;
    id->nNo = no;
    memcpy(id->szProduct, product, PRODUCT_SIZE);
    id->szProduct[PRODUCT_SIZE] = 0;
    return result;
}

/* read "1/1" at disc into *no and *all. the disc number takes up to two
 * digits before a slash, and the disc count up to two before a space. 1 when
 * each byte taken for a number is a digit, else 0. *no and *all are set
 * either way */
static Sint32 read_disc_numbers(const char *disc, Sint32 *all, Sint32 *no)
{
    Sint32 valid = 1;
    Sint32 count = 0;
    Sint32 number = 0;
    Sint32 i = 0;
    Sint32 digits;

    while (i < DISC_DIGITS) {
        if (disc[i] == '/') {
            break;
        }
        if (!isdigit(disc[i])) {
            valid = 0;
        }
        number = (number * 10) + disc[i] - '0';
        ++i;
    }
    ++i;
    for (digits = 0; digits < DISC_DIGITS; ++digits) {
        if (disc[i] == ' ') {
            break;
        }
        if (!isdigit(disc[i])) {
            valid = 0;
        }
        count = (count * 10) + disc[i] - '0';
        ++i;
    }
    *all = count;
    *no = number;
    return valid;
}

void syBtGetSystemIDAddress(char **boot, char **current)
{
    *boot = BOOT_HEADER;
    *current = CURRENT_HEADER;
}
