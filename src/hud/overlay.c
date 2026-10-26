/* overlays: the mission card, a demo's press start, and the pause menu */
#include <ninja.h>
#include <sg_xpt.h>

#include "hud/overlay.h"
#include "main.h"
#include "object/common.h"
#include "player/player.h"
#include "progress/demo.h"
#include "progress/emblem.h"
#include "save/card.h"
#include "save/save.h"
#include "stage/stage.h"
#include "system/task.h"
#include "system/texlists.h" /* NOLINT(misc-include-cleaner) */

#define TEXT_1_LENGTH 13
#define TEXT_2_LENGTH 17
#define TEXT_3_LENGTH 14
#define TEXT_4_LENGTH 13
#define TEXT_5_LENGTH 19
#define TEXT_6_LENGTH 19
#define TEXT_7_LENGTH 14
#define TEXT_8_LENGTH 16
#define TEXT_9_LENGTH 14
#define TEXT_10_LENGTH 17
#define TEXT_11_LENGTH 17
#define TEXT_12_LENGTH 14
#define TEXT_13_LENGTH 13
#define TEXT_14_LENGTH 14
#define TEXT_15_LENGTH 14
#define TEXT_16_LENGTH 16
#define TEXT_17_LENGTH 14
#define TEXT_18_LENGTH 14
#define TEXT_19_LENGTH 17
#define TEXT_20_LENGTH 19
#define TEXT_21_LENGTH 17
#define TEXT_22_LENGTH 15
#define TEXT_23_LENGTH 14
#define TEXT_24_LENGTH 15
#define TEXT_25_LENGTH 14
#define TEXT_26_LENGTH 17
#define TEXT_27_LENGTH 19
#define TEXT_28_LENGTH 17
#define TEXT_29_LENGTH 14
#define TEXT_30_LENGTH 17
#define TEXT_31_LENGTH 17
#define TEXT_32_LENGTH 16
#define TEXT_33_LENGTH 14
#define TEXT_34_LENGTH 14
#define TEXT_35_LENGTH 14
#define TEXT_36_LENGTH 14
#define TEXT_37_LENGTH 14
#define TEXT_38_LENGTH 13
#define TEXT_39_LENGTH 14
#define TEXT_40_LENGTH 14
#define TEXT_41_LENGTH 13
#define TEXT_42_LENGTH 14
#define TEXT_43_LENGTH 14
#define TEXT_44_LENGTH 14
#define TEXT_45_LENGTH 15
#define TEXT_46_LENGTH 14
#define TEXT_47_LENGTH 15
#define TEXT_48_LENGTH 14
#define TEXT_49_LENGTH 14
#define TEXT_50_LENGTH 14
#define TEXT_51_LENGTH 14
#include "hud/overlay/text.inc"

/* the cards, by their textures' names */
enum card {
    CARD_S_BOX,
    CARD_S_RINGBOX,
    CARD_S_3MIN,
    CARD_T_BOX,
    CARD_T_RINGSONIC,
    CARD_T_FASTSONIC,
    CARD_K_3EME,
    CARD_K_NOHINT,
    CARD_K_1MIN,
    CARD_A_BALZERO,
    CARD_A_BALRING,
    CARD_A_ATWI,
    CARD_G_103,
    CARD_G_BWIN,
    CARD_G_AWIN,
    CARD_BIG_FROG,
    CARD_BIG_1K,
    CARD_BIG_2K,
    CARD_S_EMEWIND,
    CARD_S_EMECASINO,
    CARD_S_EMESNOW,
    CARD_S_TAILS,
    CARD_S_EGGC,
    CARD_S_ISEKI,
    CARD_S_FEGG,
    CARD_T_EMEWIND,
    CARD_T_EMECASINO,
    CARD_T_EMESNOW,
    CARD_T_MISS,
    CARD_T_RINGEGG,
    CARD_T_FASTEGG,
    CARD_G_SONICD,
    CARD_G_BFIN,
    CARD_G_AFIN,
    CARD_G_FROG,
    CARD_G_BEME,
    CARD_G_AEME,
    CARD_G_104,
    CARD_G_BRED,
    CARD_G_ARED,
    CARD_G_105,
    CARD_G_BHOT,
    CARD_G_AHOT,
    CARD_S_2MIN,
    CARD_S_2MINH,
    CARD_S_4MIN,
    CARD_S_4MINH,
    CARD_S_5MIN,
    CARD_K_2MIN,
    CARD_A_AHOT,
    CARD_A_AFIN,
    CARD_COUNT
};

enum card_mode { CARD_INIT, CARD_WIDEN, CARD_GROW, CARD_HOLD, CARD_FADE };

/* the press start's angle turns a sixty-fourth of a turn a step */
#define PRESS_START_TURN 1024

/* the task's work as the card and the press start read it. x and y are
 * struct task_work's pos.x and pos.y, and scale_x, scale_y and alpha its
 * scl.x, scl.y and scl.z */
struct card_work {
    Sint8 mode;
    Uint8 unk_01[5];
    Uint16 timer; /* the card's hold in steps, or the press start's angle */
    Uint8 unk_08;
    Uint8 character; /* the card's, which nothing reads */
    Uint8 unk_0a[22];
    Float x;
    Float y;
    Uint8 unk_28[4];
    Float scale_x;
    Float scale_y;
    Float alpha;
};

/* task's work as a card_work */
#define WORK(task) ((struct card_work *)(task)->work)

/* set the constant material to a, r, g and b, through an NJS_ARGB of the
 * caller's own */
/* @hack spelling: a macro where a function would do, so that the image
 * holds each use's initializer apart. a function in its place fails the
 * build's data step; as a function, count unmeasured */
#define MATERIAL_SET(a, r, g, b)                                               \
    do {                                                                       \
        NJS_ARGB material = {(a), (r), (g), (b)};                              \
                                                                               \
        njSetConstantMaterial(&material);                                      \
    } while (0)

/* a sprite of the pause menu, which pause_draw_sprite draws */
struct pause_sprite {
    Sint32 half_w, half_h; /* in pixels, at a scale of 1 */
    Uint32 col[4]; /* each corner's color, as CORNER numbers the corners */
    Uint32 texture; /* its texture's global index */
};

/* the pause menu's sprites, by their textures in pause_texlist, whose names
 * follow */
enum pause_part {
    PART_PLATE, /* nbutton_ita: a choice's plate, under its word */
    PART_SHADOW, /* nbutton_kage: a plate's shadow */
    PART_FRAME, /* nbutton_waku: the frame round the choice under the cursor */
    PART_BANNER, /* npa_pause: the banner above the choices */
    PART_RESTART, /* npa_restart: restart's word */
    PART_CONTINUE, /* npa_saikai: continue's word */
    PART_QUIT, /* npa_stop: quit's word */
    PART_CORNER, /* nss_b_kado: the back's corner tiles */
    PART_MIDDLE, /* nss_b_naka: the back's other tiles */
    PART_CORNER_SHADOW, /* kado_shado: every tile's shadow */
    PART_SIDE_SHADOW, /* yoko_shado, which nothing draws */
    PART_COUNT
};

/* the back's tiles: BACK_COLUMNS by BACK_ROWS, each TILE_SIZE pixels
 * square */
/* each shadow lies this many pixels down and right of what casts it */
#define SHADOW_OFFSET 8.0F
#define BACK_COLUMNS 16
#define BACK_ROWS 12
#define TILE_SIZE 16
/* the column and the row of the tile centered on the screen's middle */
#define MIDDLE_COLUMN 8
#define MIDDLE_ROW 6

/* a tile of the back, by column and row */
/* @hack type: x and y in one struct, which pause_draw_back keeps side by
 * side at the top of its frame and reaches through R14 (0x8c015adc to
 * 0x8c015ae4); as two Sint16 locals, 542 of 566 words differ */
struct tile {
    Sint16 x, y;
};

/* set corner i of quad from pause_sprites[part], around pos and at its
 * size times scale. corners 0 and 1 run down the sprite's left side, and 2
 * and 3 down its right */
/* @hack spelling: a switch on i, though i is a constant. each corner loads
 * i into R0 and tests it against every case (0x8c015816 to 0x8c015832 for
 * corner 1); as each corner's own statements, 423 of 426 words differ */
/* @hack redundant: a loop of one pass, in which each corner forms
 * pause_sprites[part]'s address once, in R13 (0x8c01581c and 0x8c015826
 * for corner 1); in a plain block, overlay.c's code runs 96 bytes longer
 * and 415 of 426 words differ */
#define CORNER(quad, i, part, pos, scale)                                      \
    do {                                                                       \
        switch (i) {                                                           \
        case 0:                                                                \
            (quad)[i].x =                                                      \
                (pos)->x - (pause_sprites[part].half_w * (scale)->x);          \
            (quad)[i].y =                                                      \
                (pos)->y - (pause_sprites[part].half_h * (scale)->y);          \
            (quad)[i].u = 0.0F;                                                \
            (quad)[i].v = 0.0F;                                                \
            break;                                                             \
        case 1:                                                                \
            (quad)[i].x =                                                      \
                (pos)->x - (pause_sprites[part].half_w * (scale)->x);          \
            (quad)[i].y =                                                      \
                (pos)->y + (pause_sprites[part].half_h * (scale)->y);          \
            (quad)[i].u = 0.0F;                                                \
            (quad)[i].v = 1.0F;                                                \
            break;                                                             \
        case 2:                                                                \
            (quad)[i].x =                                                      \
                (pos)->x + (pause_sprites[part].half_w * (scale)->x);          \
            (quad)[i].y =                                                      \
                (pos)->y - (pause_sprites[part].half_h * (scale)->y);          \
            (quad)[i].u = 1.0F;                                                \
            (quad)[i].v = 0.0F;                                                \
            break;                                                             \
        case 3:                                                                \
            (quad)[i].x =                                                      \
                (pos)->x + (pause_sprites[part].half_w * (scale)->x);          \
            (quad)[i].y =                                                      \
                (pos)->y + (pause_sprites[part].half_h * (scale)->y);          \
            (quad)[i].u = 1.0F;                                                \
            (quad)[i].v = 1.0F;                                                \
            break;                                                             \
        }                                                                      \
        (quad)[i].z = (pos)->z;                                                \
        (quad)[i].col = pause_sprites[part].col[i];                            \
    } while (0)

/* copy a color's bits into a sprite corner's Uint32 */
/* @hack redundant: a loop of one pass round each copy, which gives the
 * address of each copy's color a frame slot of its own: twenty slots of the
 * image's 136-byte frame (0x8c015ad8 to 0x8c015b50). as plain statements,
 * the frame is 96 bytes and 540 of 566 words differ */
#define COLOR_COPY(to, from)                                                   \
    do {                                                                       \
        (to) = *(Uint32 *)&(from);                                             \
    } while (0)

/* set part's corner colors: top at its top corners, and bottom at its
 * bottom ones */
#define SPRITE_COLORS(part, top, bottom)                                       \
    COLOR_COPY(pause_sprites[part].col[0], top);                               \
    COLOR_COPY(pause_sprites[part].col[1], bottom);                            \
    COLOR_COPY(pause_sprites[part].col[2], top);                               \
    COLOR_COPY(pause_sprites[part].col[3], bottom)

/* set color to the back's color share of the way down it, from
 * pause_back_colors[0] at its top to pause_back_colors[1] at its bottom */
/* @hack redundant: a loop of one pass round the components, in which each
 * loads pause_back_colors[0]'s address from the frame again (0x8c015b74 to
 * 0x8c015be8); as plain statements, which keep the address in a register,
 * 466 of 566 words differ */
#define BACK_COLOR(color, share)                                               \
    do {                                                                       \
        (color).a =                                                            \
            pause_back_colors[0].a +                                           \
            ((pause_back_colors[1].a - pause_back_colors[0].a) * (share));     \
        (color).r =                                                            \
            pause_back_colors[0].r +                                           \
            ((pause_back_colors[1].r - pause_back_colors[0].r) * (share));     \
        (color).g =                                                            \
            pause_back_colors[0].g +                                           \
            ((pause_back_colors[1].g - pause_back_colors[0].g) * (share));     \
        (color).b =                                                            \
            pause_back_colors[0].b +                                           \
            ((pause_back_colors[1].b - pause_back_colors[0].b) * (share));     \
    } while (0)

static NJS_TEXNAME mission_card_textures[CARD_COUNT] = {
    {TEXT_1, 0, 0},  {TEXT_2, 0, 0},  {TEXT_3, 0, 0},  {TEXT_4, 0, 0},
    {TEXT_5, 0, 0},  {TEXT_6, 0, 0},  {TEXT_7, 0, 0},  {TEXT_8, 0, 0},
    {TEXT_9, 0, 0},  {TEXT_10, 0, 0}, {TEXT_11, 0, 0}, {TEXT_12, 0, 0},
    {TEXT_13, 0, 0}, {TEXT_14, 0, 0}, {TEXT_15, 0, 0}, {TEXT_16, 0, 0},
    {TEXT_17, 0, 0}, {TEXT_18, 0, 0}, {TEXT_19, 0, 0}, {TEXT_20, 0, 0},
    {TEXT_21, 0, 0}, {TEXT_22, 0, 0}, {TEXT_23, 0, 0}, {TEXT_24, 0, 0},
    {TEXT_25, 0, 0}, {TEXT_26, 0, 0}, {TEXT_27, 0, 0}, {TEXT_28, 0, 0},
    {TEXT_29, 0, 0}, {TEXT_30, 0, 0}, {TEXT_31, 0, 0}, {TEXT_32, 0, 0},
    {TEXT_33, 0, 0}, {TEXT_34, 0, 0}, {TEXT_35, 0, 0}, {TEXT_36, 0, 0},
    {TEXT_37, 0, 0}, {TEXT_38, 0, 0}, {TEXT_39, 0, 0}, {TEXT_40, 0, 0},
    {TEXT_41, 0, 0}, {TEXT_42, 0, 0}, {TEXT_43, 0, 0}, {TEXT_44, 0, 0},
    {TEXT_45, 0, 0}, {TEXT_46, 0, 0}, {TEXT_47, 0, 0}, {TEXT_48, 0, 0},
    {TEXT_49, 0, 0}, {TEXT_50, 0, 0}, {TEXT_51, 0, 0},
};

static NJS_TEXANIM mission_card_texanim = {
#include "hud/overlay/mission_card_texanim.inc"
};
static NJS_TEXANIM press_start_texanim = {
#include "hud/overlay/press_start_texanim.inc"
};

static NJS_TEXLIST pause_texlist = {
#include "hud/overlay/pause_texlist.inc"
};

/* indexed by pause_part. pause_draw_back sets the colors of the tiles and
 * their shadows at each draw */
static struct pause_sprite pause_sprites[PART_COUNT] = {
    {64, 16, {0xff12b4ff, 0xff12b4ff, 0xff12b4ff, 0xff12b4ff}, 0x594b1},
    {64, 16, {0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff}, 0x594b2},
    {64, 16, {0xffec9800, 0xffffeb00, 0xffec9800, 0xffffeb00}, 0x594b0},
    {128, 32, {0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff}, 0x594af},
    {64, 16, {0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff}, 0x594b5},
    {64, 16, {0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff}, 0x594b4},
    {64, 16, {0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff}, 0x594b3},
    {8, 8, {0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff}, 0x594ad},
    {8, 8, {0xa0000000, 0xa0000000, 0xa0000000, 0xa0000000}, 0x594ae},
    {8, 8, {0xa0000000, 0xa0000000, 0xa0000000, 0xa0000000}, 0x594cc},
    {8, 8, {0xa0000000, 0xa0000000, 0xa0000000, 0xa0000000}, 0x594cd},
};

/* the back's colors at its top and at its bottom */
static NJS_BGRA pause_back_colors[2] = {
#include "hud/overlay/pause_back_colors.inc"
};

/* the same two colors again, which nothing reads */
static NJS_BGRA pause_back_colors_copy[2] = {
#include "hud/overlay/pause_back_colors_copy.inc"
};

static NJS_POINT3 pause_banner_pos = {312.0F, 144.0F, 11.0F};
static NJS_POINT2 pause_banner_scale = {1.0F, 1.0F};

/* the card's one texture, which each create function points at an entry
 * of mission_card_textures */
static NJS_TEXLIST mission_card_texlist;

static void pause_draw_sprite(enum pause_part part, NJS_POINT3 *pos,
                              NJS_POINT2 *scale);
static void pause_draw_back(void);
static void pause_draw_banner(void);
static void pause_draw_choice(enum pause_part part, int y, Bool chosen,
                              Float push);

/* the card of the next mission the character has to win in stage: C, then
 * B, then A. returns -1 once the save holds A's emblem */
static int mission_card_find(int character, int stage)
{
    int card;

    if (emblem_check_mission(&saves[save_slot], character, stage, MISSION_A)) {
        return -1;
    }
    switch (character) {
    default: /* Eggman and Tikal get Sonic's cards */
    case CHARACTER_SONIC:
        if (emblem_check_mission(&saves[save_slot], character, stage,
                                 MISSION_B)) {
            switch (stage) {
            case STAGE_BEACH:
                card = CARD_S_2MIN;
                break;
            case STAGE_SKY_DECK:
            case STAGE_CASINO:
                card = CARD_S_5MIN;
                break;
            case STAGE_ICECAP:
                card = CARD_S_4MIN;
                break;
            case STAGE_HIGHWAY:
                card = CARD_S_2MINH;
                break;
            case STAGE_RUIN:
            case STAGE_FINAL:
                card = CARD_S_4MINH;
                break;
            default:
                card = CARD_S_3MIN;
                break;
            }
        } else if (emblem_check_mission(&saves[save_slot], character, stage,
                                        MISSION_C)) {
            card = CARD_S_RINGBOX;
        } else {
            switch (stage) {
            case STAGE_BEACH:
                card = CARD_S_TAILS;
                break;
            case STAGE_WINDY:
                card = CARD_S_EMEWIND;
                break;
            case STAGE_MOUNTAIN:
                card = CARD_S_EGGC;
                break;
            case STAGE_RUIN:
                card = CARD_S_ISEKI;
                break;
            case STAGE_ICECAP:
                card = CARD_S_EMESNOW;
                break;
            case STAGE_CASINO:
                card = CARD_S_EMECASINO;
                break;
            case STAGE_FINAL:
                card = CARD_S_FEGG;
                break;
            default:
                card = CARD_S_BOX;
                break;
            }
        }
        break;
    case CHARACTER_TAILS:
        if (emblem_check_mission(&saves[save_slot], character, stage,
                                 MISSION_B)) {
            if (stage == STAGE_HIGHWAY) {
                card = CARD_T_FASTEGG;
            } else {
                card = CARD_T_FASTSONIC;
            }
        } else if (emblem_check_mission(&saves[save_slot], character, stage,
                                        MISSION_C)) {
            if (stage == STAGE_HIGHWAY) {
                card = CARD_T_RINGEGG;
            } else {
                card = CARD_T_RINGSONIC;
            }
        } else {
            switch (stage) {
            case STAGE_WINDY:
                card = CARD_T_EMEWIND;
                break;
            case STAGE_CASINO:
                card = CARD_T_EMECASINO;
                break;
            case STAGE_ICECAP:
                card = CARD_T_EMESNOW;
                break;
            case STAGE_HIGHWAY:
                card = CARD_T_MISS;
                break;
            default:
                card = CARD_T_BOX;
                break;
            }
        }
        break;
    case CHARACTER_KNUCKLES:
        if (emblem_check_mission(&saves[save_slot], character, stage,
                                 MISSION_B)) {
            if (stage == STAGE_SKY_DECK) {
                card = CARD_K_2MIN;
            } else {
                card = CARD_K_1MIN;
            }
        } else if (emblem_check_mission(&saves[save_slot], character, stage,
                                        MISSION_C)) {
            card = CARD_K_NOHINT;
        } else {
            card = CARD_K_3EME;
        }
        break;
    case CHARACTER_AMY:
        if (emblem_check_mission(&saves[save_slot], character, stage,
                                 MISSION_B)) {
            switch (stage) {
            case STAGE_SHELTER:
                card = CARD_A_AHOT;
                break;
            case STAGE_FINAL:
                card = CARD_A_AFIN;
                break;
            default:
                card = CARD_A_ATWI;
                break;
            }
        } else if (emblem_check_mission(&saves[save_slot], character, stage,
                                        MISSION_C)) {
            card = CARD_A_BALRING;
        } else {
            card = CARD_A_BALZERO;
        }
        break;
    case CHARACTER_E102:
        if (emblem_check_mission(&saves[save_slot], character, stage,
                                 MISSION_B)) {
            switch (stage) {
            case STAGE_FINAL:
                card = CARD_G_AFIN;
                break;
            case STAGE_BEACH:
                card = CARD_G_AEME;
                break;
            case STAGE_MOUNTAIN:
                card = CARD_G_ARED;
                break;
            case STAGE_SHELTER:
                card = CARD_G_AHOT;
                break;
            default:
                card = CARD_G_AWIN;
                break;
            }
        } else if (emblem_check_mission(&saves[save_slot], character, stage,
                                        MISSION_C)) {
            switch (stage) {
            case STAGE_FINAL:
                card = CARD_G_BFIN;
                break;
            case STAGE_BEACH:
                card = CARD_G_BEME;
                break;
            case STAGE_MOUNTAIN:
                card = CARD_G_BRED;
                break;
            case STAGE_SHELTER:
                card = CARD_G_BHOT;
                break;
            default:
                card = CARD_G_BWIN;
                break;
            }
        } else {
            switch (stage) {
            case STAGE_FINAL:
                card = CARD_G_SONICD;
                break;
            case STAGE_BEACH:
                card = CARD_G_FROG;
                break;
            case STAGE_MOUNTAIN:
                card = CARD_G_104;
                break;
            case STAGE_SHELTER:
                card = CARD_G_105;
                break;
            default:
                card = CARD_G_103;
                break;
            }
        }
        break;
    case CHARACTER_BIG:
        if (emblem_check_mission(&saves[save_slot], character, stage,
                                 MISSION_B)) {
            card = CARD_BIG_2K;
        } else if (emblem_check_mission(&saves[save_slot], character, stage,
                                        MISSION_C)) {
            card = CARD_BIG_1K;
        } else {
            card = CARD_BIG_FROG;
        }
        break;
    }
    return card;
}

/* widen the card over 20 steps, grow it tall over 20, hold it for its
 * timer's steps, then fade it out over 40. while hud_hidden is below 0, the
 * card neither steps nor draws */
static void mission_card_step(struct task *task)
{
    struct card_work *work;
    NJS_SPRITE sprite;
    NJS_ARGB color;

    work = WORK(task);
    if (hud_hidden < 0) {
        return;
    }
    switch (work->mode) {
    case CARD_INIT:
        work->scale_x = 0.0F;
        work->scale_y = 0.0F;
        work->alpha = 1.0F;
        work->mode = CARD_WIDEN;
        break;
    case CARD_WIDEN:
        work->scale_x += 0.05F;
        if (work->scale_x >= 1.0F) {
            work->scale_x = 1.0F;
            work->mode = CARD_GROW;
        }
        break;
    case CARD_GROW:
        work->scale_y += 0.05F;
        if (work->scale_y >= 1.0F) {
            work->scale_y = 1.0F;
            work->mode = CARD_HOLD;
        }
        break;
    case CARD_HOLD:
        /* @hack spelling: task->work read again at each use in the hold, where
         * the rest of the step reads work (0x8c01523e, 0x8c015246 and
         * 0x8c015250); through work, overlay.c's code runs 32 bytes shorter
         * and 97 of 157 words differ */
        WORK(task)->timer--;
        if (WORK(task)->timer <= 0) {
            WORK(task)->mode = CARD_FADE;
        }
        break;
    case CARD_FADE:
        work->alpha -= 0.025F;
        if (work->alpha <= 0.0F) {
            task_kill(task);
            return;
        }
    }
    sprite.tlist = &mission_card_texlist;
    sprite.tanim = &mission_card_texanim;
    sprite.sx = work->scale_x;
    sprite.sy = work->scale_y;
    sprite.p.x = work->x;
    sprite.p.y = work->y;
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_INVSRCALPHA);
    njSetTexture(&mission_card_texlist);
    color.a = work->alpha;
    color.r = 1.0F;
    color.g = 1.0F;
    color.b = 1.0F;
    njSetConstantMaterial(&color);
    njDrawSprite2D(&sprite, 0, 0.0F, NJD_SPRITE_COLOR | NJD_SPRITE_ALPHA);
    color.a = 0.0F;
    color.r = 0.0F;
    color.g = 0.0F;
    color.b = 0.0F;
    njSetConstantMaterial(&color);
}

static void mission_card_destroy(struct task *task)
{
    njReleaseTexture(&mission_card_texlist);
}

void mission_card_create(void)
{
    int stage;
    int character;
    int card;
    struct task *task;

    stage = stage_get_stage_act() >> 8;
    character = get_character();
    if (stage <= STAGE_PRACTICE || stage > STAGE_ACTION_LAST) {
        return;
    }
    if (stage == STAGE_ICECAP && game_mode == GAME_MODE_TRIAL &&
        menu_state.trial[0] == MENU_TRIAL_MINI_GAME) {
        return;
    }
    if (stage == STAGE_MOUNTAIN && character == CHARACTER_TAILS) {
        return;
    }
    if (stage == STAGE_BEACH && character == CHARACTER_TAILS) {
        return;
    }
    card = mission_card_find(character, stage);
    if (card < 0) {
        return;
    }
    mission_card_texlist.textures = &mission_card_textures[card];
    mission_card_texlist.nbTexture = 1;
    njLoadTexture(&mission_card_texlist);
    task = task_create(TASK_WORK, 1, mission_card_step);
    /* @hack spelling: task->work read again for each store, not kept in a local
     * (0x8c0153e4 to 0x8c0153fc); through a local, 31 of 91 words differ */
    WORK(task)->mode = CARD_INIT;
    WORK(task)->character = character;
    WORK(task)->timer = 120;
    WORK(task)->x = SCREEN_MIDDLE_X;
    WORK(task)->y = 360.0F;
    task->destructor = mission_card_destroy;
}

void mission_card_create_score(void)
{
    int stage;
    int character;
    int card;
    struct task *task;

    stage = stage_get_stage_act() >> 8;
    character = get_character();
    if (stage == STAGE_PRACTICE || stage > STAGE_ACTION_LAST) {
        return;
    }
    card = mission_card_find(character, stage);
    if (card < 0) {
        return;
    }
    mission_card_texlist.textures = &mission_card_textures[card];
    mission_card_texlist.nbTexture = 1;
    njLoadTexture(&mission_card_texlist);
    task = task_create(TASK_WORK, 1, mission_card_step);
    /* @hack spelling: task->work read again for each store, not kept in a local
     * (0x8c01546a to 0x8c01548c); through a local, 28 of 67 words differ */
    WORK(task)->mode = CARD_INIT;
    WORK(task)->character = character;
    WORK(task)->timer = 60;
    WORK(task)->x = SCREEN_MIDDLE_X;
    WORK(task)->y = SCREEN_MIDDLE_Y;
    task->destructor = mission_card_destroy;
}

void overlay_stage_clear(void)
{
}

void press_start_step(struct task *task)
{
    struct card_work *work;
    NJS_SPRITE sprite;
    NJS_ARGB color;

    work = WORK(task);
    sprite.tlist = &press_start_texlist;
    sprite.tanim = &press_start_texanim;
    sprite.sx = 1.0F;
    sprite.sy = 1.0F;
    sprite.p.x = 496.0F;
    sprite.p.y = 48.0F;
    work->timer += PRESS_START_TURN;
    work->alpha = (njSin(work->timer) * 0.5F) + 0.5F;
    njSetTexture(&press_start_texlist);
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_INVSRCALPHA);
    color.a = work->alpha;
    color.r = 1.0F;
    color.g = 1.0F;
    color.b = 1.0F;
    njSetConstantMaterial(&color);
    njDrawSprite2D(&sprite, 0, -1.1F, NJD_SPRITE_COLOR | NJD_SPRITE_ALPHA);
    color.a = 0.0F;
    color.r = 0.0F;
    color.g = 0.0F;
    color.b = 0.0F;
    njSetConstantMaterial(&color);
}

void press_start_destroy(struct task *task)
{
    njReleaseTexture(&press_start_texlist);
}

void pause_draw(enum pause_choice choice)
{
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_INVSRCALPHA);
    njSetTexture(&pause_texlist);
    pause_draw_back();
    pause_draw_banner();
    pause_draw_choice(PART_CONTINUE, -48, choice == PAUSE_CONTINUE, 0.0F);
    pause_draw_choice(PART_RESTART, 0, choice == PAUSE_RESTART, 0.0F);
    pause_draw_choice(PART_QUIT, 48, choice == PAUSE_QUIT, 0.0F);
    MATERIAL_SET(0.0F, 0.0F, 0.0F, 0.0F);
}

void pause_draw_no_restart(int choice)
{
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_INVSRCALPHA);
    njSetTexture(&pause_texlist);
    pause_draw_back();
    pause_draw_banner();
    pause_draw_choice(PART_CONTINUE, -32, choice == 0, 0.0F);
    pause_draw_choice(PART_QUIT, 32, choice == 1, 0.0F);
    MATERIAL_SET(0.0F, 0.0F, 0.0F, 0.0F);
}

void pause_draw_no_choices(void)
{
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_INVSRCALPHA);
    njSetTexture(&pause_texlist);
    pause_draw_back();
    pause_draw_banner();
    MATERIAL_SET(0.0F, 0.0F, 0.0F, 0.0F);
}

/* draw part's sprite with its middle at pos, at its size times scale */
static void pause_draw_sprite(enum pause_part part, NJS_POINT3 *pos,
                              NJS_POINT2 *scale)
{
    NJS_TEXTURE_VTX quad[4];

    CORNER(quad, 0, part, pos, scale);
    CORNER(quad, 1, part, pos, scale);
    CORNER(quad, 2, part, pos, scale);
    CORNER(quad, 3, part, pos, scale);
    njDrawTexture(quad, 4, pause_sprites[part].texture, 1);
}

/* draw the back's tiles around (312, 232). BACK_COLOR gives each row its
 * colors at its top and at its bottom. each tile casts a shadow, and each
 * corner tile flips to face its corner */
static void pause_draw_back(void)
{
    struct tile tile;
    /* @bug uninitialized: no path writes color[2] or color[3], but each
     * shadow reads them, first by MOV.L @R11,R3 at 0x8c015d76 and MOV.L
     * @R10,R2 at 0x8c015d7a */
    NJS_BGRA color[4]; /* the row's top and bottom colors, then the shadows' */
    NJS_POINT3 pos;
    NJS_POINT2 scale;
    Float share;

    for (tile.y = 0; tile.y < BACK_ROWS; tile.y++) {
        share = tile.y * (1.0F / BACK_ROWS);
        BACK_COLOR(color[0], share);
        share = (tile.y + 1) * (1.0F / BACK_ROWS);
        BACK_COLOR(color[1], share);
        for (tile.x = 0; tile.x < BACK_COLUMNS; tile.x++) {
            pos.x = ((tile.x - MIDDLE_COLUMN) * TILE_SIZE) + 320;
            pos.y = ((tile.y - MIDDLE_ROW) * TILE_SIZE) + 240;
            pos.z = 10.0F;
            if (tile.x == 0 && tile.y == 0) {
                scale.x = 1.0F;
                scale.y = 1.0F;
                SPRITE_COLORS(PART_CORNER, color[0], color[1]);
                pause_draw_sprite(PART_CORNER, &pos, &scale);
                pos.x += SHADOW_OFFSET;
                pos.y += SHADOW_OFFSET;
                pos.z = 9.0F;
                SPRITE_COLORS(PART_CORNER_SHADOW, color[2], color[3]);
                pause_draw_sprite(PART_CORNER_SHADOW, &pos, &scale);
            } else if (tile.x == 0 && tile.y == BACK_ROWS - 1) {
                scale.x = 1.0F;
                scale.y = -1.0F;
                SPRITE_COLORS(PART_CORNER, color[1], color[0]);
                pause_draw_sprite(PART_CORNER, &pos, &scale);
                pos.x += SHADOW_OFFSET;
                pos.y += SHADOW_OFFSET;
                pos.z = 9.0F;
                SPRITE_COLORS(PART_CORNER_SHADOW, color[3], color[2]);
                pause_draw_sprite(PART_CORNER_SHADOW, &pos, &scale);
            } else if (tile.x == BACK_COLUMNS - 1 && tile.y == 0) {
                scale.x = -1.0F;
                scale.y = 1.0F;
                SPRITE_COLORS(PART_CORNER, color[0], color[1]);
                pause_draw_sprite(PART_CORNER, &pos, &scale);
                pos.x += SHADOW_OFFSET;
                pos.y += SHADOW_OFFSET;
                pos.z = 9.0F;
                SPRITE_COLORS(PART_CORNER_SHADOW, color[2], color[3]);
                pause_draw_sprite(PART_CORNER_SHADOW, &pos, &scale);
            } else if (tile.x == BACK_COLUMNS - 1 && tile.y == BACK_ROWS - 1) {
                scale.x = -1.0F;
                scale.y = -1.0F;
                SPRITE_COLORS(PART_CORNER, color[1], color[0]);
                pause_draw_sprite(PART_CORNER, &pos, &scale);
                pos.x += SHADOW_OFFSET;
                pos.y += SHADOW_OFFSET;
                pos.z = 9.0F;
                SPRITE_COLORS(PART_CORNER_SHADOW, color[3], color[2]);
                pause_draw_sprite(PART_CORNER_SHADOW, &pos, &scale);
            } else {
                scale.x = 1.0F;
                scale.y = 1.0F;
                SPRITE_COLORS(PART_MIDDLE, color[0], color[1]);
                pause_draw_sprite(PART_MIDDLE, &pos, &scale);
                pos.x += SHADOW_OFFSET;
                pos.y += SHADOW_OFFSET;
                pos.z = 9.0F;
                /* the side shadow's colors are set, but the corner shadow
                 * is drawn, in the colors the last corner tile gave it */
                SPRITE_COLORS(PART_SIDE_SHADOW, color[2], color[3]);
                pause_draw_sprite(PART_CORNER_SHADOW, &pos, &scale);
            }
        }
    }
}

static void pause_draw_banner(void)
{
    NJS_ARGB color = {1.0F, 1.0F, 1.0F, 1.0F};

    njSetConstantMaterial(&color);
    pause_draw_sprite(PART_BANNER, &pause_banner_pos, &pause_banner_scale);
}

/* draw a choice's word on its plate, y pixels below the screen's middle,
 * with the plate's shadow. chosen marks the choice under the cursor, which
 * is larger and framed. push moves the plate, the word and the frame toward
 * the shadow, and every caller passes 0 */
static void pause_draw_choice(enum pause_part part, int y, Bool chosen,
                              Float push)
{
    NJS_POINT3 pos;
    NJS_POINT2 scale;

    if (chosen) {
        scale.x = 1.5F;
        scale.y = 1.5F;
    } else {
        scale.x = 1.25F;
        scale.y = 1.0F;
    }
    MATERIAL_SET(1.0F, 1.0F, 1.0F, 1.0F);
    pos.x = SCREEN_MIDDLE_X;
    pos.y = y + 240 + SHADOW_OFFSET;
    pos.z = 12.0F;
    pause_draw_sprite(PART_SHADOW, &pos, &scale);
    pos.x = push + 312.0F;
    pos.y = y + 240 + push;
    pos.z = 13.0F;
    pause_draw_sprite(PART_PLATE, &pos, &scale);
    if (chosen) {
        scale.x = 1.5F;
        scale.y = 1.5F;
    } else {
        scale.x = 1.0F;
        scale.y = 1.0F;
    }
    pos.x = push + 312.0F;
    pos.y = y + 240 + push + 2.0F;
    pos.z = 14.0F;
    pause_draw_sprite(part, &pos, &scale);
    if (chosen) {
        pos.x = push + 312.0F;
        pos.y = y + 240 + push;
        pos.z = 15.0F;
        pause_draw_sprite(PART_FRAME, &pos, &scale);
    }
}
