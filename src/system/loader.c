/* reads from the disc: whole files, PRS files, a stage's programs and files,
 * and a queue of reads in the background */
#include <sg_xpt.h>
#include <shinobi.h>
#include <stdlib.h>
#include <string.h>

#include "main.h"
#include "player/player.h"
#include "progress/progress.h"
#include "save/card.h"
#include "stage/stage.h"
#include "system/loader.h"
#include "system/prs.h"
#include "system/sbinit.h"

/* where a stage's other programs load, beside its module in MODULE_AREA */
#define FIELD_AREA ((Uint8 *)0x0c920000) /* ADV0100 or ADV0130 */
#define TIKAL_AREA ((Uint8 *)0x0cb00000) /* TIKAL_PROG */
#define EXTRA_AREA ((Uint8 *)0x0cb80000) /* SHTPATH, AL_GARDEN or AL_RACE */

#define READ_QUEUE_SIZE 16

enum read_state { READ_FREE = 0, READ_WAITING = 1, READ_READING = -1 };

struct read_request {
    enum read_state state;
    char name[32];
    void *dest;
};

static int loader_read(char *name, void *dest);
static void loader_read_set(char *name, void *dest);
static void loader_read_camera(char *name, void *dest);

static GDFS loader_file;
static GDFS read_queue_file;
static struct read_request read_queue[READ_QUEUE_SIZE];
static int read_queue_current; /* the selected slot plus one, or 0 */

/* each character's letters at the end of a set or camera file's name */
static char *loader_suffixes[CHARACTER_COUNT] = {"S",  "EG", "M", "K",
                                                 "TI", "A",  "E", "B"};
static char *loader_last_story_suffix = "L";

/* read the file name into dest, rounded up to whole sectors. it returns -1
 * when the open fails, and 0 otherwise: the size query and the read go
 * unchecked */
static int loader_read(char *name, void *dest)
{
    Sint32 size;
    int part;

    loader_file = gdFsOpen(name, NULL);
    if (loader_file == NULL) {
        return -1;
    }
    gdFsGetFileSize(loader_file, &size);
    if (size % SECTOR_SIZE != 0) {
        part = 1;
    } else {
        part = 0;
    }
    size = (size / SECTOR_SIZE) + part;
    gdFsRead(loader_file, size, dest);
    gdFsClose(loader_file);
    return 0;
}

/* read set<name><letters>.bin into dest, the letters the character's, or
 * in the last story L. when that file won't open, it reads Sonic's */
static void loader_read_set(char *name, void *dest)
{
    char path[32];
    char *suffix;
    /* @hack type: file, a volatile copy of path, which SHC keeps in the
     * frame at @R15 (MOV.L R14,@R15 at 0x8c0a2810), below name's and dest's
     * slots; without volatile, 8 of 69 words differ */
    char *volatile file;
    int result;

    if (last_story) {
        suffix = loader_last_story_suffix;
    } else {
        suffix = loader_suffixes[get_character()];
    }
    strcpy(path, "set");
    strcat(path, name);
    strcat(path, suffix);
    strcat(path, ".bin");
    result = loader_read(file = path, dest);
    /* @hack spelling: the result tested by a switch of one case (CMP/EQ #-1,R0
     * at 0x8c0a2816); as an if, 5 of 69 words differ */
    switch (result) {
    case -1:
        suffix = loader_suffixes[CHARACTER_SONIC];
        strcpy(path, "set");
        strcat(path, name);
        strcat(path, suffix);
        strcat(path, ".bin");
        loader_read(file, dest);
        break;
    }
}

/* read cam<name><letters>.bin into dest, the letters the character's. when
 * that file won't open, it reads Sonic's */
static void loader_read_camera(char *name, void *dest)
{
    char path[32];
    char *suffix;
    /* @hack type: file, a volatile local that holds the suffix and
     * then the path in one frame slot, at @R15 (MOV.L R2,@R15 at 0x8c0a286c,
     * MOV.L R14,@R15 at 0x8c0a2888); without volatile, 4 of 59 words
     * differ */
    char *volatile file;
    int result;

    file = loader_suffixes[get_character()];
    strcpy(path, "cam");
    strcat(path, name);
    strcat(path, file);
    strcat(path, ".bin");
    result = loader_read(file = path, dest);
    /* @hack spelling: the result tested by a switch of one case (CMP/EQ #-1,R0
     * at 0x8c0a288e); as an if, 5 of 59 words differ */
    switch (result) {
    case -1:
        suffix = loader_suffixes[CHARACTER_SONIC];
        strcpy(path, "cam");
        strcat(path, name);
        strcat(path, suffix);
        strcat(path, ".bin");
        loader_read(file, dest);
        break;
    }
}

void loader_poll_drive(void)
{
    Sint32 status;

    if (loader_error_flag && card_saving == 0) {
        sbExitSystem();
        syBtExit();
    } else {
        status = gdFsGetDrvStat();
        if (status == GDD_DRVSTAT_OPEN || status == GDD_DRVSTAT_BUSY) {
            gdFsReqDrvStat();
        }
    }
}

void loader_note_error(void *obj, Sint32 err)
{
    if (err == GDD_ERR_TRAYOPEND || err == GDD_ERR_UNITATTENT) {
        loader_error_flag = TRUE;
    }
}

void loader_read_stage_files(void)
{
    Uint16 stage;
    char *camera;
    void *dest;
    /* NOLINTNEXTLINE(clang-diagnostic-unused-but-set-variable) */
    int result;

    stage = stage_get_stage_act() >> 8;
    /* @hack spelling: a case that ends in a camera read sets camera and dest
     * and jumps to another case's read, as the image's cases branch to two
     * shared tails (0x8c0a333e, 0x8c0a3374); with a call in each case, 164
     * of 827 words differ */
    switch (stage) {
    case STAGE_PRACTICE:
        loader_read_prs("stg00.PRS", MODULE_AREA);
        loader_read_set("0000", SET_AREA_0);
        loader_read_set("0001", SET_AREA_1);
        camera = "0000";
        dest = CAMERA_AREA_0;
        goto e101_r_camera;
    case STAGE_BEACH:
        loader_read_prs("stg01.PRS", MODULE_AREA);
        loader_read_set("0100", SET_AREA_0);
        loader_read_set("0101", SET_AREA_1);
        loader_read_set("0102", SET_AREA_2);
        loader_read_camera("0100", CAMERA_AREA_0);
        loader_read_camera("0101", CAMERA_AREA_1);
        dest = CAMERA_AREA_2;
        camera = "0102";
        goto alife_race_camera;
    case STAGE_WINDY:
        loader_read_prs("stg02.PRS", MODULE_AREA);
        loader_read_set("0200", SET_AREA_0);
        loader_read_set("0201", SET_AREA_1);
        loader_read_set("0202", SET_AREA_2);
        loader_read_camera("0200", CAMERA_AREA_0);
        loader_read_camera("0201", CAMERA_AREA_1);
        loader_read_camera("0202", CAMERA_AREA_2);
        return;
    case STAGE_TWINKLE:
        loader_read_prs("stg03.PRS", MODULE_AREA);
        loader_read_set("0300", SET_AREA_0);
        loader_read_set("0301", SET_AREA_1);
        loader_read_set("0302", SET_AREA_2);
        loader_read_camera("0300", CAMERA_AREA_0);
        loader_read_camera("0301", CAMERA_AREA_1);
        dest = CAMERA_AREA_2;
        camera = "0302";
        goto alife_race_camera;
    case STAGE_HIGHWAY:
        loader_read_prs("stg04.PRS", MODULE_AREA);
        loader_read_set("0400", SET_AREA_0);
        loader_read_set("0401", SET_AREA_1);
        loader_read_set("0402", SET_AREA_2);
        loader_read_camera("0400", CAMERA_AREA_0);
        loader_read_camera("0401", CAMERA_AREA_1);
        loader_read_camera("0402", CAMERA_AREA_2);
        return;
    case STAGE_MOUNTAIN:
        loader_read_prs("stg05.PRS", MODULE_AREA);
        loader_read_set("0500", SET_AREA_0);
        loader_read_set("0501", SET_AREA_1);
        loader_read_set("0502", SET_AREA_2);
        loader_read_camera("0500", CAMERA_AREA_0);
        loader_read_camera("0501", CAMERA_AREA_1);
        dest = CAMERA_AREA_2;
        camera = "0502";
        goto alife_race_camera;
    case STAGE_SKY_DECK:
        loader_read_prs("stg06.PRS", MODULE_AREA);
        loader_read_set("0600", SET_AREA_0);
        loader_read_set("0601", SET_AREA_1);
        loader_read_set("0602", SET_AREA_2);
        loader_read_camera("0600", CAMERA_AREA_0);
        loader_read_camera("0601", CAMERA_AREA_1);
        dest = CAMERA_AREA_2;
        camera = "0602";
    sky_deck_camera:
        loader_read_camera(camera, dest);
        return;
    case STAGE_RUIN:
        loader_read_prs("stg07.PRS", MODULE_AREA);
        loader_read_set("0700", SET_AREA_0);
        loader_read_set("0701", SET_AREA_1);
        loader_read_set("0702", SET_AREA_2);
        loader_read_camera("0700", CAMERA_AREA_0);
        loader_read_camera("0701", CAMERA_AREA_1);
        dest = CAMERA_AREA_2;
        camera = "0702";
        goto alife_race_camera;
    case STAGE_ICECAP:
        loader_read_prs("stg08.PRS", MODULE_AREA);
        loader_read_set("0800", SET_AREA_0);
        loader_read_set("0801", SET_AREA_1);
        loader_read_set("0802", SET_AREA_2);
        loader_read_set("0803", SET_AREA_3);
        loader_read_camera("0800", CAMERA_AREA_0);
        loader_read_camera("0801", CAMERA_AREA_1);
        loader_read_camera("0802", CAMERA_AREA_2);
        loader_read_camera("0803", CAMERA_AREA_3);
        return;
    case STAGE_CASINO:
        loader_read_prs("stg09.PRS", MODULE_AREA);
        loader_read_set("0900", SET_AREA_0);
        loader_read_set("0901", SET_AREA_1);
        loader_read_set("0902", SET_AREA_2);
        loader_read_set("0903", SET_AREA_3);
        loader_read_camera("0900", CAMERA_AREA_0);
        loader_read_camera("0901", CAMERA_AREA_1);
        loader_read_camera("0902", CAMERA_AREA_2);
        dest = CAMERA_AREA_3;
        camera = "0903";
        goto alife_race_camera;
    case STAGE_FINAL:
        loader_read_prs("stg10.PRS", MODULE_AREA);
        loader_read_set("1000", SET_AREA_0);
        loader_read_set("1001", SET_AREA_1);
        loader_read_set("1002", SET_AREA_2);
        loader_read_set("1003", SET_AREA_3);
        loader_read_camera("1000", CAMERA_AREA_0);
        loader_read_camera("1001", CAMERA_AREA_1);
        loader_read_camera("1002", CAMERA_AREA_2);
        dest = CAMERA_AREA_3;
        camera = "1003";
        loader_read_camera(camera, dest);
        return;
    case STAGE_SHELTER:
        loader_read_prs("stg12.PRS", MODULE_AREA);
        loader_read_set("1200", SET_AREA_0);
        loader_read_set("1201", SET_AREA_1);
        loader_read_set("1202", SET_AREA_2);
        loader_read_set("1203", SET_AREA_3);
        loader_read_camera("1200", CAMERA_AREA_0);
        loader_read_camera("1201", CAMERA_AREA_1);
        loader_read_camera("1202", CAMERA_AREA_2);
        dest = CAMERA_AREA_3;
        camera = "1203";
        goto alife_race_camera;
    case STAGE_CHAOS0:
        loader_read_prs("b_chaos0.PRS", MODULE_AREA);
        loader_read_set("1500", SET_AREA_0);
        dest = CAMERA_AREA_0;
        camera = "b000";
        loader_read_camera(camera, dest);
        return;
    case STAGE_CHAOS2:
        loader_read_prs("b_chaos2.PRS", MODULE_AREA);
        loader_read_set("1600", SET_AREA_0);
        camera = "0000";
        dest = CAMERA_AREA_0;
        goto e101_r_camera;
    case STAGE_CHAOS4:
        loader_read_prs("b_chaos4.PRS", MODULE_AREA);
        loader_read_set("1700", SET_AREA_0);
        dest = CAMERA_AREA_0;
        camera = "0000";
    /* @hack redundant: a label nothing jumps to, which keeps EGGMOBILE1's case
     * ending in its own copy of this case's last three instructions (0x8c0a2dbc
     * to 0x8c0a2dc0). without it, SHC has that case branch to this one's, and
     * 501 of 827 words differ */
    /* NOLINTNEXTLINE(clang-diagnostic-unused-label) */
    chaos4_camera:
        loader_read_camera(camera, dest);
        return;
    case STAGE_CHAOS6:
        loader_read_prs("b_chaos6.PRS", MODULE_AREA);
        loader_read_set("1800", SET_AREA_0);
        loader_read_set("1801", SET_AREA_1);
        dest = CAMERA_AREA_0;
        camera = "0000";
        goto alife_mr_camera;
    case STAGE_CHAOS7:
        loader_read_prs("b_chaos7.PRS", MODULE_AREA);
        loader_read_set("1900", SET_AREA_0);
        loader_read_camera("1900", CAMERA_AREA_0);
        loader_read_prs("TIKAL_PROG.PRS", TIKAL_AREA);
        return;
    case STAGE_EGGMOBILE1:
        loader_read_prs("b_egm1.PRS", MODULE_AREA);
        loader_read_set("egm1", SET_AREA_0);
        dest = CAMERA_AREA_0;
        camera = "0000";
        goto sky_deck_camera;
    case STAGE_EGGMOBILE2:
        loader_read_prs("b_egm2.PRS", MODULE_AREA);
        loader_read_set("egm2", SET_AREA_0);
        dest = CAMERA_AREA_0;
        camera = "0000";
    /* @hack redundant: a label nothing jumps to, which keeps EGGMOBILE1's case
     * ending in its own copy of this case's last three instructions (0x8c0a2dbc
     * to 0x8c0a2dc0). without it, SHC has that case branch to this one's, and
     * 501 of 827 words differ */
    /* NOLINTNEXTLINE(clang-diagnostic-unused-label) */
    eggmobile2_camera:
        loader_read_camera(camera, dest);
        return;
    case STAGE_EGGMOBILE3:
        loader_read_prs("b_egm3.PRS", MODULE_AREA);
        loader_read_set("egm3", SET_AREA_0);
        camera = "0000";
        dest = CAMERA_AREA_0;
        goto alife_race_camera;
    case STAGE_EGGMANROBO:
        loader_read_prs("b_robo.PRS", MODULE_AREA);
        loader_read_set("zero", SET_AREA_0);
        dest = CAMERA_AREA_0;
        camera = "0000";
    /* @hack redundant: a label nothing jumps to, which keeps EGGMOBILE1's case
     * ending in its own copy of this case's last three instructions (0x8c0a2dbc
     * to 0x8c0a2dc0). without it, SHC has that case branch to this one's, and
     * 478 of 827 words differ */
    /* NOLINTNEXTLINE(clang-diagnostic-unused-label) */
    eggmanrobo_camera:
        loader_read_camera(camera, dest);
        return;
    case STAGE_E101:
        loader_read_prs("b_e101.PRS", MODULE_AREA);
        loader_read_set("e101", SET_AREA_0);
        dest = CAMERA_AREA_0;
        camera = "0000";
        goto alife_race_camera;
    case STAGE_E101_R:
        loader_read_prs("b_e101_r.PRS", MODULE_AREA);
        loader_read_set("e101r", SET_AREA_0);
        dest = CAMERA_AREA_0;
        camera = "0000";
    e101_r_camera:
        loader_read_camera(camera, dest);
        return;
    case STAGE_STATION_SQUARE:
    case STAGE_STATION_SQUARE_2:
    case STAGE_STATION_SQUARE_3:
        loader_read_prs("adv00.PRS", MODULE_AREA);
        loader_read_set("SS00", SET_AREA_0);
        loader_read_set("SS01", SET_AREA_1);
        loader_read_set("SS02", SET_AREA_2);
        loader_read_set("SS03", SET_AREA_3);
        loader_read_set("SS04", SET_AREA_4);
        loader_read_set("SS05", SET_AREA_5);
        loader_read_camera("SS00", CAMERA_AREA_0);
        loader_read_camera("SS01", CAMERA_AREA_1);
        loader_read_camera("SS02", CAMERA_AREA_2);
        loader_read_camera("SS03", CAMERA_AREA_3);
        loader_read_camera("SS04", CAMERA_AREA_4);
        dest = CAMERA_AREA_5;
        camera = "SS05";
        goto alife_race_camera;
    case STAGE_EGG_CARRIER:
        loader_read_prs("adv01obj.PRS", MODULE_AREA);
        loader_read_prs("adv0100.PRS", FIELD_AREA);
        loader_read_set("EC00", SET_AREA_0);
        loader_read_set("EC01", SET_AREA_1);
        loader_read_set("EC02", SET_AREA_2);
        loader_read_set("EC03", SET_AREA_3);
        loader_read_set("EC04", SET_AREA_4);
        loader_read_set("EC05", SET_AREA_5);
        loader_read_camera("EC00", CAMERA_AREA_0);
        loader_read_camera("EC01", CAMERA_AREA_1);
        loader_read_camera("EC02", CAMERA_AREA_2);
        loader_read_camera("EC03", CAMERA_AREA_3);
        loader_read_camera("EC04", CAMERA_AREA_4);
        loader_read_camera("EC05", CAMERA_AREA_5);
        return;
    case STAGE_EGG_CARRIER_INSIDE:
        loader_read_prs("adv01obj.PRS", MODULE_AREA);
        loader_read_prs("adv0130.PRS", FIELD_AREA);
        loader_read_set("EC30", SET_AREA_0);
        loader_read_set("EC31", SET_AREA_1);
        loader_read_set("EC32", SET_AREA_2);
        loader_read_set("EC33", SET_AREA_3);
        loader_read_set("EC34", SET_AREA_4);
        loader_read_set("EC35", SET_AREA_5);
        loader_read_camera("EC30", CAMERA_AREA_0);
        loader_read_camera("EC31", CAMERA_AREA_1);
        loader_read_camera("EC32", CAMERA_AREA_2);
        loader_read_camera("EC33", CAMERA_AREA_3);
        loader_read_camera("EC34", CAMERA_AREA_4);
        dest = CAMERA_AREA_5;
        camera = "EC35";
        goto alife_race_camera;
    case STAGE_MYSTIC_RUINS:
        loader_read_prs("adv02.PRS", MODULE_AREA);
        loader_read_set("MR00", SET_AREA_0);
        loader_read_set("MR01", SET_AREA_1);
        loader_read_set("MR02", SET_AREA_2);
        loader_read_set("MR03", SET_AREA_3);
        loader_read_camera("MR00", CAMERA_AREA_0);
        loader_read_camera("MR01", CAMERA_AREA_1);
        loader_read_camera("MR02", CAMERA_AREA_2);
        loader_read_camera("MR03", CAMERA_AREA_3);
        /* @hack redundant: result, set to the fog read's result and never
         * read, which keeps the read a BSR that returns here (0x8c0a3062).
         * with the result dropped, SHC makes the read a jump, the code runs
         * 32 bytes short, and 96 of 827 words differ */
        result = loader_read("FOG_MR02.bin", CAMERA_AREA_5);
        return;
    case STAGE_PAST:
        loader_read_prs("adv03.PRS", MODULE_AREA);
        loader_read_set("Past00", SET_AREA_0);
        loader_read_set("Past01", SET_AREA_1);
        loader_read_set("Past02", SET_AREA_2);
        loader_read_camera("Past00", CAMERA_AREA_0);
        loader_read_camera("Past01", CAMERA_AREA_1);
        loader_read_camera("Past02", CAMERA_AREA_2);
        loader_read_prs("TIKAL_PROG.PRS", TIKAL_AREA);
        return;
    case STAGE_CART:
        loader_read_prs("minicart.PRS", MODULE_AREA);
        loader_read_set("MCart00", SET_AREA_0);
        loader_read_set("MCart01", SET_AREA_1);
        loader_read_set("MCart02", SET_AREA_2);
        loader_read_set("MCart03", SET_AREA_3);
        loader_read_set("MCart04", SET_AREA_4);
        loader_read_set("MCart05", SET_AREA_5);
        loader_read_camera("MCart00", CAMERA_AREA_0);
        loader_read_camera("MCart01", CAMERA_AREA_1);
        loader_read_camera("MCart02", CAMERA_AREA_2);
        loader_read_camera("MCart03", CAMERA_AREA_3);
        loader_read_camera("MCart04", CAMERA_AREA_4);
        dest = CAMERA_AREA_5;
        camera = "MCart05";
        goto alife_mr_camera;
    case STAGE_SHOOTING1:
        loader_read_prs("shooting.PRS", MODULE_AREA);
        loader_read_prs("shtpath1.PRS", EXTRA_AREA);
        loader_read_set("Sht1", SET_AREA_0);
        loader_read_camera("Sht1", CAMERA_AREA_0);
        return;
    case STAGE_SHOOTING2:
        loader_read_prs("shooting.PRS", MODULE_AREA);
        loader_read_prs("shtpath2.PRS", EXTRA_AREA);
        loader_read_set("Sht2", SET_AREA_0);
        dest = CAMERA_AREA_0;
        camera = "Sht2";
        goto alife_mr_camera;
    case STAGE_SUNABOARD:
        loader_read_prs("sboard.PRS", MODULE_AREA);
        if (get_character() == CHARACTER_SONIC ||
            get_character() == CHARACTER_TAILS) {
            loader_read_prs("s_sbmot.PRS", SONIC_BOARD_AREA);
            loader_read_prs("m_sbmot.PRS", TAILS_BOARD_AREA);
        }
        loader_read_set("SBoard00", SET_AREA_0);
        loader_read_set("SBoard01", SET_AREA_1);
        loader_read_camera("SBoard00", CAMERA_AREA_0);
        loader_read_camera("SBoard01", CAMERA_AREA_1);
        break;
    case STAGE_ALIFE_SS:
        loader_read_prs("al_main.PRS", MODULE_AREA);
        loader_read_prs("samp_lcd.PRS", SET_AREA_1);
        loader_read_prs("al_garden00.PRS", EXTRA_AREA);
        loader_read_set("Garden00", SET_AREA_0);
        loader_read_camera("Garden00", CAMERA_AREA_0);
        break;
    case STAGE_ALIFE_EG:
        loader_read_prs("al_main.PRS", MODULE_AREA);
        loader_read_prs("samp_lcd.PRS", SET_AREA_1);
        loader_read_prs("al_garden01.PRS", EXTRA_AREA);
        loader_read_set("Garden01", SET_AREA_0);
        loader_read_camera("Garden01", CAMERA_AREA_0);
        break;
    case STAGE_ALIFE_MR:
        loader_read_prs("al_main.PRS", MODULE_AREA);
        loader_read_prs("samp_lcd.PRS", SET_AREA_1);
        loader_read_prs("al_garden02.PRS", EXTRA_AREA);
        loader_read_set("Garden02", SET_AREA_0);
        dest = CAMERA_AREA_0;
        camera = "Garden02";
    alife_mr_camera:
        loader_read_camera(camera, dest);
        break;
    case STAGE_ALIFE_RACE:
        loader_read_prs("al_main.PRS", MODULE_AREA);
        loader_read_prs("al_race.PRS", EXTRA_AREA);
        loader_read_set("AL_Race00", SET_AREA_0);
        loader_read_set("AL_Race01", SET_AREA_1);
        loader_read_camera("AL_Race00", CAMERA_AREA_0);
        dest = CAMERA_AREA_1;
        camera = "AL_Race01";
    alife_race_camera:
        loader_read_camera(camera, dest);
        return;
    default:
        for (;;) {
        }
    }
}

void loader_read_character_motions(int character)
{
    switch (character) {
    case CHARACTER_SONIC:
    case CHARACTER_TAILS:
        loader_read_prs("s_mot.PRS", MOTION_AREA);
        break;
    case CHARACTER_BIG:
        loader_read_prs("b_mot.PRS", MOTION_AREA);
        break;
    case CHARACTER_AMY:
        loader_read_prs("a_mot.PRS", MOTION_AREA);
        break;
    case CHARACTER_KNUCKLES:
        loader_read_prs("k_mot.PRS", MOTION_AREA);
        break;
    case CHARACTER_E102:
        loader_read_prs("e_mot.PRS", MOTION_AREA);
        break;
    default:
        for (;;) {
        }
    }
}

void *loader_read_alloc(char *name, int result)
{
    int size;
    void *memory;
    char *file;

    if ((size = loader_file_size(name)) == -1) {
        return NULL;
    }
    if ((memory = malloc(size)) == NULL) {
        return NULL;
    }
    /* @hack spelling: name passed through file, a copy, which SHC loads
     * ahead of the BSR (MOV.L @R15,R4 at 0x8c0a3448); passed straight, it
     * loads in the slot, and 2 of 33 words differ */
    file = name;
    result = loader_read(file, memory);
    /* @hack spelling: the result tested by a switch of one case and a default
     * (CMP/EQ #-1,R0 at 0x8c0a344e); as an if, 2 of 33 words differ */
    switch (result) {
    case -1:
        free(memory);
        return NULL;
    default:
        return memory;
    }
}

void loader_free(void *memory)
{
    free(memory);
}

int loader_read_file(char *name, void *dest)
{
    return loader_read(name, dest);
}

int loader_read_prs(char *name, void *dest)
{
    int result;

    result = loader_read(name, WORK_AREA);
    if (result >= 0) {
#ifdef GCC
        /* the instruction cache fills from RAM, but the operand cache copies
         * back, holding the decoder's stores until their lines are written
         * back. so the decoded bytes go back to RAM before syCacheICI, and
         * code in dest runs as decoded */
        syCacheOCWB(dest, prs_decompress(WORK_AREA, dest));
#else
        prs_decompress(WORK_AREA, dest);
#endif
    }
    syCacheICI();
    return result;
}

int loader_file_size(char *name)
{
    Sint32 size;

    loader_file = gdFsOpen(name, NULL);
    if (loader_file == NULL) {
        return -1;
    }
    if (!gdFsGetFileSize(loader_file, &size)) {
        size = -1;
    }
    gdFsClose(loader_file);
    return size;
}

void read_queue_init(void)
{
    int i;

    read_queue_current = 0;
    for (i = 0; i < READ_QUEUE_SIZE; i++) {
        read_queue[i].state = READ_FREE;
    }
}

int read_queue_selected_slot(void)
{
    return read_queue_current;
}

Bool read_queue_add(char *name, void *dest)
{
    Bool added = FALSE;
    int i;

    for (i = 0; i < READ_QUEUE_SIZE; i++) {
        if (read_queue[i].state == READ_FREE) {
            added = TRUE;
            read_queue[i].state = READ_WAITING;
            read_queue[i].dest = dest;
            strcpy(read_queue[i].name, name);
            break;
        }
    }
    return added;
}

void read_queue_cancel(void)
{
    int i;

    for (i = 0; i < READ_QUEUE_SIZE; i++) {
        read_queue[i].state = READ_FREE;
    }
    if (read_queue_current != 0) {
        gdFsClose(read_queue_file);
    }
    read_queue_current = 0;
}

void read_queue_serve(void)
{
    Sint32 status;
    int i;
    Sint32 size;
    /* @hack redundant: 32 bytes of frame that no code reads, of a type unknown,
     * declared as a char[32]. the image's ADD #-36,R15 reserves them
     * (0x8c0a3670); without them, 2 of 175 words differ */
    char unused[32];
    int part;

    if (read_queue_current != 0) {
        status = gdFsGetStat(read_queue_file);
        if (status == GDD_STAT_READ) {
            return;
        }
        if (status == GDD_STAT_ERR) {
            gdFsClose(read_queue_file);
            read_queue_file =
                gdFsOpen(read_queue[read_queue_current - 1].name, NULL);
            if (read_queue_file == NULL) {
                return;
            }
            gdFsGetFileSize(read_queue_file, &size);
            if (size % SECTOR_SIZE != 0) {
                part = 1;
            } else {
                part = 0;
            }
            size = part + (size / SECTOR_SIZE);
            gdFsReqRd32(read_queue_file, size,
                        read_queue[read_queue_current - 1].dest);
            read_queue[read_queue_current - 1].state = READ_READING;
            return;
        } else {
            gdFsClose(read_queue_file);
            read_queue[read_queue_current - 1].state = READ_FREE;
            read_queue_current = 0;
        }
    }
    for (i = 0; i < READ_QUEUE_SIZE; i++) {
        if (read_queue[i].state == READ_WAITING) {
            read_queue_current = i + 1;
            read_queue_file =
                gdFsOpen(read_queue[read_queue_current - 1].name, NULL);
            if (read_queue_file == NULL) {
                return;
            }
            gdFsGetFileSize(read_queue_file, &size);
            if (size % SECTOR_SIZE != 0) {
                part = 1;
            } else {
                part = 0;
            }
            size = part + (size / SECTOR_SIZE);
            gdFsReqRd32(read_queue_file, size,
                        read_queue[read_queue_current - 1].dest);
            read_queue[read_queue_current - 1].state = READ_READING;
            break;
        }
    }
}
