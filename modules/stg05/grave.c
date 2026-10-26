/* O GRAVE, a gravestone that rocks between two lit candles
 *
 * the grave draws at one more than its set entry's scale x, and a scale x of
 * -1 or less counts as 0. its scale z is the degrees the rocking advances
 * each step, truncated to a whole number. the grave ignores its scale y. the
 * set editor labels scale y FIRE SCL and scale z SPEED ROT */
#include <ninja.h>
#include <sg_xpt.h>

#include "camera/camera.h"
#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/model.h"
#include "draw/tree.h"
#include "effect/dust.h"
#include "effect/flame.h"
#include "object/placement.h"
#include "stg05/grave.h"
#include "system/task.h"
#include "system/util.h"

extern NJS_TEXLIST obj_mountain_texlist;
extern NJS_MODEL grave_base_model;
extern NJS_OBJECT grave_stone_object;
extern NJS_OBJECT grave_candle_1_object;
extern NJS_OBJECT grave_candle_2_object;

struct grave_vars {
    Float lean; /* the stone's lean, in degrees */
};

#define VARS(work) ((struct grave_vars *)&(work)->vars)
#define PHASE(work) TASK_TIMER(work) /* the rocking's phase, in degrees */

/* the step does nothing in GRAVE_IDLE, which nothing in this file sets */
enum grave_mode { GRAVE_INIT = 0, GRAVE_LIT = 1, GRAVE_IDLE = 3 };

/* the stone's largest lean either way, in degrees */
#define GRAVE_LEAN 10.0F

/* a capsule about the stone, which grave_step scales with the grave */
static struct collision_shape grave_shape = {
    SHAPE_CAPSULE,
    0,
    PUSH_ALL | PUSH_FIRM(PUSH_ALL),
    {0.0F, 8.0F, 0.0F},
    6.0F,
    5.0F,
    0.0F,
    {0},
    0,
    NULL,
};

/* the candles' flames: the first untinted, the second red */
static struct dust_params grave_flame_1 = {
    0.2F,
    0.01F,
    0.19F,
    0.065F,
    0.025F,
    {0.0F, 0.0F, 0.0F},
    {0.0F, 0.0F, 0.0F},
    {1.0F, 1.0F, 1.0F, 1.0F},
};
static struct dust_params grave_flame_2 = {
    0.2F,
    0.01F,
    0.19F,
    0.05F,
    0.0F,
    {0.0F, 0.0F, 0.0F},
    {0.0F, 0.5F, 0.0F},
    {1.0F, 1.0F, 0.2F, 0.1F},
};

static void grave_destroy(struct task *task);
static void grave_draw(struct task *task);

void grave_step(struct task *task)
{
    struct task_work *work = task->work;
    struct collision_shape *shape;

    if (placement_try_leave(task)) {
        return;
    }
    switch (work->mode) {
    case GRAVE_INIT:
        collision_init(task, &grave_shape, COLLISION_OBJECT, 1, 1);
        task->destructor = grave_destroy;
        task->draw = grave_draw;
        if (work->scl.x <= -1.0F) {
            work->scl.x = 0.0F;
        }
        shape = work->collision->shapes;
        shape->radius = shape->radius * (work->scl.x + 1.0F);
        shape->size_y = shape->size_y * (work->scl.x + 1.0F);
        shape->size_z = shape->size_z * (work->scl.x + 1.0F);
        shape->center.x = shape->center.x * (work->scl.x + 1.0F);
        shape->center.y = shape->center.y * (work->scl.x + 1.0F);
        shape->center.z = shape->center.z * (work->scl.x + 1.0F);
        work->mode = GRAVE_LIT;
        break;
    case GRAVE_LIT:
        if (placement_editing(task)) {
            njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_Y), "<- FIRE SCL");
            njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_Z), "<- SPEED ROT");
        }
        collision_register(work);
        /* this block raises a flame from each candle's tip. it then moves
         * each tip along z, as the camera's y and x angles turn it. the move
         * is 4 times one more than scale x. no code reads the moved tips */
        {
            /* @hack redundant: origin built in the block after the editor's
             * labels and never read, which gives the frame 64 bytes (ADD
             * #-64,R15 at 0x0c906b96); without it, the frame is 52, the
             * function is 18 bytes shorter and 448 of 568 words differ */
            NJS_POINT3 origin = {0.0F, 0.0F, 0.0F};
            NJS_POINT3 tip_1;
            NJS_POINT3 flame_1;
            NJS_POINT3 tip_2;
            NJS_POINT3 flame_2;
            NJS_OBJECT *stone = &grave_stone_object;
            NJS_OBJECT *candle_1 = &grave_candle_1_object;
            NJS_OBJECT *candle_2 = &grave_candle_2_object;
            Float scale;
            Angle ang;

            tip_1.x = 0.0F;
            tip_1.y = 0.8F;
            tip_1.z = 0.0F;
            njPushMatrix(&_unit_matrix_);
            njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
            TURN_VIA(njRotateZ, work->ang.z, ang);
            TURN_VIA(njRotateX, work->ang.x, ang);
            TURN_VIA(njRotateY, work->ang.y, ang);
            scale = work->scl.x + 1.0F;
            njScale(NULL, scale, scale, scale);
            njTranslate(NULL, stone->pos[0], stone->pos[1], stone->pos[2]);
            TURN_VIA(njRotateZ, stone->ang[2], ang);
            TURN_VIA(njRotateX, stone->ang[0], ang);
            TURN_VIA(njRotateY, stone->ang[1], ang);
            njTranslate(NULL, candle_1->pos[0], candle_1->pos[1],
                        candle_1->pos[2]);
            TURN_VIA(njRotateZ, candle_1->ang[2], ang);
            TURN_VIA(njRotateX, candle_1->ang[0], ang);
            TURN_VIA(njRotateY, candle_1->ang[1], ang);
            njCalcPoint(NULL, &tip_1, &flame_1);
            njPopMatrix(1);
            grave_flame_1.pos = flame_1;
            flame_create_from(&grave_flame_1);
            tip_1.x = (work->scl.x + 1.0F) * 0.0F;
            tip_1.y = (work->scl.x + 1.0F) * 0.0F;
            tip_1.z = (work->scl.x + 1.0F) * 4.0F;
            njPushMatrix(&_unit_matrix_);
            njTranslate(NULL, flame_1.x, flame_1.y, flame_1.z);
            TURN_VIA(njRotateY, camera_work->ang.y, ang);
            TURN_VIA(njRotateX, camera_work->ang.x, ang);
            njCalcPoint(NULL, &tip_1, &flame_1);
            njPopMatrix(1);
            tip_2.x = 0.0F;
            tip_2.y = 1.1F;
            tip_2.z = 0.0F;
            njPushMatrix(&_unit_matrix_);
            njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
            TURN_VIA(njRotateZ, work->ang.z, ang);
            TURN_VIA(njRotateX, work->ang.x, ang);
            TURN_VIA(njRotateY, work->ang.y, ang);
            scale = work->scl.x + 1.0F;
            njScale(NULL, scale, scale, scale);
            njTranslate(NULL, stone->pos[0], stone->pos[1], stone->pos[2]);
            TURN_VIA(njRotateZ, stone->ang[2], ang);
            TURN_VIA(njRotateX, stone->ang[0], ang);
            TURN_VIA(njRotateY, stone->ang[1], ang);
            njTranslate(NULL, candle_2->pos[0], candle_2->pos[1],
                        candle_2->pos[2]);
            TURN_VIA(njRotateZ, candle_2->ang[2], ang);
            TURN_VIA(njRotateX, candle_2->ang[0], ang);
            TURN_VIA(njRotateY, candle_2->ang[1], ang);
            njCalcPoint(NULL, &tip_2, &flame_2);
            njPopMatrix(1);
            grave_flame_2.pos = flame_2;
            flame_create_from(&grave_flame_2);
            tip_2.x = (work->scl.x + 1.0F) * 0.0F;
            tip_2.y = (work->scl.x + 1.0F) * 0.0F;
            tip_2.z = (work->scl.x + 1.0F) * 4.0F;
            njPushMatrix(&_unit_matrix_);
            njTranslate(NULL, flame_2.x, flame_2.y, flame_2.z);
            TURN_VIA(njRotateY, camera_work->ang.y, ang);
            TURN_VIA(njRotateX, camera_work->ang.x, ang);
            njCalcPoint(NULL, &tip_2, &flame_2);
            njPopMatrix(1);
        }
        grave_draw(task);
        PHASE(work) += (int)work->scl.z;
        VARS(work)->lean =
            njSin((Angle)(PHASE(work) * ANGLE_PER_DEGREE)) * GRAVE_LEAN;
        break;
    case GRAVE_IDLE:
        break;
    }
}

static void grave_destroy(struct task *task)
{
}

/* the candles turn back upright against the stone's lean */
static void grave_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *stone;
    Float scale;
    Angle ang;

    if (draw_in_view(work->pos.x, work->pos.y, work->pos.z, 30.0F)) {
        njSetTexture(&obj_mountain_texlist);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateZ, work->ang.z, ang);
        TURN_VIA(njRotateX, work->ang.x, ang);
        TURN_VIA(njRotateY, work->ang.y, ang);
        scale = work->scl.x + 1.0F;
        njScale(NULL, scale, scale, scale);
        draw_model(&grave_base_model);
        njPushMatrix(NULL);
        stone = &grave_stone_object;
        njTranslate(NULL, stone->pos[0], stone->pos[1], stone->pos[2]);
        TURN_VIA(njRotateZ, stone->ang[2], ang);
        TURN_VIA(njRotateX, stone->ang[0], ang);
        TURN_VIA(njRotateY, stone->ang[1], ang);
        TURN_VIA(njRotateZ, (Angle)(VARS(work)->lean * ANGLE_PER_DEGREE), ang);
        draw_model(stone->model);
        njPushMatrix(NULL);
        TURN_VIA(njRotateZ, (Angle)(-(VARS(work)->lean * ANGLE_PER_DEGREE)),
                 ang);
        draw_tree(&grave_candle_1_object);
        draw_tree(&grave_candle_2_object);
        njPopMatrix(1);
        njPopMatrix(1);
        njPopMatrix(1);
    }
}
