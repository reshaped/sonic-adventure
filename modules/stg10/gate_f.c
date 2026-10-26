/* O GATE_F door whose leaves slide apart while the nearest player is near,
 * and stay shut for E-102 */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "debug/print.h"
#include "draw/model.h"
#include "object/placement.h"
#include "player/player.h"
#include "stage/stage.h"
#include "stg10/gate_f.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT gate_f_object;
/* @hack type: the leaves declared as arrays, and read through ->. SHC then
 * loads each leaf's address and reads its model 4 bytes past it (MOV.L
 * @(4,R4),R4 at 0x0c90906e). as NJS_OBJECTs read through ., 54 of
 * gate_f_draw's 88 words differ */
extern NJS_OBJECT gate_f_left_object[];
extern NJS_OBJECT gate_f_right_object[];

/* destroyed beyond 500 units of the camera and player 0 */
#define RANGE (500.0F * 500.0F)
/* @bug logic: the collision record's radius is 150 squared, though the
 * checks take it as a plain distance, so the record reaches 22500 units
 * (the store of 22500.0 at 0x0c908dae) */
#define REACH (150.0F * 150.0F)

/* the leaves open while the nearest player is within OPEN_NEAR of the door
 * along its z axis */
#define OPEN_NEAR 80.0F
#define SLIDE_MAX 33.0F /* how far each leaf slides to open */
/* the leaves' top speed, and the speed they brake to, in units a step */
#define SLIDE_SPEED 1.5F
#define SLIDE_MIN_SPEED 0.1F
/* the speed's gain each step, and its loss each step while braking */
#define SLIDE_ACCEL 0.05F
#define SLIDE_BRAKE 0.08F
/* opening leaves brake once their slide reaches OPEN_BRAKE, and shutting
 * ones once it falls to SHUT_BRAKE */
#define OPEN_BRAKE (SLIDE_MAX * 0.6F)
#define SHUT_BRAKE (SLIDE_MAX * 0.4F)
/* the leaves' x offsets while shut. LEFT_X and RIGHT_X place their models,
 * and -SHAPE_X and SHAPE_X place their boxes' centers */
#define LEFT_X (-53.099606F)
#define RIGHT_X 53.899777F
#define SHAPE_X 19.25F

struct gate_vars {
    Float speed; /* units a step, opening while positive */
    Float slide; /* each leaf's distance from shut, 0 to SLIDE_MAX */
};

#define VARS(work) ((struct gate_vars *)&(work)->vars)
/* 1 while the leaves open, and 0 while they shut */
#define OPENING(work) ((work)->submode)

enum gate_mode { GATE_INIT, GATE_RUN, GATE_SHUT };

static void gate_f_init(struct task *task);
static void gate_f_place(struct task *task);
static void gate_f_slide(struct task *task);
static void gate_f_shut(struct task *task);
static void gate_f_draw(struct task *task);
static void gate_f_destroy(struct task *task);

static struct collision_shape gate_f_shapes[2] = {
    {SHAPE_RECTANGLE,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {-SHAPE_X, 29.3F, 0.5F},
     19.25F,
     30.7F,
     2.5F,
     {0},
     0,
     NULL},
    {SHAPE_RECTANGLE,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {SHAPE_X, 29.3F, 0.5F},
     19.25F,
     30.7F,
     2.5F,
     {0},
     0,
     NULL},
};

void gate_f_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave_by(task, RANGE)) {
        return;
    }
    if (!placement_editing(task)) {
        switch (work->mode) {
        case GATE_INIT:
            gate_f_init(task);
            break;
        case GATE_RUN:
            gate_f_draw(task);
            gate_f_slide(task);
            break;
        case GATE_SHUT:
            gate_f_draw(task);
            gate_f_shut(task);
            break;
        }
    } else {
        switch (work->mode) {
        case GATE_INIT:
            gate_f_init(task);
            break;
        case GATE_RUN:
            gate_f_place(task);
            debug_print(EDITOR_LABEL(15), ": x = %f", work->pos.x);
            debug_print(EDITOR_LABEL(16), ": y = %f", work->pos.y);
            debug_print(EDITOR_LABEL(17), ": z = %f", work->pos.z);
            /* @bug mismatched-call: %f reads a Float, but the call pushes
             * work->ang.y, an Angle (MOV.L R2,@-R15 at 0x0c908d10,
             * where the calls above push with FMOV.S) */
            debug_print(EDITOR_LABEL(19), ": angy = %f", work->ang.y);
            gate_f_draw(task);
            gate_f_slide(task);
            break;
        }
    }
}

static void gate_f_init(struct task *task)
{
    struct task_work *work = task->work;

    task->draw = gate_f_draw;
    task->destructor = gate_f_destroy;
    collision_init(task, gate_f_shapes, COLLISION_OBJECT, 1, 2);
    work->collision->radius = REACH;
    VARS(work)->slide = 0.0F;
    VARS(work)->speed = 0.0F;
    OPENING(work) = 0;
    gate_f_place(task);
    if (get_character() == CHARACTER_E102) {
        work->mode = GATE_SHUT;
    } else {
        work->mode = GATE_RUN;
    }
}

/* the door takes gate_f_object's position and angle y over its set entry's */
static void gate_f_place(struct task *task)
{
    struct task_work *work = task->work;

    work->pos.x = gate_f_object.pos[0];
    work->pos.y = gate_f_object.pos[1];
    work->pos.z = gate_f_object.pos[2];
    work->ang.y = gate_f_object.ang[1];
}

static void gate_f_slide(struct task *task)
{
    struct task_work *work = task->work;
    NJS_POINT3 pos;
    NJS_POINT3 local;
    Angle ang;

    pos = player_work[player_nearest(&work->pos)]->pos;
    njPushMatrix(&_unit_matrix_);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    njInvertMatrix(NULL);
    njCalcPoint(NULL, &pos, &local);
    njPopMatrix(1);
    OPENING(work) = local.z >= -OPEN_NEAR && local.z <= OPEN_NEAR;
    if (OPENING(work)) {
        if (VARS(work)->slide < OPEN_BRAKE) {
            VARS(work)->speed += SLIDE_ACCEL;
            if (VARS(work)->speed > SLIDE_SPEED) {
                VARS(work)->speed = SLIDE_SPEED;
            }
        } else {
            VARS(work)->speed -= SLIDE_BRAKE;
            if (VARS(work)->speed < SLIDE_MIN_SPEED) {
                VARS(work)->speed = SLIDE_MIN_SPEED;
            }
        }
    } else {
        if (VARS(work)->slide > SHUT_BRAKE) {
            VARS(work)->speed -= SLIDE_ACCEL;
            if (VARS(work)->speed < -SLIDE_SPEED) {
                VARS(work)->speed = -SLIDE_SPEED;
            }
        } else {
            VARS(work)->speed += SLIDE_BRAKE;
            if (VARS(work)->speed > -SLIDE_MIN_SPEED) {
                VARS(work)->speed = -SLIDE_MIN_SPEED;
            }
        }
    }
    VARS(work)->slide += VARS(work)->speed;
    if (VARS(work)->slide < 0.0F) {
        VARS(work)->slide = 0.0F;
    } else if (VARS(work)->slide > SLIDE_MAX) {
        VARS(work)->slide = SLIDE_MAX;
    }
    work->collision->shapes[0].center.x = -SHAPE_X - VARS(work)->slide;
    work->collision->shapes[1].center.x = SHAPE_X + VARS(work)->slide;
    collision_register(work);
}

static void gate_f_shut(struct task *task)
{
    struct task_work *work = task->work;

    VARS(work)->slide = 0.0F;
    work->collision->shapes[0].center.x = -SHAPE_X - VARS(work)->slide;
    work->collision->shapes[1].center.x = SHAPE_X + VARS(work)->slide;
    collision_register(work);
}

static void gate_f_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (frame_step == 0) {
        texture_set_stage();
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        njPushMatrix(NULL);
        njTranslate(NULL, LEFT_X - VARS(work)->slide,
                    gate_f_left_object->pos[1], gate_f_left_object->pos[2]);
        draw_model(gate_f_left_object->model);
        njPopMatrix(1);
        njPushMatrix(NULL);
        njTranslate(NULL, RIGHT_X + VARS(work)->slide,
                    gate_f_right_object->pos[1], gate_f_right_object->pos[2]);
        draw_model(gate_f_right_object->model);
        njPopMatrix(1);
        njPopMatrix(1);
    }
}

static void gate_f_destroy(struct task *task)
{
}
