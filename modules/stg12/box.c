/* O HAKO, a box of carry_set_kind's medium kind, which Big can carry and
 * other players push. it falls once nothing holds it up, to the ground below
 * where it was placed */
#include <ninja.h>

#include "collision/collision.h"
#include "collision/ground.h"
#include "debug/editor.h"
#include "debug/print.h"
#include "draw/tree.h"
#include "effect/bomb.h"
#include "object/common.h"
#include "object/placement.h"
#include "stg12/box.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT shelter_box_object;

/* destroyed beyond 500 units of the camera and player 0 */
#define RANGE (500.0F * 500.0F)

/* the fall speed the box gains each step */
#define FALL_ACCEL 0.02F

/* the box's height. a puff rises from its top on each step an attack hurts
 * it */
#define HEIGHT 9.8F

/* BOX_STAND waits until nothing holds the box up while it is above the
 * ground. BOX_DROP lasts one step, then BOX_FALL drops the box while nothing
 * holds it up. once it reaches the ground, BOX_DOWN never lets it fall again.
 * no code sets BOX_UNK_6, which acts as BOX_DOWN, or BOX_RESTAND, which goes
 * back to BOX_STAND */
enum shelter_box_mode {
    BOX_INIT,
    BOX_STAND,
    BOX_DROP,
    BOX_FALL,
    BOX_RESTAND,
    BOX_DOWN,
    BOX_UNK_6
};

/* GROUND is the height of the ground below where the box was placed, as init
 * measures it */
#define GROUND(work) ((work)->scl.x)
#define FALL_SPEED(work) ((work)->scl.y)

static void shelter_box_init(struct task *task);
static void shelter_box_register(struct task *task);
static void shelter_box_draw(struct task *task);
static void shelter_box_destroy(struct task *task);
static void shelter_box_drop_when_unsupported(struct task *task);
static void shelter_box_fall(struct task *task);

/* the box, 34 units square and 9.8 tall, and a sphere at its center. the
 * sphere ignores players and enemies. while it touches nothing, nothing holds
 * the box up */
static struct collision_shape shelter_box_shapes[2] = {
    {SHAPE_RECTANGLE,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 4.9F, 0.0F},
     17.0F,
     4.9F,
     17.0F,
     {0},
     0,
     NULL},
    {SHAPE_SPHERE | SHAPE_IGNORE_PLAYERS | SHAPE_IGNORE_ENEMIES,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 4.9F, 0.0F},
     4.9F,
     0.0F,
     0.0F,
     {0},
     0,
     NULL},
};

void shelter_box_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave_by(task, RANGE)) {
        return;
    }
    if (!placement_editing(task)) {
        switch (work->mode) {
        case BOX_INIT:
            shelter_box_init(task);
            break;
        case BOX_STAND:
            shelter_box_draw(task);
            carry_step_large(task);
            shelter_box_drop_when_unsupported(task);
            shelter_box_register(task);
            break;
        case BOX_DROP:
            shelter_box_draw(task);
            shelter_box_register(task);
            work->mode = BOX_FALL;
            break;
        case BOX_FALL:
            shelter_box_draw(task);
            carry_step_large(task);
            shelter_box_fall(task);
            shelter_box_register(task);
            break;
        case BOX_RESTAND:
            shelter_box_draw(task);
            shelter_box_register(task);
            work->mode = BOX_STAND;
            break;
        /* @hack spelling: BOX_DOWN's and BOX_UNK_6's cases kept apart, with
         * the same calls in each, as the image has code for each at
         * 0x0c908154 and 0x0c908168. as one case, 105 of shelter_box_step's
         * 134 words differ */
        case BOX_DOWN:
            shelter_box_draw(task);
            carry_step_large(task);
            shelter_box_register(task);
            break;
        case BOX_UNK_6:
            shelter_box_draw(task);
            carry_step_large(task);
            shelter_box_register(task);
            break;
        }
    } else {
        switch (work->mode) {
        case BOX_INIT:
            shelter_box_init(task);
            break;
        case BOX_STAND:
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_X), "-> ROTATE X");
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Y), "-> ROTATE Y");
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Z), "-> ROTATE Z");
            shelter_box_draw(task);
            shelter_box_register(task);
            break;
        }
    }
}

static void shelter_box_init(struct task *task)
{
    struct task_work *work = task->work;

    task->draw = shelter_box_draw;
    task->destructor = shelter_box_destroy;
    collision_init(task, shelter_box_shapes, COLLISION_OBJECT_MUTUAL, 1, 2);
    carry_set_kind(work, task->motion, CARRY_KIND_MEDIUM);
    GROUND(work) =
        ground_below(work->pos.x, work->pos.y, work->pos.z, &work->ang);
    FALL_SPEED(work) = 0.0F;
    work->mode = BOX_STAND;
}

static void shelter_box_register(struct task *task)
{
    struct task_work *work = task->work;
    NJS_POINT3 top;

    top = work->pos;
    top.y += HEIGHT;
    if (work->flags & COLLISION_DAMAGED) {
        bomb_create(&top, 2.0F);
    }
    collision_register(task->work);
}

static void shelter_box_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (frame_step == 0) {
        texture_set_stage();
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        draw_tree(&shelter_box_object);
        njPopMatrix(1);
    }
}

static void shelter_box_destroy(struct task *task)
{
}

static void shelter_box_drop_when_unsupported(struct task *task)
{
    struct task_work *work = task->work;

    if (work->collision->shapes[1].hit == NULL) {
        if (work->pos.y > GROUND(work)) {
            work->mode = BOX_DROP;
        }
    }
}

static void shelter_box_fall(struct task *task)
{
    struct task_work *work = task->work;

    if (work->collision->shapes[1].hit == NULL) {
        if (work->pos.y > GROUND(work)) {
            FALL_SPEED(work) += FALL_ACCEL;
            work->pos.y -= FALL_SPEED(work);
        } else {
            work->pos.y = GROUND(work);
            FALL_SPEED(work) = 0.0F;
            work->mode = BOX_DOWN;
        }
    }
}
