/* O EV paternoster of six cars that circle its frame without stopping */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/world.h"
#include "debug/editor.h"
#include "draw/model.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "player/player.h"
#include "player/target.h"
#include "sound/sound.h"
#include "stg04/paternoster.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT paternoster_object;
/* @hack type: an array of one, whose address paternoster_step keeps in R8
 * for its four draws of the car (0x0c90144c). declared as one object, 168 of
 * the 865 words of paternoster_step and paternoster_car_step differ */
extern NJS_OBJECT paternoster_car_object[1];

/* destroyed beyond 1010 units of the camera and player 0 */
#define PATERNOSTER_RANGE (1010.0F * 1010.0F)
/* how near player 0 or 1 must be for the world to test the frame or a car */
#define PATERNOSTER_NEAR 170.0F

#define FRAME_FLAGS (WORLD_HUGE | WORLD_TURNED | WORLD_MOVES)
#define CAR_FLAGS (WORLD_SMALL | WORLD_TURNED | WORLD_MOVES)

/* the loop's corners lie LOOP_HEIGHT above and below the frame's position,
 * and LOOP_HALF_WIDTH to either side. a car moves CAR_SPEED each step */
#define LOOP_HEIGHT 110.0F
#define LOOP_HALF_WIDTH 70.0F
#define CAR_SPEED 0.7F

/* a car's wheels turn a 256th of a turn each step */
#define WHEEL_TURN 0x100
/* the radius of the target each car offers a falling player */
#define CAR_TARGET 10.0F

/* the running paternoster's sound, and the updates each step keeps it on for */
#define SE_PATERNOSTER 102
#define SOUND_UPDATES 30

/* the frame's modes. no code sets PATERNOSTER_STOPPED */
enum paternoster_mode {
    PATERNOSTER_INIT = 0,
    PATERNOSTER_RUN = 1,
    PATERNOSTER_STOPPED = 3
};
enum car_mode { CAR_INIT, CAR_FORWARD, CAR_REVERSE, CAR_STOPPED };

/* the side of the loop a car moves along, in its work->submode. its task_spec
 * gives the side it starts on, as the work's flags. no code sets SIDE_STOP,
 * which would stop the car */
enum car_side { SIDE_RIGHT, SIDE_TOP, SIDE_LEFT, SIDE_BOTTOM, SIDE_STOP };

struct paternoster_vars {
    NJS_OBJECT *object; /* the task's object in the collision world */
    NJS_OBJECT *model; /* the object it draws */
};

#define VARS(work) ((struct paternoster_vars *)&(work)->vars)
#define WHEELS(work) TASK_TIMER(work)
#define REVERSED(work) ((work)->scl.x < 0.0F)

static void paternoster_car_step(struct task *task);

/* the six cars' specs, then a spec with a null step, which ends the table.
 * each car starts at pos, on the side its flags give. var sets the car's
 * VARS(work)->object to its model, which its first step replaces with its
 * world object */
static struct task_spec paternoster_cars[7] = {
    {paternoster_car_step,
     TASK_WORK | TASK_FORCE,
     SIDE_BOTTOM,
     {0, 0, 0},
     {60.0F, -110.0F, 0.0F},
     (Sint32)paternoster_car_object},
    {paternoster_car_step,
     TASK_WORK | TASK_FORCE,
     SIDE_RIGHT,
     {0, 0, 0},
     {70.0F, 0.0F, 0.0F},
     (Sint32)paternoster_car_object},
    {paternoster_car_step,
     TASK_WORK | TASK_FORCE,
     SIDE_TOP,
     {0, 0, 0},
     {60.0F, 110.0F, 0.0F},
     (Sint32)paternoster_car_object},
    {paternoster_car_step,
     TASK_WORK | TASK_FORCE,
     SIDE_TOP,
     {0, 0, 0},
     {-60.0F, 110.0F, 0.0F},
     (Sint32)paternoster_car_object},
    {paternoster_car_step,
     TASK_WORK | TASK_FORCE,
     SIDE_LEFT,
     {0, 0, 0},
     {-70.0F, 0.0F, 0.0F},
     (Sint32)paternoster_car_object},
    {paternoster_car_step,
     TASK_WORK | TASK_FORCE,
     SIDE_BOTTOM,
     {0, 0, 0},
     {-60.0F, -110.0F, 0.0F},
     (Sint32)paternoster_car_object},
    {NULL, 0, 0, {0, 0, 0}, {0.0F, 0.0F, 0.0F}, 0},
};

/* move the task's object in the collision world to the task's position. the
 * world tests the object only while player 0 or 1 is within PATERNOSTER_NEAR
 * of the task */
static void paternoster_follow(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object = VARS(work)->object;

    object->pos[0] = work->pos.x;
    object->pos[1] = work->pos.y;
    object->pos[2] = work->pos.z;
    if (player_in_range(&work->pos, PATERNOSTER_NEAR)) {
        work->flags |= WORLD_TESTED;
    } else {
        work->flags &= ~WORLD_TESTED;
    }
}

static void paternoster_draw(struct task *task)
{
    struct task_work *work = task->work;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateY, work->ang.y);
    draw_model(VARS(work)->object->model);
    njPopMatrix(1);
}

static void paternoster_destroy(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_editing(task) && VARS(work)->object != NULL) {
        world_remove(task, VARS(work)->object);
        world_object_free(VARS(work)->object);
    }
    task_destroy_children(task);
}

/* in range, run the frame and its cars. while the set editor is open, draw
 * the frame with a car at the loop's corners instead. while the editor holds
 * the frame, also print which way the cars go */
void paternoster_step(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object;

    if (placement_try_leave_by(task, PATERNOSTER_RANGE)) {
        return;
    }
    if (edit_mode) {
        NJS_POINT3 top_right = {LOOP_HALF_WIDTH, LOOP_HEIGHT, 0.0F};
        NJS_POINT3 top_left = {-LOOP_HALF_WIDTH, LOOP_HEIGHT, 0.0F};
        NJS_POINT3 bottom_left = {-LOOP_HALF_WIDTH, -LOOP_HEIGHT, 0.0F};
        NJS_POINT3 bottom_right = {LOOP_HALF_WIDTH, -LOOP_HEIGHT, 0.0F};
        void (*translate)(NJS_MATRIX *matrix, NJS_VECTOR *vector);

        work->ang.z = 0;
        work->ang.x = 0;
        njPushMatrix(NULL);
        /* @hack spelling: njTranslateV called through translate, which gives
         * njPushMatrix R11 and njTranslateV R12 (0x0c901408, 0x0c90141a).
         * called by name, the two trade, and 12 of paternoster_step's 274
         * words differ */
        translate = njTranslateV;
        translate(NULL, &work->pos);
        TURN(njRotateY, work->ang.y);
        texture_set_stage();
        draw_tree(&paternoster_object);
        njPushMatrix(NULL);
        translate(NULL, &top_right);
        draw_tree(paternoster_car_object);
        njPopMatrix(1);
        njPushMatrix(NULL);
        translate(NULL, &top_left);
        draw_tree(paternoster_car_object);
        njPopMatrix(1);
        njPushMatrix(NULL);
        translate(NULL, &bottom_left);
        draw_tree(paternoster_car_object);
        njPopMatrix(1);
        njPushMatrix(NULL);
        translate(NULL, &bottom_right);
        draw_tree(paternoster_car_object);
        njPopMatrix(1);
        njPopMatrix(1);
        if (placement_editing(task)) {
            if (REVERSED(work)) {
                njPrintC(NJM_LOCATION(10, 5), "  XSCL >= 0.0: NORMAL ");
                njPrintC(NJM_LOCATION(10, 6), "* XSCL <  0.0: REVERSE");
            } else {
                njPrintC(NJM_LOCATION(10, 5), "* XSCL >= 0.0: NORMAL ");
                njPrintC(NJM_LOCATION(10, 6), "  XSCL <  0.0: REVERSE");
            }
        }
        if (task->destructor == NULL) {
            task->destructor = task_kill;
        }
    } else {
        switch (work->mode) {
        case PATERNOSTER_INIT:
            work->mode = PATERNOSTER_RUN;
            VARS(work)->model = &paternoster_object;
            task->destructor = paternoster_destroy;
            task->draw = paternoster_draw;
            VARS(work)->object = object = world_object_alloc();
            object->evalflags |= VARS(work)->model->evalflags &
                                 ~(NJD_EVAL_UNIT_POS | NJD_EVAL_UNIT_ANG);
            object->scl[0] = object->scl[1] = object->scl[2] = 1.0F;
            object->ang[0] = work->ang.x = 0;
            object->ang[1] = work->ang.y;
            object->ang[2] = work->ang.z = 0;
            object->pos[0] = work->pos.x;
            object->pos[1] = work->pos.y;
            object->pos[2] = work->pos.z;
            object->model = VARS(work)->model->model;
            object->child = VARS(work)->model->child;
            object->sibling = VARS(work)->model->sibling;
            world_add(FRAME_FLAGS, task, object);
            task_create_children(paternoster_cars, task);
            break;
        case PATERNOSTER_RUN:
            sound_play_at_for(SE_PATERNOSTER, (int)work, 1, 0, work->pos.x,
                              work->pos.y, work->pos.z, SOUND_UPDATES);
            paternoster_follow(task);
            paternoster_draw(task);
            task_step_children(task);
            break;
        case PATERNOSTER_STOPPED:
        default:
            break;
        }
    }
}

static void paternoster_car_destroy(struct task *task)
{
    struct task_work *work = task->work;

    if (VARS(work)->object != NULL) {
        world_remove(task, VARS(work)->object);
        world_object_free(VARS(work)->object);
    }
}

/* draw the car, and its two wheels turned about z by WHEELS */
static void paternoster_car_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *car = VARS(work)->model;
    NJS_OBJECT *wheel = car->child;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateY, work->ang.y);
    draw_model(car->model);
    njPushMatrix(NULL);
    TURN(njRotateZ, (Angle)WHEELS(work));
    draw_model(wheel->model);
    njPopMatrix(1);
    njPushMatrix(NULL);
    wheel = wheel->sibling;
    TURN(njRotateZ, (Angle)WHEELS(work));
    draw_model(wheel->model);
    njPopMatrix(2);
}

/* move the car along its side of the loop, and turn its wheels. a car goes up
 * the right side, across the top, down the left and back along the bottom. it
 * goes the other way round while the frame's scale x is below 0. its force
 * carries the players on it, and it offers a falling player a target */
static void paternoster_car_step(struct task *task)
{
    /* @hack spelling: frame declared ahead of work, which puts the load of
     * task->parent after FMOV.S @R0,FR14 (MOV.L @(8,R10),R12 at 0x0c90179a).
     * declared after it, the two trade, and 2 of paternoster_car_step's 591
     * words differ */
    struct task_work *frame = task->parent->work;
    struct task_work *work = task->work;
    struct task_force *force = task->force;
    NJS_OBJECT *object;
    Float to[3];

    switch (work->mode) {
    case CAR_INIT:
        VARS(work)->model = paternoster_car_object;
        VARS(work)->object = object = world_object_alloc();
        object->evalflags |= VARS(work)->model->evalflags &
                             ~(NJD_EVAL_UNIT_POS | NJD_EVAL_UNIT_ANG);
        object->scl[0] = object->scl[1] = object->scl[2] = 1.0F;
        object->ang[0] = work->ang.x = 0;
        object->ang[1] = work->ang.y = frame->ang.y;
        object->ang[2] = work->ang.z = 0;
        object->pos[0] = work->pos.x;
        object->pos[1] = work->pos.y;
        object->pos[2] = work->pos.z;
        object->model = VARS(work)->model->model;
        object->child = VARS(work)->model->child;
        object->sibling = VARS(work)->model->sibling;
        world_add(CAR_FLAGS, task, object);
        task->destructor = paternoster_car_destroy;
        task->draw = paternoster_car_draw;
        if (!REVERSED(frame)) {
            work->mode = CAR_FORWARD;
            work->submode = work->flags;
        } else {
            work->mode = CAR_REVERSE;
            work->submode = work->flags;
        }
        force[0].speed.x = 0.0F;
        force[0].speed.y = 0.0F;
        force[0].speed.z = 0.0F;
        force[0].turn.x = 0;
        force[0].turn.y = 0;
        force[0].turn.z = 0;
        break;
    case CAR_FORWARD:
        switch (work->submode) {
        case SIDE_RIGHT:
            force[0].speed.x = 0.0F;
            force[0].speed.y = CAR_SPEED;
            force[0].speed.z = 0.0F;
            work->pos.y += force[0].speed.y;
            to[1] = work->pos.y - frame->pos.y;
            if (to[1] >= LOOP_HEIGHT) {
                work->submode = SIDE_TOP;
            }
            break;
        case SIDE_TOP:
            force[0].speed.x = -CAR_SPEED * njCos(-work->ang.y);
            force[0].speed.y = 0.0F;
            force[0].speed.z = -CAR_SPEED * njSin(-work->ang.y);
            work->pos.x += force[0].speed.x;
            work->pos.z += force[0].speed.z;
            to[0] = work->pos.x - frame->pos.x;
            to[2] = work->pos.z - frame->pos.z;
            to[0] = njSqrt(powf(to[0], 2.0F) + powf(to[2], 2.0F));
            if (to[0] >= LOOP_HALF_WIDTH) {
                work->submode = SIDE_LEFT;
            }
            break;
        case SIDE_LEFT:
            force[0].speed.x = 0.0F;
            force[0].speed.y = -CAR_SPEED;
            force[0].speed.z = 0.0F;
            work->pos.y += force[0].speed.y;
            to[1] = work->pos.y - frame->pos.y;
            if (to[1] <= -LOOP_HEIGHT) {
                work->submode = SIDE_BOTTOM;
            }
            break;
        case SIDE_BOTTOM:
            force[0].speed.x = CAR_SPEED * njCos(-work->ang.y);
            force[0].speed.y = 0.0F;
            force[0].speed.z = CAR_SPEED * njSin(-work->ang.y);
            work->pos.x += force[0].speed.x;
            work->pos.z += force[0].speed.z;
            to[0] = work->pos.x - frame->pos.x;
            to[2] = work->pos.z - frame->pos.z;
            to[0] = njSqrt(powf(to[0], 2.0F) + powf(to[2], 2.0F));
            if (to[0] >= LOOP_HALF_WIDTH) {
                work->submode = SIDE_RIGHT;
            }
            break;
        case SIDE_STOP:
        default:
            work->mode = CAR_STOPPED;
            break;
        }
        WHEELS(work) += WHEEL_TURN;
        paternoster_follow(task);
        paternoster_car_draw(task);
        break;
    case CAR_REVERSE:
        switch (work->submode) {
        case SIDE_RIGHT:
            force[0].speed.x = 0.0F;
            force[0].speed.y = -CAR_SPEED;
            force[0].speed.z = 0.0F;
            work->pos.y += force[0].speed.y;
            to[1] = work->pos.y - frame->pos.y;
            if (to[1] <= -LOOP_HEIGHT) {
                work->submode = SIDE_BOTTOM;
            }
            break;
        case SIDE_BOTTOM:
            force[0].speed.x = -CAR_SPEED * njCos(-work->ang.y);
            force[0].speed.y = 0.0F;
            force[0].speed.z = -CAR_SPEED * njSin(-work->ang.y);
            work->pos.x += force[0].speed.x;
            work->pos.z += force[0].speed.z;
            to[0] = work->pos.x - frame->pos.x;
            to[2] = work->pos.z - frame->pos.z;
            to[0] = njSqrt(powf(to[0], 2.0F) + powf(to[2], 2.0F));
            if (to[0] >= LOOP_HALF_WIDTH) {
                work->submode = SIDE_LEFT;
            }
            break;
        case SIDE_LEFT:
            force[0].speed.x = 0.0F;
            force[0].speed.y = CAR_SPEED;
            force[0].speed.z = 0.0F;
            work->pos.y += force[0].speed.y;
            to[1] = work->pos.y - frame->pos.y;
            if (to[1] >= LOOP_HEIGHT) {
                work->submode = SIDE_TOP;
            }
            break;
        case SIDE_TOP:
            force[0].speed.x = CAR_SPEED * njCos(-work->ang.y);
            force[0].speed.y = 0.0F;
            force[0].speed.z = CAR_SPEED * njSin(-work->ang.y);
            work->pos.x += force[0].speed.x;
            work->pos.z += force[0].speed.z;
            to[0] = work->pos.x - frame->pos.x;
            to[2] = work->pos.z - frame->pos.z;
            to[0] = njSqrt(powf(to[0], 2.0F) + powf(to[2], 2.0F));
            if (to[0] >= LOOP_HALF_WIDTH) {
                work->submode = SIDE_RIGHT;
            }
            break;
        case SIDE_STOP:
        default:
            work->mode = CAR_STOPPED;
            break;
        }
        WHEELS(work) -= WHEEL_TURN;
        paternoster_follow(task);
        paternoster_car_draw(task);
        break;
    case CAR_STOPPED:
    default:
        break;
    }
    force[1] = force[0];
    player_add_target(work->pos.x, work->pos.y, work->pos.z, CAR_TARGET);
}
