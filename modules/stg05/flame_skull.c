/* o_honoo, a skull that opens its jaw and breathes flames that hurt
 * players */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "draw/model.h"
#include "effect/flame.h"
#include "object/common.h"
#include "object/placement.h"
#include "stg05/flame_skull.h"
#include "system/task.h"
#include "system/util.h"

extern NJS_TEXLIST obj_mountain_texlist;
extern NJS_ACTION flame_skull_action;

struct flame_flicker_vars {
    Uint16 turn_x;
    Uint16 turn_y;
    /* the scale and height the flame has wandered to */
    Float scale;
    Float rise;
};

#define FLICKER(work) ((struct flame_flicker_vars *)&(work)->vars)

/* FLICKER_INIT makes the flicker's position an offset from its parent's.
 * nothing sets FLICKER_RUN, so a flicker would subtract its parent's
 * position again on every step */
enum flame_flicker_mode { FLICKER_INIT, FLICKER_RUN };

/* the jaw opens, the skull burns and pauses, and the jaw shuts and waits.
 * work->tick counts the steps of each burn, pause and wait */
enum flame_skull_mode {
    SKULL_INIT,
    SKULL_OPEN,
    SKULL_BURN,
    SKULL_PAUSE,
    SKULL_CLOSE,
    SKULL_WAIT
};

/* the jaw action's frame. it rises by one each step as the jaw opens, and
 * falls by one as the jaw shuts */
#define JAW_FRAME(work) TASK_TIMER(work)
/* the jaw opens until its frame passes this */
#define SKULL_OPENED 13
/* a phase goes on while work->tick is at most its limit. the burn counts a
 * step before it tests the tick, and the pause and the wait after */
#define SKULL_BURN_LIMIT 180 /* a burn, which lasts 181 steps */
#define SKULL_PAUSE_LIMIT 20 /* the pause after a burn, 22 steps */
#define SKULL_WAIT_LIMIT 180 /* the wait after the jaw shuts, 182 steps */
/* the burn turns its flames' shape on at this tick, its 61st step */
#define SKULL_HURT_FROM 60

/* a flame's scale relative to the skull's */
#define SKULL_FLAME_SCALE 1.0F

static void flame_skull_flicker_step(struct task *task);
static void flame_skull_draw(struct task *task);

/* two flickers over the skull, as specs no code reads */
static struct task_spec flame_skull_flickers[3] = {
    {flame_skull_flicker_step,
     TASK_WORK,
     0,
     {0, 0, 0},
     {-1.0F, 7.3F, -0.5F},
     0},
    {flame_skull_flicker_step, TASK_WORK, 0, {0, 0, 0}, {1.0F, 7.3F, -0.5F}, 0},
    {NULL, 0, 0, {0, 0, 0}, {0.0F, 0.0F, 0.0F}, 0},
};

/* the flames' reach ahead of the jaw, and the skull. the flames' shape
 * starts on, so it hurts players until the first burn ends */
static struct collision_shape flame_skull_shapes[2] = {
    {SHAPE_CAPSULE | SHAPE_HURTABLE,
     SHAPE_HURTS_PLAYERS | GUARD(3) | SHAPE_ATTACK,
     PUSH_FIRM(PUSH_PLAYERS),
     {0.0F, 10.0F, 18.0F},
     5.0F,
     22.0F,
     0.0F,
     {0x4000, 0, 0},
     0,
     NULL},
    {SHAPE_SPHERE,
     GUARD(3),
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 11.0F, -6.0F},
     10.0F,
     0.0F,
     0.0F,
     {0, 0, 0},
     0,
     NULL},
};

/* create a flame at the flicker's offset from its parent, wandering in height
 * and size */
static void flame_skull_flicker_step(struct task *task)
{
    struct task_work *work = task->work;
    NJS_POINT3 out;
    NJS_POINT3 pos;
    NJS_VECTOR still;
    Angle ang;

    switch (work->mode) {
    case FLICKER_INIT:
        work->pos.x -= task->parent->work->pos.x;
        work->pos.y -= task->parent->work->pos.y;
        work->pos.z -= task->parent->work->pos.z;
        /* fall through */
    case FLICKER_RUN:
        pos.x = task->parent->work->pos.x + work->pos.x;
        pos.y = task->parent->work->pos.y + work->pos.y;
        pos.z = task->parent->work->pos.z + work->pos.z;
        break;
    }
    njPushMatrix(&_unit_matrix_);
    FLICKER(work)->rise += RANDOM(-0.5F, 0.5F) * 0.1F;
    FLICKER(work)->scale += RANDOM(-0.5F, 0.5F) * 0.1F;
    /* each added turn is under 0.25, which truncates to 0. so turn_x and
     * turn_y stay 0, and the flame never turns */
    FLICKER(work)->turn_x += (Angle)(rand() * (0.25F * RAND_SCALE));
    FLICKER(work)->turn_y += (Angle)(rand() * (0.25F * RAND_SCALE));
    if (njAbs(FLICKER(work)->scale) > 0.25F) {
        FLICKER(work)->scale = FLICKER(work)->scale > 0.0F ? 0.25F : -0.25F;
    }
    if (njAbs(FLICKER(work)->rise) > 2.5F) {
        FLICKER(work)->rise = FLICKER(work)->rise > 0.0F ? 2.5F : -2.5F;
    }
    njTranslate(NULL, 0.0F, FLICKER(work)->rise, 0.0F);
    TURN_VIA(njRotateX, FLICKER(work)->turn_x, ang);
    TURN_VIA(njRotateY, FLICKER(work)->turn_y, ang);
    njCalcPoint(NULL, &pos, &out);
    njPopMatrix(1);
    still.x = still.y = still.z = 0.0F;
    flame_create(&out, &still, FLICKER(work)->scale);
}

/* create a flame of scale at pos, moving at speed, both in the skull's own
 * space */
static void flame_skull_emit(struct task *task, Float scale, NJS_POINT3 pos,
                             NJS_VECTOR speed)
{
    struct task_work *work = task->work;
    NJS_POINT3 out_pos;
    NJS_VECTOR out_speed;
    Angle ang;

    njPushMatrix(&_unit_matrix_);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    njScale(NULL, work->scl.x, work->scl.x, work->scl.x);
    njCalcPoint(NULL, &pos, &out_pos);
    njCalcVector(NULL, &speed, &out_speed);
    njPopMatrix(1);
    flame_create(&out_pos, &out_speed, scale);
}

/* o_honoo's step. outside the set editor, the skull draws at twice its
 * model's size, whatever the set entry's scale x. a scale y of exactly 1
 * holds it burning, its jaw at frame 30 */
void flame_skull_step(struct task *task)
{
    struct task_work *work = task->work;
    NJS_POINT3 pos;
    NJS_VECTOR speed;
    Float scale;

    if (placement_try_leave(task)) {
        return;
    }
    if (placement_editing(task)) {
        ACTION(work)->action = &flame_skull_action;
    } else {
        switch (work->mode) {
        case SKULL_INIT:
            work->mode = SKULL_OPEN;
            work->scl.x = 2.0F;
            ACTION(work)->texlist = &obj_mountain_texlist;
            ACTION(work)->action = &flame_skull_action;
            collision_init(task, flame_skull_shapes, COLLISION_OBJECT, 1, 2);
            work->scl.z = 0.0F;
            break;
        case SKULL_OPEN:
            if (++JAW_FRAME(work) > SKULL_OPENED) {
                work->mode = SKULL_BURN;
            }
            break;
        case SKULL_BURN:
            pos.x = 0.0F;
            pos.y = 5.0F;
            pos.z = 0.0F;
            speed.x = 0.0F;
            speed.y = 0.0F;
            speed.z = 2.5F;
            /* @hack spelling: the scale computed before pos and speed are
             * copied for the call (0x0c905540 on); computed in the call, 31 of
             * 214 words differ */
            scale = work->scl.x * SKULL_FLAME_SCALE;
            flame_skull_emit(task, scale, pos, speed);
            if (work->tick == SKULL_HURT_FROM) {
                work->collision->shapes[0].flags &= ~SHAPE_OFF;
            }
            if (++work->tick > SKULL_BURN_LIMIT) {
                work->mode = SKULL_PAUSE;
                work->tick = 0;
                work->collision->shapes[0].flags |= SHAPE_OFF;
            }
            break;
        case SKULL_PAUSE:
            if (work->tick++ > SKULL_PAUSE_LIMIT) {
                work->mode = SKULL_CLOSE;
                work->tick = 0;
            }
            break;
        case SKULL_CLOSE:
            if (--JAW_FRAME(work) < 1) {
                work->mode = SKULL_WAIT;
            }
            break;
        case SKULL_WAIT:
            if (work->tick++ > SKULL_WAIT_LIMIT) {
                work->mode = SKULL_OPEN;
                work->tick = 0;
            }
            task_step_children(task);
            break;
        }
        if (work->scl.y == 1.0F) {
            work->mode = SKULL_BURN;
            work->collision->shapes[0].flags &= ~SHAPE_OFF;
            JAW_FRAME(work) = 30;
        }
        collision_register(work);
    }
    flame_skull_draw(task);
}

static void flame_skull_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (draw_in_view(work->pos.x, work->pos.y, work->pos.z, 100.0F)) {
        njSetTexture(&obj_mountain_texlist);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        njScale(NULL, work->scl.x, work->scl.x, work->scl.x);
        draw_divide_light(work->scl.x);
        draw_action(ACTION(work)->action, JAW_FRAME(work));
        draw_reset_light();
        njPopMatrix(1);
    }
}
