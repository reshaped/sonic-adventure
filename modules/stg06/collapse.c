/* the collapse of the second act's land. it starts once player 0 is at
 * negative z and high enough. the land at z 0 or more then breaks away
 * behind a front. the front moves toward higher x and y. the land's pieces
 * come loose as it starts. each falls, turning, a while after the front
 * reaches it */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "collision/world.h"
#include "draw/model.h"
#include "draw/tree.h"
#include "player/player.h"
#include "stage/stage.h"
#include "stg06/collapse.h"
#include "stg06/land1.h"
#include "stg06/sky_deck.h"
#include "stg06/skydeck02_textures.h"
#include "system/task.h"
#include "system/util.h"

/* the second act, as stage_get_stage_act counts from 0 */
#define COLLAPSE_ACT 1
/* bits of a land entry's blocks. PIECE_BLOCK marks the entries that come
 * loose. FLOOR_BLOCK marks the floor under them, which stays solid until the
 * collapse starts */
#define PIECE_BLOCK 0x2
#define FLOOR_BLOCK 0x4

/* it starts once player 0's z is negative and its y above START_HEIGHT */
#define START_HEIGHT 100.0F
/* the front stands at FRONT_UNSET until the collapse starts. the start
 * puts it at FRONT_START_X and FRONT_START_Y, and the collapse's timer then
 * counts down from FRONT_WAIT. after that, the front gains FRONT_SPEED in x
 * and in y each step */
#define FRONT_UNSET (-9999.0F)
#define FRONT_START_X (-1000.0F)
#define FRONT_START_Y (-100.0F)
#define FRONT_SPEED 4.0F
#define FRONT_WAIT 120
/* the collapse sets sky_deck_clock to CLOCK_START as it starts */
#define CLOCK_START 360

#define PIECE_VIEW 1000.0F
/* once the front reaches a piece, its timer takes a random count from 1 to
 * PIECE_WAIT. once that runs out, each step it turns PIECE_TURN, falls
 * PIECE_FALL and drifts PIECE_DRIFT along x */
#define PIECE_WAIT 120.0F
#define PIECE_TURN 20
#define PIECE_DRIFT 0.5F
#define PIECE_FALL 0.9F

enum collapse_mode {
    COLLAPSE_INIT = 0,
    COLLAPSE_READY = 1,
    COLLAPSE_WAIT = 2,
    COLLAPSE_SPREAD = 4
};

enum piece_mode {
    PIECE_INIT,
    PIECE_FIXED,
    PIECE_LOOSE,
    PIECE_PASSED,
    PIECE_FALLING
};

struct piece_vars {
    Uint8 unk_00[4];
    struct land_entry *entry;
    NJS_OBJECT *object;
};

#define VARS(work) ((struct piece_vars *)&(work)->vars)
/* a piece's offset from its place in the land as it falls, kept in its
 * work's scl */
#define FALLEN(work) ((work)->scl)

static void collapse_destroy(struct task *task);
static void collapse_start(struct task *task);
static void collapse_create_pieces(struct task *task);
static void collapse_piece_step(struct task *task);
static void collapse_piece_draw(struct task *task);

/* the front. the land at z 0 or more has broken away up to it in x and in
 * y */
static Float collapse_front_x;
static Float collapse_front_y;

void collapse_reset(void)
{
    int i;
    int count = sky_deck_act1_land.count;
    struct land_entry *entry = sky_deck_act1_land.entries;

    for (i = 0; i < count; i++) {
        if (entry->blocks & FLOOR_BLOCK) {
            entry->flags |= WORLD_SOLID;
        }
        entry++;
    }
    collapse_front_x = FRONT_UNSET;
    collapse_front_y = FRONT_UNSET;
}

Bool collapse_reached(Float x, Float y, Float z)
{
    if ((Uint8)stage_get_stage_act() != COLLAPSE_ACT) {
        return FALSE;
    }
    if (z < 0.0F) {
        return FALSE;
    }
    if (x > collapse_front_x) {
        return FALSE;
    }
    if (y > collapse_front_y) {
        return FALSE;
    }
    return TRUE;
}

void collapse_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case COLLAPSE_INIT:
        work->mode = COLLAPSE_READY;
        task->destructor = collapse_destroy;
        collapse_create_pieces(task);
        /* until the collapse starts, the only block the land draws is
         * PIECE_BLOCK */
        world_land_blocks = PIECE_BLOCK;
        break;
    case COLLAPSE_READY:
        if (player_work[0]->pos.z < 0.0F &&
            player_work[0]->pos.y > START_HEIGHT) {
            collapse_start(task);
            work->mode = COLLAPSE_WAIT;
            collapse_front_y = FRONT_START_Y;
            collapse_front_x = FRONT_START_X;
            work->timer = FRONT_WAIT;
            sky_deck_clock = CLOCK_START;
        }
        break;
    case COLLAPSE_WAIT:
        if (TASK_TIMER(work)-- == 0) {
            work->mode = COLLAPSE_SPREAD;
        }
        break;
    case COLLAPSE_SPREAD:
        /* no break ends the last case. with one, the tail jump to
         * task_step_children takes R2, not R3 (0x0c906882), and 2 of 85 words
         * differ */
        collapse_front_y += FRONT_SPEED;
        collapse_front_x += FRONT_SPEED;
    }
    task_step_children(task);
}

static void collapse_destroy(struct task *task)
{
    task_destroy_children(task);
}

/* the floor under the pieces gives way, and the land stops drawing them:
 * they draw themselves */
static void collapse_start(struct task *task)
{
    int i;
    int count = sky_deck_act1_land.count;
    struct land_entry *entry = sky_deck_act1_land.entries;

    for (i = 0; i < count; i++) {
        if (entry->blocks & FLOOR_BLOCK) {
            entry->flags &= ~WORLD_SOLID;
        }
        entry++;
    }
    world_land_blocks &= ~PIECE_BLOCK;
}

/* a child task for each land entry in PIECE_BLOCK. each child's vars hold
 * its entry and the entry's object, and its submode the entry's index */
static void collapse_create_pieces(struct task *task)
{
    int i;
    int count = sky_deck_act1_land.count;
    struct land_entry *entry = sky_deck_act1_land.entries;
    struct task *child;
    NJS_OBJECT *object;

    for (i = 0; i < count; i++, entry++) {
        if (entry->blocks & PIECE_BLOCK) {
            child = task_create_child(TASK_WORK, collapse_piece_step, task);
            child->work->submode = i;
            object = entry->object;
            VARS(child->work)->entry = entry;
            VARS(child->work)->object = object;
        }
    }
}

/* a piece comes loose as the collapse starts. it waits for the front to
 * reach it, then a while longer, and falls */
static void collapse_piece_step(struct task *task)
{
    struct task_work *work = task->work;
    struct land_entry *entry = VARS(work)->entry;
    NJS_OBJECT *object = VARS(work)->object;

    switch (work->mode) {
    case PIECE_INIT:
        task->draw = collapse_piece_draw;
        work->pos.x = entry->center.x;
        work->pos.y = entry->center.y;
        work->pos.z = entry->center.z;
        /* the object takes no move of its own, so it draws where the
         * piece's matrix puts it. none of its children draw */
        object->evalflags = NJD_EVAL_UNIT_POS | NJD_EVAL_UNIT_ANG |
                            NJD_EVAL_UNIT_SCL | NJD_EVAL_BREAK;
        work->mode = PIECE_FIXED;
        break;
    case PIECE_FIXED:
        if (task->parent->work->mode == COLLAPSE_WAIT) {
            work->mode = PIECE_LOOSE;
        }
        break;
    case PIECE_LOOSE:
        if (task->parent->work->mode == COLLAPSE_SPREAD &&
            collapse_reached(work->pos.x, work->pos.y, work->pos.z)) {
            work->mode = PIECE_PASSED;
            work->timer = (Sint16)RANDOM(1.0F, PIECE_WAIT + 1.0F);
        }
        break;
    case PIECE_PASSED:
        if (TASK_TIMER(work)-- == 0) {
            work->mode = PIECE_FALLING;
        }
        break;
    case PIECE_FALLING:
        work->ang.z += PIECE_TURN;
        FALLEN(work).y -= PIECE_FALL;
        FALLEN(work).x += PIECE_DRIFT;
        break;
    }
    collapse_piece_draw(task);
}

/* once loose, a piece draws its model's center at the entry's center,
 * moved by its fall. the model turns about that point */
static void collapse_piece_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object;
    NJS_MODEL *model;
    Angle ang;

    if (draw_in_view(work->pos.x, work->pos.y, work->pos.z, PIECE_VIEW)) {
        if (work->mode >= PIECE_LOOSE) {
            njSetTexture(&skydeck02_texlist);
            object = VARS(work)->object;
            model = object->model;
            njPushMatrix(NULL);
            njTranslate(NULL, work->pos.x + FALLEN(work).x,
                        work->pos.y + FALLEN(work).y,
                        work->pos.z + FALLEN(work).z);
            TURN_VIA(njRotateZ, work->ang.z, ang);
            TURN_VIA(njRotateY, work->ang.y, ang);
            TURN_VIA(njRotateX, work->ang.x, ang);
            njTranslate(NULL, -model->center.x, -model->center.y,
                        -model->center.z);
            draw_tree(object);
            njPopMatrix(1);
        }
    }
}
