/* the tornado's gauge: a bar at the bottom of the screen that fills as the
 * chase starts, then follows the tornado's lives down */
#include <ninja.h>
#include <sg_xpt.h>

#include "draw/draw.h"
#include "shooting/gauge.h"
#include "stage/stage.h"
#include "system/task.h"

#define TEXT_1_LENGTH 14
#define TEXT_2_LENGTH 13
#include "shooting/gauge/text.inc"

/* the bar's pieces, as indexes into the sprite's texanims from the cap */
#define PIECE_CAP 0
#define PIECE_FRAME 1
#define PIECE_FRAME_END 2
#define PIECE_FILL 3
#define PIECE_FILL_END 4
#define PIECE_LABEL 5
#define PIECES 6

#define UNIT_SIZE 32 /* the bar's pixels for a unit, which is a life */
#define PIECE_SIZE 16 /* a frame or fill piece's width in pixels */

/* the units the fill moves in a step: a unit a second, at 60 steps a
 * second */
#define FILL_STEP (1.0F / 60.0F)

enum gauge_mode { GAUGE_INIT, GAUGE_FILL, GAUGE_FOLLOW };

/* the count of the sprite's pieces, which nothing reads */
/* NOLINTNEXTLINE(clang-diagnostic-unused-const-variable) */
static const Sint32 gauge_texanim_count = PIECES;

static NJS_TEXNAME gauge_textures[] = {
    {TEXT_1, 0, 0},
    {TEXT_2, 0, 0},
};
NJS_TEXLIST gauge_texlist = {
#include "shooting/gauge/gauge_texlist.inc"
};
/* the bar's pieces, which the sprite reads as one array from the cap, in
 * PIECE_ order */
/* @hack type: six objects, not one array, which keep njDrawSprite2D's
 * address in R11 and PIECE_SIZE in R12 in the draw (0x0c908f8a,
 * 0x0c908fca); as one array, the two trade registers and 15 of 190 words
 * differ */
#ifdef GCC
/* keeps the read the tag names: GCC's six are one array */
static NJS_TEXANIM gauge_pieces[PIECES] = {
    {
#include "shooting/gauge/gauge_cap.inc"
    },
    {
#include "shooting/gauge/gauge_frame.inc"
    },
    {
#include "shooting/gauge/gauge_frame_end.inc"
    },
    {
#include "shooting/gauge/gauge_fill.inc"
    },
    {
#include "shooting/gauge/gauge_fill_end.inc"
    },
    {
#include "shooting/gauge/gauge_label.inc"
    },
};
#define gauge_cap gauge_pieces[PIECE_CAP]
#define gauge_frame_end gauge_pieces[PIECE_FRAME_END]
#define gauge_fill_end gauge_pieces[PIECE_FILL_END]
#else
static NJS_TEXANIM gauge_cap = {
#include "shooting/gauge/gauge_cap.inc"
};
static NJS_TEXANIM gauge_frame = {
#include "shooting/gauge/gauge_frame.inc"
};
static NJS_TEXANIM gauge_frame_end = {
#include "shooting/gauge/gauge_frame_end.inc"
};
static NJS_TEXANIM gauge_fill = {
#include "shooting/gauge/gauge_fill.inc"
};
static NJS_TEXANIM gauge_fill_end = {
#include "shooting/gauge/gauge_fill_end.inc"
};
static NJS_TEXANIM gauge_label = {
#include "shooting/gauge/gauge_label.inc"
};
#endif
static NJS_SPRITE gauge_sprite = {
#include "shooting/gauge/gauge_sprite.inc"
};
/* 92 bytes that no word in this image or 1ST_READ.BIN points into.
 * src/hud/life_gauge.c, which this source copies, keeps the same 92 after
 * its sprite. the first 42 read as six rows of seven Sint8 of 0, 1 or 2.
 * two zero bytes and 12 Floats of 0.0 or 1.0 make up the other 50 */
static Uint8 dat_0cad42e8[92] = {
#include "shooting/gauge/dat_0cad42e8.inc"
};

static Float gauge_filled; /* the fill's length, in units */
static Float gauge_length; /* the frame's length, in units */
static Sint8 gauge_units; /* the bar's full length, in units */
static Sint8 gauge_whole; /* gauge_health's whole part, which nothing reads */
/* the lives the bar follows, in units, which the hud sets each step. SHC
 * lays out bss in the order of each name's first declaration. a declaration
 * in gauge.h would put gauge_health before the statics above, so it has
 * none */
Float gauge_health; /* NOLINT(misc-use-internal-linkage) */

/* draw the label above the left end of the full bar. the frame and the
 * fill then run leftward from the right cap, each for its length. nothing
 * draws while hud_hidden is below 0 */
static void gauge_draw(struct task *task)
{
    /* @hack redundant: register on i, which keeps i in R14 and PIECE_SIZE
     * in R12 (0x0c908fd4, 0x0c908fca); as a plain int, i takes R11 and
     * PIECE_SIZE R13, and 63 of 190 words differ */
    register int i;

    if (hud_hidden < 0) {
        return;
    }
    draw_set_material(1.0F, 1.0F, 1.0F, 1.0F);
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_INVSRCALPHA);
    gauge_sprite.p.x = 264 - (gauge_units * UNIT_SIZE);
    gauge_sprite.p.y = 388.0F;
    njDrawSprite2D(&gauge_sprite, PIECE_LABEL, -3.0F,
                   NJD_SPRITE_ALPHA | NJD_SPRITE_COLOR);
    gauge_sprite.p.x = 276.0F;
    gauge_sprite.p.y = 420.0F;
    njDrawSprite2D(&gauge_sprite, PIECE_CAP, -2.0F,
                   NJD_SPRITE_ALPHA | NJD_SPRITE_COLOR | NJD_SPRITE_HFLIP);
    for (i = (int)(gauge_length * UNIT_SIZE); i >= PIECE_SIZE;
         i -= PIECE_SIZE) {
        gauge_sprite.p.x -= PIECE_SIZE;
        njDrawSprite2D(&gauge_sprite, PIECE_FRAME, -3.0F,
                       NJD_SPRITE_ALPHA | NJD_SPRITE_COLOR);
    }
    if (i > 0) {
        gauge_sprite.p.x -= i;
        gauge_frame_end.sx = i;
        gauge_frame_end.u1 = (28 - i) * 256 / 128;
        njDrawSprite2D(&gauge_sprite, PIECE_FRAME_END, -3.0F,
                       NJD_SPRITE_ALPHA | NJD_SPRITE_COLOR);
    }
    gauge_sprite.p.x -= 12.0F;
    njDrawSprite2D(&gauge_sprite, PIECE_CAP, -3.0F,
                   NJD_SPRITE_ALPHA | NJD_SPRITE_COLOR);
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_ONE);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
    gauge_sprite.p.x = 276.0F;
    gauge_sprite.p.y = 427.0F;
    for (i = (int)(gauge_filled * UNIT_SIZE); i >= PIECE_SIZE;
         i -= PIECE_SIZE) {
        gauge_sprite.p.x -= PIECE_SIZE;
        njDrawSprite2D(&gauge_sprite, PIECE_FILL, -2.0F,
                       NJD_SPRITE_ALPHA | NJD_SPRITE_COLOR);
    }
    if (i > 0) {
        gauge_sprite.p.x -= i;
        gauge_fill_end.sx = i;
        gauge_fill_end.u1 = (16 - i) * 256 / 128;
        njDrawSprite2D(&gauge_sprite, PIECE_FILL_END, -2.0F,
                       NJD_SPRITE_ALPHA | NJD_SPRITE_COLOR);
    }
}

/* the gauge has nothing of its own to free */
static void gauge_destroy(struct task *task)
{
}

/* fill the bar from empty by FILL_STEP a step. the fill then follows the
 * lives, down by at most FILL_STEP a step and up at once. once the lives
 * are 0 or less, the frame's length is the fill times the units. the frame
 * then shrinks with the fill, and the gauge destroys itself when the fill is
 * empty */
static void gauge_step(struct task *task)
{
    struct task_work *work = task->work;

    gauge_whole = gauge_health;
    switch (work->mode) {
    case GAUGE_INIT:
        gauge_filled = 0.0F;
        gauge_length = gauge_units;
        task->draw = gauge_draw;
        task->destructor = gauge_destroy;
        work->mode = GAUGE_FILL;
        /* fall through */
    case GAUGE_FILL:
        gauge_filled = gauge_units < gauge_filled + FILL_STEP
                           ? gauge_units
                           : gauge_filled + FILL_STEP;
        if ((int)gauge_filled == gauge_units) {
            work->mode = GAUGE_FOLLOW;
        }
        break;
    case GAUGE_FOLLOW:
        gauge_filled = gauge_health > gauge_filled - FILL_STEP
                           ? gauge_health
                           : gauge_filled - FILL_STEP;
        if (gauge_health <= 0.0F) {
            gauge_length = gauge_units * gauge_filled;
            if (gauge_filled <= 0.0F) {
                task_destroy(task);
                return;
            }
        }
        break;
    }
    gauge_draw(task);
}

void gauge_create(int x, int y, int units)
{
    struct task *task = task_create(TASK_WORK, 6, gauge_step);
    struct task_work *work = task->work;

    if (x > 0) {
        work->pos.x = x;
        work->pos.y = y;
    } else {
        work->pos.x = 288.0F;
        work->pos.y = 420.0F;
    }
    gauge_whole = gauge_units = units;
    gauge_health = units;
}

void gauge_create_child(int x, int y, int units, struct task *parent)
{
    struct task *task = task_create_child(TASK_WORK, gauge_step, parent);
    struct task_work *work = task->work;

    if (x > 0) {
        work->pos.x = x;
        work->pos.y = y;
    } else {
        work->pos.x = 288.0F;
        work->pos.y = 420.0F;
    }
    gauge_whole = gauge_units = units;
    gauge_health = units;
}
