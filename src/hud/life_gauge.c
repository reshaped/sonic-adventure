/* the life gauge: a bar at the top right of the screen that fills as a
 * fight starts, then follows the boss's health down */
#include <ninja.h>
#include <sg_xpt.h>

#include "draw/draw.h"
#include "hud/life_gauge.h"
#include "stage/stage.h"
#include "system/frame.h"
#include "system/task.h"

#define TEXT_1_LENGTH 12
#define TEXT_2_LENGTH 9
#include "hud/life_gauge/text.inc"

/* the bar's pieces, as indexes into the sprite's texanims from the cap */
#define PIECE_CAP 0
#define PIECE_FRAME 1
#define PIECE_FRAME_END 2
#define PIECE_FILL 3
#define PIECE_FILL_END 4
#define PIECE_LABEL 5
#define PIECES 6

#define UNIT_SIZE 32 /* the bar's pixels for a unit of health */
#define PIECE_SIZE 16 /* a frame or fill piece's width in pixels */

/* the units the fill moves in a step: a unit a second, at 60 steps a
 * second */
#define FILL_STEP (1.0F / 60.0F)

enum life_gauge_mode { LIFE_GAUGE_INIT, LIFE_GAUGE_FILL, LIFE_GAUGE_FOLLOW };

/* the sprite's count of pieces. nothing reads it, but the image holds it */
/* NOLINTNEXTLINE(clang-diagnostic-unused-const-variable) */
static const Sint32 life_gauge_texanim_count = PIECES;

static NJS_TEXNAME life_gauge_textures[] = {
    {TEXT_1, 0, 0},
    {TEXT_2, 0, 0},
};
NJS_TEXLIST chaos_lifegauge_texlist = {
#include "hud/life_gauge/chaos_lifegauge_texlist.inc"
};
/* the bar's pieces, which the sprite reads as one array from the cap, in
 * PIECE_ order */
/* @hack type: six objects, not one array, which keep njDrawSprite2D's
 * address in R11 and PIECE_SIZE in R12 in the draw (0x8c0fd396,
 * 0x8c0fd3d6); as one array, the two trade registers and 15 of 194 words
 * differ */
#ifdef GCC
/* keeps the sprite's read from the cap: GCC's six are one array */
static NJS_TEXANIM life_gauge_pieces[PIECES] = {
    {
#include "hud/life_gauge/life_gauge_cap.inc"
    },
    {
#include "hud/life_gauge/life_gauge_frame.inc"
    },
    {
#include "hud/life_gauge/life_gauge_frame_end.inc"
    },
    {
#include "hud/life_gauge/life_gauge_fill.inc"
    },
    {
#include "hud/life_gauge/life_gauge_fill_end.inc"
    },
    {
#include "hud/life_gauge/life_gauge_label.inc"
    },
};
#define life_gauge_cap life_gauge_pieces[PIECE_CAP]
#define life_gauge_frame_end life_gauge_pieces[PIECE_FRAME_END]
#define life_gauge_fill_end life_gauge_pieces[PIECE_FILL_END]
#else
static NJS_TEXANIM life_gauge_cap = {
#include "hud/life_gauge/life_gauge_cap.inc"
};
static NJS_TEXANIM life_gauge_frame = {
#include "hud/life_gauge/life_gauge_frame.inc"
};
static NJS_TEXANIM life_gauge_frame_end = {
#include "hud/life_gauge/life_gauge_frame_end.inc"
};
static NJS_TEXANIM life_gauge_fill = {
#include "hud/life_gauge/life_gauge_fill.inc"
};
static NJS_TEXANIM life_gauge_fill_end = {
#include "hud/life_gauge/life_gauge_fill_end.inc"
};
static NJS_TEXANIM life_gauge_label = {
#include "hud/life_gauge/life_gauge_label.inc"
};
#endif
static NJS_SPRITE life_gauge_sprite = {
#include "hud/life_gauge/life_gauge_sprite.inc"
};
/* 92 bytes that no word in any of the disc's images points into.
 * modules/shooting/gauge.c, a copy of this source, keeps the same 92 after
 * its sprite */
static Uint8 dat_8c5a6be0[92] = {
#include "hud/life_gauge/dat_8c5a6be0.inc"
};

Sint8 life_gauge_whole;
Float life_gauge_health;
static Float life_gauge_filled; /* the fill's length, in units */
static Float life_gauge_length; /* the frame's length, in units */
static Sint8 life_gauge_units; /* the bar's full length, in units */

/* draw the label above the left end of the full bar. the frame and the
 * fill then run leftward from the right cap, each for its length. it draws
 * only on a frame's first step, and not while hud_hidden is below 0 */
static void life_gauge_draw(struct task *task)
{
    /* @hack redundant: register on i, which keeps i in R14 and PIECE_SIZE
     * in R12 (0x8c0fd3de, 0x8c0fd3e0); as a plain int, i takes R11 and
     * PIECE_SIZE R13, and 63 of 194 words differ */
    register int i;

    if (frame_step) {
        return;
    }
    if (hud_hidden < 0) {
        return;
    }
#ifdef GCC
    /* the port's 16:9 squeezes the gauge about the screen's right edge */
    port_wide_anchor = WIDE_RIGHT;
#endif
    draw_set_material(1.0F, 1.0F, 1.0F, 1.0F);
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_INVSRCALPHA);
    life_gauge_sprite.p.x = 576 - (life_gauge_units * UNIT_SIZE);
    life_gauge_sprite.p.y = 16.0F;
    njDrawSprite2D(&life_gauge_sprite, PIECE_LABEL, -3.0F,
                   NJD_SPRITE_ALPHA | NJD_SPRITE_COLOR);
    life_gauge_sprite.p.x = 588.0F;
    life_gauge_sprite.p.y = 48.0F;
    njDrawSprite2D(&life_gauge_sprite, PIECE_CAP, -2.0F,
                   NJD_SPRITE_ALPHA | NJD_SPRITE_COLOR | NJD_SPRITE_HFLIP);
    for (i = (int)(life_gauge_length * UNIT_SIZE); i >= PIECE_SIZE;
         i -= PIECE_SIZE) {
        life_gauge_sprite.p.x -= PIECE_SIZE;
        njDrawSprite2D(&life_gauge_sprite, PIECE_FRAME, -3.0F,
                       NJD_SPRITE_ALPHA | NJD_SPRITE_COLOR);
    }
    if (i > 0) {
        life_gauge_sprite.p.x -= i;
        life_gauge_frame_end.sx = i;
        life_gauge_frame_end.u1 = (28 - i) * 256 / 64;
        njDrawSprite2D(&life_gauge_sprite, PIECE_FRAME_END, -3.0F,
                       NJD_SPRITE_ALPHA | NJD_SPRITE_COLOR);
    }
    life_gauge_sprite.p.x -= 12.0F;
    njDrawSprite2D(&life_gauge_sprite, PIECE_CAP, -3.0F,
                   NJD_SPRITE_ALPHA | NJD_SPRITE_COLOR);
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_ONE);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
    life_gauge_sprite.p.x = 588.0F;
    life_gauge_sprite.p.y = 55.0F;
    for (i = (int)(life_gauge_filled * UNIT_SIZE); i >= PIECE_SIZE;
         i -= PIECE_SIZE) {
        life_gauge_sprite.p.x -= PIECE_SIZE;
        njDrawSprite2D(&life_gauge_sprite, PIECE_FILL, -2.0F,
                       NJD_SPRITE_ALPHA | NJD_SPRITE_COLOR);
    }
    if (i > 0) {
        life_gauge_sprite.p.x -= i;
        life_gauge_fill_end.sx = i;
        life_gauge_fill_end.u1 = (16 - i) * 256 / 64;
        njDrawSprite2D(&life_gauge_sprite, PIECE_FILL_END, -2.0F,
                       NJD_SPRITE_ALPHA | NJD_SPRITE_COLOR);
    }
#ifdef GCC
    port_wide_anchor = WIDE_MIDDLE;
#endif
}

/* the gauge has nothing of its own to free */
static void life_gauge_destroy(struct task *task)
{
}

/* fill the bar from empty by FILL_STEP a step. the fill then follows the
 * health, down by at most FILL_STEP a step and up at once. once the health
 * is 0 or less, the frame shrinks with the fill's last unit to nothing. the
 * gauge destroys itself when the fill is empty */
static void life_gauge_step(struct task *task)
{
    struct task_work *work = task->work;

    life_gauge_whole = life_gauge_health;
    switch (work->mode) {
    case LIFE_GAUGE_INIT:
        life_gauge_filled = 0.0F;
        life_gauge_length = life_gauge_units;
        task->draw = life_gauge_draw;
        task->destructor = life_gauge_destroy;
        work->mode = LIFE_GAUGE_FILL;
        /* fall through */
    case LIFE_GAUGE_FILL:
        life_gauge_filled = life_gauge_units < life_gauge_filled + FILL_STEP
                                ? life_gauge_units
                                : life_gauge_filled + FILL_STEP;
        if ((int)life_gauge_filled == life_gauge_units) {
            work->mode = LIFE_GAUGE_FOLLOW;
        }
        break;
    case LIFE_GAUGE_FOLLOW:
        life_gauge_filled = life_gauge_health > life_gauge_filled - FILL_STEP
                                ? life_gauge_health
                                : life_gauge_filled - FILL_STEP;
        if (life_gauge_health <= 0.0F) {
            life_gauge_length =
                life_gauge_filled * life_gauge_units < life_gauge_units
                    ? life_gauge_filled * life_gauge_units
                    : life_gauge_units;
            if (life_gauge_filled <= 0.0F) {
                task_destroy(task);
                return;
            }
        }
        break;
    }
    life_gauge_draw(task);
}

void life_gauge_create(int x, int y, int units)
{
    struct task *task = task_create(TASK_WORK, 6, life_gauge_step);
    struct task_work *work = task->work;

    if (x > 0) {
        work->pos.x = x;
        work->pos.y = y;
    } else {
        work->pos.x = 600.0F;
        work->pos.y = 48.0F;
    }
    life_gauge_whole = life_gauge_units = units;
    life_gauge_health = units;
}

void life_gauge_create_child(int x, int y, int units, struct task *parent)
{
    struct task *task = task_create_child(TASK_WORK, life_gauge_step, parent);
    struct task_work *work = task->work;

    if (x > 0) {
        work->pos.x = x;
        work->pos.y = y;
    } else {
        work->pos.x = 600.0F;
        work->pos.y = 48.0F;
    }
    life_gauge_whole = life_gauge_units = units;
    life_gauge_health = units;
}
