/* O KAOSE: the Chaos Emerald that ends the stage for Sonic and Tails */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/tree.h"
#include "object/capsule.h"
#include "object/placement.h"
#include "player/player.h"
#include "player/race.h"
#include "progress/progress.h"
#include "stage/stage.h"
#include "stg08/emerald.h"
#include "stg08/emerald_textures.h"
#include "system/task.h"
#include "system/util.h"

extern NJS_OBJECT icecap_emerald_blue_object;
extern NJS_OBJECT icecap_emerald_green_object;
extern NJS_OBJECT icecap_emerald_white_object;

#define GLOW_FRAMES 3
#define GLOW_SIZE 0.35F
#define GLOW_SPRITE                                                            \
    (NJD_SPRITE_ANGLE | NJD_SPRITE_COLOR | NJD_SPRITE_SCALE | NJD_SPRITE_ALPHA)
/* the glow's base brightness, which GLOW_EXTRA adds to, and its pulse's
 * swing either way */
#define GLOW_BASE 0.7F
#define GLOW_PULSE 0.2F
/* the degrees the pulse moves on each step, with the color index added */
#define GLOW_PULSE_RATE 2.0F
/* how far above its position the emerald and its glow are drawn */
#define EMERALD_RISE 10.0F
#define GLOW_RISE 15.0F

/* a player reaches the emerald within this distance times its size */
#define EMERALD_REACH 14.0F

struct emerald_vars {
    /* the event flag set once the character has cleared the stage */
    Sint32 clear_flag;
};

#define VARS(work) ((struct emerald_vars *)&(work)->vars)
/* the glow's pulse, in degrees, in the work's timer */
#define PULSE(work) TASK_TIMER(work)
/* in work->scl: the emerald's size less 1, its color, and the glow's added
 * brightness */
#define EMERALD_SIZE(work) ((work)->scl.x)
#define EMERALD_COLOR(work) ((work)->scl.y)
#define GLOW_EXTRA(work) ((work)->scl.z)

enum emerald_mode {
    EMERALD_INIT = 0,
    EMERALD_WAIT = 2,
    EMERALD_CAPSULE = 3,
    EMERALD_TAKEN = 4
};
/* EMERALD_COLOR's values. any other draws white */
enum emerald_color { EMERALD_BLUE, EMERALD_WHITE, EMERALD_GREEN };

static void icecap_emerald_draw(struct task *task);
static void icecap_emerald_glow_init(void);
static void icecap_emerald_glow(struct task *task);
static void icecap_emerald_draw_reach(struct task *task);
static void icecap_emerald_touch(struct task *task);

/* the glow's textures: the last three of the emeralds' */
static NJS_TEXLIST icecap_emerald_glow_texlist = {
#include "stg08/emerald/icecap_emerald_glow_texlist.inc"
};
static NJS_TEXANIM icecap_emerald_glow_texanims[GLOW_FRAMES] = {
#include "stg08/emerald/icecap_emerald_glow_texanims.inc"
};

static NJS_SPRITE icecap_emerald_glow_sprite;

/* draw the emerald until a player reaches it. once the character has
 * cleared the stage, put a capsule in its place instead */
void icecap_emerald_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task *capsule;
    Uint16 stage;

    if (placement_try_leave(task)) {
        return;
    }
    switch (work->mode) {
    case EMERALD_INIT:
        work->mode = EMERALD_WAIT;
        icecap_emerald_glow_init();
        stage = (Uint8)(stage_get_stage_act() >> 8);
        switch (stage) {
        case STAGE_WINDY:
            if (get_character() == CHARACTER_SONIC) {
                VARS(work)->clear_flag = FLAG_SONIC_CLEAR_WINDY;
            } else {
                VARS(work)->clear_flag = FLAG_TAILS_CLEAR_WINDY;
            }
            break;
        case STAGE_ICECAP:
            if (get_character() == CHARACTER_SONIC) {
                VARS(work)->clear_flag = FLAG_SONIC_CLEAR_ICECAP;
            } else {
                VARS(work)->clear_flag = FLAG_TAILS_CLEAR_ICECAP;
            }
            break;
        case STAGE_CASINO:
            if (get_character() == CHARACTER_SONIC) {
                VARS(work)->clear_flag = FLAG_SONIC_CLEAR_CASINO;
            } else {
                VARS(work)->clear_flag = FLAG_TAILS_CLEAR_CASINO;
            }
            break;
        }
        break;
    case EMERALD_WAIT:
        if (event_flag_check(VARS(work)->clear_flag)) {
            work->mode = EMERALD_CAPSULE;
        } else {
            if (!placement_editing(task)) {
                icecap_emerald_touch(task);
            }
            if (placement_editing(task)) {
                njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_X), "<- SCALE");
                njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_Y), "<- COLOR");
                icecap_emerald_draw_reach(task);
            }
            icecap_emerald_draw(task);
            icecap_emerald_glow(task);
        }
        break;
    case EMERALD_CAPSULE:
        if (capsule_get() == NULL) {
            capsule = task_create(TASK_WORK, 2, capsule_start);
            capsule->work->pos.x = work->pos.x;
            capsule->work->pos.y = work->pos.y;
            capsule->work->pos.z = work->pos.z;
            work->mode = EMERALD_TAKEN;
        }
        break;
    case EMERALD_TAKEN:
        break;
    }
}

static void icecap_emerald_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object;

    njSetTexture(&icecap_emerald_texlist);
    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x, work->pos.y + EMERALD_RISE, work->pos.z);
    TURN(njRotateY, work->ang.y);
    switch ((int)EMERALD_COLOR(work)) {
    case EMERALD_BLUE:
        object = &icecap_emerald_blue_object;
        break;
    case EMERALD_GREEN:
        object = &icecap_emerald_green_object;
        break;
    case EMERALD_WHITE:
    default:
        object = &icecap_emerald_white_object;
        /* the last case ends without a break. with one, draw_tree's and
         * njPopMatrix's addresses trade R3 and R2 (0x0c91004c, 0x0c910058) */
    }
    draw_tree(object);
    njPopMatrix(1);
}

static void icecap_emerald_glow_init(void)
{
    icecap_emerald_glow_sprite.tlist = &icecap_emerald_glow_texlist;
    icecap_emerald_glow_sprite.tanim = icecap_emerald_glow_texanims;
    icecap_emerald_glow_sprite.sx = GLOW_SIZE;
    icecap_emerald_glow_sprite.sy = GLOW_SIZE;
    icecap_emerald_glow_sprite.p.x = 0.0F;
    icecap_emerald_glow_sprite.p.y = 0.0F;
    icecap_emerald_glow_sprite.p.z = 0.0F;
}

static void icecap_emerald_glow(struct task *task)
{
    struct task_work *work = task->work;
    NJS_ARGB color;
    Float bright;
    int frame;

    njSetTexture(&icecap_emerald_glow_texlist);
    color.a = -1.0F;
    bright = (njSin((Angle)(PULSE(work) * ANGLE_PER_DEGREE)) * GLOW_PULSE) +
             GLOW_EXTRA(work) + GLOW_BASE;
    color.r = color.g = color.b = bright;
    njSetConstantMaterial(&color);
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_ONE);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
    PULSE(work) += EMERALD_COLOR(work) + GLOW_PULSE_RATE;
    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x, work->pos.y + GLOW_RISE, work->pos.z);
    njFogDisable();
    frame = (int)EMERALD_COLOR(work);
    if (frame > GLOW_FRAMES - 1) {
        frame = GLOW_FRAMES - 1;
    }
    njDrawSprite3D(&icecap_emerald_glow_sprite, frame, GLOW_SPRITE);
    njPopMatrix(1);
    njFogEnable();
    color.a = 0.0F;
    color.b = 0.0F;
    color.g = 0.0F;
    color.r = 0.0F;
    njSetConstantMaterial(&color);
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_BOTHSRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_BOTHSRCALPHA);
}

/* the set editor's marker, scaled by the emerald's size */
static void icecap_emerald_draw_reach(struct task *task)
{
    struct task_work *work = task->work;
    /* @hack type: size, a vector of which only x is used, which gives the
     * function its 12-byte frame (ADD #-12,R15 at 0x0c9101dc); as one
     * Float, size stays in FR15 and 40 of 41 words differ */
    NJS_VECTOR size;

    size.x = EMERALD_SIZE(work) + 1.0F;
    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
    njScale(NULL, size.x, size.x, size.x);
    draw_tree(&marker_object);
    njPopMatrix(1);
}

/* for Sonic, clear the stage once player 0 is within reach. for any other
 * character, the first of players 0 and 1 within reach clears the stage.
 * race_set_won records player 0's arrival, and race_set_lost player 1's */
static void icecap_emerald_touch(struct task *task)
{
    struct task_work *work = task->work;
    int player;

    if (get_character() == CHARACTER_SONIC) {
        if (player_in_range(&work->pos,
                            (EMERALD_SIZE(work) + 1.0F) * EMERALD_REACH) == 1) {
            stage_clear();
            work->mode = EMERALD_TAKEN;
        }
    } else {
        /* @hack spelling: player assigned in the test, which SHC tests in R0
         * at 0x0c91027a; assigned first, it tests a copy in R4, and 3 of 63
         * words differ */
        if ((player = player_in_range(&work->pos, (EMERALD_SIZE(work) + 1.0F) *
                                                      EMERALD_REACH))) {
            if (player == 1) {
                race_set_won();
            } else {
                race_set_lost();
            }
            stage_clear();
            work->mode = EMERALD_TAKEN;
        }
    }
}
