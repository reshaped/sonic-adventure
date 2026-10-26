/* O Hyo1NoExp to O Hyo3Exp: ice blocks in three sizes, standing or
 * breakable */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/model.h"
#include "object/common.h"
#include "object/placement.h"
#include "sound/sound.h"
#include "stg08/ice.h"
#include "system/cache.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"
#include "vmu/vibration.h"

extern NJS_MODEL icecap_ice_1_model;
extern NJS_MODEL icecap_ice_1_snow_model;
extern NJS_MODEL icecap_ice_2_model;
extern NJS_MODEL icecap_ice_2_snow_model;
extern NJS_MODEL icecap_ice_3_model;
extern NJS_MODEL icecap_ice_3_snow_model;
extern NJS_MODEL icecap_ice_1_piece_1_model;
extern NJS_MODEL icecap_ice_1_piece_2_model;
extern NJS_MODEL icecap_ice_1_piece_3_model;
extern NJS_MODEL icecap_ice_1_piece_4_model;
extern NJS_MODEL icecap_ice_1_piece_5_model;
extern NJS_MODEL icecap_ice_1_piece_6_model;
extern NJS_MODEL icecap_ice_1_piece_7_model;
extern NJS_MODEL icecap_ice_1_piece_8_model;
extern NJS_MODEL icecap_ice_1_piece_9_model;
extern NJS_MODEL icecap_ice_1_piece_10_model;
extern NJS_MODEL icecap_ice_1_piece_11_model;
extern NJS_MODEL icecap_ice_1_piece_12_model;
extern NJS_MODEL icecap_ice_2_piece_1_model;
extern NJS_MODEL icecap_ice_2_piece_2_model;
extern NJS_MODEL icecap_ice_2_piece_3_model;
extern NJS_MODEL icecap_ice_2_piece_4_model;
extern NJS_MODEL icecap_ice_2_piece_5_model;
extern NJS_MODEL icecap_ice_2_piece_6_model;
extern NJS_MODEL icecap_ice_2_piece_7_model;
extern NJS_MODEL icecap_ice_2_piece_8_model;
extern NJS_MODEL icecap_ice_2_piece_9_model;
extern NJS_MODEL icecap_ice_2_piece_10_model;
extern NJS_MODEL icecap_ice_2_piece_11_model;
extern NJS_MODEL icecap_ice_2_piece_12_model;
extern NJS_MODEL icecap_ice_2_piece_13_model;
extern NJS_MODEL icecap_ice_2_piece_14_model;
extern NJS_MODEL icecap_ice_2_piece_15_model;
extern NJS_MODEL icecap_ice_2_piece_16_model;
extern NJS_MODEL icecap_ice_2_piece_17_model;
extern NJS_MODEL icecap_ice_2_piece_18_model;
extern NJS_MODEL icecap_ice_3_piece_1_model;
extern NJS_MODEL icecap_ice_3_piece_2_model;
extern NJS_MODEL icecap_ice_3_piece_3_model;
extern NJS_MODEL icecap_ice_3_piece_4_model;
extern NJS_MODEL icecap_ice_3_piece_5_model;
extern NJS_MODEL icecap_ice_3_piece_6_model;
extern NJS_MODEL icecap_ice_3_piece_7_model;
extern NJS_MODEL icecap_ice_3_piece_8_model;
extern NJS_MODEL icecap_ice_3_piece_9_model;
extern NJS_MODEL icecap_ice_3_piece_10_model;
extern NJS_MODEL icecap_ice_3_piece_11_model;
extern NJS_MODEL icecap_ice_3_piece_12_model;
extern NJS_MODEL icecap_ice_3_piece_13_model;

/* a burst rises at ICE_RISE a step, and its rise slows by ICE_SLOW each
 * step. it ends once its rise is below ICE_GONE. its pieces spread
 * ICE_SPREAD and turn ICE_SPIN each step. each piece shrinks by ICE_SHRINK
 * of its size per unit of spread */
#define ICE_RISE 3.0F
#define ICE_SLOW 0.2F
#define ICE_SPREAD 1.0F
#define ICE_SHRINK 0.02F
#define ICE_SPIN 0x400
#define ICE_GONE (-10.0F)

#define SE_ICE_BURST 236

struct ice_vars {
    Float spread;
    Angle spin;
    Float rise; /* the burst's upward speed */
};

#define VARS(work) ((struct ice_vars *)&(work)->vars)

/* in work->mode */
enum ice_size { ICE_1, ICE_2, ICE_3 };

static void icecap_ice_draw(struct task_work *work);

/* one or two capsules about each size of block. a capsule pushes nothing,
 * and items, enemies, pickups and rings can't push it */
#define ICE_SHAPE(x, y, z, radius, height)                                     \
    {SHAPE_CAPSULE,                                                            \
     0,                                                                        \
     PUSH_FIRM(PUSH_ENEMIES | PUSH_OTHERS),                                    \
     {(x), (y), (z)},                                                          \
     (radius),                                                                 \
     (height),                                                                 \
     0.0F,                                                                     \
     {0},                                                                      \
     0,                                                                        \
     NULL}
static struct collision_shape icecap_ice_1_shapes[1] = {
    ICE_SHAPE(0.0F, 50.0F, 0.0F, 26.0F, 50.0F),
};
static struct collision_shape icecap_ice_2_shapes[2] = {
    ICE_SHAPE(-25.0F, 30.0F, 0.0F, 45.0F, 22.0F),
    ICE_SHAPE(25.0F, 20.0F, -35.0F, 25.0F, 15.0F),
};
static struct collision_shape icecap_ice_3_shapes[1] = {
    ICE_SHAPE(0.0F, 40.0F, 0.0F, 38.0F, 40.0F),
};

static void icecap_ice_destroy(struct task *task)
{
}

/* draw a piece at x, y and z, turned and shrunk with the burst. the spread
 * moves it out along the angle way, and the rise moves it up. the block's
 * matrix is then restored */
static void icecap_ice_piece(struct task_work *work, Angle way, Float x,
                             Float y, Float z, NJS_MODEL *model)
{
    Float out_x = njSin(way) * VARS(work)->spread;
    Float out_z = njCos(way) * VARS(work)->spread;
    Float size = 1.0F - (VARS(work)->spread * ICE_SHRINK);

    njTranslate(NULL, x + out_x, y + VARS(work)->rise, z + out_z);
    TURN(njRotateY, VARS(work)->spin);
    njScale(NULL, size, size, size);
    draw_model(model);
    matrix_load_parent();
}

static void icecap_ice_burst_1(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return;
    }
    if (VARS(work)->rise < ICE_GONE) {
        placement_kill(task);
        return;
    }
    VARS(work)->spread += ICE_SPREAD;
    VARS(work)->spin += ICE_SPIN;
    VARS(work)->rise -= ICE_SLOW;
    work->pos.y += VARS(work)->rise;
    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateZ, work->ang.z);
    TURN(njRotateX, work->ang.x);
    TURN(njRotateY, work->ang.y);
    njPushMatrix(NULL);
    icecap_ice_piece(work, 0x0000, 0.0F, 96.0F, 0.0F,
                     &icecap_ice_1_piece_1_model);
    icecap_ice_piece(work, 0x29fb, 5.0F, 74.0F, 3.0F,
                     &icecap_ice_1_piece_2_model);
    icecap_ice_piece(work, 0xb605, -4.0F, 73.0F, -1.0F,
                     &icecap_ice_1_piece_3_model);
    icecap_ice_piece(work, 0x4000, 9.0F, 49.0F, 0.0F,
                     &icecap_ice_1_piece_4_model);
    icecap_ice_piece(work, 0xed1c, -3.0F, 48.0F, 6.0F,
                     &icecap_ice_1_piece_5_model);
    icecap_ice_piece(work, 0x9946, -5.0F, 47.0F, -7.0F,
                     &icecap_ice_1_piece_6_model);
    icecap_ice_piece(work, 0x2ad9, 14.0F, 17.0F, 8.0F,
                     &icecap_ice_1_piece_7_model);
    icecap_ice_piece(work, 0x5b22, 14.0F, 18.0F, -11.0F,
                     &icecap_ice_1_piece_8_model);
    icecap_ice_piece(work, 0x78ac, 2.0F, 18.0F, -11.0F,
                     &icecap_ice_1_piece_9_model);
    icecap_ice_piece(work, 0xdc9d, -13.0F, 17.0F, 11.0F,
                     &icecap_ice_1_piece_10_model);
    icecap_ice_piece(work, 0xa5c9, -12.0F, 15.0F, -9.0F,
                     &icecap_ice_1_piece_11_model);
    icecap_ice_piece(work, 0xf9c8, -2.0F, 17.0F, 13.0F,
                     &icecap_ice_1_piece_12_model);
    njPopMatrix(1);
    njPopMatrix(1);
}

static void icecap_ice_burst_2(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return;
    }
    if (VARS(work)->rise < ICE_GONE) {
        placement_kill(task);
        return;
    }
    VARS(work)->spread += ICE_SPREAD;
    VARS(work)->spin += ICE_SPIN;
    VARS(work)->rise -= ICE_SLOW;
    work->pos.y += VARS(work)->rise;
    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateZ, work->ang.z);
    TURN(njRotateX, work->ang.x);
    TURN(njRotateY, work->ang.y);
    njPushMatrix(NULL);
    icecap_ice_piece(work, 0x0000, 0.0F, 96.0F, 0.0F,
                     &icecap_ice_2_piece_1_model);
    icecap_ice_piece(work, 0x29fb, 5.0F, 74.0F, 3.0F,
                     &icecap_ice_2_piece_2_model);
    icecap_ice_piece(work, 0xb605, -4.0F, 73.0F, -1.0F,
                     &icecap_ice_2_piece_3_model);
    icecap_ice_piece(work, 0x4000, 9.0F, 49.0F, 0.0F,
                     &icecap_ice_2_piece_4_model);
    icecap_ice_piece(work, 0xed1c, -3.0F, 48.0F, 6.0F,
                     &icecap_ice_2_piece_5_model);
    icecap_ice_piece(work, 0x9946, -5.0F, 47.0F, -7.0F,
                     &icecap_ice_2_piece_6_model);
    icecap_ice_piece(work, 0x2ad9, 14.0F, 17.0F, 8.0F,
                     &icecap_ice_2_piece_7_model);
    icecap_ice_piece(work, 0x5b22, 14.0F, 18.0F, -11.0F,
                     &icecap_ice_2_piece_8_model);
    icecap_ice_piece(work, 0x78ac, 2.0F, 18.0F, -11.0F,
                     &icecap_ice_2_piece_9_model);
    icecap_ice_piece(work, 0xdc9d, -13.0F, 17.0F, 11.0F,
                     &icecap_ice_2_piece_10_model);
    icecap_ice_piece(work, 0xa5c9, -12.0F, 15.0F, -9.0F,
                     &icecap_ice_2_piece_11_model);
    icecap_ice_piece(work, 0xf9c8, -2.0F, 17.0F, 13.0F,
                     &icecap_ice_2_piece_12_model);
    icecap_ice_piece(work, 0xf9c8, -2.0F, 17.0F, 13.0F,
                     &icecap_ice_2_piece_13_model);
    icecap_ice_piece(work, 0xf9c8, -2.0F, 17.0F, 13.0F,
                     &icecap_ice_2_piece_14_model);
    icecap_ice_piece(work, 0xf9c8, -2.0F, 17.0F, 13.0F,
                     &icecap_ice_2_piece_15_model);
    icecap_ice_piece(work, 0xf9c8, -2.0F, 17.0F, 13.0F,
                     &icecap_ice_2_piece_16_model);
    icecap_ice_piece(work, 0xf9c8, -2.0F, 17.0F, 13.0F,
                     &icecap_ice_2_piece_17_model);
    icecap_ice_piece(work, 0xf9c8, -2.0F, 17.0F, 13.0F,
                     &icecap_ice_2_piece_18_model);
    njPopMatrix(1);
    njPopMatrix(1);
}

static void icecap_ice_burst_3(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return;
    }
    if (VARS(work)->rise < ICE_GONE) {
        placement_kill(task);
        return;
    }
    VARS(work)->spread += ICE_SPREAD;
    VARS(work)->spin += ICE_SPIN;
    VARS(work)->rise -= ICE_SLOW;
    work->pos.y += VARS(work)->rise;
    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateZ, work->ang.z);
    TURN(njRotateX, work->ang.x);
    TURN(njRotateY, work->ang.y);
    njPushMatrix(NULL);
    icecap_ice_piece(work, 0x0000, 0.0F, 96.0F, 0.0F,
                     &icecap_ice_3_piece_1_model);
    icecap_ice_piece(work, 0x29fb, 5.0F, 74.0F, 3.0F,
                     &icecap_ice_3_piece_2_model);
    icecap_ice_piece(work, 0xb605, -4.0F, 73.0F, -1.0F,
                     &icecap_ice_3_piece_3_model);
    icecap_ice_piece(work, 0x4000, 9.0F, 49.0F, 0.0F,
                     &icecap_ice_3_piece_4_model);
    icecap_ice_piece(work, 0xed1c, -3.0F, 48.0F, 6.0F,
                     &icecap_ice_3_piece_5_model);
    icecap_ice_piece(work, 0x9946, -5.0F, 47.0F, -7.0F,
                     &icecap_ice_3_piece_6_model);
    icecap_ice_piece(work, 0x2ad9, 14.0F, 17.0F, 8.0F,
                     &icecap_ice_3_piece_7_model);
    icecap_ice_piece(work, 0x5b22, 14.0F, 18.0F, -11.0F,
                     &icecap_ice_3_piece_8_model);
    icecap_ice_piece(work, 0x78ac, 2.0F, 18.0F, -11.0F,
                     &icecap_ice_3_piece_9_model);
    icecap_ice_piece(work, 0xdc9d, -13.0F, 17.0F, 11.0F,
                     &icecap_ice_3_piece_10_model);
    icecap_ice_piece(work, 0xa5c9, -12.0F, 15.0F, -9.0F,
                     &icecap_ice_3_piece_11_model);
    icecap_ice_piece(work, 0xf9c8, -2.0F, 17.0F, 13.0F,
                     &icecap_ice_3_piece_12_model);
    icecap_ice_piece(work, 0xf9c8, -2.0F, 17.0F, 13.0F,
                     &icecap_ice_3_piece_13_model);
    njPopMatrix(1);
    njPopMatrix(1);
}

static void icecap_ice_draw(struct task_work *work)
{
    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateZ, work->ang.z);
    TURN(njRotateX, work->ang.x);
    TURN(njRotateY, work->ang.y);
    switch (work->mode) {
    case ICE_1:
        draw_model(&icecap_ice_1_model);
        draw_model(&icecap_ice_1_snow_model);
        break;
    case ICE_2:
        draw_model(&icecap_ice_2_model);
        draw_model(&icecap_ice_2_snow_model);
        break;
    case ICE_3:
        draw_model(&icecap_ice_3_model);
        draw_model(&icecap_ice_3_snow_model);
        break;
    }
    njPopMatrix(1);
}

static void icecap_ice_stand(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_try_leave(task)) {
        if (!placement_editing(task)) {
            collision_register(work);
            object_clear_touch(work, NULL);
        }
        icecap_ice_draw(work);
    }
}

static void icecap_ice_wait(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_try_leave(task)) {
        icecap_ice_draw(work);
        if (work->collision->flags & COLLISION_HIT) {
            /* a hit bursts the block. the burst's draw is its step, so it
             * moves on paused frames too */
            switch (work->mode) {
            case ICE_1:
                task->step = icecap_ice_burst_1;
                task->draw = task->step;
                break;
            case ICE_2:
                task->step = icecap_ice_burst_2;
                task->draw = task->step;
                break;
            case ICE_3:
                task->step = icecap_ice_burst_3;
                task->draw = task->step;
                break;
            }
            sound_play(SE_ICE_BURST, 0, 0, 0);
            vibration_start(0, 3, 7, 0);
        }
        if (!placement_editing(task)) {
            collision_register(work);
            object_clear_touch(work, NULL);
        }
    }
}

void icecap_ice_1_step(struct task *task)
{
    struct task_work *work = task->work;

    collision_init(task, icecap_ice_1_shapes, COLLISION_OBJECT, 1, 1);
    work->mode = ICE_1;
    task->step = icecap_ice_stand;
    task->draw = task->step;
    task->destructor = icecap_ice_destroy;
}

void icecap_ice_1_breakable_step(struct task *task)
{
    struct task_work *work = task->work;

    VARS(work)->rise = ICE_RISE;
    collision_init(task, icecap_ice_1_shapes, COLLISION_OBJECT, 1, 1);
    work->mode = ICE_1;
    task->step = icecap_ice_wait;
    task->draw = task->step;
    task->destructor = icecap_ice_destroy;
}

void icecap_ice_2_step(struct task *task)
{
    struct task_work *work = task->work;

    collision_init(task, icecap_ice_2_shapes, COLLISION_OBJECT, 1, 2);
    work->mode = ICE_2;
    task->step = icecap_ice_stand;
    task->draw = task->step;
    task->destructor = icecap_ice_destroy;
}

void icecap_ice_2_breakable_step(struct task *task)
{
    struct task_work *work = task->work;

    VARS(work)->rise = ICE_RISE;
    collision_init(task, icecap_ice_2_shapes, COLLISION_OBJECT, 1, 2);
    work->mode = ICE_2;
    task->step = icecap_ice_wait;
    task->draw = task->step;
    task->destructor = icecap_ice_destroy;
}

void icecap_ice_3_step(struct task *task)
{
    struct task_work *work = task->work;

    collision_init(task, icecap_ice_3_shapes, COLLISION_OBJECT, 1, 1);
    work->mode = ICE_3;
    task->step = icecap_ice_stand;
    task->draw = task->step;
    task->destructor = icecap_ice_destroy;
}

void icecap_ice_3_breakable_step(struct task *task)
{
    struct task_work *work = task->work;

    VARS(work)->rise = ICE_RISE;
    collision_init(task, icecap_ice_3_shapes, COLLISION_OBJECT, 1, 1);
    work->mode = ICE_3;
    task->step = icecap_ice_wait;
    task->draw = task->step;
    task->destructor = icecap_ice_destroy;
}
