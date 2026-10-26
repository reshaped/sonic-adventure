/* O SARM: an arm that swings through its action, resting 250 steps between
 * swings */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "draw/pose.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "stg09/arm.h"
#include "stg09/casino.h"
#include "system/task.h"
#include "system/util.h"

extern NJS_OBJECT arm_object;
extern NJS_ACTION arm_action;

/* the task destroys itself beyond about 250 of the camera and player 0.
 * placement_try_leave_by takes the range squared */
#define RANGE ((250.0F * 250.0F) + 10.0F)

/* arm_action's frame, 0 while the arm rests */
#define FRAME(work) ((work)->vars.f[0])

static Float arm_speed = 0.2F; /* the frames the action moves on each step */
static int arm_wait = 250; /* a swing starts once the timer passes this */
/* the arm's capsule at rest. while the arm swings, arm_run moves the record's
 * copy as arm_pose poses it */
static struct collision_shape arm_shape = {
    SHAPE_CAPSULE,
    0,
    PUSH_ALL | PUSH_FIRM(PUSH_ALL),
    {-5.0F, 17.5F, -4.5F},
    5.0F,
    17.5F,
    0.0F,
    {0xfa00, 0, 0},
    0,
    NULL,
};

/* set point to arm_shape's center as arm_action's root node carries it at
 * frame, in the arm's coordinates. angle, unless null, gets the node's turn
 * about x. returns TRUE, as pose_action always does */
static Bool arm_pose(Float frame, NJS_POINT3 *point, Angle *angle)
{
    /* Ninja's matrix calls need the matrix to start on 8 bytes, and the stack
     * holds it on 4. matrix is room's address rounded down to 8: room itself,
     * or pad, the 4 bytes below it. the matrix fits from either. the struct
     * keeps matrix ahead of pad and room in the frame, as the image has it */
    struct {
        NJS_MATRIX *matrix;
        Float pad;
        NJS_MATRIX room;
    } pose;
    NJS_POINT3 center;
    NJS_VECTOR up;
    Float *at;

    pose.matrix = (NJS_MATRIX *)((Uint32)&pose.room & ~7);
    njPushMatrix(&_unit_matrix_);
    if (pose_action(NULL, &arm_action, frame, 0, pose.matrix)) {
        /* @hack spelling: the center read through arm_shape's address, which
         * loads it into R3 (0x0c917278) for the three reads. as
         * arm_shape.center, 3 of 89 words differ */
        center = (&arm_shape)->center;
        /* @hack spelling: up stored through a cursor, which ADD #4 steps
         * (0x0c91729a to 0x0c9172a4). member by member, 53 of 89 words
         * differ */
        at = &up.x;
        *at++ = 0.0F;
        *at++ = 1.0F;
        *at = 0.0F;
        njCalcVector(pose.matrix, &up, &up);
        njCalcPoint(pose.matrix, &center, point);
        njPopMatrix(1);
        if (angle) {
            *angle = (Angle)(atan2f(up.z, up.y) * ANGLE_PER_RADIAN);
        }
        return TRUE;
    }
    njPopMatrix(1);
    return FALSE;
}

static void arm_init(struct task *task);
static void arm_destroy(struct task *task);
static void arm_run(struct task *task);
static void arm_draw(struct task *task);

void arm_step(struct task *task)
{
    /* @hack redundant: work, read and never used, which keeps task in R14
     * across the range check (MOV R4,R14 at 0x0c9172fa). without it, 12 of
     * 16 words differ */
    struct task_work *work = task->work;

    if (!placement_try_leave_by(task, RANGE)) {
        arm_init(task);
    }
}

static void arm_init(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_editing(task)) {
        collision_init(task, &arm_shape, COLLISION_OBJECT, 1, 1);
    }
    task->draw = arm_draw;
    task->step = arm_run;
    task->destructor = arm_destroy;
    work->timer = 0;
    FRAME(work) = 0.0F;
}

static void arm_destroy(struct task *task)
{
}

static void arm_run(struct task *task)
{
    struct task_work *work = task->work;
    struct collision_shape *shape;
    NJS_POINT3 point;
    Angle angle;

    if (placement_try_leave_by(task, RANGE)) {
        return;
    }
    arm_draw(task);
    if (placement_editing(task)) {
        return;
    }
    if (FRAME(work) == 0.0F) {
        if (TASK_TIMER(work) > arm_wait) {
            FRAME(work) += arm_speed;
        }
        /* @hack spelling: arm_shape through shape, set inside the first
         * copy, which keeps its address in R4 for both reads (0x0c91740a).
         * as arm_shape.center and arm_shape.ang.x, 78 of 113 words differ */
        work->collision->shapes->center = (shape = &arm_shape)->center;
        work->collision->shapes->ang.x = shape->ang.x;
    } else {
        FRAME(work) += arm_speed;
        if (FRAME(work) >= arm_action.motion->nbFrame) {
            FRAME(work) = 0.0F;
            work->timer = 0;
        }
        if (arm_pose(FRAME(work), &point, &angle)) {
            work->collision->shapes->center = point;
            /* @hack spelling: ang.x read through arm_shape's address, which
             * loads it into R1 (0x0c917470). as arm_shape.ang.x, 9 of 113
             * words differ */
            work->collision->shapes->ang.x = angle + (&arm_shape)->ang.x;
        }
    }
    collision_register(work);
    /* @hack spelling: the test for the wrap to 0, written <= 0 for the
     * image's CMP/PL (0x0c91748e). the timer is unsigned, so the test holds
     * at 0 alone, and the decrement holds the timer at 65535. as == 0, the
     * test is TST, and 2 of 113 words differ */
    if (++TASK_TIMER(work) <= 0) {
        --TASK_TIMER(work);
    }
}

static void arm_draw(struct task *task)
{
    struct task_work *work = task->work;
    /* @hack spelling: object and action through locals, set as they are
     * declared, ahead of the range check. their addresses wait in the frame
     * (0x0c9174b0, 0x0c9174b4). as &arm_object and &arm_action at the draws,
     * 54 of 57 words differ */
    NJS_OBJECT *object = &arm_object;
    NJS_ACTION *action = &arm_action;
    Angle ang;

    if (placement_try_leave_by(task, RANGE)) {
        return;
    }
    casino_set_texture(CASINO_TEXTURES_ACTS_12);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    if (FRAME(work) == 0.0F) {
        draw_tree(object);
    } else {
        draw_action(action, FRAME(work));
    }
    njPopMatrix(1);
}
