/* O BOX SWITCH, a box a player can carry to its place in a box hole. there
 * it sinks, and turns its switch on */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "debug/print.h"
#include "draw/tree.h"
#include "effect/bomb.h"
#include "object/common.h"
#include "object/placement.h"
#include "stg12/box_switch.h"
#include "stg12/texlists.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/util.h"

/* the box's model in each of its four colors */
extern NJS_OBJECT box_switch_0_object;
extern NJS_OBJECT box_switch_1_object;
extern NJS_OBJECT box_switch_2_object;
extern NJS_OBJECT box_switch_3_object;
/* the four plates of the hole's action. a box sinks its plate as it sinks */
extern NJS_OBJECT box_hole_plate_0_object;
extern NJS_OBJECT box_hole_plate_1_object;
extern NJS_OBJECT box_hole_plate_2_object;
extern NJS_OBJECT box_hole_plate_3_object;

/* destroyed beyond 200 units of the camera and player 0, except while a player
 * holds it */
#define RANGE (200.0F * 200.0F)

/* a box within 5 units of its place slides into it over 45 steps. FIT_RANGE
 * is that distance squared */
#define FIT_RANGE (5.0F * 5.0F)
#define SLIDE_STEPS 45

/* after sliding, the box makes a quarter turn about y over 16 steps */
#define TURN_STEPS 16
#define TURN_SPEED (NJM_DEG_ANG(90) / TURN_STEPS)

/* after turning, the box waits 5 steps, then sinks 5 units over 90 steps */
#define WAIT_STEPS 5
#define SINK_DEPTH 5.0F
#define SINK_STEPS 90

/* a loose box that rests more than 1 unit below its place's height is lost */
#define LOST_DEPTH 1.0F

enum box_switch_mode {
    BOX_INIT,
    BOX_LOOSE,
    BOX_SLIDE,
    BOX_TURN,
    BOX_WAIT,
    BOX_SINK,
    BOX_DOWN,
    BOX_LOST /* it vanishes in a puff, and its task ends */
};

struct box_switch_vars {
    Uint8 box; /* which box: scale x, masked to 0 to 7 */
};

#define VARS(work) ((struct box_switch_vars *)&(work)->vars)
/* the box's move each step as it slides, and its drop each step as it sinks.
 * they take the scale's place once init has read it */
#define SLIDE_STEP_X(work) ((work)->scl.x)
#define SINK_STEP(work) ((work)->scl.y)
#define SLIDE_STEP_Z(work) ((work)->scl.z)

static void box_switch_init(struct task *task);
static void box_switch_register(struct task *task);
static void box_switch_draw(struct task *task);
static void box_switch_destroy(struct task *task);
static void box_switch_fit(struct task *task);
static void box_switch_slide(struct task_work *work);
static void box_switch_turn(struct task_work *work);
static void box_switch_wait(struct task_work *work);
static void box_switch_sink(struct task_work *work);

/* for each box: its model, the plate under its place, and the place. boxes n
 * and n + 4 share a model and a plate. both holes draw the same four plates */
static NJS_OBJECT *box_switch_models[BOX_SWITCHES] = {
    &box_switch_0_object, &box_switch_1_object, &box_switch_2_object,
    &box_switch_3_object, &box_switch_0_object, &box_switch_1_object,
    &box_switch_2_object, &box_switch_3_object,
};

static NJS_OBJECT *box_switch_plates[BOX_SWITCHES] = {
    &box_hole_plate_0_object, &box_hole_plate_1_object,
    &box_hole_plate_2_object, &box_hole_plate_3_object,
    &box_hole_plate_0_object, &box_hole_plate_1_object,
    &box_hole_plate_2_object, &box_hole_plate_3_object,
};

static NJS_POINT3 box_switch_places[BOX_SWITCHES] = {
#include "stg12/box_switch/box_switch_places.inc"
};

static struct collision_shape box_switch_shape = {
    SHAPE_RECTANGLE,
    0,
    PUSH_ALL | PUSH_FIRM(PUSH_ALL),
    {0.0F, 2.5F, 0.0F},
    2.5F,
    2.5F,
    2.2F,
    {0},
    0,
    NULL,
};

/* each box's switch, set once the box has sunk into its place.
 * box_switch_reset clears them as act 1 starts */
static Bool box_switch_on[BOX_SWITCHES] = {FALSE};

void box_switch_step(struct task *task)
{
    struct task_work *work = task->work;
    Float below;

    if (!(work->flags & CARRY_HELD) && placement_try_leave_by(task, RANGE)) {
        return;
    }
    if (!placement_editing(task)) {
        switch (work->mode) {
        case BOX_INIT:
            box_switch_init(task);
            break;
        case BOX_LOOSE:
            box_switch_draw(task);
            below = work->pos.y - box_switch_places[VARS(work)->box].y;
            if ((work->flags & CARRY_GROUND) && below < -LOST_DEPTH) {
                work->mode = BOX_LOST;
                break;
            }
            work->flags |= CARRY_WALLS;
            carry_step_small(task);
            if (!(work->flags & CARRY_HELD)) {
                box_switch_fit(task);
            }
            break;
        case BOX_SLIDE:
            box_switch_draw(task);
            box_switch_slide(work);
            break;
        case BOX_TURN:
            box_switch_draw(task);
            box_switch_turn(work);
            break;
        case BOX_WAIT:
            box_switch_draw(task);
            box_switch_wait(work);
            break;
        case BOX_SINK:
            box_switch_draw(task);
            box_switch_sink(work);
            collision_register(task->work);
            break;
        case BOX_DOWN:
            box_switch_draw(task);
            break;
        case BOX_LOST:
            box_switch_draw(task);
            bomb_create(&work->pos, 0.5F);
            task_kill(task);
            break;
        }
    } else {
        switch (work->mode) {
        case BOX_INIT:
            box_switch_init(task);
            break;
        case BOX_LOOSE:
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_X), "->");
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Y), "-> ROTATE Y");
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Z), "->");
            debug_print(EDITOR_LABEL(EDITOR_ROW_SCL_X), "-> COLOR (0-7)");
            debug_print(EDITOR_LABEL(EDITOR_ROW_SCL_Y), "->");
            debug_print(EDITOR_LABEL(EDITOR_ROW_SCL_Z), "->");
            box_switch_draw(task);
            box_switch_register(task);
            break;
        }
    }
}

/* a box whose switch is on starts down in its place, turned a quarter, and
 * sinks its plate. any other starts loose, and raises its plate */
static void box_switch_init(struct task *task)
{
    struct task_work *work = task->work;
    Bool down;

    VARS(work)->box = (int)work->scl.x;
    VARS(work)->box &= BOX_SWITCHES - 1;
    task->draw = box_switch_draw;
    task->destructor = box_switch_destroy;
    collision_init(task, &box_switch_shape, COLLISION_OBJECT_MUTUAL, 1, 1);
    carry_set_kind(work, task->motion, CARRY_KIND_SMALL);
    down = box_switch_on[VARS(work)->box];
    if (down) {
        box_switch_plates[VARS(work)->box]->pos[1] = -SINK_DEPTH;
        work->pos = box_switch_places[VARS(work)->box];
        work->pos.y -= SINK_DEPTH;
        work->ang.x = 0;
        work->ang.y = NJM_DEG_ANG(90);
        work->ang.z = 0;
        work->mode = BOX_DOWN;
    } else {
        box_switch_plates[VARS(work)->box]->pos[1] = 0.0F;
        work->ang.x = 0;
        work->ang.y = 0;
        work->ang.z = 0;
        work->mode = BOX_LOOSE;
    }
}

static void box_switch_register(struct task *task)
{
    struct task_work *work = task->work;

    if (!(work->flags & CARRY_HELD)) {
        collision_register(task->work);
    }
}

static void box_switch_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (frame_step == 0) {
        njSetTexture(&hotshelter2_texlist);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        draw_tree(box_switch_models[VARS(work)->box]);
        njPopMatrix(1);
    }
}

static void box_switch_destroy(struct task *task)
{
}

/* start the slide once the box is within 5 units of its place in x and z.
 * its height, rounded to the nearest unit, must be the place's */
static void box_switch_fit(struct task *task)
{
    struct task_work *work = task->work;
    Float level = njRoundOff(work->pos.y + 0.5F);
    Float x;
    Float z;

    if (level == box_switch_places[VARS(work)->box].y) {
        x = box_switch_places[VARS(work)->box].x - work->pos.x;
        z = box_switch_places[VARS(work)->box].z - work->pos.z;
        if ((x * x) + (z * z) < FIT_RANGE) {
            SLIDE_STEP_X(work) = x * (1.0F / SLIDE_STEPS);
            SLIDE_STEP_Z(work) = z * (1.0F / SLIDE_STEPS);
            TASK_TIMER(work) = SLIDE_STEPS;
            work->mode = BOX_SLIDE;
        }
    }
}

static void box_switch_slide(struct task_work *work)
{
    work->pos.x += SLIDE_STEP_X(work);
    work->pos.z += SLIDE_STEP_Z(work);
    if (--TASK_TIMER(work) <= 0) {
        TASK_TIMER(work) = TURN_STEPS;
        work->mode = BOX_TURN;
    }
}

static void box_switch_turn(struct task_work *work)
{
    work->ang.y += TURN_SPEED;
    if (--TASK_TIMER(work) <= 0) {
        TASK_TIMER(work) = WAIT_STEPS;
        work->mode = BOX_WAIT;
    }
}

static void box_switch_wait(struct task_work *work)
{
    if (--TASK_TIMER(work) <= 0) {
        TASK_TIMER(work) = SINK_STEPS;
        SINK_STEP(work) = SINK_DEPTH / SINK_STEPS;
        work->mode = BOX_SINK;
    }
}

static void box_switch_sink(struct task_work *work)
{
    work->pos.y -= SINK_STEP(work);
    box_switch_plates[VARS(work)->box]->pos[1] -= SINK_STEP(work);
    if (--TASK_TIMER(work) <= 0) {
        box_switch_on[VARS(work)->box] = TRUE;
        work->mode = BOX_DOWN;
    }
}

void box_switch_reset(void)
{
    int i;

    for (i = 0; i < BOX_SWITCHES; i++) {
        box_switch_on[i] = FALSE;
    }
}

Bool box_switch_is_on(int box)
{
    return box_switch_on[box];
}
