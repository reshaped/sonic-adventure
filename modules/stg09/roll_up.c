/* RollUpR, RollUpL and RollDownL, the platforms on the tables that turn
 * about y */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/world.h"
#include "draw/model.h"
#include "object/common.h"
#include "player/player.h"
#include "stg09/casino.h"
#include "stg09/roll_up.h"
#include "system/task.h"
#include "system/util.h"

extern NJS_MODEL roll_up_right_model_0;
extern NJS_MODEL roll_up_right_model_1;
extern NJS_MODEL roll_up_right_model_2;
extern NJS_MODEL roll_up_right_model_3;
extern NJS_MODEL roll_up_right_model_4;
extern NJS_MODEL roll_up_right_model_5;
extern NJS_MODEL roll_up_left_model_0;
extern NJS_MODEL roll_up_left_model_1;
extern NJS_MODEL roll_down_left_model_0;
extern NJS_MODEL roll_down_left_model_1;
extern NJS_MODEL roll_down_left_model_2;
extern NJS_OBJECT roll_up_right_object;
extern NJS_OBJECT roll_up_left_object;
extern NJS_OBJECT roll_down_left_object;

struct roll_up_vars {
    NJS_OBJECT *object; /* the platform's copy in the collision world */
    Angle swing; /* RollDownL's phase, whose sine sets its y angle */
    Sint32 wait; /* steps left to stay stopped */
};

#define VARS(work) ((struct roll_up_vars *)&(work)->vars)

#define ROLL_UP_WORLD (WORLD_MEDIUM | WORLD_SOLID)
#define ROLL_DOWN_WORLD (WORLD_MEDIUM | WORLD_DEFLECT | WORLD_SOLID)

/* each platform's turn or swing phase a step, and the steps RollUpL and
 * RollDownL stay stopped */
#define ROLL_UP_RIGHT_TURN 0x80
#define ROLL_UP_LEFT_TURN 0x200
#define ROLL_DOWN_LEFT_SWING 0x200
#define ROLL_UP_LEFT_WAIT 60
#define ROLL_DOWN_LEFT_WAIT 30

/* ang within its turn, rounded down to a multiple of 0x100. RollUpL and
 * RollDownL stop where it is a quarter turn */
#define STOP_ANGLE(ang) ((ang) & 0xff00)

static void roll_up_destroy(struct task *task)
{
    NJS_OBJECT *object = VARS(task->work)->object;

    world_remove(task, object);
    world_object_free(object);
}

static void roll_up_right_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    casino_set_texture(CASINO_TEXTURES_ACTS_34);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_model(&roll_up_right_model_0);
    draw_model(&roll_up_right_model_1);
    draw_model(&roll_up_right_model_2);
    draw_model(&roll_up_right_model_3);
    draw_model(&roll_up_right_model_4);
    draw_model(&roll_up_right_model_5);
    njPopMatrix(1);
}

static void roll_up_left_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    casino_set_texture(CASINO_TEXTURES_ACTS_34);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_model(&roll_up_left_model_0);
    draw_model(&roll_up_left_model_1);
    njPopMatrix(1);
}

static void roll_down_left_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    casino_set_texture(CASINO_TEXTURES_ACTS_34);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_model(&roll_down_left_model_0);
    draw_model(&roll_down_left_model_1);
    draw_model(&roll_down_left_model_2);
    njPopMatrix(1);
}

static void roll_up_right_run(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object = VARS(work)->object;

    if (player_in_range(&work->pos, object->model->r + 20.0F)) {
        work->flags |= WORLD_TESTED;
    } else {
        work->flags &= ~WORLD_TESTED;
    }
    work->ang.y = object->ang[1] = work->ang.y + ROLL_UP_RIGHT_TURN;
    roll_up_right_draw(task);
    /* @hack redundant: a return where the function ends, which makes the frame
     * with ADD #-4,R15 and stores task in it across player_in_range
     * (0x0c905926, 0x0c90592c). without it, a push does both, and 38 of 39
     * words differ */
    /* NOLINTNEXTLINE(readability-redundant-control-flow) */
    return;
}

static void roll_up_left_run(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object = VARS(work)->object;
    Angle quarter;

    if (player_in_range(&work->pos, object->model->r + 20.0F)) {
        work->flags |= WORLD_TESTED;
    } else {
        work->flags &= ~WORLD_TESTED;
    }
    if (VARS(work)->wait) {
        VARS(work)->wait--;
    } else {
        work->ang.y = object->ang[1] = work->ang.y - ROLL_UP_LEFT_TURN;
        /* the turn keeps the angle's remainder by ROLL_UP_LEFT_TURN. the
         * platform stops only if that remainder is under 0x100 */
        quarter = STOP_ANGLE(work->ang.y);
        if (quarter == 0 || quarter == NJM_DEG_ANG(90) ||
            quarter == NJM_DEG_ANG(180) || quarter == NJM_DEG_ANG(270)) {
            VARS(work)->wait = ROLL_UP_LEFT_WAIT;
        }
    }
    roll_up_left_draw(task);
    /* @hack redundant: a return where the function ends, which makes the frame
     * with ADD #-4,R15 and stores task in it across player_in_range
     * (0x0c905974, 0x0c90597a). without it, a push does both, and 23 of 62
     * words differ */
    /* NOLINTNEXTLINE(readability-redundant-control-flow) */
    return;
}

/* the swing sets RollDownL's y angle, from 0 to -360 degrees, in place of
 * its set entry's */
static void roll_down_left_run(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object = VARS(work)->object;
    Angle quarter;

    if (player_in_range(&work->pos, object->model->r + 20.0F)) {
        work->flags |= WORLD_TESTED;
    } else {
        work->flags &= ~WORLD_TESTED;
    }
    if (VARS(work)->wait) {
        VARS(work)->wait--;
    } else {
        VARS(work)->swing += ROLL_DOWN_LEFT_SWING;
        quarter = STOP_ANGLE(VARS(work)->swing);
        if (quarter == NJM_DEG_ANG(90) || quarter == NJM_DEG_ANG(270)) {
            VARS(work)->wait = ROLL_DOWN_LEFT_WAIT;
        }
    }
    work->ang.y = object->ang[1] =
        (Angle)(njSin(VARS(work)->swing) * (180.0F * ANGLE_PER_DEGREE)) -
        NJM_DEG_ANG(180);
    roll_down_left_draw(task);
    /* @hack redundant: a return where the function ends, which makes the frame
     * with ADD #-4,R15 and stores task in it across player_in_range
     * (0x0c905a12, 0x0c905a18). without it, a push does both, and 23 of 68
     * words differ */
    /* NOLINTNEXTLINE(readability-redundant-control-flow) */
    return;
}

void roll_up_right_step(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object;

    VARS(work)->object =
        object_add_world(&roll_up_right_object, task, ROLL_UP_WORLD);
    object = VARS(work)->object;
    object->scl[0] = 1.0F;
    object->scl[1] = 1.0F;
    object->scl[2] = 1.0F;
    task->step = roll_up_right_run;
    task->draw = roll_up_right_draw;
    task->destructor = roll_up_destroy;
}

void roll_up_left_step(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object;

    VARS(work)->object =
        object_add_world(&roll_up_left_object, task, ROLL_UP_WORLD);
    object = VARS(work)->object;
    object->scl[0] = 1.0F;
    object->scl[1] = 1.0F;
    object->scl[2] = 1.0F;
    VARS(work)->wait = 0;
    task->step = roll_up_left_run;
    task->draw = roll_up_left_draw;
    task->destructor = roll_up_destroy;
}

void roll_down_left_step(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object;

    VARS(work)->object =
        object_add_world(&roll_down_left_object, task, ROLL_DOWN_WORLD);
    object = VARS(work)->object;
    object->scl[0] = 1.0F;
    object->scl[1] = 1.0F;
    object->scl[2] = 1.0F;
    task->step = roll_down_left_run;
    task->draw = roll_down_left_draw;
    task->destructor = roll_up_destroy;
}
