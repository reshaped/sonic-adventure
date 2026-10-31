/* the memory card: the saves, the mail and the downloads, and the act's
 * lights and their editor */
#include <ninja.h>
#include <sg_xpt.h>
#include <shinobi.h>
#include <string.h>

#include "debug/editor.h"
#include "debug/host.h"
#include "draw/light.h"
#include "draw/model.h"
#include "draw/text.h"
#include "object/download.h"
#include "player/player.h"
#include "save/card.h"
#include "save/save.h"
#include "stage/stage.h"
#include "system/frame.h"
#include "system/loader.h"
#include "system/pad.h"
#include "system/task.h"
#include "system/timer.h"

#define TEXT_1_LENGTH 15
#define TEXT_2_LENGTH 30
#define TEXT_3_LENGTH 12
#define TEXT_4_LENGTH 28
#define TEXT_5_LENGTH 13
#define TEXT_6_LENGTH 32
#define TEXT_7_LENGTH 10
#define TEXT_8_LENGTH 32
#define TEXT_9_LENGTH 9
#define TEXT_10_LENGTH 32
#define TEXT_11_LENGTH 12
#define TEXT_12_LENGTH 30
#define TEXT_13_LENGTH 28
#define TEXT_14_LENGTH 24
#define TEXT_15_LENGTH 16
#define TEXT_16_LENGTH 16
#define TEXT_17_LENGTH 16
#define TEXT_18_LENGTH 15
#define TEXT_19_LENGTH 15
#include "save/card/text.inc"

/* buStat's BUD_STAT_BUSY and buCalcBackupFileSize's BUD_ERR_INVALID_PARAM,
 * as the Sint32s the calls return: the SDK spells both unsigned */
#define DRIVE_BUSY ((Sint32)BUD_STAT_BUSY)
#define BAD_HEADER ((Sint32)BUD_ERR_INVALID_PARAM)

/* a card's block, and a file's header: 128 bytes, then 512 for each frame
 * of its icon */
#define BLOCK_SIZE 512
#define FILE_HEADER(icons) (128 + ((icons) * 512))

/* the saves' file is ten blocks: a header with a two-frame icon, then the
 * saves */
#define SAVES_BLOCKS 10
#define SAVES_DATA ((SAVES_BLOCKS * BLOCK_SIZE) - FILE_HEADER(2))

/* where save_write_file builds a file's image and save_read_file loads
 * one: 1 MB into WORK_AREA, clear of the data at its start */
#define FILE_IMAGE ((char *)WORK_AREA + 0x100000)

/* the sources an act's light has room for */
#define LIGHT_SOURCES 30

/* the palettes the light editor and a palettes file cover: the first six
 * of draw_palettes' eight */
#define EDIT_PALETTES 6

/* a palettes file's data is the six palettes. a sources file's is the
 * 0x600 bytes from light_sources on, past the array's end */
#define PALETTES_DATA 0x3000
#define SOURCES_DATA 0x600

/* the blocks the card readers load of a light file. a palettes file takes
 * 26 blocks, so the palettes' last 3712 bytes come from whatever
 * card_buffer held */
#define PALETTES_BLOCKS 18
#define SOURCES_BLOCKS 5

/* card_saving while a file write runs. card_frame_update counts it down
 * once the write ends, so it reads nonzero for three frames after */
#define SAVING_HOLD 4
/* the letters of NOW SAVING, which light up a tenth of the write each */
#define SAVING_LETTERS 10

/* the free editor shows the sources within this distance of the player */
#define SHOWN_RANGE 800.0F

/* a palette editor's move or change: COARSE_STEP entries or levels, or
 * FINE_STEP with R held */
#define COARSE_STEP 8
#define FINE_STEP 1
/* the passes a held direction waits before it moves the cursor, or a
 * color, again: FIRST_WAIT after the first move, then REPEAT_WAIT */
#define FIRST_WAIT 8
#define REPEAT_WAIT 1
/* the channels a color change takes, by the buttons held: A, Y and B */
#define CHANNEL_RED 4
#define CHANNEL_GREEN 2
#define CHANNEL_BLUE 1

#define LIGHT_INNER NJM_DEG_ANG(15)
#define LIGHT_OUTER NJM_DEG_ANG(70)

struct light_source {
    Bool used;
    NJS_POINT3 pos;
    Float range;
    Float rate;
    Float r, g, b;
    /* where light_place_point_and_test measures from, rather than pos. only a
     * file's read sets it */
    NJS_POINT3 touch_pos;
};

/* a file's name on the card, in a struct so one assignment copies it */
struct card_name {
    char text[16];
};

/* the missing-card notice's states. save_commit raises it at NOTICE_SHOW,
 * and card_frame_update moves it down one state a frame, but for
 * NOTICE_WAIT */
enum card_notice {
    NOTICE_NONE,
    NOTICE_CLEAR, /* gone, until card_frame_update makes it NOTICE_NONE */
    NOTICE_WAIT, /* holding the game until B or X, or a mounted card */
    NOTICE_SHOWN, /* holding the game for a frame */
    NOTICE_SHOW /* raised, for card_frame_update to draw */
};

/* the choice a light editor menu waits on: the option on its right or on
 * its left */
enum { CHOICE_NONE, CHOICE_RIGHT, CHOICE_LEFT };

/* the one way a marked run of colors blends */
enum { BLEND_LINEAR };

enum light_editor_mode light_editor_mode;
/* the source the free editor's pick took up last. only code that never
 * runs sets it */
static int light_picked;
/* where the sources are measured from: the player, in the editor, or a
 * model's center, in light_place_point_and_test */
static Float light_point_x;
static Float light_point_y;
static Float light_point_z;
/* the act's light's direction, and light_down, straight down, which
 * light_ry and light_rz turn into it */
static NJS_VECTOR light_world_dir;
static NJS_VECTOR light_down;
/* a Ninja light for each source. each editor pass deletes those of the
 * sources in use, though nothing here creates them */
static NJS_LIGHT source_lights[LIGHT_SOURCES];
/* the act's light as Ninja's own drawing takes it */
static NJS_LIGHT light_main;
/* the sources in use, as the last editor pass counted them */
static int source_count;
#ifdef GCC
/* the statics from light_sources to dat_8c6b9168, in the order the image's
 * bss holds them. light_write_sources and light_read_card_sources copy
 * SOURCES_DATA bytes from light_sources on. light_read_disc_sources reads
 * whole sectors to it. each runs past the sources into the statics after
 * them. the bytes a sources file holds past its sources give the light its
 * turn, color and strengths, light_ry to light_ambient. GCC's bss doesn't
 * keep the statics' order, so under GCC they are the members of one
 * object. each member has its static's name, and a macro below takes that
 * name to the member. SL_10B.BIN's turn, 0x5a0 and 0x5a4 bytes in, is
 * 0xac00 and 0x1000 */
struct sources_file {
    struct light_source light_sources[LIGHT_SOURCES];
    Angle light_ry;
    Angle light_rz;
    Float light_r, light_g, light_b;
    Float light_specular, light_diffuse, light_ambient;
    Float source_range, source_rate, source_r, source_g, source_b;
    int source_list[LIGHT_SOURCES];
    int source_list_count;
    char dat_8c6b9168[4096];
} sources_file;
_Static_assert(__builtin_offsetof(struct sources_file, light_ry) == 0x5a0,
               "the turn starts 0x5a0 into the file");
_Static_assert(__builtin_offsetof(struct sources_file, dat_8c6b9168) == 0x650,
               "dat_8c6b9168 starts 0x650 into the file");
#define light_sources (sources_file.light_sources)
#define light_ry (sources_file.light_ry)
#define light_rz (sources_file.light_rz)
#define light_r (sources_file.light_r)
#define light_g (sources_file.light_g)
#define light_b (sources_file.light_b)
#define light_specular (sources_file.light_specular)
#define light_diffuse (sources_file.light_diffuse)
#define light_ambient (sources_file.light_ambient)
#define source_range (sources_file.source_range)
#define source_rate (sources_file.source_rate)
#define source_r (sources_file.source_r)
#define source_g (sources_file.source_g)
#define source_b (sources_file.source_b)
#define source_list (sources_file.source_list)
#define source_list_count (sources_file.source_list_count)
/* ADV02's Mystic Ruins sets light_ry and light_rz by name, and STG09's
 * mirror ball reads and sets light_ry. the symbols below put those names at
 * their members of sources_file. sources_file has external linkage so that
 * GCC expects code outside card.c to read and write it, as the modules do
 * through those names */
__asm__("\t.global\t_light_ry\n"
        "\t.type\t_light_ry, @object\n"
        "\t.size\t_light_ry, 4\n"
        "\t.set\t_light_ry, %a0\n"
        "\t.global\t_light_rz\n"
        "\t.type\t_light_rz, @object\n"
        "\t.size\t_light_rz, 4\n"
        "\t.set\t_light_rz, %a1\n"
        :
        : "i"(&light_ry), "i"(&light_rz));
#else
/* the act's sources, then its light's turn, color and strengths */
static struct light_source light_sources[LIGHT_SOURCES];
static Angle light_ry;
static Angle light_rz;
static Float light_r;
static Float light_g;
static Float light_b;
static Float light_specular;
static Float light_diffuse;
static Float light_ambient;
/* the free editor's new source: its range, rate and color */
static Float source_range;
static Float source_rate;
static Float source_r;
static Float source_g;
static Float source_b;
/* sources by their index in light_sources, and how many.
 * light_place_point_and_test reads the first */
static int source_list[LIGHT_SOURCES];
static int source_list_count;
/* nothing names it. light_read_disc reads whole sectors. read to
 * light_sources, a one-sector sources file fills 2048 bytes: the 0x650 up
 * to this array, and its first 0x1b0 */
static char dat_8c6b9168[4096];
#endif
/* buInit's work, for one drive of a 128 KB card */
static char card_library_work[BUM_WORK_SIZE(BUD_CAPACITY_128KB, 1)];
/* each drive's record, which card_complete and card_progress keep */
static struct card_drive card_drives[CARD_DRIVES];
/* where the saves' or a light file's image is made, and where a file
 * loaded from the card lands */
static char card_buffer[16384];
/* the light files' icon: one array, all zero, for its palette and its
 * pixels */
static char light_icon[512];
/* save_drive's status, as the last card_frame_update found it. nothing
 * reads it */
static int card_status;
/* the blocks and time of the saves' file image save_commit makes, which
 * save_send writes */
static int save_blocks;
static BUS_TIME save_time;
/* the light editor's row, and the choice its menu waits on */
static int light_editor_row;
static int light_editor_choice;
/* the palette editor's waits, in passes, before the cursor or a color
 * moves again */
static int palette_move_wait;
static int palette_change_wait;
/* TRUE while start is held to mark a run, as the last pass saw it */
static Bool palette_marking;

/* the ball the free editor draws at the player, and at each source within
 * SHOWN_RANGE of the player */
static NJS_MATERIAL source_materials[1] = {
#include "save/card/source_materials.inc"
};

static Sint16 source_meshes[310] = {
#include "save/card/source_meshes.inc"
};

static NJS_MESHSET source_meshsets[1] = {
#include "save/card/source_meshsets.inc"
};

static NJS_POINT3 source_points[112] = {
#include "save/card/source_points.inc"
};

static NJS_VECTOR source_normals[112] = {
#include "save/card/source_normals.inc"
};

static NJS_MODEL source_model = {
#include "save/card/source_model.inc"
};

static NJS_OBJECT source_object = {
#include "save/card/source_object.inc"
};

/* the saves' file's icon: two frames, 32 by 32, of 4-bit pixels */
static Uint8 save_icon[1024] = {
#include "save/card/save_icon.inc"
};

/* the save icon's palette, 16 colors in argb4444 */
static Uint16 save_palette[16] = {
#include "save/card/save_palette.inc"
};

/* the icons of the game record, the event and the cart record */
static Uint8 record_icon[1024] = {
#include "save/card/record_icon.inc"
};

static Uint8 event_icon[1024] = {
#include "save/card/event_icon.inc"
};

static Uint8 cart_icon[1536] = {
#include "save/card/cart_icon.inc"
};

/* the palette the record, event and cart icons share */
static Uint16 record_palette[16] = {
#include "save/card/record_palette.inc"
};

static Uint8 chao_icon[1024] = {
#include "save/card/chao_icon.inc"
};

static Uint16 chao_palette[16] = {
#include "save/card/chao_palette.inc"
};

/* each kind's icon: its frames, pixels and palette. the saves' file takes
 * SAVE_FILE_NET's */
static int card_icon_frames[SAVE_FILE_KINDS] = {2, 1, 1, 1, 1};
static Uint8 *card_icons[SAVE_FILE_KINDS] = {save_icon, chao_icon, record_icon,
                                             cart_icon, event_icon};
static Uint16 *card_palettes[SAVE_FILE_KINDS] = {
    save_palette, chao_palette, record_palette, record_palette, record_palette};

int save_slot = 0;
int save_drive = 0;
Bool save_disabled = TRUE;
int card_saving = 0;
/* the missing-card notice's state, and TRUE while it may go up */
static enum card_notice card_notice = NOTICE_NONE;
static Bool card_notice_on = TRUE;
static char save_file_name[16] = "SONICADV_SYS";
/* the palette editor's palette, its cursor's entry, and its column.
 * column 0 holds the base colors, and 1 the offset colors, which the
 * editor labels SPEC */
static int palette_bank = 0;
static int palette_entry = 0;
static int palette_column = 0;
static int palette_blend_mode = BLEND_LINEAR;
/* the entry a marked run starts at, which follows the cursor while start
 * is up */
static int palette_mark = 0;
/* TRUE while a held direction keeps moving the cursor, or a color: each
 * move then waits REPEAT_WAIT passes rather than FIRST_WAIT */
static Bool palette_moving = FALSE;
static Bool palette_changing = FALSE;

static void card_init_done(void);
static int save_send(void);
static void light_editor_step(void);
static void palette_editor_step(void);
static Uint32 palette_blend(int mode, int from, int to, Float share);
static void palette_editor_draw(void);
static void palette_curve(int column, int shift, const Uint32 *colors);
static void palette_box(Float x0, Float y0, Float x1, Float y1,
                        const Uint32 *colors);
static void palette_line(Float x0, Float y0, Float x1, Float y1,
                         const Uint32 *colors);

void card_init(void)
{
    memset(card_drives, 0, sizeof(card_drives));
    buInit(BUD_CAPACITY_128KB, 1 << save_drive, card_library_work,
           card_init_done);
    card_saving = 0;
    card_notice = NOTICE_NONE;
    card_notice_on = TRUE;
}

void card_finish(void)
{
    while (buStat(save_drive) == DRIVE_BUSY) {
    }
    card_notice_on = FALSE;
}

int card_notice_busy(void)
{
    if (!card_notice_on) {
        return 0;
    }
    if (card_notice > NOTICE_CLEAR && card_notice <= NOTICE_SHOWN) {
        return -1;
    }
    return 0;
}

/* while the notice waits, draw_off is set, so the screen keeps the frame
 * that drew the notice */
void card_frame_update(void)
{
    struct card_drive *drive;
    Uint32 op;
    int count;
    int max;
    Float lit;

    drive = &card_drives[save_drive];
    if (card_notice != NOTICE_NONE && card_notice_on) {
        switch (card_notice) {
        case NOTICE_SHOW:
            frame_step = 0;
            text_set_color(0xf0f08080);
            text_print_front(NJM_LOCATION(12, 5), TEXT_18);
            text_print_front(NJM_LOCATION(12, 6), TEXT_19);
            draw_off = FALSE;
            card_notice--;
            break;
        case NOTICE_SHOWN:
            card_notice--;
            break;
        case NOTICE_WAIT:
            draw_off = TRUE;
            if ((per_info[0]->on & PDD_DGT_TB) ||
                (per_info[0]->on & PDD_DGT_TX)) {
                card_notice = NOTICE_CLEAR;
                draw_off = FALSE;
            } else if (drive->mounted == 1) {
                save_send();
                card_notice = NOTICE_CLEAR;
                draw_off = FALSE;
            }
            break;
        case NOTICE_CLEAR:
            card_notice = NOTICE_NONE;
            break;
        default:
            break;
        }
    } else {
        card_status = buStat(save_drive);
        card_saving--;
        if (card_saving < 0) {
            card_saving = 0;
        } else {
            /* @hack redundant: an empty else, which keeps saving's template
             * address in R3 and its copy in the frame in R2 (0x8c010d8c,
             * 0x8c010d8e); without it, the two trade and 18 of 320 words
             * differ */
        }
        op = drive->op;
        count = drive->count;
        max = drive->max;
        if (op == BUD_OP_SAVEFILE) {
            char saving[SAVING_LETTERS + 1] = {
#include "save/card/saving.inc"
            };
            char letter[16];
            int step;
            int i;

            lit = (Float)count / (Float)max * (Float)SAVING_LETTERS;
            for (i = 0; i < SAVING_LETTERS; i++) {
                if (lit > (Float)i) {
                    text_set_color(0xf060a0f0);
                } else {
                    text_set_color(0xf0f0f0f0);
                }
                letter[0] = saving[i];
                letter[1] = 0;
                /* text draws only on a frame's first step */
                step = frame_step;
                frame_step = 0;
                text_print_front(NJM_LOCATION(i + 27, 3), letter);
                frame_step = step;
            }
            card_saving = SAVING_HOLD;
        }
    }
}

Sint32 card_complete(Sint32 drive, Sint32 op, Sint32 status, Uint32 param)
{
    struct card_drive *record;
    Sint32 result;

    record = &card_drives[drive];
    switch (op) {
    case BUD_OP_MOUNT:
        if (status == BUD_ERR_OK) {
            record->mounted = 1;
            result = buGetDiskInfo(drive, &record->info);
            if (result == BUD_ERR_OK) {
                record->info_read = 1;
            }
            record->error = 0;
        }
        break;
    case BUD_OP_UNMOUNT:
        record->mounted = 0;
        record->info_read = 0;
        record->count = 0;
        record->max = 0;
        record->op = 0;
        memset(&record->info, 0, sizeof(record->info));
        break;
    default:
        record->error = status;
        buGetDiskInfo(drive, &record->info);
        break;
    }
    record->op = 0;
    return BUD_CBRET_OK;
}

Sint32 card_progress(Sint32 drive, Sint32 op, Sint32 count, Sint32 max)
{
    struct card_drive *record;

    record = &card_drives[drive];
    record->count = count;
    record->max = max;
    record->op = op;
    return BUD_CBRET_OK;
}

/* frame_vsync runs the library's server itself, so this removes the
 * handler buInit adds for it */
static void card_init_done(void)
{
    buDisableServer();
    buSetCompleteCallback(card_complete);
    buSetProgressCallback(card_progress);
}

int card_free_blocks(void)
{
    BUS_DISKINFO info;

    buGetDiskInfo(save_drive, &info);
    return info.free_blocks;
}

static void card_get_time(BUS_TIME *time)
{
    SYS_RTC_DATE date;

    syRtcGetDate(&date);
    time->year = date.year;
    time->month = date.month;
    time->day = date.day;
    time->hour = date.hour;
    time->minute = date.minute;
    time->second = date.second;
    time->dayofweek = date.dayofweek;
}

/* card_find on drive: 0 if it holds a file named name, -1 with saving
 * disabled, and otherwise the last strcmp result */
static int card_find_file(int drive, char *name)
{
    Sint32 found;
    int differs;
    char file[16];

    if (save_disabled) {
        return -1;
    }
    found = buFindFirstFile(drive, file);
    /* @bug uninitialized: the first compare reads file even when the first
     * find fails and writes no name, as on a card with no files. it then
     * compares whatever the stack held with name: no test of found comes
     * between buFindFirstFile's JSR at 0x8c0110b8 and strcmp's at 0x8c0110c4 */
    differs = strcmp(file, name);
    while (found >= 0 && differs != 0) {
        found = buFindNextFile(drive, file);
        /* @hack type: file as (char *)&file, whose address the loop keeps
         * in R11 for the compares (0x8c0110ce) while each find forms its own
         * in R5 (0x8c0110da); as file, 40 of 59 words differ */
        differs = strcmp((char *)&file, name);
    }
    return differs;
}

/* digit's value as a hex digit, its index in card_hex_digits, or -1 */
static int card_hex_value(char digit)
{
    int i;
    int value;

    value = -1;
    for (i = 0; i < 16; i++) {
        if (digit == card_hex_digits[i]) {
            value = i;
            break;
        }
    }
    return value;
}

/* a download's name: SONICADV_, then its number in three hex digits */
#define DOWNLOAD_NAME(file)                                                    \
    ((file)[0] == 'S' && (file)[1] == 'O' && (file)[2] == 'N' &&               \
     (file)[3] == 'I' && (file)[4] == 'C' && (file)[5] == 'A' &&               \
     (file)[6] == 'D' && (file)[7] == 'V' && (file)[8] == '_' &&               \
     card_hex_value((file)[9]) >= 0 && card_hex_value((file)[10]) >= 0 &&      \
     card_hex_value((file)[11]) >= 0)
#define DOWNLOAD_NUMBER(file)                                                  \
    ((card_hex_value((file)[9]) << 8) + (card_hex_value((file)[10]) << 4) +    \
     card_hex_value((file)[11]))

int card_newest_download(int drive)
{
    Sint32 found;
    int newest;
    int number;
    char file[16];

    newest = -1;
    if (save_disabled) {
        return -1;
    }
    found = buFindFirstFile(drive, file);
    /* @bug uninitialized: the first test reads file even when the first find
     * fails and writes no name, as on a card with no files. it then tests
     * whatever the stack held: no test of found comes between
     * buFindFirstFile's JSR at 0x8c011188 and the read of file's first byte
     * at 0x8c01118e */
    if (DOWNLOAD_NAME(file)) {
        number = DOWNLOAD_NUMBER(file);
        if (number > newest) {
            newest = number;
        }
    }
    while (found >= 0) {
        found = buFindNextFile(drive, file);
        if (DOWNLOAD_NAME(file)) {
            number = DOWNLOAD_NUMBER(file);
            if (number > newest) {
                newest = number;
            }
        }
    }
    return newest;
}

Bool save_file_exists(void)
{
    Bool exists;
    char name[16] = "SONICADV_SYS";

    if (save_disabled) {
        return FALSE;
    }
    card_free_blocks();
    exists = FALSE;
    if (card_find_file(save_drive, name) == 0) {
        exists = TRUE;
    }
    return exists;
}

/* build the saves' file image in card_buffer, and start writing it to
 * save_drive. it returns 0 once the card takes the write, or -1 if the
 * header or the request fails. with no card mounted, it raises the
 * missing-card notice and returns 4 */
static int save_commit(void)
{
    struct card_drive *drive;
    char *image;
    void *data;
    BUS_BACKUPFILEHEADER header;
    Sint32 size;

    image = card_buffer;
    drive = &card_drives[save_drive];
    card_free_blocks();
    data = saves;
    card_get_time(&save_time);
    strcpy(header.vms_comment, TEXT_1);
    strcpy(header.btr_comment, TEXT_2);
    /* the game's name and 001 */
    strcpy((char *)header.game_name, TEXT_15);
    header.icon_palette = card_palettes[0];
    header.icon_data = card_icons[0];
    header.icon_num = card_icon_frames[0];
    header.icon_speed = 20;
    header.visual_data = NULL;
    header.visual_type = BUD_VISUALTYPE_NONE;
    header.save_data = data;
    header.save_size = SAVES_DATA;
    size =
        buCalcBackupFileSize(header.icon_num, header.visual_type, SAVES_DATA);
    if (size == BAD_HEADER) {
        return -1;
    }
    save_blocks = buMakeBackupFileImage(image, &header);
    if (drive->mounted == 0) {
        card_notice = NOTICE_SHOW;
    } else {
        return save_send();
    }
    /* @bug uninitialized: with no card mounted, the function returns no value.
     * R0 keeps the NOTICE_SHOW that MOV #4,R0 at 0x8c0114d8 loads for
     * card_notice's store, so it returns 4 */
#ifdef GCC
    /* repairs the missing return the tag names: with no card mounted, GCC
     * returns -1, as for a failed request */
    return -1;
#endif
    /* NOLINTNEXTLINE(clang-diagnostic-return-type) */
}

/* start writing the image save_commit made to save_drive as SONICADV_SYS:
 * 0 once the card takes the write, or -1 */
static int save_send(void)
{
    Sint32 result;
    /* @hack spelling: card_buffer through a local, which loads its address
     * into R6 (0x8c0114fe) before save_blocks into R7 (0x8c011500); passed
     * as card_buffer, 7 of 24 words differ */
    char *image;

    image = card_buffer;
    result = buSaveFile(save_drive, save_file_name, image, save_blocks,
                        &save_time, BUD_FLAG_VERIFY);
    if (result != BUD_ERR_OK) {
        return -1;
    }
    return 0;
}

int save_reset_all_write(void)
{
    save_reset_all();
    if (save_disabled) {
        return -1;
    }
    while (buStat(save_drive) == DRIVE_BUSY) {
    }
    return save_commit();
}

int save_reset_write(void)
{
    save_reset();
    if (save_disabled) {
        return -1;
    }
    while (buStat(save_drive) == DRIVE_BUSY) {
    }
    return save_commit();
}

int save_write(void)
{
    save_store();
    if (save_disabled) {
        return -1;
    }
    while (buStat(save_drive) == DRIVE_BUSY) {
    }
    return save_commit();
}

int save_write_wait(void)
{
    save_store();
    if (save_disabled) {
        return -1;
    }
    while (buStat(save_drive) == DRIVE_BUSY) {
    }
    if (save_commit() != 0) {
        return -1;
    }
    while (buStat(save_drive) == DRIVE_BUSY) {
    }
    return 0;
}

int save_update_write_wait(void)
{
    save_update();
    if (save_disabled) {
        return -1;
    }
    while (buStat(save_drive) == DRIVE_BUSY) {
    }
    if (save_commit() != 0) {
        return -1;
    }
    while (buStat(save_drive) == DRIVE_BUSY) {
    }
    return 0;
}

int save_update_write(void)
{
    save_update();
    if (save_disabled) {
        return -1;
    }
    while (buStat(save_drive) == DRIVE_BUSY) {
    }
    if (save_commit() != 0) {
        return -1;
    }
    return 0;
}

int save_read(void)
{
    Sint32 blocks;
    /* time and header go unused, here and in the readers below, but the
     * image's frames keep room for them */
    BUS_TIME time;
    char name[16] = "SONICADV_SYS";
    BUS_BACKUPFILEHEADER header;
    char *from;
    char *to;
    Uint32 i;
    int differs;

    from = card_buffer;
    if (save_disabled) {
        return -3;
    }
    while (buStat(save_drive) == DRIVE_BUSY) {
    }
    card_free_blocks();
    to = (char *)saves;
    blocks = buGetFileSize(save_drive, name);
    differs = card_find_file(save_drive, name);
    if (differs != 0) {
        return -1;
    }
    buLoadFile(save_drive, name, from, blocks);
    while (buStat(save_drive) == DRIVE_BUSY) {
    }
    from += FILE_HEADER(2);
    if (!save_check_all((struct save *)from)) {
        return -2;
    }
    i = 0;
    while (i < sizeof(saves)) {
        *to = *from;
        to++;
        from++;
        i++;
    }
    save_load();
    return 0;
}

struct save *save_peek(void)
{
    Sint32 blocks;
    BUS_TIME time;
    char name[16] = "SONICADV_SYS";
    BUS_BACKUPFILEHEADER header;
    char *from;
    int differs;

    from = card_buffer;
    while (buStat(save_drive) == DRIVE_BUSY) {
    }
    card_free_blocks();
    blocks = buGetFileSize(save_drive, name);
    differs = card_find_file(save_drive, name);
    if (differs != 0) {
        return NULL;
    }
    buLoadFile(save_drive, name, from, blocks);
    while (buStat(save_drive) == DRIVE_BUSY) {
    }
    from += FILE_HEADER(2);
    return (struct save *)from;
}

int card_find(char *name)
{
    card_free_blocks();
    return card_find_file(save_drive, name);
}

int save_write_file(char *name, int size, int kind)
{
    char *image;
    Sint32 result;
    Sint32 check;
    Sint32 blocks;
    BUS_TIME time;
    BUS_BACKUPFILEHEADER header;

    image = FILE_IMAGE;
    if (save_disabled) {
        return -1;
    }
    card_free_blocks();
    card_get_time(&time);
    switch (kind) {
    case SAVE_FILE_CHAO:
        strcpy(header.vms_comment, TEXT_3);
        strcpy(header.btr_comment, TEXT_4);
        break;
    case SAVE_FILE_RECORD:
        strcpy(header.vms_comment, TEXT_5);
        strcpy(header.btr_comment, TEXT_6);
        break;
    case SAVE_FILE_CART:
        strcpy(header.vms_comment, TEXT_7);
        strcpy(header.btr_comment, TEXT_8);
        break;
    case SAVE_FILE_EVENT:
        strcpy(header.vms_comment, TEXT_9);
        strcpy(header.btr_comment, TEXT_10);
        break;
    default:
        strcpy(header.vms_comment, TEXT_11);
        strcpy(header.btr_comment, TEXT_12);
        break;
    }
    /* the game's name and 000, whose last digit becomes the digit 3 + kind */
    strcpy((char *)header.game_name, TEXT_16);
    header.game_name[15] = kind + '3';
    header.icon_palette = card_palettes[kind];
    header.icon_data = card_icons[kind];
    header.icon_num = card_icon_frames[kind];
    header.icon_speed = 20;
    header.visual_data = NULL;
    header.visual_type = BUD_VISUALTYPE_NONE;
    header.save_data = WORK_AREA;
    header.save_size = size;
    check = buCalcBackupFileSize(header.icon_num, header.visual_type,
                                 header.save_size);
    if (check == BAD_HEADER) {
        return -1;
    }
    blocks = buMakeBackupFileImage(image, &header);
    result = buSaveFile(save_drive, name, image, blocks, &time,
                        BUD_FLAG_VERIFY | BUD_FLAG_COPYDISABLE);
    if (result != BUD_ERR_OK) {
        return -1;
    }
    if (kind != SAVE_FILE_EVENT) {
        while (buStat(save_drive) == DRIVE_BUSY) {
        }
    }
    return 0;
}

int save_read_file(char *name)
{
    char *image;
    BUS_TIME time;
    BUS_BACKUPFILEHEADER header;
    char *from;
    char *to;
    Sint32 blocks;
    int size;
    int i;
    int differs;

    image = FILE_IMAGE;
    if (save_disabled) {
        return -1;
    }
    while (buStat(save_drive) == DRIVE_BUSY) {
    }
    card_free_blocks();
    to = (char *)WORK_AREA;
    blocks = buGetFileSize(save_drive, name);
    differs = card_find_file(save_drive, name);
    if (differs != 0) {
        return -1;
    }
    buLoadFile(save_drive, name, image, blocks);
    while (buStat(save_drive) == DRIVE_BUSY) {
    }
    from = image + FILE_HEADER(1);
    size = (blocks * BLOCK_SIZE) - FILE_HEADER(1);
    for (i = 0; i < size; i++) {
        *to = *from;
        to++;
        from++;
    }
    return size;
}

int card_write_work(char *name, int size)
{
    char *from;
    BUS_TIME time;
    Sint32 result;
    Sint32 blocks;

    from = (char *)WORK_AREA;
    if (save_disabled) {
        return -1;
    }
    card_free_blocks();
    card_get_time(&time);
    blocks = (size + BLOCK_SIZE - 1) / BLOCK_SIZE;
    result = buSaveFile(save_drive, name, from, blocks, &time, BUD_FLAG_VERIFY);
    if (result != BUD_ERR_OK) {
        return -1;
    }
    while (buStat(save_drive) == DRIVE_BUSY) {
    }
    return 0;
}

int light_write_palettes(int stage, int act)
{
    static const struct card_name file = {"SONICADV_P00"};
    /* @hack redundant: register on name_text, which keeps it in R14 and
     * strcpy's address in R13 (MOV R15,R14 at 0x8c011d30, JSR @R13 at
     * 0x8c011df0); without it, the two trade and 14 of 161 words differ */
    register char *name_text;
    char *data;
    char *image;
    BUS_TIME time;
    BUS_BACKUPFILEHEADER header;
    struct card_name name;
    /* @hack redundant: register on number, which keeps image in R12 and
     * strcpy's address in R13 (0x8c011d8c, 0x8c011df0); without it, the two
     * trade and 10 of 161 words differ */
    register int number;
    /* @hack redundant: register on digit, which works out the first digit
     * in R4 (0x8c011d7e to 0x8c011d9e) and keeps image in R12; without it,
     * the digits take an instruction fewer, image and strcpy's address
     * trade, and 31 of 161 words differ */
    register int digit;
    Sint32 blocks;
    Sint32 result;
    Sint32 check;
    char *place;

    /* @hack spelling: name_text set before the template is copied into name,
     * which forms it in R14 among the copy's first instructions (MOV
     * R15,R14 at 0x8c011d30); set after, 47 of 161 words differ */
    name_text = (char *)&name;
    name = file;
    image = card_buffer;
    data = (char *)draw_palettes;
    number = ((Uint16)stage << 3) | (Uint16)act;
    digit = (number >> 5) & 31;
    place = name_text + 10;
    if (digit <= 9) {
        digit += '0';
    } else {
        digit += 'A' - 10;
    }
    *place = digit;
    digit = number & 31;
    place = name_text + 11;
    if (digit <= 9) {
        digit += '0';
    } else {
        digit += 'A' - 10;
    }
    *place = digit;
    card_get_time(&time);
    strcpy(header.vms_comment, "LANTERN_PALETTE");
    strcpy(header.btr_comment, TEXT_13);
    strcpy((char *)header.game_name, TEXT_17);
    header.icon_palette = (Uint16 *)light_icon;
    header.icon_data = (Uint8 *)light_icon;
    header.icon_num = 1;
    header.icon_speed = 1;
    header.visual_data = NULL;
    header.visual_type = BUD_VISUALTYPE_NONE;
    header.save_data = data;
    header.save_size = PALETTES_DATA;
    check = buCalcBackupFileSize(header.icon_num, header.visual_type,
                                 PALETTES_DATA);
    if (check == BAD_HEADER) {
        return -1;
    }
    blocks = buMakeBackupFileImage(image, &header);
    result = buSaveFile(save_drive, name_text, image, blocks, &time,
                        BUD_FLAG_VERIFY);
    if (result != BUD_ERR_OK) {
        return -1;
    }
    while (buStat(save_drive) == DRIVE_BUSY) {
    }
    return 0;
}

/* load the act's palettes from save_drive's card into draw_palettes: 0, or
 * -1 without the file */
static int light_read_card_palettes(int stage, int act)
{
    BUS_TIME time;
    char name[16] = "SONICADV_P00";
    BUS_BACKUPFILEHEADER header;
    /* @hack redundant: register on from, which keeps it in R14 and name's
     * address in R12 (0x8c011f36, MOV R15,R12 at 0x8c011ef2); without it,
     * from takes R12, name's address R11, and 33 of 126 words differ */
    register char *from;
    /* @hack redundant: register on to, which keeps it in R13 (0x8c011f4a)
     * and name's address in R12; without it, to takes R12, name's address
     * R11, and 27 of 126 words differ */
    register char *to;
    int number;
    /* @hack redundant: register on digit, which works out each digit in R4
     * (0x8c011f40 to 0x8c011f5e); without it, the digits go through R0 and
     * R5, from and to load in the other order, and 41 of 126 words
     * differ */
    register int digit;
    int i;
    int differs;
    char *place;

    from = card_buffer;
    to = (char *)draw_palettes;
    /* @hack spelling: the stage and act in two statements, which pack them in
     * R6 (EXTU.W R4,R6 at 0x8c011f2e to OR R5,R6 at 0x8c011f3e); as one
     * statement, R4 and R5 take them and 12 of 126 words differ */
    number = (Uint16)stage << 3;
    number |= (Uint16)act;
    digit = (number >> 5) & 31;
    place = name + 10;
    if (digit <= 9) {
        digit += '0';
    } else {
        digit += 'A' - 10;
    }
    *place = digit;
    digit = number & 31;
    place = name + 11;
    if (digit <= 9) {
        digit += '0';
    } else {
        digit += 'A' - 10;
    }
    *place = digit;
    differs = card_find_file(save_drive, name);
    if (differs != 0) {
        /* @bug logic: the retry stores '0' at name[4], where the disc
         * readers keep the act's digit, but here name[4] is the C of
         * SONICADV (MOV.B R0,@(4,R12) at 0x8c011f88). it looks for
         * SONI0ADV_P and the act's digits, which light_write_palettes never
         * writes */
        name[4] = '0';
        differs = card_find_file(save_drive, name);
        if (differs != 0) {
            return -1;
        }
    }
    buLoadFile(save_drive, name, from, PALETTES_BLOCKS);
    while (buStat(save_drive) == DRIVE_BUSY) {
    }
    from += FILE_HEADER(1);
    for (i = 0; i < PALETTES_DATA; i++) {
        *to = *from;
        to++;
        from++;
    }
    return 0;
}

int light_write_sources(int stage, int act)
{
    static const struct card_name file = {"SONICADV_S00"};
    /* @hack redundant: register on name_text, which keeps it in R14 and
     * strcpy's address in R13 (MOV R15,R14 at 0x8c012014, JSR @R13 at
     * 0x8c0120c8); without it, the two trade and 14 of 162 words differ */
    register char *name_text;
    char *data;
    char *image;
    BUS_TIME time;
    BUS_BACKUPFILEHEADER header;
    struct card_name name;
    /* @hack redundant: register on number, which keeps image in R12 and
     * strcpy's address in R13 (0x8c012070, 0x8c0120c8); without it, the two
     * trade and 10 of 162 words differ */
    register int number;
    /* @hack redundant: register on digit, which works out the first digit
     * in R4 (0x8c012062 to 0x8c0120a2) and keeps image in R12; without it,
     * the digits take an instruction fewer, the function runs 2 bytes short
     * and 100 of 162 words differ */
    register int digit;
    Sint32 blocks;
    Sint32 result;
    Sint32 check;
    char *place;

    /* @hack spelling: name_text set before the template is copied into name,
     * which forms it in R14 among the copy's first instructions (MOV
     * R15,R14 at 0x8c012014); set after, 47 of 162 words differ */
    name_text = (char *)&name;
    name = file;
    image = card_buffer;
#ifdef GCC
    /* the file's SOURCES_DATA bytes run past light_sources into the members
     * of sources_file after it, so GCC's data points at the whole object */
    data = (char *)&sources_file;
#else
    data = (char *)light_sources;
#endif
    number = ((Uint16)stage << 3) | (Uint16)act;
    digit = (number >> 5) & 31;
    place = name_text + 10;
    if (digit <= 9) {
        digit += '0';
    } else {
        digit += 'A' - 10;
    }
    *place = digit;
    digit = number & 31;
    place = name_text + 11;
    if (digit <= 9) {
        digit += '0';
    } else {
        digit += 'A' - 10;
    }
    *place = digit;
    card_get_time(&time);
    strcpy(header.vms_comment, "LANTERN_SOURCE");
    strcpy(header.btr_comment, TEXT_14);
    strcpy((char *)header.game_name, TEXT_17);
    header.icon_palette = (Uint16 *)light_icon;
    header.icon_data = (Uint8 *)light_icon;
    header.icon_num = 1;
    header.icon_speed = 1;
    header.visual_data = NULL;
    header.visual_type = BUD_VISUALTYPE_NONE;
    header.save_data = data;
    /* @bug memory: the file takes 0x600 bytes from light_sources,
     * 1440 long, and with them the 96 after it, light_ry to source_b and
     * source_list's first 11 entries (MOV.W at 0x8c0120e0, the copy in
     * buMakeBackupFileImage, JSR at 0x8c012126) */
    header.save_size = SOURCES_DATA;
    check =
        buCalcBackupFileSize(header.icon_num, header.visual_type, SOURCES_DATA);
    if (check == BAD_HEADER) {
        return -1;
    }
    blocks = buMakeBackupFileImage(image, &header);
    result = buSaveFile(save_drive, name_text, image, blocks, &time,
                        BUD_FLAG_VERIFY);
    if (result != BUD_ERR_OK) {
        return -1;
    }
    while (buStat(save_drive) == DRIVE_BUSY) {
    }
    return 0;
}

/* load the act's sources from save_drive's card into light_sources and the
 * 96 bytes past it: 0, or -1 without the file */
static int light_read_card_sources(int stage, int act)
{
    BUS_TIME time;
    char name[16] = "SONICADV_S00";
    BUS_BACKUPFILEHEADER header;
    /* @hack redundant: register on from, which keeps it in R14 and name's
     * address in R12 (0x8c01220e, MOV R15,R12 at 0x8c0121ca); without it,
     * from takes R12, name's address R11, and 33 of 128 words differ */
    register char *from;
    /* @hack redundant: register on to, which keeps it in R13 (0x8c012222)
     * and name's address in R12; without it, to takes R12, name's address
     * R11, and 27 of 128 words differ */
    register char *to;
    int number;
    /* @hack redundant: register on digit, which works out each digit in R4
     * (0x8c012218 to 0x8c012236); without it, the digits go through R0 and
     * R5, from and to load in the other order, and 43 of 128 words
     * differ */
    register int digit;
    int i;
    int differs;
    char *place;

    from = card_buffer;
#ifdef GCC
    /* the copy's SOURCES_DATA bytes run past light_sources into the members
     * of sources_file after it, so GCC's to points at the whole object */
    to = (char *)&sources_file;
#else
    to = (char *)light_sources;
#endif
    /* @hack spelling: the stage and act in two statements, which pack them in
     * R6 (EXTU.W R4,R6 at 0x8c012206 to OR R5,R6 at 0x8c012216); as one
     * statement, R4 and R5 take them and 12 of 128 words differ */
    number = (Uint16)stage << 3;
    number |= (Uint16)act;
    digit = (number >> 5) & 31;
    place = name + 10;
    if (digit <= 9) {
        digit += '0';
    } else {
        digit += 'A' - 10;
    }
    *place = digit;
    digit = number & 31;
    place = name + 11;
    if (digit <= 9) {
        digit += '0';
    } else {
        digit += 'A' - 10;
    }
    *place = digit;
    differs = card_find_file(save_drive, name);
    if (differs != 0) {
        /* @bug logic: the retry stores '0' in the C of SONICADV, as
         * light_read_card_palettes does (MOV.B R0,@(4,R12) at 0x8c012264),
         * and looks for SONI0ADV_S and the act's digits, which
         * light_write_sources never writes */
        name[4] = '0';
        differs = card_find_file(save_drive, name);
        if (differs != 0) {
            return -1;
        }
    }
    buLoadFile(save_drive, name, from, SOURCES_BLOCKS);
    while (buStat(save_drive) == DRIVE_BUSY) {
    }
    from += FILE_HEADER(1);
    /* @bug memory: the copy writes 0x600 bytes from light_sources on
     * (0x8c0122b8, the loop at 0x8c0122bc to 0x8c0122d0), and with them the
     * 96 after it, light_ry to source_b and source_list's first 11 entries,
     * which take what the save took from there */
    for (i = 0; i < SOURCES_DATA; i++) {
        *to = *from;
        to++;
        from++;
    }
    return 0;
}

int card_read_xzxz(void)
{
    char *to;
    char name[8] = {'X', 'Z', 'X', 'Z', 'X', 'Z', 'X', 'Z'};
    BUS_BACKUPFILEHEADER header;

    to = card_buffer;
    card_find_file(save_drive, name);
    buLoadFile(save_drive, name, to, 1);
    while (buStat(save_drive) == DRIVE_BUSY) {
    }
    return 0;
}

/* load the file name from save_drive's card to WORK_AREA whole, header
 * included: its size in bytes, or -1 if the load fails */
static int card_read_work(char *name)
{
    char *to;
    BUS_TIME time;
    BUS_BACKUPFILEHEADER header;
    Sint32 blocks;
    Sint32 result;

    to = (char *)WORK_AREA;
    blocks = buGetFileSize(save_drive, name);
    result = buLoadFile(save_drive, name, to, blocks);
    if (result != BUD_ERR_OK) {
        return -1;
    }
    while (buStat(save_drive) == DRIVE_BUSY) {
    }
    return blocks * BLOCK_SIZE;
}

void light_load_act(void)
{
    NJS_VECTOR dir;
    int stage;
    int act;
    int palettes;
    int sources;
    int i;
    int j;
    int bank;
    Uint16 stage_act;
    /* @hack spelling: light_ry through ry, which keeps its address in R8 from
     * 0x8c012452 to the turn's reads (MOV.L @R8 at 0x8c0125ce and
     * 0x8c0125d6); as light_ry, 102 of 280 words differ */
    Angle *ry;

    light_editor_mode = EDIT_OFF;
    stage_act = stage_get_stage_act();
    act = (Uint8)stage_act;
    /* @hack spelling: the stage masked and shifted in two statements, which
     * keep it in R12 and the act in R11 (0x8c012420 to 0x8c012430); as one
     * statement, the two trade and 12 of 280 words differ */
    stage = stage_act & STAGE_MASK;
    stage >>= 8;
    palettes = -1;
    sources = -1;
    pdGetPeripheral(PDD_PORT_A0);
    if (buIsReady(save_drive) != 0) {
        sources = light_read_card_sources(stage, act);
        palettes = light_read_card_palettes(stage, act);
    }
    ry = &light_ry;
    if (sources != 0 || palettes != 0) {
        for (i = 0; i < LIGHT_SOURCES; i++) {
            light_sources[i].used = FALSE;
        }
        source_list_count = 0;
        source_range = 20.0F;
        source_rate = 1.0F;
        source_r = source_g = source_b = 1.0F;
        *ry = 0;
        light_rz = 0;
        light_r = light_g = light_b = 1.0F;
        light_specular = 0.8F;
        light_diffuse = 1.0F;
        light_ambient = 0.25F;
        /* each entry's base color white, and its offset color black */
        for (bank = 0; bank < EDIT_PALETTES; bank++) {
            for (j = 0; j < DRAW_PALETTE_ENTRIES * 2; j += 2) {
                draw_palettes[bank][j] = 0xffffffff;
                draw_palettes[bank][j + 1] = 0xff000000;
            }
        }
        light_read_disc_sources(stage, act);
        light_read_disc_palettes(stage, act);
    }
    light_down.x = 0.0F;
    light_down.y = -1.0F;
    light_down.z = 0.0F;
    njPushMatrix(NULL);
    njUnitMatrix(NULL);
    if (*ry != 0) {
        njRotateY(NULL, *ry);
    }
    /* @hack type: light_rz tested as a Uint32, which reads it again for
     * the turn (MOV.L @R2 at 0x8c0125de and 0x8c0125e6); tested as an
     * Angle, 108 of 280 words differ */
    if (*(Uint32 *)&light_rz != 0) {
        njRotateZ(NULL, light_rz);
    } else {
        /* @hack redundant: an empty else, which has the calls after the turn
         * load their addresses into R3, R2, R3, R2 and R3 (0x8c0125ec to
         * 0x8c01260c); without it, R2 and R3 trade and 10 of 280 words
         * differ */
    }
    njCalcVector(NULL, &light_down, &light_world_dir);
    njPopMatrix(1);
    njPushMatrix(NULL);
    njCalcVector(NULL, &light_world_dir, &dir);
    njPopMatrix(1);
    draw_set_light(dir.x, dir.y, dir.z);
    njSetEasyLight(dir.x, dir.y, dir.z);
    njSetEasyLightIntensity(light_diffuse, (light_ambient + 0.1F) * 1.44F);
    njSetEasyLightColor(light_r, light_g, light_b);
    njCreateLight(&light_main, NJD_PHONG_DIR);
    njSetLightIntensity(&light_main, light_specular, light_diffuse,
                        light_ambient);
    njSetLightColor(&light_main, light_r, light_g, light_b);
    njSetLightAngle(&light_main, LIGHT_INNER, LIGHT_OUTER);
    njSetLightDirection(&light_main, light_world_dir.x, light_world_dir.y,
                        light_world_dir.z);
}

/* @hack type: a source's offset cut to a Sint16, which keeps the image's
 * EXTS.W before each source's address in light_editor_step's unreached
 * presses (0x8c012dc4, 0x8c012de4); subscripted, 96 of 961 words differ */
#define SOURCE(sources, i)                                                     \
    ((struct light_source *)((char *)(sources) +                               \
                             (Sint16)((i) * sizeof(struct light_source))))

/* a pass of the light editor. it makes the six palettes opaque, deletes
 * the Ninja lights of the sources in use, and shows the mode's heading and
 * settings. it reads no pad, so each setting moves by 0. the code that took
 * presses stands in if (0) blocks, which never run */
static void light_editor_step(void)
{
    int i;
    int j;
    Uint16 stage_act;
    NJS_POINT3 pos;
    BUS_DISKINFO info;
    char file[16];
    Float step;
    Sint16 act;
    Sint16 stage;
    int row;
    int rows;
    Sint32 found;
    int size;
    int best;
    Float least;
    Float distance;
    int turn;
    struct light_source *source;
    char *kind;
    char *letter;
    char *form;

    for (i = 0; i < EDIT_PALETTES; i++) {
        for (j = 0; j < DRAW_PALETTE_ENTRIES * 2; j++) {
            draw_palettes[i][j] |= 0xff000000;
        }
    }
    light_point_x = player_work[0]->pos.x;
    light_point_y = player_work[0]->pos.y;
    light_point_z = player_work[0]->pos.z;
    stage_act = stage_get_stage_act();
    act = (Uint8)stage_act;
    stage = STAGE_OF(stage_act);
    source_count = 0;
    for (i = 0; i < LIGHT_SOURCES; i++) {
        if (light_sources[i].used) {
            source_count++;
            njDeleteLight(&source_lights[i]);
        }
    }
    turn = 0;
    step = 0.0F;
    if (light_editor_mode == EDIT_OFF) {
        return;
    }
    if (light_editor_mode != EDIT_PALETTE) {
        njPrintC(NJM_LOCATION(7, 27), "   /   ");
        njPrintH(NJM_LOCATION(7, 27), source_count, 3);
        njPrintH(NJM_LOCATION(11, 27), LIGHT_SOURCES, 3);
    }
    /* @hack redundant: a press of start, which turned the editor off, never
     * run: it keeps njDeleteLight's address in R10 and source_lights' in R9
     * (0x8c0127b4, 0x8c0127b8); without it, the two trade and 10 of 1365 words
     * differ */
    if (0) {
        light_editor_mode = EDIT_OFF;
    }
#ifdef GCC
    /* repairs the unset rows the tag after the switch names: the palette,
     * file and host modes have none, so GCC's row goes back to 0 */
    rows = 0;
#endif
    /* NOLINTBEGIN(clang-diagnostic-sometimes-uninitialized) */
    switch (light_editor_mode) {
    case EDIT_PALETTE:
        palette_editor_step();
        break;
    case EDIT_FILE:
        if (light_editor_choice == CHOICE_RIGHT) {
            njPrintC(NJM_LOCATION(7, 26), "[SAVE]   ->SURE");
            /* @hack redundant: a press that wrote the act's sources to the
             * card, never run: it keeps 0x800 in R12 and 6 in R10 (0x8c01275a,
             * 0x8c012760); without it, the two trade and 444 of 1365 words
             * differ */
            if (0) {
                if (buIsReady(save_drive) != 0) {
                    light_write_sources(stage, act);
                }
                light_editor_choice = CHOICE_NONE;
            }
        } else if (light_editor_choice == CHOICE_LEFT) {
            njPrintC(NJM_LOCATION(7, 26), "[LOAD] SURE<-");
            /* @hack redundant: a press of back, which cleared the choice, never
             * run: it keeps njDeleteLight's address in R10 and source_lights'
             * in R9 (0x8c0127b4, 0x8c0127b8); without it, the two trade and 204
             * of 1365 words differ */
            if (0) {
                light_editor_choice = CHOICE_NONE;
            }
            /* @hack redundant: a press that loaded the act's lights again and
             * counted the sources, never run: the image keeps the block's loop,
             * and its registers, 0 in R11, 0x800 in R12 and 6 in R10
             * (0x8c012758 to 0x8c012760); without it, the function runs 192
             * bytes short and 1132 of 1365 words differ */
            if (0) {
                if (buIsReady(save_drive) != 0) {
                    light_load_act();
                }
                light_editor_choice = CHOICE_NONE;
                source_count = 0;
                for (i = 0; i < LIGHT_SOURCES; i++) {
                    if (light_sources[i].used) {
                        source_count++;
                    }
                }
            }
        } else {
            njPrintC(NJM_LOCATION(7, 26), "[FILE] LOAD<- ->SAVE");
        }
        row = 5;
        buGetDiskInfo(save_drive, &info);
        njPrintC(NJM_LOCATION(5, 3), " - SLOT 1 -");
        njPrintC(NJM_LOCATION(5, 4), "FREE:   BLK");
        njPrintD(NJM_LOCATION(10, 4), info.free_blocks, 3);
        for (found = buFindFirstFile(save_drive, file); found >= 0;
             found = buFindNextFile(save_drive, file)) {
            njPrintC(NJM_LOCATION(5, row++), file);
        }
        row = 5;
        buGetDiskInfo(BUD_DRIVE_A2, &info);
        /* @hack redundant: a step of row that never runs, which keeps row's 5
         * in R13 and 0x140000 in R12; without it, the two trade and 5 of 1365
         * words differ */
        if (0) {
            row++;
        }
        njPrintC(NJM_LOCATION(20, 3), " - SLOT 2 -");
        njPrintC(NJM_LOCATION(20, 4), "FREE:   BLK");
        njPrintD(NJM_LOCATION(25, 4), info.free_blocks, 3);
        for (found = buFindFirstFile(BUD_DRIVE_A2, file); found >= 0;
             found = buFindNextFile(BUD_DRIVE_A2, file)) {
            njPrintC(NJM_LOCATION(20, row++), file);
        }
        break;
    case EDIT_VMS_TO_PC:
        if (light_editor_choice == CHOICE_RIGHT) {
            njPrintC(NJM_LOCATION(7, 26), "[VMS->PC] ->SURE");
            /* @hack redundant: a press of back, which cleared the choice, never
             * run: it keeps njDeleteLight's address in R10 and source_lights'
             * in R9 (0x8c0127b4, 0x8c0127b8); without it, the two trade and 80
             * of 1365 words differ */
            if (0) {
                light_editor_choice = CHOICE_NONE;
            }
            /* @hack redundant: a press that copied each file of the card to the
             * host, a light file as a .bin past its header, never run: the
             * image keeps the block's loop; without it, SHC drops
             * card_read_work, the function runs 256 bytes short and 1139 of
             * 1365 words differ */
            if (0) {
                /* a light file's name holds its kind at [9], P or S, an L
                 * at [10] and its form at [11]. the host's copy of form B
                 * drops a header with no icon, and of form C a header with
                 * one, and C becomes B */
                kind = &file[9];
                letter = kind + 1;
                form = letter + 1;
                for (found = buFindFirstFile(save_drive, file); found >= 0;
                     found = buFindNextFile(save_drive, file)) {
                    if ((*kind == 'P' || *kind == 'S') && *letter == 'L') {
                        size = card_read_work(file);
                        for (i = 0; file[i]; i++) {
                        }
                        file[i++] = '.';
                        file[i++] = 'b';
                        file[i++] = 'i';
                        file[i++] = 'n';
                        file[i] = 0;
                        switch (*form) {
                        case 'B':
                            host_save_file(file, WORK_AREA + FILE_HEADER(0),
                                           size - FILE_HEADER(0));
                            break;
                        case 'C':
                            *form = 'B';
                            host_save_file(file, WORK_AREA + FILE_HEADER(1),
                                           size - FILE_HEADER(1));
                            break;
                        default:
                            host_save_file(file, WORK_AREA, size);
                            break;
                        }
                    } else {
                        host_save_file(file, WORK_AREA, card_read_work(file));
                    }
                }
                light_editor_choice = CHOICE_NONE;
            }
            /* @hack redundant: a clear of the choice that never runs; without
             * it, SHC puts the transfer's last store in the branch's delay
             * slot, where the image has a nop, the addresses of njDeleteLight
             * and source_lights trade R10 and R9, pos.x and pos.y trade FR7 and
             * FR6, and 25 of 1365 words differ */
            if (0) {
                light_editor_choice = CHOICE_NONE;
            }
        } else {
            njPrintC(NJM_LOCATION(7, 26), "[VMS->PC] ->PC");
        }
        row = 5;
        buGetDiskInfo(save_drive, &info);
        njPrintC(NJM_LOCATION(5, 3), " - SLOT 1 -");
        njPrintC(NJM_LOCATION(5, 4), "FREE:   BLK");
        njPrintD(NJM_LOCATION(10, 4), info.free_blocks, 3);
        for (found = buFindFirstFile(save_drive, file); found >= 0;
             found = buFindNextFile(save_drive, file)) {
            njPrintC(NJM_LOCATION(5, row++), file);
        }
        break;
    case EDIT_PC_TO_VMS:
        if (light_editor_choice == CHOICE_RIGHT) {
            njPrintC(NJM_LOCATION(7, 26), "[PC->VMS] ->SURE");
            /* @hack redundant: a press that copied the host's file to the card,
             * never run: it keeps the image's 96-byte frame (0x8c012754);
             * without it, the frame takes 100 bytes, the function runs 32 bytes
             * long and 830 of 1365 words differ */
            if (0) {
                card_write_work("SONICADV.VMS",
                                host_load_file("SONICADV.DL0", WORK_AREA));
                light_editor_choice = CHOICE_NONE;
            }
        } else {
            njPrintC(NJM_LOCATION(7, 26), "[PC->VMS] ->VMS");
        }
        break;
    case EDIT_FREE:
        njPrintC(NJM_LOCATION(7, 26), "[FREE]  RD RT R  G  B");
        njPrintH(NJM_LOCATION(15, 27), (Sint32)source_range, 2);
        njPrintH(NJM_LOCATION(18, 27), (Sint32)(source_rate * 255.0F), 2);
        njPrintH(NJM_LOCATION(21, 27), (Sint32)(source_r * 255.0F), 2);
        njPrintH(NJM_LOCATION(24, 27), (Sint32)(source_g * 255.0F), 2);
        njPrintH(NJM_LOCATION(27, 27), (Sint32)(source_b * 255.0F), 2);
        rows = 5;
        source = light_sources;
        /* @hack redundant: a press that added a new source at the player,
         * never run, under a 0 && 1.0F that SHC doesn't fold: it keeps a test
         * of the 0, the branches round the block, and the block's tests of
         * source_count and ->used, all of which if (0) drops; without it, the
         * function runs 160 bytes short and 780 of 1365 words differ, and
         * under a plain 0, 32 bytes short and 689 */
        if (0 && 1.0F) {
            if (source_count < LIGHT_SOURCES) {
                for (i = 0; SOURCE(source, i)->used && i < LIGHT_SOURCES; i++) {
                }
                SOURCE(source, i)->used = TRUE;
                SOURCE(source, i)->range = source_range;
                SOURCE(source, i)->rate = source_rate;
                SOURCE(source, i)->r = source_r;
                SOURCE(source, i)->g = source_g;
                SOURCE(source, i)->b = source_b;
                SOURCE(source, i)->pos.x = light_point_x;
                SOURCE(source, i)->pos.y = light_point_y;
                SOURCE(source, i)->pos.z = light_point_z;
            }
        }
        /* @hack redundant: a press that picked up the nearest source, never
         * run: the image keeps the block's loop, and its registers, 0 in R11
         * and 6 in R10 (0x8c012758, 0x8c012760); without it, the function runs
         * 512 bytes short and 716 of 1365 words differ */
        if (0) {
            pos.x = light_point_x;
            pos.y = light_point_y;
            pos.z = light_point_z;
            best = -1;
            least = 1000000.0F;
            for (i = 0; i < LIGHT_SOURCES; i++) {
                distance = ((pos.x - SOURCE(source, i)->pos.x) *
                            (pos.x - SOURCE(source, i)->pos.x)) +
                           ((pos.y - SOURCE(source, i)->pos.y) *
                            (pos.y - SOURCE(source, i)->pos.y)) +
                           ((pos.z - SOURCE(source, i)->pos.z) *
                            (pos.z - SOURCE(source, i)->pos.z));
                if (least > distance) {
                    least = distance;
                    best = i;
                }
            }
            if (best >= 0) {
                /* @hack redundant: a store of pos.z to least that never runs,
                 * which keeps the loop's pos.y and pos.z in FR7 and FR5;
                 * without it, they take FR5 and FR7 and 10 of 1365 words
                 * differ */
                if (0) {
                    least = pos.z;
                }
                light_picked = best;
                light_sources[best].used = FALSE;
                light_point_x = light_sources[best].pos.x;
                light_point_y = light_sources[best].pos.y;
                light_point_z = light_sources[best].pos.z;
                source_range = light_sources[best].range;
                source_rate = light_sources[best].rate;
                source_r = light_sources[best].r;
                source_g = light_sources[best].g;
                source_b = light_sources[best].b;
                player_work[0]->pos.x = light_point_x;
                player_work[0]->pos.y = light_point_y;
                player_work[0]->pos.z = light_point_z;
            }
        }
        break;
    case EDIT_SLAVE:
        njPrintC(NJM_LOCATION(7, 26), "[SLAVE] RD RT R  G  B");
        njPrintH(NJM_LOCATION(15, 27), (Sint32)source_range, 2);
        njPrintH(NJM_LOCATION(18, 27), (Sint32)(source_rate * 255.0F), 2);
        njPrintH(NJM_LOCATION(21, 27), (Sint32)(source_r * 255.0F), 2);
        njPrintH(NJM_LOCATION(24, 27), (Sint32)(source_g * 255.0F), 2);
        njPrintH(NJM_LOCATION(27, 27), (Sint32)(source_b * 255.0F), 2);
        rows = 4;
        break;
    case EDIT_ENV:
        njPrintC(NJM_LOCATION(7, 26), "[ENV]   RY RZ R  G  B  SP DI AM");
        njPrintH(NJM_LOCATION(15, 27), light_ry / 256, 2);
        njPrintH(NJM_LOCATION(18, 27), light_rz / 256, 2);
        njPrintH(NJM_LOCATION(21, 27), (Sint32)(light_r * 255.0F), 2);
        njPrintH(NJM_LOCATION(24, 27), (Sint32)(light_g * 255.0F), 2);
        njPrintH(NJM_LOCATION(27, 27), (Sint32)(light_b * 255.0F), 2);
        njPrintH(NJM_LOCATION(30, 27), (Sint32)(light_specular * 255.0F), 2);
        njPrintH(NJM_LOCATION(33, 27), (Sint32)(light_diffuse * 255.0F), 2);
        njPrintH(NJM_LOCATION(36, 27), (Sint32)(light_ambient * 255.0F), 2);
        rows = 8;
        break;
    default:
        rows = 3;
        break;
    }
    /* @bug uninitialized: the palette, file and host modes set no rows, so
     * the row's wrap reads whatever R7 holds. they join here past the
     * default's MOV #3,R7 (0x8c013260) straight from a call (BRA at
     * 0x8c0129c6 among them), and CMP/GE R7,R3 at 0x8c013266 and ADD
     * #-1,R7 at 0x8c013272 read what the callee left in R7 */
    if (light_editor_row >= rows) {
        light_editor_row = 0;
    }
    /* NOLINTEND(clang-diagnostic-sometimes-uninitialized) */
    if (light_editor_row < 0) {
        light_editor_row = rows - 1;
    }
    if (light_editor_mode != EDIT_FILE && light_editor_mode != EDIT_VMS_TO_PC &&
        light_editor_mode != EDIT_PALETTE) {
        njPrintC(NJM_LOCATION((light_editor_row * 3) + 15, 25), "__");
    }
    if (light_editor_mode == EDIT_FREE || light_editor_mode == EDIT_SLAVE) {
        switch (light_editor_row) {
        case 0:
            source_range += step;
            if (source_range < 0.0F) {
                source_range = 0.0F;
            }
            break;
        case 1:
            source_rate += 0.0F;
            if (source_rate < 0.0F) {
                source_rate = 0.0F;
            }
            if (source_rate > 1.0F) {
                source_rate = 1.0F;
            }
            break;
        case 2:
            source_r += 0.0F;
            if (source_r < 0.0F) {
                source_r = 0.0F;
            }
            if (source_r > 1.0F) {
                source_r = 1.0F;
            }
            break;
        case 3:
            source_g += 0.0F;
            if (source_g < 0.0F) {
                source_g = 0.0F;
            }
            if (source_g > 1.0F) {
                source_g = 1.0F;
            }
            break;
        case 4:
            source_b += 0.0F;
            if (source_b < 0.0F) {
                source_b = 0.0F;
            }
            if (source_b > 1.0F) {
                source_b = 1.0F;
            }
            break;
        }
    } else if (light_editor_mode == EDIT_ENV) {
        switch (light_editor_row) {
        case 0:
            light_ry = (Uint16)(light_ry + turn);
            break;
        case 1:
            light_rz = (Uint16)(light_rz + turn);
            break;
        case 2:
            light_r += 0.0F;
            if (light_r < 0.0F) {
                light_r = 0.0F;
            }
            if (light_r > 1.0F) {
                light_r = 1.0F;
            }
            break;
        case 3:
            light_g += 0.0F;
            if (light_g < 0.0F) {
                light_g = 0.0F;
            }
            if (light_g > 1.0F) {
                light_g = 1.0F;
            }
            break;
        case 4:
            light_b += 0.0F;
            if (light_b < 0.0F) {
                light_b = 0.0F;
            }
            if (light_b > 1.0F) {
                light_b = 1.0F;
            }
            break;
        case 5:
            light_specular += 0.0F;
            if (light_specular < 0.0F) {
                light_specular = 0.0F;
            }
            if (light_specular > 1.0F) {
                light_specular = 1.0F;
            }
            break;
        case 6:
            light_diffuse += 0.0F;
            if (light_diffuse < 0.0F) {
                light_diffuse = 0.0F;
            }
            if (light_diffuse > 1.0F) {
                light_diffuse = 1.0F;
            }
            break;
        case 7:
            light_ambient += 0.0F;
            if (light_ambient < 0.0F) {
                light_ambient = 0.0F;
            }
            if (light_ambient > 1.0F) {
                light_ambient = 1.0F;
            }
            break;
        }
    }
}

void light_apply(void)
{
    NJS_VECTOR dir;

    njPushMatrix(NULL);
    njUnitMatrix(NULL);
    /* @hack redundant: a jump to the next statement, which SHC drops but which
     * keeps light_ry's address in R2 and its value in R3 (0x8c013532,
     * 0x8c013534), and light_rz's likewise; without it, they trade, so do the
     * calls' addresses after the turn, and 22 of 119 words differ */
    goto turn;
turn:
    /* @hack type: light_ry tested as a Uint32, which reads it again for
     * the turn (MOV.L @R2 at 0x8c013534 and 0x8c01353c); tested as an
     * Angle, 109 of 119 words differ */
    if (*(Uint32 *)&light_ry != 0) {
        njRotateY(NULL, light_ry);
    }
    /* @hack type: light_rz tested as a Uint32, which reads it again for
     * the turn (MOV.L @R2 at 0x8c013544 and 0x8c01354c); tested as an
     * Angle, 101 of 119 words differ */
    if (*(Uint32 *)&light_rz != 0) {
        njRotateZ(NULL, light_rz);
    } else {
        /* @hack redundant: an empty else, which has the calls after the turn
         * load their addresses into R3, R2, R3, R2 and R3 (0x8c013552 to
         * 0x8c013572); without it, R2 and R3 trade and 10 of 119 words
         * differ */
    }
    njCalcVector(NULL, &light_down, &light_world_dir);
    njPopMatrix(1);
    njPushMatrix(NULL);
    njCalcVector(NULL, &light_world_dir, &dir);
    njPopMatrix(1);
    draw_set_light(dir.x, dir.y, dir.z);
    njSetEasyLight(dir.x, dir.y, dir.z);
    njSetEasyLightIntensity(light_diffuse, (light_ambient + 0.1F) * 1.44F);
    njSetEasyLightColor(light_r, light_g, light_b);
    njSetLightIntensity(&light_main, light_specular, light_diffuse,
                        light_ambient);
    njSetLightColor(&light_main, light_r, light_g, light_b);
    njSetLightAngle(&light_main, LIGHT_INNER, LIGHT_OUTER);
    njSetLightDirection(&light_main, light_world_dir.x, light_world_dir.y,
                        light_world_dir.z);
    draw_take_point_lights();
}

int light_place_point_and_test(NJS_MODEL *model)
{
    NJS_POINT3 center;
    Float radius;
    Float reach;
    Float x;
    Float y;
    Float z;
    Float distance;
    int i;
    int touches;

    njCalcPoint(NULL, &model->center, &center);
    light_point_x = center.x;
    light_point_y = center.y;
    light_point_z = center.z;
    radius = model->r;
    touches = 0;
    if (source_list_count > 0) {
        i = source_list[0];
        /* @hack redundant: a jump to the next statement, which SHC drops but
         * which keeps light_sources' address in R2 and light_point_y's in R1
         * (0x8c0136e0, 0x8c0136e4); without it, the two trade and 4 of 73
         * words differ */
        goto measure;
    measure:
        x = light_sources[i].touch_pos.x;
        y = light_sources[i].touch_pos.y;
        z = light_sources[i].touch_pos.z;
        distance = ((light_point_x - x) * (light_point_x - x)) +
                   ((light_point_y - y) * (light_point_y - y)) +
                   ((light_point_z - z) * (light_point_z - z));
        reach = light_sources[i].range + radius;
        reach *= reach;
        if (reach > distance) {
            touches++;
        }
    }
    return touches;
}

void light_editor_draw(void)
{
    int i;
    Float distance;
    Float scale;
    struct light_source *source;

    /* @hack redundant: stores that never run, which keep light_point_y's
     * address in R12 and light_point_z's in R10 (0x8c013780, 0x8c013786);
     * without them, the two trade and 257 of 478 words differ */
    if (0) {
        i = light_picked;
        scale = source_g;
        source_b = scale;
        distance = 0.0F;
        source_rate = distance;
    }
    light_point_x = player_work[0]->pos.x;
    light_point_y = player_work[0]->pos.y;
    light_point_z = player_work[0]->pos.z;
    if (light_editor_mode == EDIT_FREE) {
        njPushMatrix(NULL);
        /* @hack redundant: a store of i to light_picked that never runs, with
         * which the distance loads light_point_x, y and z into FR5, FR6 and FR4
         * (0x8c01389e to 0x8c0138a6); without it, they take FR4, FR5 and FR6
         * and 12 of 478 words differ */
        if (0) {
            light_picked = i;
        }
        njTranslate(NULL, light_point_x, light_point_y, light_point_z);
        njScale(NULL, source_range * 0.2F, source_range * 0.2F,
                source_range * 0.2F);
        njEasyDrawObject(&source_object);
        njPopMatrix(1);
        for (i = 0; i < LIGHT_SOURCES; i++) {
            if (light_sources[i].used) {
                source = &light_sources[i];
                distance = ((light_point_x - source->pos.x) *
                            (light_point_x - source->pos.x)) +
                           ((light_point_y - source->pos.y) *
                            (light_point_y - source->pos.y)) +
                           ((light_point_z - source->pos.z) *
                            (light_point_z - source->pos.z));
                if (distance < SHOWN_RANGE * SHOWN_RANGE) {
                    njPushMatrix(NULL);
                    njTranslate(NULL, light_sources[i].pos.x,
                                light_sources[i].pos.y, light_sources[i].pos.z);
                    scale = light_sources[i].range * 0.2F;
                    njScale(NULL, scale, scale, scale);
                    njEasyDrawObject(&source_object);
                    njPopMatrix(1);
                }
            }
        }
        /* @hack redundant: stores of 0 to distance and source_rate that never
         * run, which keep light_point_y's address in R12 and light_point_z's in
         * R10 (0x8c013780, 0x8c013786); without them, the two trade and 95 of
         * 478 words differ */
        if (0) {
            distance = 0.0F;
            source_rate = distance;
        }
    }
}

void editor_take_buttons(int port, Uint32 buttons)
{
    buttons = ~buttons;
    per_info[port]->press &= buttons;
    pad_records[port].on &= buttons;
    pad_records[port].old &= buttons;
    pad_records[port].press &= buttons;
    per_info[port]->x1 = 0;
    per_info[port]->y1 = 0;
    pad_records[port].x1 = 0;
    pad_records[port].y1 = 0;
    per_info[port]->l = 0;
    per_info[port]->r = 0;
    pad_records[port].l = 0;
    pad_records[port].r = 0;
}

/* a pass of the palette editor. with X held, a press of right steps to the
 * next palette, and of left flips between the base and offset colors.
 * otherwise A, Y and B take red, green and blue, which up and down raise
 * and lower in the cursor's entry. with L held, they change every entry.
 * with no channel taken, left and right move the cursor. a move or change
 * is COARSE_STEP, or FINE_STEP with R held. once start is let go, the run
 * marked from its press blends from the color at one end to the other's */
static void palette_editor_step(void)
{
    Bool all;
    Bool marking;
    int dx;
    int dy;
    /* @hack spelling: 1 through one, which the image keeps in R12 (MOV
     * R6,R12 at 0x8c013c7c) with a 24-byte frame (ADD #-24,R15 at
     * 0x8c013c12); as constants, the frame takes 20 bytes and 347 of 430
     * words differ */
    int one;
    int step;
    int channels;
    int i;
    int start;
    int end;
    int color;
    int alpha;
    int red;
    int green;
    int blue;
    int lo;
    int hi;
    int from;
    int to;
    Float share;

    channels = 0;
    all = FALSE;
    marking = palette_marking;
    palette_marking = FALSE;
    if (per_info[0]->on & PDD_DGT_TX) {
        if (per_info[0]->press & PDD_DGT_KR) {
            palette_bank++;
        }
        if (per_info[0]->press & PDD_DGT_KL) {
            palette_column++;
        }
        if (palette_bank < 0) {
            palette_bank = EDIT_PALETTES - 1;
        }
        if (palette_bank >= EDIT_PALETTES) {
            palette_bank = 0;
        }
        palette_column &= 1;
    } else {
        dx = 0;
        dy = 0;
        one = 1;
        if (per_info[0]->on & PDD_DGT_KR) {
            dx = one;
        }
        if (per_info[0]->on & PDD_DGT_KL) {
            dx = -1;
        }
        if (per_info[0]->on & PDD_DGT_KU) {
            dy = -1;
        }
        if (per_info[0]->on & PDD_DGT_KD) {
            dy = one;
        }
        if (per_info[0]->on & PDD_DGT_TR) {
            step = FINE_STEP;
        } else {
            step = COARSE_STEP;
        }
        if (per_info[0]->on & PDD_DGT_TL) {
            all = TRUE;
        }
        if (per_info[0]->on & PDD_DGT_TA) {
            channels = CHANNEL_RED;
        }
        if (per_info[0]->on & PDD_DGT_TY) {
            channels |= CHANNEL_GREEN;
        }
        if (per_info[0]->on & PDD_DGT_TB) {
            channels |= CHANNEL_BLUE;
        }
        if (channels) {
            dx = -dy;
            if (dx < 0) {
                dx = -step;
            }
            if (dx > 0) {
                dx = step;
            }
            if (dx) {
                palette_change_wait--;
                if (palette_change_wait <= 0) {
                    if (palette_changing) {
                        palette_change_wait = REPEAT_WAIT;
                    } else {
                        palette_change_wait = FIRST_WAIT;
                    }
                    palette_changing = TRUE;
                    start = palette_entry;
                    end = start;
                    if (all) {
                        start = 0;
                        end = DRAW_PALETTE_ENTRIES - 1;
                    }
                    for (i = start; i <= end; i++) {
                        color = draw_palettes[palette_bank]
                                             [(i * 2) + palette_column];
                        alpha = (color >> 24) & 0xff;
                        red = (color >> 16) & 0xff;
                        green = (color >> 8) & 0xff;
                        blue = color & 0xff;
                        if (channels & CHANNEL_RED) {
                            red += dx;
                            if (red < 0) {
                                red = 0;
                            }
                            if (red > 255) {
                                red = 255;
                            }
                        }
                        if (channels & CHANNEL_GREEN) {
                            green += dx;
                            if (green < 0) {
                                green = 0;
                            }
                            if (green > 255) {
                                green = 255;
                            }
                        }
                        if (channels & CHANNEL_BLUE) {
                            blue += dx;
                            if (blue < 0) {
                                blue = 0;
                            }
                            if (blue > 255) {
                                blue = 255;
                            }
                        }
                        color =
                            (alpha << 24) | (red << 16) | (green << 8) | blue;
                        draw_palettes[palette_bank][(i * 2) + palette_column] =
                            color;
                    }
                }
            } else {
                palette_change_wait = 0;
                palette_changing = FALSE;
            }
            marking = FALSE;
        } else {
            if (dx < 0) {
                dx = -step;
            }
            if (dx > 0) {
                dx = step;
            }
            if (dx) {
                palette_move_wait--;
                if (palette_move_wait <= 0) {
                    if (palette_moving) {
                        palette_move_wait = REPEAT_WAIT;
                    } else {
                        palette_move_wait = FIRST_WAIT;
                    }
                    palette_moving = TRUE;
                    /* the cursor wraps from 0 back to the last entry. a
                     * coarse step back from there lands on a multiple of
                     * COARSE_STEP */
                    if (palette_entry == DRAW_PALETTE_ENTRIES - 1 &&
                        dx == -COARSE_STEP) {
                        palette_entry = DRAW_PALETTE_ENTRIES - COARSE_STEP;
                    } else {
                        palette_entry += dx;
                    }
                }
            } else {
                palette_move_wait = 0;
                palette_moving = FALSE;
            }
            if (palette_entry < 0) {
                palette_entry = DRAW_PALETTE_ENTRIES - 1;
            }
            if (palette_entry > DRAW_PALETTE_ENTRIES - 1) {
                palette_entry = 0;
            }
            if (per_info[0]->press & PDD_DGT_ST) {
                palette_mark = palette_entry;
            }
            if (per_info[0]->on & PDD_DGT_ST) {
                palette_marking = TRUE;
            }
        }
        /* @hack spelling: marking compared with TRUE, CMP/EQ #1 (0x8c013f12).
         * marking holds FALSE or a copy of palette_marking, which only ever
         * holds TRUE or FALSE. as a plain test, 10 of 430 words differ */
        if (marking == TRUE && !palette_marking &&
            palette_mark != palette_entry) {
            if (palette_mark < palette_entry) {
                lo = palette_mark;
                hi = palette_entry;
            } else {
                lo = palette_entry;
                hi = palette_mark;
            }
            from = draw_palettes[palette_bank][(lo * 2) + palette_column];
            to = draw_palettes[palette_bank][(hi * 2) + palette_column];
            for (i = lo; i < hi; i++) {
                share = (Float)(i - lo) / (Float)(hi - lo);
                color = palette_blend(palette_blend_mode, from, to, share);
                draw_palettes[palette_bank][(i * 2) + palette_column] = color;
            }
        }
        if (!palette_marking) {
            palette_mark = palette_entry;
        }
    }
    palette_editor_draw();
    editor_take_buttons(0, PDD_DGT_TL | PDD_DGT_TR | PDD_DGT_TX | PDD_DGT_TY |
                               PDD_DGT_ST | PDD_DGT_TA | PDD_DGT_TB);
}

/* the color share of the way from from to to, channel by channel, with
 * from's alpha. it blends straight whatever mode is, as BLEND_LINEAR is the
 * only mode */
static Uint32 palette_blend(int mode, int from, int to, Float share)
{
    int from_a;
    int from_r;
    int from_g;
    int from_b;
    int to_r;
    int to_g;
    int to_b;
    Float red;
    Float green;
    Float blue;
    int new_r;
    int new_g;
    int new_b;
    Uint32 color;

    from_r = (from >> 16) & 0xff;
    from_g = (from >> 8) & 0xff;
    from_b = from & 0xff;
    to_r = (to >> 16) & 0xff;
    to_g = (to >> 8) & 0xff;
    to_b = to & 0xff;
    from_a = (from >> 24) & 0xff;
    red = from_r + (share * (to_r - from_r));
    green = from_g + (share * (to_g - from_g));
    blue = from_b + (share * (to_b - from_b));
    new_r = red;
    new_g = green;
    new_b = blue;
    color = (from_a << 24) | (new_r << 16) | (new_g << 8) | new_b;
    return color;
}

/* draw the palette near the bottom of the screen. its colors make a
 * strip, and below it each channel makes a curve, over a band from the
 * mark to the cursor. the cursor's entry shows in numbers */
static void palette_editor_draw(void)
{
    Uint32 colors[4];
    NJS_POINT2COL quad;
    NJS_POINT2 points[4];
    NJS_COLOR col[4];
    NJS_COLOR tex[4];
    int value;
    int red;
    int green;
    int blue;
    int i;

    colors[0] = 0xff808080;
    colors[1] = 0xff202020;
    colors[2] = 0xffc0c0c0;
    colors[3] = 0xff404040;
    palette_box(110.0F, 450.0F, 626.0F, 368.0F, colors);
    colors[1] = 0xff808080;
    colors[0] = 0xff202020;
    colors[3] = 0xffc0c0c0;
    colors[2] = 0xff404040;
    palette_box(12.0F, 450.0F, 114.0F, 365.0F, colors);
    if (palette_marking) {
        colors[1] = 0xff808080;
        colors[0] = 0xff202020;
        colors[3] = 0xffc0c0c0;
        colors[2] = 0xff404040;
        palette_box(28.0F, 368.0F, 116.0F, 348.0F, colors);
        colors[0] = 0xff000000;
        colors[1] = 0xff303030;
        colors[2] = 0xff000000;
        colors[3] = 0xff303030;
        palette_box(30.0F, 368.0F, 114.0F, 350.0F, colors);
    }
    colors[0] = 0xff101010;
    colors[1] = 0xff101010;
    colors[2] = 0xff404040;
    colors[3] = 0xff404040;
    palette_box(112.0F, 448.0F, 624.0F, 382.0F, colors);
    colors[0] = 0xff101010;
    colors[1] = 0xff101010;
    colors[2] = 0xff404040;
    colors[3] = 0xff404040;
    palette_box(112.0F, 380.0F, 624.0F, 370.0F, colors);
    colors[0] = 0xff000000;
    colors[1] = 0xff303030;
    colors[2] = 0xff000000;
    colors[3] = 0xff303030;
    palette_box(14.0F, 448.0F, 112.0F, 367.0F, colors);
    colors[2] = 0xff80ff80;
    colors[3] = 0xff80ff80;
    colors[0] = 0xff008000;
    colors[1] = 0xff008000;
    palette_box((palette_mark * 2) + 112.0F, 448.0F,
                (palette_entry * 2) + 113.0F, 384.0F, colors);
    colors[0] = 0xff000000;
    colors[1] = 0xffff0000;
    colors[2] = 0xff000000;
    colors[3] = 0xffff0000;
    palette_curve(palette_column, 16, colors);
    palette_box(16.0F, 382.0F, 110.0F, 397.0F, colors);
    colors[0] = 0xff000000;
    colors[1] = 0xff00ff00;
    colors[2] = 0xff000000;
    colors[3] = 0xff00ff00;
    palette_curve(palette_column, 8, colors);
    palette_box(16.0F, 398.0F, 110.0F, 413.0F, colors);
    colors[0] = 0xff000000;
    colors[1] = 0xff0000ff;
    colors[2] = 0xff000000;
    colors[3] = 0xff0000ff;
    palette_curve(palette_column, 0, colors);
    palette_box(16.0F, 414.0F, 110.0F, 429.0F, colors);
    value = draw_palettes[palette_bank][(palette_entry * 2) + palette_column];
    red = (value >> 16) & 0xff;
    green = (value >> 8) & 0xff;
    blue = value & 0xff;
    njPrintC(NJM_LOCATION(3, 24), "R:");
    njPrintH(NJM_LOCATION(5, 24), red, 2);
    njPrintC(NJM_LOCATION(3, 25), "G:");
    njPrintH(NJM_LOCATION(5, 25), green, 2);
    njPrintC(NJM_LOCATION(3, 26), "B:");
    njPrintH(NJM_LOCATION(5, 26), blue, 2);
    if (palette_column) {
        njPrintC(NJM_LOCATION(1, 27), "SPEC");
    } else {
        njPrintC(NJM_LOCATION(1, 27), "BASE");
    }
    njPrintH(NJM_LOCATION(5, 27), palette_bank, 2);
    if (palette_marking) {
        njPrintC(NJM_LOCATION(4, 22), ">");
        njPrintH(NJM_LOCATION(2, 22), palette_mark, 2);
        njPrintH(NJM_LOCATION(5, 22), palette_entry, 2);
    }
    /* @hack spelling: a switch of one case, which tests the mode with CMP/EQ
     * #0,R0 (0x8c014438); as an if, 3 of 703 words differ */
    switch (palette_blend_mode) {
    case BLEND_LINEAR:
        njPrintC(NJM_LOCATION(1, 23), "LINIER");
        break;
    }
    quad.p = points;
    quad.col = col;
    quad.tex = tex;
    quad.num = 4;
    tex[0].tex.u = 0;
    tex[0].tex.v = 0;
    tex[1].tex.u = 0;
    tex[1].tex.v = 0;
    tex[2].tex.u = 0;
    tex[2].tex.v = 0;
    tex[3].tex.u = 0;
    tex[3].tex.v = 0;
    value = draw_palettes[palette_bank][palette_column];
    points[0].x = 112.0F;
    points[0].y = 382.0F;
    col[0].color = value;
    points[1].x = 112.0F;
    points[1].y = 370.0F;
    col[1].color = value;
    for (i = 1; i < DRAW_PALETTE_ENTRIES; i++) {
        value = draw_palettes[palette_bank][palette_column + (i * 2)];
        points[2].x = (i * 2) + 112.0F;
        points[2].y = 382.0F;
        col[2].color = value;
        points[3].x = (i * 2) + 112.0F;
        points[3].y = 370.0F;
        col[3].color = value;
        njDrawPolygon2D(&quad, 4, -1.0F, NJD_FILL);
        points[0].x = points[2].x;
        points[0].y = points[2].y;
        col[0].color = col[2].color;
        points[1].x = points[3].x;
        points[1].y = points[3].y;
        col[1].color = col[3].color;
    }
}

/* draw one channel of column's colors, the byte shift bits up, as a line
 * that rises a pixel for each 4 of its value */
static void palette_curve(int column, int shift, const Uint32 *colors)
{
    int i;
    int last;
    int next;
    int index;

    index = column;
    next = draw_palettes[palette_bank][index];
    next = (Uint8)(next >> shift);
    last = next;
    for (i = 1; i < DRAW_PALETTE_ENTRIES; i++) {
        index = column + (i * 2);
        next = draw_palettes[palette_bank][index];
        next = (Uint8)(next >> shift);
        /* NOLINTBEGIN(bugprone-integer-division) */
        palette_line(((i - 1) * 2) + 112.0F, 448.0F - (last / 4),
                     (i * 2) + 112.0F, 448.0F - (next / 4), colors);
        /* NOLINTEND(bugprone-integer-division) */
        last = next;
    }
}

/* fill the box from x0, y0 to x1, y1. its corners at x0, y0, at x1, y0,
 * at x1, y1 and at x0, y1 take colors[0] to colors[3] */
static void palette_box(Float x0, Float y0, Float x1, Float y1,
                        const Uint32 *colors)
{
    NJS_POINT2COL quad;
    NJS_POINT2 points[4];
    NJS_COLOR col[4];
    NJS_COLOR tex[4];

    quad.p = points;
    quad.col = col;
    quad.tex = tex;
    quad.num = 4;
    points[0].x = x0;
    points[0].y = y0;
    col[0].color = colors[0];
    tex[0].tex.u = 0;
    tex[0].tex.v = 0;
    points[1].x = x1;
    points[1].y = y0;
    col[1].color = colors[1];
    tex[1].tex.u = 255;
    tex[1].tex.v = 0;
    points[2].x = x1;
    points[2].y = y1;
    col[2].color = colors[2];
    tex[2].tex.u = 255;
    tex[2].tex.v = 255;
    points[3].x = x0;
    points[3].y = y1;
    col[3].color = colors[3];
    tex[3].tex.u = 0;
    tex[3].tex.v = 255;
    njDrawPolygon2D(&quad, 4, -1.0F, NJD_FILL);
}

/* draw a line from x0, y0 to x1, y1 in colors[1], taking colors as
 * palette_box does */
static void palette_line(Float x0, Float y0, Float x1, Float y1,
                         const Uint32 *colors)
{
    NJS_POINT2COL line;
    NJS_POINT2 points[2];
    NJS_COLOR col[2];
    NJS_COLOR tex[2];

    line.p = points;
    line.col = col;
    line.tex = tex;
    line.num = 4;
    points[0].x = x0;
    points[0].y = y0;
    col[0].color = colors[1];
    tex[0].tex.u = 0;
    tex[0].tex.v = 0;
    points[1].x = x1;
    points[1].y = y1;
    col[1].color = colors[1];
    tex[1].tex.u = 255;
    tex[1].tex.v = 255;
    njDrawLine2D(&line, 1, -0.5F, NJD_TRANSPARENT);
}

Bool light_read_disc(const char *name, void *dest)
{
    GDFS file;
    Sint32 size;
    int part;

    file = gdFsOpen(name, NULL);
    if (file == NULL) {
        return FALSE;
    }
    gdFsGetFileSize(file, &size);
    if (size % SECTOR_SIZE) {
        part = 1;
    } else {
        part = 0;
    }
    size = (size / SECTOR_SIZE) + part;
    gdFsRead(file, size, dest);
    gdFsClose(file);
    return TRUE;
}

Bool light_read_disc_sources(Uint16 stage, int act)
{
    char name[16] = "SL_";
    char *dest;
    Bool result;

#ifdef GCC
    /* the read's whole sectors run past light_sources into the members of
     * sources_file after it, so GCC's dest points at the whole object */
    dest = (char *)&sources_file;
#else
    dest = (char *)light_sources;
#endif
    if (stage <= 9) {
        name[3] = stage + '0';
    } else {
        name[3] = stage + ('A' - 10);
    }
    if (name[3] > 'Z') {
        name[3] -= 26;
        name[2] = '1';
    }
    name[4] = act + '0';
    name[5] = 'B';
    name[6] = '.';
    name[7] = 'b';
    name[8] = 'i';
    name[9] = 'n';
    name[10] = 0;
    result = light_read_disc(name, dest);
    /* @hack spelling: the result compared with TRUE, CMP/EQ #1 (0x8c014be6).
     * light_read_disc returns only TRUE or FALSE, so as a plain test, 3 of 99
     * words differ */
    if (result != TRUE) {
        name[4] = '0';
        result = light_read_disc(name, dest);
    }
    return result;
}

Bool light_read_disc_palettes(Uint16 stage, int act)
{
    char name[16] = "PL_";
    char *dest;
    Bool result;

    dest = (char *)draw_palettes;
    if (stage <= 9) {
        name[3] = stage + '0';
    } else {
        name[3] = stage + ('A' - 10);
    }
    if (name[3] > 'Z') {
        name[3] -= 26;
        name[2] = '1';
    }
    name[4] = act + '0';
    name[5] = 'B';
    name[6] = '.';
    name[7] = 'b';
    name[8] = 'i';
    name[9] = 'n';
    name[10] = 0;
    result = light_read_disc(name, dest);
    /* @hack spelling: the result compared with TRUE, CMP/EQ #1 (0x8c014cac).
     * light_read_disc returns only TRUE or FALSE, so as a plain test, 3 of 99
     * words differ */
    if (result != TRUE) {
        name[4] = '0';
        result = light_read_disc(name, dest);
    }
    return result;
}

void light_editor_init(void)
{
    light_load_act();
}

int light_editor_exec(void)
{
    if (light_editor_mode == EDIT_OFF) {
        if (per_info[0]->on & PDD_DGT_TA) {
            return -1;
        }
    }
    light_editor_step();
    return 0;
}
