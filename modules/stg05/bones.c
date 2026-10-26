/* O BONEHEAD, a burning skull, and O BONESTICK, a stick topped by one in
 * either of two poses
 *
 * each draws at one more than its set entry's scale x, and its flame at
 * one more than its scale y. the stick's pose is 0 when scale z's whole part
 * is even, and 1 when it's odd. the set editor labels the stick's scale y
 * FIRE SCL and its scale z TYPE CHANGE */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/model.h"
#include "effect/dust.h"
#include "effect/flame.h"
#include "object/common.h"
#include "object/placement.h"
#include "stg05/bones.h"
#include "system/task.h"
#include "system/util.h"

extern NJS_TEXLIST obj_mountain_texlist;
extern NJS_MODEL bone_skull_model;
extern NJS_MODEL bone_stick_model;

/* the step does nothing in BONE_IDLE, which nothing in this file sets */
enum bone_mode { BONE_INIT = 0, BONE_LIT = 1, BONE_IDLE = 3 };

/* the stick's pose, 0 or 1 */
#define POSE(work) ((work)->tick)

/* a flame's scale at a scale y of 0 */
#define FLAME_SCALE 0.12F
/* the skull's scale on the stick at a scale x of 0, over the stick's own */
#define SKULL_SCALE 1.18F

/* the skull's and the stick's shapes, which object_scale_shapes scales by
 * one more than the set entry's scale */
static struct collision_shape bone_head_shape = {
    SHAPE_SPHERE,
    0,
    PUSH_ALL | PUSH_FIRM(PUSH_ALL),
    {0.0F, 0.0F, 0.0F},
    1.5F,
    0.0F,
    0.0F,
    {0, 0, 0},
    0,
    NULL,
};
static struct collision_shape bone_stick_shape = {
    SHAPE_CYLINDER,
    0,
    PUSH_ALL | PUSH_FIRM(PUSH_ALL),
    {0.0F, 5.0F, 0.0F},
    2.0F,
    5.0F,
    0.0F,
    {0, 0, 0},
    0,
    NULL,
};

/* both objects' flames. each step sets pos, and sets scale to FLAME_SCALE
 * times one more than scale y */
static struct dust_params bone_flame = {
    FLAME_SCALE,
    0.01F,
    0.19F,
    0.065F,
    0.005F,
    {0.0F, 0.0F, 0.0F},
    {0.0F, 0.0F, 0.0F},
    {1.0F, 1.0F, 1.0F, 1.0F},
};

/* the skull's place and angles on the stick in poses 0 and 1, in the stick's
 * model space */
static NJS_POINT3 bone_skull_pos_0 = {0.2F, 9.72F, -0.0285F};
static struct angles bone_skull_ang_0 = {3950, 0, 0};
static NJS_POINT3 bone_skull_pos_1 = {0.601F, 9.0882F, 0.23F};
static struct angles bone_skull_ang_1 = {29127, 63715, 29673};

static const NJS_POINT3 bone_origin = {0.0F, 0.0F, 0.0F};

static void bone_head_destroy(struct task *task);
static void bone_head_draw(struct task *task);
static void bone_stick_destroy(struct task *task);
static void bone_stick_draw(struct task *task);
static void bone_stick_labels(struct task_work *work);
static void bone_stick_pick(struct task_work *work);

/* each step, a flame starts at the skull's center */
void bone_head_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return;
    }
    switch (work->mode) {
    case BONE_INIT:
        collision_init(task, &bone_head_shape, COLLISION_OBJECT, 1, 1);
        object_scale_shapes(work);
        task->destructor = bone_head_destroy;
        task->draw = bone_head_draw;
        work->mode = BONE_LIT;
        break;
    case BONE_LIT:
        /* @hack redundant: placement_editing's result tested for nothing, which
         * keeps task in R14 and work in R13 (0x0c906482, 0x0c90648c); without
         * it, the two trade, the function is 8 bytes shorter and 41 of 70
         * words differ */
        if (placement_editing(task)) {
        }
        collision_register(work);
        bone_flame.pos = work->pos;
        bone_flame.scale = (work->scl.y + 1.0F) * FLAME_SCALE;
        flame_create_from(&bone_flame);
        bone_head_draw(task);
        break;
    case BONE_IDLE:
        break;
    }
}

static void bone_head_destroy(struct task *task)
{
}

static void bone_head_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;
    Float scale;

    if (!draw_in_view(work->pos.x, work->pos.y, work->pos.z, 20.0F)) {
        return;
    }
    njSetTexture(&obj_mountain_texlist);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    TURN_VIA(njRotateY, work->ang.y, ang);
    scale = work->scl.x + 1.0F;
    njScale(NULL, scale, scale, scale);
    draw_model(&bone_skull_model);
    njPopMatrix(1);
}

/* each step, a flame starts above the stick's foot, at the skull's height in
 * its pose. the flame ignores the stick's turn and the skull's place along x
 * and z */
void bone_stick_step(struct task *task)
{
    struct task_work *work = task->work;
    NJS_POINT3 pos;
    /* NOLINTNEXTLINE(clang-diagnostic-unused-but-set-variable) */
    NJS_POINT3 origin;
    /* @hack redundant: top_1 and top_0, a point for each pose, each copied to
     * pos. the two give the stack frame 48 bytes (ADD #-48,R15 at
     * 0x0c906626); with one point for both, the frame takes 36, the function
     * is 10 bytes shorter and 129 of 138 words differ */
    NJS_POINT3 top_1;
    NJS_POINT3 top_0;
    Float height;
    Float one;

    if (placement_try_leave(task)) {
        return;
    }
    switch (work->mode) {
    case BONE_INIT:
        collision_init(task, &bone_stick_shape, COLLISION_OBJECT, 1, 1);
        object_scale_shapes(work);
        task->destructor = bone_stick_destroy;
        task->draw = bone_stick_draw;
        work->mode = BONE_LIT;
        break;
    case BONE_LIT:
        if (placement_editing(task)) {
            bone_stick_labels(work);
        }
        collision_register(work);
        bone_stick_pick(work);
        /* count the stick's steps, which nothing reads */
        work->timer++;
        /* @hack redundant: bone_origin copied to origin and never read;
         * without it, the function is 44 bytes shorter and 129 of 138 words
         * differ */
        origin = bone_origin;
        /* @hack spelling: 1 through a local, which loads it once, into FR5 at
         * 0x0c9066b6, before the copy of bone_origin; as the literal, it is
         * loaded later, into FR4, and 9 of 138 words differ */
        one = 1.0F;
        if (POSE(work) == 1) {
            height = bone_skull_pos_1.y * (work->scl.x + one);
            top_1 = work->pos;
            top_1.y += height;
            pos = top_1;
        } else {
            height = bone_skull_pos_0.y * (work->scl.x + one);
            top_0 = work->pos;
            top_0.y += height;
            pos = top_0;
        }
        bone_flame.pos = pos;
        bone_flame.scale = (work->scl.y + one) * FLAME_SCALE;
        flame_create_from(&bone_flame);
        bone_stick_draw(task);
        break;
    case BONE_IDLE:
        break;
    }
}

static void bone_stick_destroy(struct task *task)
{
}

/* draw the stick, then the skull at its pose's place on it, scaled by scale
 * x plus SKULL_SCALE */
static void bone_stick_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;
    Float scale;

    if (!draw_in_view(work->pos.x, work->pos.y, work->pos.z, 20.0F)) {
        return;
    }
    njSetTexture(&obj_mountain_texlist);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    TURN_VIA(njRotateY, work->ang.y, ang);
    scale = work->scl.x + 1.0F;
    njScale(NULL, scale, scale, scale);
    draw_model(&bone_stick_model);
    njSetTexture(&obj_mountain_texlist);
    if (POSE(work) == 0) {
        njTranslateV(NULL, &bone_skull_pos_0);
        TURN_VIA(njRotateZ, bone_skull_ang_0.z, ang);
        TURN_VIA(njRotateX, bone_skull_ang_0.x, ang);
        TURN_VIA(njRotateY, bone_skull_ang_0.y, ang);
    } else {
        njTranslateV(NULL, &bone_skull_pos_1);
        TURN_VIA(njRotateZ, bone_skull_ang_1.z, ang);
        TURN_VIA(njRotateX, bone_skull_ang_1.x, ang);
        TURN_VIA(njRotateY, bone_skull_ang_1.y, ang);
    }
    scale = work->scl.x + SKULL_SCALE;
    njScale(NULL, scale, scale, scale);
    draw_model(&bone_skull_model);
    njPopMatrix(1);
}

static void bone_stick_labels(struct task_work *work)
{
    njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_Y), "SCL Y -> FIRE SCL");
    njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_Z), "SCL Z -> TYPE CHANGE");
}

static void bone_stick_pick(struct task_work *work)
{
    if ((int)work->scl.z % 2 == 0) {
        POSE(work) = 0;
    } else {
        POSE(work) = 1;
    }
}
