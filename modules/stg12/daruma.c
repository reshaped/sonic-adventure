/* O DARUMA, a figure that drops to the ground below once nothing holds it
 * up */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "collision/ground.h"
#include "debug/editor.h"
#include "debug/print.h"
#include "draw/draw.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "stg12/daruma.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT daruma_object;
extern NJS_OBJECT daruma_part_a_object;
extern NJS_OBJECT daruma_part_b_object;
extern NJS_MOTION daruma_part_a_motion;
extern NJS_MOTION daruma_part_a_shape;
extern NJS_MOTION daruma_part_b_motion;
extern NJS_MOTION daruma_part_b_shape;
/* @hack type: the action declared as an array, so SHC pools its address, at
 * 0x0c908028, and daruma_animate reads its motion through it at 0x0c907f74.
 * declared as one, SHC pools the motion field's address in its place. then 3
 * of daruma_animate's 24 words differ, as does 1 of the 68 words of the pool
 * and pad inside daruma_drop_when_unsupported */
extern NJS_ACTION daruma_part_a_action[];

/* destroyed beyond 500 units of the camera and player 0 */
#define RANGE (500.0F * 500.0F)

/* the fall speed the figure gains each step */
#define FALL_ACCEL 0.1F

/* DARUMA_STAND waits until nothing holds the figure up while it is above the
 * ground. DARUMA_DROP lasts one step, then DARUMA_FALL drops the figure while
 * nothing holds it up. once it reaches the ground, DARUMA_DOWN never lets it
 * fall again. no code sets DARUMA_RESTAND, which goes back to DARUMA_STAND */
enum daruma_mode {
    DARUMA_INIT,
    DARUMA_STAND,
    DARUMA_DROP,
    DARUMA_FALL,
    DARUMA_RESTAND,
    DARUMA_DOWN
};

struct daruma_vars {
    Float frame;
};

#define VARS(work) ((struct daruma_vars *)&(work)->vars)
/* GROUND is the height of the ground below where the figure was placed, as
 * init measures it */
#define GROUND(work) ((work)->scl.x)
#define FALL_SPEED(work) ((work)->scl.y)

static void daruma_init(struct task *task);
static void daruma_draw(struct task *task);
static void daruma_destroy(struct task *task);
static void daruma_animate(struct task *task);
static void daruma_drop_when_unsupported(struct task *task);
static void daruma_fall(struct task *task);

/* the figure, 38.15 units square and 29.85 tall, and a sphere at its base.
 * the sphere ignores players and enemies. while it touches nothing, nothing
 * holds the figure up */
static struct collision_shape daruma_shapes[2] = {
    {SHAPE_RECTANGLE,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 14.92F, 0.0F},
     19.075F,
     14.925F,
     19.075F,
     {0},
     0,
     NULL},
    {SHAPE_SPHERE | SHAPE_IGNORE_PLAYERS | SHAPE_IGNORE_ENEMIES,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 0.0F, 0.0F},
     10.0F,
     0.0F,
     0.0F,
     {0},
     0,
     NULL},
};

/* @hack spelling: the registration as a helper SHC expands, which reads the
 * work from task again at each call, as MOV.L @(32,R4),R4 at 0x0c907e06
 * shows; written in place, the step runs 2 bytes long and 100 of its 118
 * words differ */
static void daruma_register(struct task *task)
{
    struct task_work *work = task->work;

    collision_register(work);
}

void daruma_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave_by(task, RANGE)) {
        return;
    }
    if (!placement_editing(task)) {
        switch (work->mode) {
        case DARUMA_INIT:
            daruma_init(task);
            break;
        case DARUMA_STAND:
            daruma_draw(task);
            daruma_animate(task);
            daruma_drop_when_unsupported(task);
            daruma_register(task);
            break;
        case DARUMA_DROP:
            daruma_draw(task);
            daruma_animate(task);
            daruma_register(task);
            work->mode = DARUMA_FALL;
            break;
        case DARUMA_FALL:
            daruma_draw(task);
            daruma_animate(task);
            daruma_fall(task);
            daruma_register(task);
            break;
        case DARUMA_RESTAND:
            daruma_draw(task);
            daruma_animate(task);
            daruma_register(task);
            work->mode = DARUMA_STAND;
            break;
        case DARUMA_DOWN:
            daruma_draw(task);
            daruma_animate(task);
            daruma_register(task);
            break;
        }
    } else {
        switch (work->mode) {
        case DARUMA_INIT:
            daruma_init(task);
            break;
        case DARUMA_STAND:
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_X), "-> ROTATE X");
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Y), "-> ROTATE Y");
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Z), "-> ROTATE Z");
            daruma_draw(task);
            daruma_register(task);
            break;
        }
    }
}

static void daruma_init(struct task *task)
{
    struct task_work *work = task->work;

    task->draw = daruma_draw;
    task->destructor = daruma_destroy;
    collision_init(task, daruma_shapes, COLLISION_OBJECT_MUTUAL, 1, 2);
    GROUND(work) =
        ground_below(work->pos.x, work->pos.y, work->pos.z, &work->ang);
    FALL_SPEED(work) = 0.0F;
    VARS(work)->frame = 0.0F;
    work->mode = DARUMA_STAND;
}

static void daruma_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_tree(&daruma_object);
    draw_shape_motion(&daruma_part_a_object, &daruma_part_a_motion,
                      &daruma_part_a_shape, VARS(work)->frame);
    draw_shape_motion(&daruma_part_b_object, &daruma_part_b_motion,
                      &daruma_part_b_shape, VARS(work)->frame);
    njPopMatrix(1);
}

static void daruma_destroy(struct task *task)
{
}

/* both parts wrap at the first part's frame count */
static void daruma_animate(struct task *task)
{
    struct task_work *work = task->work;

    VARS(work)->frame += 1.0F;
    if (VARS(work)->frame >= daruma_part_a_action->motion->nbFrame) {
        VARS(work)->frame = 0.0F;
    }
}

static void daruma_drop_when_unsupported(struct task *task)
{
    struct task_work *work = task->work;

    if (work->collision->shapes[1].hit == NULL) {
        if (work->pos.y > GROUND(work)) {
            work->mode = DARUMA_DROP;
        }
    }
}

static void daruma_fall(struct task *task)
{
    struct task_work *work = task->work;

    if (work->collision->shapes[1].hit == NULL) {
        if (work->pos.y > GROUND(work)) {
            FALL_SPEED(work) += FALL_ACCEL;
            work->pos.y -= FALL_SPEED(work);
        } else {
            work->pos.y = GROUND(work);
            FALL_SPEED(work) = 0.0F;
            work->mode = DARUMA_DOWN;
        }
    }
}
