/* backup file images: build one, count its blocks, parse one into a header,
 * and compute a CRC */
#include <sg_xpt.h>
#include <shinobi.h>
#include <string.h>

#define BLOCK_SIZE 512
#define ICONS_MAX 3
#define PALETTE_AT 96 /* icon_palette's byte offset in struct image_header */

/* a backup file image's header. the icons follow it, then the visual picture
 * if visual_type isn't BUD_VISUALTYPE_NONE, then save_size bytes of save data.
 * zeros pad the image to whole blocks. crc is buCalcCRC of the image up to its
 * padding, computed with crc 0 */
struct image_header {
    char vms_comment[16]; /* padded with spaces, with no NUL */
    char btr_comment[32]; /* padded with spaces, with no NUL */
    Uint8 game_name[16]; /* padded with NULs */
    Uint16 icon_num;
    Uint16 icon_speed;
    Uint16 visual_type;
    Uint16 crc;
    Uint32 save_size;
    Uint8 reserved[20];
    Uint16 icon_palette[16];
};

/* the bytes an image's icons take, by icon_num, and the bytes its visual
 * picture takes, by visual_type */
static Uint32 icon_bytes[ICONS_MAX + 1] = {0, 512, 1024, 1536};
static Uint32 visual_bytes[BUD_VISUALTYPE_C + 1] = {0, 8064, 4544, 2048};

Sint32 buMakeBackupFileImage(void *buf, const BUS_BACKUPFILEHEADER *hdr)
{
    struct image_header *image;
    const char *src;
    char *dst;
    int i;
    Uint32 icon_len;
    Uint32 visual_len;
    Uint32 size;
    Uint32 icon_at;
    Uint32 visual_at;
    Uint32 save_at;
    Uint32 rest;

    if (hdr->icon_num == 0) {
        return BUD_ERR_INVALID_PARAM;
    }
    if (hdr->icon_num > ICONS_MAX) {
        return BUD_ERR_INVALID_PARAM;
    }
    if (hdr->visual_type > BUD_VISUALTYPE_C) {
        return BUD_ERR_INVALID_PARAM;
    }
    if (hdr->icon_palette == NULL) {
        return BUD_ERR_INVALID_PARAM;
    }
    if (hdr->icon_data == NULL) {
        return BUD_ERR_INVALID_PARAM;
    }
    if (hdr->visual_type != BUD_VISUALTYPE_NONE && hdr->visual_data == NULL) {
        return BUD_ERR_INVALID_PARAM;
    }
    image = buf;
    memset(image, 0, PALETTE_AT);
    memset(image->vms_comment, ' ', sizeof(image->vms_comment));
    memset(image->btr_comment, ' ', sizeof(image->btr_comment));
    memset(image->game_name, 0, sizeof(image->game_name));
    dst = image->vms_comment;
    src = hdr->vms_comment;
    for (i = 0; i < (int)sizeof(image->vms_comment); i++) {
        if (!*src) {
            break;
        }
        *dst++ = *src++;
    }
    dst = image->btr_comment;
    src = hdr->btr_comment;
    for (i = 0; i < (int)sizeof(image->btr_comment); i++) {
        if (!*src) {
            break;
        }
        *dst++ = *src++;
    }
    dst = (char *)image->game_name;
    src = (const char *)hdr->game_name;
    for (i = 0; i < (int)sizeof(image->game_name); i++) {
        if (!*src) {
            break;
        }
        *dst++ = *src++;
    }
    /* the checks above keep icon_num and visual_type at most 3, so & 3 leaves
     * both as they are */
    icon_len = icon_bytes[hdr->icon_num & 3];
    visual_len = visual_bytes[hdr->visual_type & 3];
    size = sizeof(*image) + icon_len + visual_len + hdr->save_size;
    image->icon_speed = hdr->icon_speed;
    image->icon_num = hdr->icon_num;
    icon_at = sizeof(*image);
    if (hdr->visual_type == BUD_VISUALTYPE_NONE || hdr->visual_data == NULL) {
        image->visual_type = BUD_VISUALTYPE_NONE;
        visual_at = 0;
    } else {
        image->visual_type = hdr->visual_type;
        visual_at = sizeof(*image) + icon_len;
    }
    if (hdr->save_size == 0 || hdr->save_data == NULL) {
        image->save_size = 0;
        save_at = 0;
    } else {
        image->save_size = hdr->save_size;
        save_at = sizeof(*image) + icon_len + visual_len;
    }
    if (icon_len) {
        memcpy((char *)buf + PALETTE_AT, hdr->icon_palette,
               sizeof(image->icon_palette));
        memcpy((char *)buf + icon_at, hdr->icon_data, icon_len);
    }
    if (visual_len) {
        memcpy((char *)buf + visual_at, hdr->visual_data, visual_len);
    }
    /* @bug memory: a null save_data with a nonzero save_size leaves save_at 0,
     * but this copy still runs, since it tests only hdr's save_size
     * (0x8c11b59e). it copies save_size bytes from address 0 over the image
     * from its start (JSR at 0x8c11b5ac) */
    if (hdr->save_size) {
        memcpy((char *)buf + save_at, hdr->save_data, hdr->save_size);
    }
    rest = size & (BLOCK_SIZE - 1);
    if (rest) {
        memset((char *)buf + size, 0, BLOCK_SIZE - rest);
    }
    image->crc = buCalcCRC(buf, size);
    return (size / BLOCK_SIZE) + (rest ? 1 : 0);
}

Sint32 buCalcBackupFileSize(Uint32 inum, Uint32 vtype, Uint32 size)
{
    Uint32 total;

    if (inum == 0) {
        return BUD_ERR_INVALID_PARAM;
    }
    if (inum > ICONS_MAX) {
        return BUD_ERR_INVALID_PARAM;
    }
    if (vtype > BUD_VISUALTYPE_C) {
        return BUD_ERR_INVALID_PARAM;
    }
    total = sizeof(struct image_header) + icon_bytes[inum] +
            visual_bytes[vtype] + size;
    return (total / BLOCK_SIZE) + ((total & (BLOCK_SIZE - 1)) ? 1 : 0);
}

Sint32 buAnalyzeBackupFileImage(BUS_BACKUPFILEHEADER *hdr, const void *buf)
{
    struct image_header *image = (struct image_header *)buf;
    char *src;
    char *dst;
    char *part;
    Uint32 icon_len;
    Uint32 visual_len;
    Uint32 size;
    int i;
    Uint16 stored;
    Uint16 crc;

    if (image->icon_num == 0) {
        return BUD_ERR_BUPFILE_ILLEGAL;
    }
    if (image->icon_num > ICONS_MAX) {
        return BUD_ERR_BUPFILE_ILLEGAL;
    }
    if (image->visual_type > BUD_VISUALTYPE_C) {
        return BUD_ERR_BUPFILE_ILLEGAL;
    }
    icon_len = icon_bytes[image->icon_num];
    visual_len = visual_bytes[image->visual_type];
    size = sizeof(*image) + icon_len + visual_len + image->save_size;
    dst = hdr->vms_comment;
    src = image->vms_comment;
    for (i = 0; i < (int)sizeof(image->vms_comment); i++) {
        *dst++ = *src++;
    }
    *dst = 0;
    dst = hdr->btr_comment;
    src = image->btr_comment;
    for (i = 0; i < (int)sizeof(image->btr_comment); i++) {
        *dst++ = *src++;
    }
    *dst = 0;
    dst = (char *)hdr->game_name;
    src = (char *)image->game_name;
    for (i = 0; i < (int)sizeof(image->game_name); i++) {
        *dst++ = *src++;
    }
    /* @bug memory: the NUL stored after hdr's game_name has no room in
     * its 16 bytes, and lands on icon_palette's first byte (MOV.B R5,@R12 at
     * 0x8c11b6f4). the store of icon_palette below then overwrites it
     * (MOV.L R3,@(R0,R4) at 0x8c11b70e) */
#ifdef GCC
    /* repairs the store past game_name the tag above names: GCC stores no
     * NUL, as game_name has no room for one. the image's name, padded with
     * NULs, fills its 16 bytes */
#else
    *dst = 0;
#endif
    hdr->icon_speed = image->icon_speed;
    hdr->icon_num = image->icon_num;
    part = (char *)buf;
    hdr->icon_palette = (char *)buf + PALETTE_AT;
    part += sizeof(*image);
    hdr->icon_data = part;
    part += icon_len;
    hdr->visual_type = image->visual_type;
    if (!image->visual_type) {
        hdr->visual_data = NULL;
    } else {
        hdr->visual_data = part;
        part += visual_len;
    }
    hdr->save_size = image->save_size;
    if (image->save_size == 0) {
        hdr->save_data = NULL;
    } else {
        hdr->save_data = part;
    }
    stored = image->crc;
    image->crc = 0;
    crc = buCalcCRC(buf, size);
    image->crc = stored;
    if (crc != stored) {
        return BUD_ERR_BUPFILE_CRC;
    }
    return BUD_ERR_OK;
}

Uint16 buCalcCRC(const void *buf, Uint32 size)
{
    const char *byte = buf;
    Uint32 i;
    Uint32 crc = 0;
    int bit;

    for (i = 0; i < size; i++) {
        crc ^= *byte++ << 8;
        for (bit = 0; bit < 8; bit++) {
            if (crc & 0x8000) {
                crc = (crc << 1) ^ 0x1021;
            } else {
                crc = crc << 1;
            }
        }
    }
    return crc;
}
