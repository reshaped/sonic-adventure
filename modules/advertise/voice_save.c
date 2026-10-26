/* the writer of the nine voice files, which nothing calls */
#include <sg_xpt.h>
#include <shinobi.h>
#include <string.h>

#include "advertise/trial.h"
#include "advertise/voice_save.h"

#define TEXT_1_LENGTH 28
#define TEXT_2_LENGTH 10
#define TEXT_3_LENGTH 28
#define TEXT_4_LENGTH 10
#define TEXT_5_LENGTH 30
#define TEXT_6_LENGTH 12
#define TEXT_7_LENGTH 26
#define TEXT_8_LENGTH 9
#define TEXT_9_LENGTH 31
#define TEXT_10_LENGTH 10
#define TEXT_11_LENGTH 26
#define TEXT_12_LENGTH 11
#define TEXT_13_LENGTH 30
#define TEXT_14_LENGTH 12
#define TEXT_15_LENGTH 28
#define TEXT_16_LENGTH 10
#define TEXT_17_LENGTH 28
#define TEXT_18_LENGTH 9
#define TEXT_19_LENGTH 16
#define TEXT_20_LENGTH 16
#define TEXT_21_LENGTH 16
#define TEXT_22_LENGTH 16
#define TEXT_23_LENGTH 16
#define TEXT_24_LENGTH 16
#define TEXT_25_LENGTH 16
#define TEXT_26_LENGTH 16
#define TEXT_27_LENGTH 16
#include "advertise/voice_save/text.inc"

#define VOICE_SAVES 9
#define ICON_SIZE 512 /* bytes: 32 by 32 pixels, 4 bits each */
/* an icon's palette: 16 ARGB4444 colors, each a little-endian word */
#define PALETTE_COLORS 16
#define PALETTE_SIZE (PALETTE_COLORS * 2)
/* the bytes an icon or a palette takes: its data, then a 0 that nothing reads.
 * voice_icon_0 takes one more, which puts every palette at an even address, as
 * copy_palettes's word reads need */
#define ICON_STORED (ICON_SIZE + 1)
#define PALETTE_STORED (PALETTE_SIZE + 1)

/* each file's time: midnight on Monday, December 14, 1998. BUS_TIME counts
 * the days of the week from 0 for Sunday */
#define SAVE_YEAR 1998
#define SAVE_MONTH 12
#define SAVE_DAY 14
#define SAVE_DAY_OF_WEEK 1

/* the header's icon_speed: the display frames, not the game's steps, each icon
 * shows before the next */
#define ICON_SPEED 8

/* a voice file: its name on the card, its header's texts, and its icons with a
 * palette for each. each text goes to the BUS_BACKUPFILEHEADER field of its
 * name */
struct voice_save {
    char *name;
    char *btr_comment;
    char *vms_comment;
    char *game_name;
    Uint8 **icons;
    Uint8 **palettes;
    Uint16 icon_count;
};

static Uint8 voice_icon_0[ICON_STORED + 1] = {
#include "advertise/voice_save/voice_icon_0.inc"
};
static Uint8 voice_palette_0[PALETTE_STORED] = {
#include "advertise/voice_save/voice_palette_0.inc"
};
static Uint8 voice_icon_1[ICON_STORED] = {
#include "advertise/voice_save/voice_icon_1.inc"
};
static Uint8 voice_palette_1[PALETTE_STORED] = {
#include "advertise/voice_save/voice_palette_1.inc"
};
static Uint8 voice_icon_2[ICON_STORED] = {
#include "advertise/voice_save/voice_icon_2.inc"
};
static Uint8 voice_palette_2[PALETTE_STORED] = {
#include "advertise/voice_save/voice_palette_2.inc"
};
static Uint8 voice_icon_3[ICON_STORED] = {
#include "advertise/voice_save/voice_icon_3.inc"
};
static Uint8 voice_palette_3[PALETTE_STORED] = {
#include "advertise/voice_save/voice_palette_3.inc"
};
static Uint8 voice_icon_4[ICON_STORED] = {
#include "advertise/voice_save/voice_icon_4.inc"
};
static Uint8 voice_palette_4[PALETTE_STORED] = {
#include "advertise/voice_save/voice_palette_4.inc"
};
static Uint8 voice_icon_5[ICON_STORED] = {
#include "advertise/voice_save/voice_icon_5.inc"
};
static Uint8 voice_palette_5[PALETTE_STORED] = {
#include "advertise/voice_save/voice_palette_5.inc"
};
static Uint8 voice_icon_6[ICON_STORED] = {
#include "advertise/voice_save/voice_icon_6.inc"
};
static Uint8 voice_palette_6[PALETTE_STORED] = {
#include "advertise/voice_save/voice_palette_6.inc"
};
static Uint8 voice_icon_7[ICON_STORED] = {
#include "advertise/voice_save/voice_icon_7.inc"
};
static Uint8 voice_palette_7[PALETTE_STORED] = {
#include "advertise/voice_save/voice_palette_7.inc"
};
static Uint8 voice_icon_8[ICON_STORED] = {
#include "advertise/voice_save/voice_icon_8.inc"
};
static Uint8 voice_palette_8[PALETTE_STORED] = {
#include "advertise/voice_save/voice_palette_8.inc"
};
static Uint8 voice_icon_9[ICON_STORED] = {
#include "advertise/voice_save/voice_icon_9.inc"
};
static Uint8 voice_palette_9[PALETTE_STORED] = {
#include "advertise/voice_save/voice_palette_9.inc"
};
static Uint8 voice_icon_10[ICON_STORED] = {
#include "advertise/voice_save/voice_icon_10.inc"
};
static Uint8 voice_palette_10[PALETTE_STORED] = {
#include "advertise/voice_save/voice_palette_10.inc"
};
static Uint8 voice_icon_11[ICON_STORED] = {
#include "advertise/voice_save/voice_icon_11.inc"
};
static Uint8 voice_palette_11[PALETTE_STORED] = {
#include "advertise/voice_save/voice_palette_11.inc"
};
static Uint8 voice_icon_12[ICON_STORED] = {
#include "advertise/voice_save/voice_icon_12.inc"
};
static Uint8 voice_palette_12[PALETTE_STORED] = {
#include "advertise/voice_save/voice_palette_12.inc"
};
static Uint8 voice_icon_13[ICON_STORED] = {
#include "advertise/voice_save/voice_icon_13.inc"
};
static Uint8 voice_palette_13[PALETTE_STORED] = {
#include "advertise/voice_save/voice_palette_13.inc"
};
static Uint8 voice_icon_14[ICON_STORED] = {
#include "advertise/voice_save/voice_icon_14.inc"
};
static Uint8 voice_palette_14[PALETTE_STORED] = {
#include "advertise/voice_save/voice_palette_14.inc"
};
static Uint8 voice_icon_15[ICON_STORED] = {
#include "advertise/voice_save/voice_icon_15.inc"
};
static Uint8 voice_palette_15[PALETTE_STORED] = {
#include "advertise/voice_save/voice_palette_15.inc"
};
static Uint8 voice_icon_16[ICON_STORED] = {
#include "advertise/voice_save/voice_icon_16.inc"
};
static Uint8 voice_palette_16[PALETTE_STORED] = {
#include "advertise/voice_save/voice_palette_16.inc"
};
static Uint8 *voice_icons[] = {
    voice_icon_0,  voice_icon_1,  voice_icon_2,  voice_icon_3,  voice_icon_4,
    voice_icon_5,  voice_icon_6,  voice_icon_7,  voice_icon_8,  voice_icon_9,
    voice_icon_10, voice_icon_11, voice_icon_12, voice_icon_13, voice_icon_14,
    voice_icon_15, voice_icon_16,
};
static Uint8 *voice_palettes[] = {
    voice_palette_0,  voice_palette_1,  voice_palette_2,  voice_palette_3,
    voice_palette_4,  voice_palette_5,  voice_palette_6,  voice_palette_7,
    voice_palette_8,  voice_palette_9,  voice_palette_10, voice_palette_11,
    voice_palette_12, voice_palette_13, voice_palette_14, voice_palette_15,
    voice_palette_16,
};

/* the nine voice files. the card comments of each of the first eight name one
 * character's voices: Sonic's, Tails', Knuckles', Amy's, E-102's, Big's,
 * Eggman's and Tikal's. SONICADV_V09's name all eight */
static const struct voice_save voice_saves[VOICE_SAVES] = {
    {"SONICADV_V01", TEXT_1, TEXT_2, TEXT_19, &voice_icons[0],
     &voice_palettes[0], 2},
    {"SONICADV_V02", TEXT_3, TEXT_4, TEXT_20, &voice_icons[2],
     &voice_palettes[2], 2},
    {"SONICADV_V03", TEXT_5, TEXT_6, TEXT_21, &voice_icons[4],
     &voice_palettes[4], 2},
    {"SONICADV_V04", TEXT_7, TEXT_8, TEXT_22, &voice_icons[6],
     &voice_palettes[6], 2},
    {"SONICADV_V05", TEXT_9, TEXT_10, TEXT_23, &voice_icons[8],
     &voice_palettes[8], 2},
    {"SONICADV_V06", TEXT_11, TEXT_12, TEXT_24, &voice_icons[10],
     &voice_palettes[10], 2},
    {"SONICADV_V07", TEXT_13, TEXT_14, TEXT_25, &voice_icons[12],
     &voice_palettes[12], 2},
    {"SONICADV_V08", TEXT_15, TEXT_16, TEXT_26, &voice_icons[14],
     &voice_palettes[14], 1},
    {"SONICADV_V09", TEXT_17, TEXT_18, TEXT_27, &voice_icons[15],
     &voice_palettes[15], 2},
};

/* the file image buMakeBackupFileImage builds, and the icons and palettes its
 * header points to, rebuilt for each file */
static char voice_image[0x2000];
static char voice_icon_buffer[0x1000];
static Uint16 voice_palette_buffer[0x400];

static void copy_icons(char *out, Uint8 **icons, Uint16 count)
{
    int i;
    Uint8 *icon;

    while (count > 0) {
        icon = *icons++;
        for (i = 0; i < ICON_SIZE; i++) {
            *out++ = *icon++;
        }
        count--;
    }
}

static void copy_palettes(Uint16 *out, Uint8 **palettes, Uint16 count)
{
    Uint16 *palette;
    int i;

    while (count > 0) {
        palette = (Uint16 *)*palettes++;
        for (i = 0; i < PALETTE_COLORS; i++) {
            *out++ = *palette++;
        }
        count--;
    }
}

/* strncpy without the padding: stop after the NUL, or at size characters */
static void copy_text(char *out, char *from, int size)
{
    for (; size > 0 && (*out++ = *from++); size--) {
    }
}

void voices_save(void)
{
    BUS_TIME time;
    BUS_BACKUPFILEHEADER header;
    int i;
    int blocks;
    int result;

    memset(&header, 0, sizeof(header));
    time.year = SAVE_YEAR;
    time.month = SAVE_MONTH;
    time.day = SAVE_DAY;
    time.hour = 0;
    time.minute = 0;
    time.second = 0;
    time.dayofweek = SAVE_DAY_OF_WEEK;
    for (i = 0; i < VOICE_SAVES; i++) {
        copy_text(header.vms_comment, voice_saves[i].vms_comment,
                  sizeof(header.vms_comment));
        copy_text(header.btr_comment, voice_saves[i].btr_comment,
                  sizeof(header.btr_comment));
        copy_text((char *)header.game_name, voice_saves[i].game_name,
                  sizeof(header.game_name));
        copy_icons(voice_icon_buffer, voice_saves[i].icons,
                   voice_saves[i].icon_count);
        copy_palettes(voice_palette_buffer, voice_saves[i].palettes,
                      voice_saves[i].icon_count);
        header.icon_palette = voice_palette_buffer;
        header.icon_data = voice_icon_buffer;
        header.icon_num = voice_saves[i].icon_count;
        header.icon_speed = ICON_SPEED;
        header.visual_data = NULL;
        header.visual_type = BUD_VISUALTYPE_NONE;
        header.save_data = (void *)voice_save_data;
        header.save_size = sizeof(voice_save_data);
        result = buCalcBackupFileSize(header.icon_num, header.visual_type,
                                      header.save_size);
        if (result == (Sint32)BUD_ERR_INVALID_PARAM) {
            break;
        }
        blocks = buMakeBackupFileImage(voice_image, &header);
        result = buSaveFile(BUD_DRIVE_A1, voice_saves[i].name, voice_image,
                            blocks, &time, BUD_FLAG_VERIFY);
        if (result != BUD_ERR_OK) {
            break;
        }
        while (buStat(BUD_DRIVE_A1) == (Sint32)BUD_STAT_BUSY) {
        }
    }
}
