/* the debug flag, debug axes and lines, and the names of frame files and
 * stages */
#include <ninja.h>
#include <sg_xpt.h>

#include "debug/debug.h"
#include "draw/light.h"
#include "stage/stage.h"
#include "system/task.h"

#define ORANGE 0xffff8000

/* an entry of a menu list, which ends with an entry of zeros and an empty
 * name */
struct menu_entry {
    Sint32 unk_00;
    const char *name;
    void (*func)(void);
    Sint32 unk_0c;
};

struct edit_item {
    const char *name;
    Float *value;
    Sint32 unk_08;
    Float unk_0c[5];
};

struct edit_menu {
    Sint32 count;
    Sint32 unk_04;
    Sint16 unk_08[4];
    struct edit_item *items;
};

static void light_choose(void);

/* -1 while the debug flag is set, and 0 while it's clear. only
 * debug_set_flag reads it */
static Sint16 debug_flag = 0;

static NJS_POINT3 debug_axis_points[6] = {
#include "debug/debug/debug_axis_points.inc"
};
static NJS_BGRA debug_axis_colors[6] = {
#include "debug/debug/debug_axis_colors.inc"
};
static NJS_POINT3COL debug_axes = {
#include "debug/debug/debug_axes.inc"
};

static char frame_file_name[] = "frame0000.bmp";
static char hex_digits[] = "0123456789ABCDEF";
static Uint16 frame_file_count = 0;

/* the light menu's list and its edit menu, which nothing reads */
static struct menu_entry light_entries[] = {
    {1, "Light", light_choose, 0},
    {0, "", NULL, 0},
};
static struct edit_item light_items[] = {
    {"DirLight R", &dir_light.attr.argb.r, 4, {0.0F, 1.0F, 0.01F, 0.1F, 1.0F}},
    {"DirLight G", &dir_light.attr.argb.g, 4, {0.0F, 1.0F, 0.01F, 0.1F, 1.0F}},
    {"DirLight B", &dir_light.attr.argb.b, 4, {0.0F, 1.0F, 0.01F, 0.1F, 1.0F}},
    {"DirLight AMB", &dir_light.attr.iamb, 4, {0.0F, 1.0F, 0.01F, 0.1F, 0.0F}},
    {"DirLight DIF", &dir_light.attr.idif, 4, {0.0F, 1.0F, 0.01F, 0.1F, 1.0F}},
    {"DirLight SPC", &dir_light.attr.ispc, 4, {0.0F, 1.0F, 0.01F, 0.1F, 0.0F}},
};
static struct edit_menu light_menu = {
    6, 0x1000, {108, 3, 200, 16}, light_items};

/* a row of fewer than STAGE_ACTS names leaves its last acts null */
static const char *stage_names[STAGE_COUNT][STAGE_ACTS] = {
    {"PRACTICE 1", "PRACTICE 2", "PRACTICE 3", "NO", "NO", "NO", "NO", "NO"},
    {"BEACH 1", "BEACH 2", "BEACH 3", "NO", "NO", "NO", "NO", "NO"},
    {"WINDY 1", "WINDY 2", "WINDY 3", "NO", "NO", "NO", "NO", "NO"},
    {"TWINKLE 1", "TWINKLE 2", "TWINKLE 3", "NO", "NO", "NO", "NO", "NO"},
    {"HIGHWAY", "BUILDING", "DOWN TOWN", "NO", "NO", "NO", "NO", "NO"},
    {"MOUNTAIN 1", "MOUNTAIN 2", "MOUNTAIN 3", "NO", "NO", "NO", "NO", "NO"},
    {"SKY DECK 1", "SKY DECK 2", "SKY DECK 3", "NO ENTRY", "NO ENTRY"},
    {"RUIN 1", "RUIN 2", "RUIN 3", "NO", "NO", "NO", "NO", "NO"},
    {"ICECAP 1", "ICECAP 2", "ICECAP 3", "ICECAP BIG", "NO", "NO", "NO", "NO"},
    {"CASINO 1", "CASINO 2", "CASINO 3", "CASINO 4", "NO", "NO", "NO", "NO"},
    {"FINAL 1", "FINAL 2", "FINAL 3", "FINAL 4", "FINAL 5", "FINAL 6", "NO",
     "NO"},
    {"NO", "NO", "NO", "NO", "NO", "NO", "NO", "NO"},
    {"SHELTER 1", "SHELTER 2", "SHELTER 3", "SHELTER 4", "NO", "NO", "NO",
     "NO"},
    {"NO", "NO", "NO", "NO", "NO", "NO", "NO", "NO"},
    {"NO", "NO", "NO", "NO", "NO", "NO", "NO", "NO"},
    {"CHAOS 0", "NO", "NO", "NO", "NO", "NO", "NO", "NO"},
    {"CHAOS 2", "NO", "NO", "NO", "NO", "NO", "NO", "NO"},
    {"CHAOS 4", "NO ENTRY", "NO ENTRY", "NO ENTRY", "NO ENTRY"},
    {"CHAOS 6", "CHAOS 6 KN", "NO ENTRY", "NO ENTRY", "NO ENTRY"},
    {"CHAOS 7 1", "CHAOS 7 2", "NO ENTRY", "NO ENTRY", "NO ENTRY"},
    {"EGGMOBILE1", "NO ENTRY", "NO ENTRY", "NO ENTRY", "NO ENTRY"},
    {"EGGMOBILE2", "NO ENTRY", "NO ENTRY", "NO ENTRY", "NO ENTRY"},
    {"EGGMOBILE3", "NO ENTRY", "NO ENTRY", "NO ENTRY", "NO ENTRY"},
    {"EGGMANROBO", "NO ENTRY", "NO ENTRY", "NO ENTRY", "NO ENTRY"},
    {"E101", "NO ENTRY", "NO ENTRY", "NO ENTRY", "NO ENTRY"},
    {"E101 R", "NO ENTRY", "NO ENTRY", "NO ENTRY", "NO ENTRY"},
    {"SD OFFICE", "SD CASINO", "SD GESUI", "SD STATION", "SD HOTEL",
     "SD TWINKLE", "NO ENTRY", "NO ENTRY"},
    {"NO ENTRY", "NO ENTRY", "NO ENTRY", "NO ENTRY", "NO ENTRY", "NO ENTRY",
     "NO ENTRY", "NO ENTRY"},
    {"NO ENTRY", "NO ENTRY", "NO ENTRY", "NO ENTRY", "NO ENTRY", "NO ENTRY",
     "NO ENTRY", "NO ENTRY"},
    {"EC DECK FB", "EC DECK F", "EC DECK B", "EC CAPTAIN", "EC LIVING",
     "EC POOL", "NO ENTRY", "NO ENTRY"},
    {"NO ENTRY", "NO ENTRY", "NO ENTRY", "NO ENTRY", "NO ENTRY", "NO ENTRY",
     "NO ENTRY", "NO ENTRY"},
    {"NO ENTRY", "NO ENTRY", "NO ENTRY", "NO ENTRY", "NO ENTRY", "NO ENTRY",
     "NO ENTRY", "NO ENTRY"},
    {"EC BOOSTER", "EC HALL", "EC MOGURA", "EC PRISON", "EC TYOSUI",
     "EC KAKUSHI", "NO ENTRY", "NO ENTRY"},
    {"MR STATION", "MR ISLAND", "MR JUNGLE", "MR BASE"},
    {"PAST KNUCK", "PAST CHAOS", "PAST FIRE"},
    {"CART", "NO", "NO", "NO", "NO", "NO", "NO", "NO"},
    {"SHOOTING1", "NO", "NO", "NO", "NO", "NO", "NO", "NO"},
    {"SHOOTING2", "NO", "NO", "NO", "NO", "NO", "NO", "NO"},
    {"SUNABOARD", "NO", "NO", "NO", "NO", "NO", "NO", "NO"},
    {"ALIFE SS", "NO", "NO", "NO", "NO", "NO", "NO", "NO"},
    {"ALIFE EG", "NO", "NO", "NO", "NO", "NO", "NO", "NO"},
    {"ALIFE MR", "NO", "NO", "NO", "NO", "NO", "NO", "NO"},
    {"ALIFE RACE", "NO", "NO", "NO", "NO", "NO", "NO", "NO"},
};

Bool debug_set_flag(Sint16 on)
{
    if (on) {
        if (debug_flag == 0) {
            debug_flag = -1;
            return TRUE;
        }
    } else if (debug_flag != 0) {
        debug_flag = 0;
        return TRUE;
    }
    return FALSE;
}

void debug_draw_axes(struct task_work *work)
{
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    njDrawLine3D(&debug_axes, 3, NJD_DRAW_NORMAL);
    njPopMatrix(1);
    /* @hack redundant: a return where the function ends anyway. with it, SHC
     * opens the frame with ADD #-4,R15 and stores work in it (0x8c035e0c,
     * 0x8c035e10); without it, one push does both, the function runs 2 bytes
     * short and 20 of 21 words differ */
    return; /* NOLINT(readability-redundant-control-flow) */
}

void debug_draw_line(NJS_POINT3 *pos, NJS_VECTOR *dir, Float length)
{
    NJS_POINT3 points[2];
    NJS_COLOR color;
    NJS_POINT3COL line;

    points[0].x = 0.0F;
    points[0].y = 0.0F;
    points[0].z = 0.0F;
    points[1].x = dir->x * length;
    points[1].y = dir->y * length;
    points[1].z = dir->z * length;
    color.color = ORANGE;
    line.p = points;
    line.col = &color;
    line.tex = NULL;
    line.num = 2;
    njPushMatrix(NULL);
    njTranslate(NULL, pos->x, pos->y, pos->z);
    njDrawLine3D(&line, 1, NJD_DRAW_NORMAL);
    njPopMatrix(1);
}

void debug_name_frame(void)
{
    frame_file_name[5] = hex_digits[(frame_file_count >> 12) & 0xf];
    frame_file_name[6] = hex_digits[(frame_file_count >> 8) & 0xf];
    frame_file_name[7] = hex_digits[(frame_file_count >> 4) & 0xf];
    frame_file_name[8] = hex_digits[frame_file_count & 0xf];
    frame_file_count++;
}

void fun_8c035f18(void)
{
}

static void light_choose(void)
{
}

void debug_print_stage_name(Int loc, Sint16 stage_act)
{
    Uint8 stage;
    Uint8 act;

    stage = STAGE_OF(stage_act);
    act = stage_act & 0xff;
    if (act >= STAGE_ACTS) {
        njPrintC(loc, "ERROR");
    } else if (stage >= STAGE_COUNT) {
        njPrintC(loc, "ERROR");
    } else {
        njPrintC(loc, stage_names[stage][act]);
    }
}
