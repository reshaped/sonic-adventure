/* O DOUBLE_GATE two doors whose leaves slide apart while the nearest player
 * is near. for E-102, they stay shut */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "debug/print.h"
#include "draw/model.h"
#include "object/placement.h"
#include "player/player.h"
#include "stage/stage.h"
#include "stg10/double_gate.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

/* the first door's leaves, then the second's */
/* @hack type: the leaves declared as arrays. SHC then pools each leaf's
 * address and adds 8 to reach pos (ADD #8,R12 at 0x0c909a0c). as
 * NJS_OBJECTs, 85 of double_gate_draw's 122 words differ */
extern NJS_OBJECT double_gate_leaf_object_0[];
extern NJS_OBJECT double_gate_leaf_object_1[];
extern NJS_OBJECT double_gate_leaf_object_2[];
extern NJS_OBJECT double_gate_leaf_object_3[];
extern NJS_MODEL double_gate_leaf_model_0;
extern NJS_MODEL double_gate_leaf_model_1;
extern NJS_MODEL double_gate_leaf_model_2;

/* the gate is destroyed beyond 500 units of the camera and player 0 */
#define RANGE (500.0F * 500.0F)
/* @bug logic: the collision record's radius is 150 squared, though the
 * checks take it as a plain distance, so the record reaches 22500 units
 * (the store of 22500.0 at 0x0c909708) */
#define REACH (150.0F * 150.0F)

/* the leaves open while the nearest player is within OPEN_NEAR of the gate
 * along its x axis */
#define OPEN_NEAR 80.0F
#define SLIDE_MAX 70.0F /* how far each leaf slides to open */
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
/* z offset of each leaf's shape while shut */
#define SHAPE_Z 55.0F

struct gate_vars {
    Float speed; /* units a step, opening while positive */
    Float slide; /* each leaf's distance from shut, 0 to SLIDE_MAX */
};

#define VARS(work) ((struct gate_vars *)&(work)->vars)
/* 1 while the leaves open, and 0 while they shut */
#define OPENING(work) ((work)->submode)

enum gate_mode { GATE_INIT, GATE_RUN, GATE_SHUT };

static void double_gate_init(struct task *task);
static void double_gate_reset(struct task *task);
static void double_gate_slide(struct task *task);
static void double_gate_shut(struct task *task);
static void double_gate_draw(struct task *task);
static void double_gate_destroy(struct task *task);

static struct collision_shape double_gate_shapes[4] = {
    {SHAPE_RECTANGLE,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {-24.0F, 46.2F, -SHAPE_Z},
     3.05F,
     46.15F,
     55.0F,
     {0},
     0,
     NULL},
    {SHAPE_RECTANGLE,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {-24.0F, 46.2F, SHAPE_Z},
     3.05F,
     46.15F,
     55.0F,
     {0},
     0,
     NULL},
    {SHAPE_RECTANGLE,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {32.0F, 46.2F, -SHAPE_Z},
     3.05F,
     46.15F,
     55.0F,
     {0},
     0,
     NULL},
    {SHAPE_RECTANGLE,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {32.0F, 46.2F, SHAPE_Z},
     3.05F,
     46.15F,
     55.0F,
     {0},
     0,
     NULL},
};

void double_gate_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave_by(task, RANGE)) {
        return;
    }
    if (!placement_editing(task)) {
        switch (work->mode) {
        case GATE_INIT:
            double_gate_init(task);
            break;
        case GATE_RUN:
            double_gate_draw(task);
            double_gate_slide(task);
            break;
        case GATE_SHUT:
            double_gate_draw(task);
            double_gate_shut(task);
            break;
        }
    } else {
        switch (work->mode) {
        case GATE_INIT:
            double_gate_init(task);
            break;
        case GATE_RUN:
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Y), "-> ROTATE Y");
            double_gate_draw(task);
            double_gate_slide(task);
            break;
        }
    }
}

static void double_gate_init(struct task *task)
{
    struct task_work *work = task->work;

    task->draw = double_gate_draw;
    task->destructor = double_gate_destroy;
    double_gate_reset(task);
    if (get_character() == CHARACTER_E102) {
        work->mode = GATE_SHUT;
    } else {
        work->mode = GATE_RUN;
    }
}

static void double_gate_reset(struct task *task)
{
    struct task_work *work = task->work;

    collision_init(task, double_gate_shapes, COLLISION_OBJECT, 1, 4);
    work->collision->radius = REACH;
    VARS(work)->slide = 0.0F;
    VARS(work)->speed = 0.0F;
    OPENING(work) = 0;
}

static void double_gate_slide(struct task *task)
{
    struct task_work *work = task->work;
    NJS_POINT3 pos;
    NJS_POINT3 local;

    pos = player_work[player_nearest(&work->pos)]->pos;
    njPushMatrix(&_unit_matrix_);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateY, work->ang.y);
    njInvertMatrix(NULL);
    njCalcPoint(NULL, &pos, &local);
    njPopMatrix(1);
    OPENING(work) = local.x >= -OPEN_NEAR && local.x <= OPEN_NEAR;
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
    work->collision->shapes[0].center.z = -SHAPE_Z - VARS(work)->slide;
    work->collision->shapes[1].center.z = SHAPE_Z + VARS(work)->slide;
    work->collision->shapes[2].center.z = -SHAPE_Z - VARS(work)->slide;
    work->collision->shapes[3].center.z = SHAPE_Z + VARS(work)->slide;
    collision_register(work);
}

static void double_gate_shut(struct task *task)
{
    /* @hack redundant: two points never used, which give the image's 24-byte
     * frame (ADD #-24,R15 at 0x0c909940); without them, 45 of
     * double_gate_shut's 46 words differ */
    NJS_POINT3 pos;
    NJS_POINT3 local;
    struct task_work *work = task->work;

    VARS(work)->slide = 0.0F;
    work->collision->shapes[0].center.z = -SHAPE_Z - VARS(work)->slide;
    work->collision->shapes[1].center.z = SHAPE_Z + VARS(work)->slide;
    work->collision->shapes[2].center.z = -SHAPE_Z - VARS(work)->slide;
    work->collision->shapes[3].center.z = SHAPE_Z + VARS(work)->slide;
    collision_register(work);
}

/* the fourth leaf draws with the second leaf's model */
static void double_gate_draw(struct task *task)
{
    struct task_work *work = task->work;

    if (frame_step == 0) {
        texture_set_stage();
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN(njRotateY, work->ang.y);
        njPushMatrix(NULL);
        njTranslate(NULL, double_gate_leaf_object_0->pos[0],
                    double_gate_leaf_object_0->pos[1],
                    double_gate_leaf_object_0->pos[2] - VARS(work)->slide);
        draw_model(&double_gate_leaf_model_0);
        njPopMatrix(1);
        njPushMatrix(NULL);
        njTranslate(NULL, double_gate_leaf_object_1->pos[0],
                    double_gate_leaf_object_1->pos[1],
                    double_gate_leaf_object_1->pos[2] + VARS(work)->slide);
        draw_model(&double_gate_leaf_model_1);
        njPopMatrix(1);
        njPushMatrix(NULL);
        njTranslate(NULL, double_gate_leaf_object_2->pos[0],
                    double_gate_leaf_object_2->pos[1],
                    double_gate_leaf_object_2->pos[2] - VARS(work)->slide);
        draw_model(&double_gate_leaf_model_2);
        njPopMatrix(1);
        njPushMatrix(NULL);
        njTranslate(NULL, double_gate_leaf_object_3->pos[0],
                    double_gate_leaf_object_3->pos[1],
                    double_gate_leaf_object_3->pos[2] + VARS(work)->slide);
        draw_model(&double_gate_leaf_model_1);
        njPopMatrix(1);
        njPopMatrix(1);
    }
}

static void double_gate_destroy(struct task *task)
{
}
