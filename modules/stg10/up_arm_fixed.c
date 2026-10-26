/* O UP_ARM_FIX robot arm of O UP_ARM, held still at one frame */
#include <ninja.h>
#include <sg_xpt.h>

#include "debug/editor.h"
#include "debug/print.h"
#include "draw/draw.h"
#include "object/placement.h"
#include "stg10/up_arm_fixed.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

/* @hack type: up_arm_action as an array. SHC then pools its address and
 * reads its motion at @(4,R5) (0x0c905484). as one NJS_ACTION, SHC pools the
 * address of its motion field, up_arm_fixed_pose runs 2 bytes short and 19
 * of its 63 words differ, and 7 of up_arm_fixed_step's 71 */
extern NJS_ACTION up_arm_action[];
/* @hack type: the hand and claws as arrays, whose addresses SHC then takes in
 * R3, R2, R1 and R3 in turn (0x0c9054d6 to 0x0c9054f0). as NJS_OBJECTs, SHC
 * takes each in R3, and 17 of up_arm_fixed_draw's 70 words differ */
extern NJS_OBJECT up_arm_hand_object[];
extern NJS_OBJECT up_arm_claw_object_0[];
extern NJS_OBJECT up_arm_claw_object_1[];
extern NJS_OBJECT up_arm_claw_object_2[];

/* destroyed beyond 500 units of the camera and player 0 */
#define RANGE (500.0F * 500.0F)

struct arm_vars {
    Float frame;
    Float scale;
};

#define VARS(work) ((struct arm_vars *)&(work)->vars)

enum arm_mode { ARM_INIT, ARM_RUN };

static void up_arm_fixed_init(struct task *task);
static void up_arm_fixed_pose(struct task *task);
static void up_arm_fixed_draw(struct task *task);
static void up_arm_fixed_destroy(struct task *task);

void up_arm_fixed_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave_by(task, RANGE)) {
        return;
    }
    if (!placement_editing(task)) {
        switch (work->mode) {
        case ARM_INIT:
            up_arm_fixed_init(task);
            break;
        case ARM_RUN:
            /* @hack redundant: the draw in a loop of one pass, which keeps it
             * a BSR followed by the step's own return (0x0c905378); alone, it
             * becomes a tail jump, the step runs 2 bytes long and 46 of 71
             * words differ */
            do {
                up_arm_fixed_draw(task);
            } while (0);
            break;
        }
    } else {
        switch (work->mode) {
        case ARM_INIT:
            up_arm_fixed_init(task);
            break;
        case ARM_RUN:
            up_arm_fixed_pose(task);
            up_arm_fixed_draw(task);
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Y), "-> ROTATE Y");
            debug_print(EDITOR_LABEL(EDITOR_ROW_SCL_X), "-> SCALE(*10)");
            debug_print(EDITOR_LABEL(EDITOR_ROW_SCL_Y), "-> ANIM FRAM(0-%d)",
                        up_arm_action->motion->nbFrame - 1);
            break;
        }
    }
}

static void up_arm_fixed_init(struct task *task)
{
    struct task_work *work = task->work;

    task->draw = up_arm_fixed_draw;
    task->destructor = up_arm_fixed_destroy;
    up_arm_fixed_pose(task);
    work->mode = ARM_RUN;
}

/* the arm's scale is a tenth of scale x, held from 0.04 to 1. its frame is
 * scale y, held from 0 to the action's last */
static void up_arm_fixed_pose(struct task *task)
{
    struct task_work *work = task->work;

    VARS(work)->scale = work->scl.x * 0.1F;
    if (VARS(work)->scale < 0.04F) {
        VARS(work)->scale = 0.04F;
    } else if (VARS(work)->scale > 1.0F) {
        VARS(work)->scale = 1.0F;
    }
    VARS(work)->frame = work->scl.y;
    if (VARS(work)->frame < 0.0F) {
        VARS(work)->frame = 0.0F;
    } else if (VARS(work)->frame > up_arm_action->motion->nbFrame - 1.0F) {
        VARS(work)->frame = up_arm_action->motion->nbFrame - 1.0F;
    }
}

/* draw the arm without its hand and claws. their hide flags stay set after
 * the draw, and up_arm_draw sets or clears them again before its own */
static void up_arm_fixed_draw(struct task *task)
{
    struct task_work *work = task->work;
    Float scale;

    if (frame_step == 0) {
        texture_set_stage();
        up_arm_hand_object->evalflags |= NJD_EVAL_HIDE;
        up_arm_claw_object_0->evalflags |= NJD_EVAL_HIDE;
        up_arm_claw_object_1->evalflags |= NJD_EVAL_HIDE;
        up_arm_claw_object_2->evalflags |= NJD_EVAL_HIDE;
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN(njRotateY, work->ang.y);
        scale = VARS(work)->scale;
        njScale(NULL, scale, scale, scale);
        draw_action(up_arm_action, VARS(work)->frame);
        njPopMatrix(1);
    }
}

static void up_arm_fixed_destroy(struct task *task)
{
}
