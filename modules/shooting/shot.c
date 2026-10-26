/* the tornado's shots. a bullet turns once toward reticle_target, then
 * flies straight. a missile homes on its lock's target, and leaves a trail
 * of rays */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "draw/draw.h"
#include "draw/tree.h"
#include "effect/beam.h"
#include "effect/dust.h"
#include "shooting/reticle.h"
#include "shooting/shooting.h"
#include "shooting/shot.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/util.h"

/* a shot's velocity, per step, and a trail ray's alpha */
#define SPEED(work) ((work)->scl)
#define TRAIL_ALPHA(work) (SHOT_VARS(work)->speed)

/* a shot's work mode, and a trail's */
enum shot_mode { SHOT_INIT, SHOT_MISSILE, SHOT_BULLET, SHOT_GONE };

/* a shot's speed, and the steps it flies */
#define SHOT_SPEED 40.0F
#define SHOT_STEPS 180
/* the most a shot turns toward its target: a missile each step, a new
 * bullet once */
#define MISSILE_TURN NJM_DEG_ANG(12)
#define BULLET_TURN NJM_DEG_ANG(30)

/* the steps a trail ray lasts, and its length */
#define TRAIL_STEPS 10
#define TRAIL_LENGTH 40.0F
/* a trail ray's alpha starts at TRAIL_ALPHA_START and falls by TRAIL_FADE a
 * step */
#define TRAIL_ALPHA_START (-0.5F)
#define TRAIL_FADE 0.05F

extern NJS_TEXLIST shooting0_texlist;
extern NJS_OBJECT bullet_object;
extern NJS_OBJECT shooting_rocket_object; /* the missile draws as it */

static void missile_trail_step(struct task *task);
static void bullet_move(struct task *task);
static void missile_move(struct task *task);
static void bullet_aim(struct task *target, struct task *task);
static Bool shot_hit(struct task *task);
static void missile_home(struct task *task);

static struct collision_shape shooting_shot_shape = {
    SHAPE_SPHERE | SHAPE_HURTABLE,
    SHAPE_HURTS_ENEMIES | ATTACK(2),
    0,
    {0.0F, 0.0F, 0.0F},
    20.0F,
    20.0F,
    0.0F,
    {0, 0, 0x4000},
    0,
    NULL};
/* a trail ray's color and width, at each end */
static Uint32 missile_trail_colors[2] = {0x70ffe000, 0x70ffe000};
static Float missile_trail_widths[2] = {1.0F, 1.0F};
/* a missile's smoke, as dust parameters. missile_trail keeps their pos at
 * the missile, but nothing in this image makes the smoke */
static struct dust_params missile_smoke = {4.0F,
                                           0.06F,
                                           0.15F,
                                           0.06F,
                                           0.0125F,
                                           {0.0F, 0.0F, 0.0F},
                                           {0.0F, 0.0F, 0.0F},
                                           {1.0F, 0.6F, 0.8F, 1.0F}};

/* draw the missile at twice its size, turned round to face the way it
 * flies. it draws at a frame's first step only */
static void missile_draw(struct task *task)
{
    struct task_work *work = task->work;

    if (frame_step == 0) {
        njFogDisable();
        njPushMatrix(NULL);
        njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
        njRotateXYZ(NULL, work->ang.x, work->ang.y, work->ang.z);
        njRotateY(NULL, 0x8000);
        njScale(NULL, 2.0F, 2.0F, 2.0F);
        njSetTexture(&shooting0_texlist);
        draw_tree(&shooting_rocket_object);
        njPopMatrix(1);
    }
}

static void bullet_draw(struct task *task)
{
    struct task_work *work = task->work;

    if (frame_step == 0) {
        njFogDisable();
        njPushMatrix(NULL);
        njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
        njRotateXYZ(NULL, work->ang.x, work->ang.y, work->ang.z);
        njScale(NULL, 1.0F, 1.0F, -1.0F);
        njSetTexture(&shooting0_texlist);
        draw_tree(&bullet_object);
        njPopMatrix(1);
    }
}

/* a missile's extra is its target's task, which task_destroy would free
 * with the shot */
static void shooting_shot_destroy(struct task *task)
{
    task->extra = NULL;
}

/* a shot starts at the tornado, flying the way the tornado faces with
 * shooting_flight's tilt. a bullet then turns toward reticle_target, if
 * any. a shot goes on a hit, or after SHOT_STEPS */
void shooting_shot_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *tornado = task->parent->work;
    NJS_VECTOR ahead = {0.0F, 0.0F, SHOT_SPEED};
    NJS_VECTOR speed;

    switch (work->mode) {
    case SHOT_INIT:
        task->destructor = shooting_shot_destroy;
        task->draw = bullet_draw;
        work->pos.x = tornado->pos.x;
        work->pos.y = tornado->pos.y;
        work->pos.z = tornado->pos.z;
        work->ang.x = tornado->ang.x;
        work->ang.y = tornado->ang.y;
        work->ang.z = 0;
        SHOT_VARS(work)->speed = SHOT_SPEED;
        SHOT_VARS(work)->unk_00 = 4;
        SHOT_VARS(work)->unk_04 = 0.0F;
        collision_init(task, &shooting_shot_shape, COLLISION_ATTACK, 1, 1);
        njPushMatrix(NULL);
        ahead.z = SHOT_VARS(work)->speed;
        njUnitMatrix(NULL);
        njRotateXYZ(NULL, work->ang.x, work->ang.y, work->ang.z);
        njRotateXYZ(NULL, shooting_flight.ang.x, shooting_flight.ang.y,
                    shooting_flight.ang.z);
        njCalcVector(NULL, &ahead, &speed);
        njPopMatrix(1);
        SPEED(work).x = speed.x;
        SPEED(work).y = speed.y;
        SPEED(work).z = speed.z;
        TASK_TIMER(work) = SHOT_STEPS;
        if (SHOT_VARS(work)->lock < 0) {
            if (reticle_target != NULL) {
                bullet_aim(reticle_target, task);
            }
            work->mode = SHOT_BULLET;
        } else {
            work->mode = SHOT_MISSILE;
        }
        break;
    case SHOT_MISSILE:
        missile_draw(task);
        missile_trail(task);
        if (shot_hit(task)) {
            work->mode = SHOT_GONE;
        } else {
            missile_move(task);
            if (--TASK_TIMER(work) <= 0) {
                work->mode = SHOT_GONE;
            }
        }
        collision_register(work);
        break;
    case SHOT_BULLET:
        bullet_draw(task);
        if (shot_hit(task)) {
            work->mode = SHOT_GONE;
        } else {
            bullet_move(task);
            if (--TASK_TIMER(work) <= 0) {
                work->mode = SHOT_GONE;
            }
        }
        collision_register(work);
        break;
    case SHOT_GONE:
        task_kill(task);
        break;
    }
    task_step_children(task);
}

static void bullet_move(struct task *task)
{
    struct task_work *work = task->work;

    work->pos.x += SPEED(work).x;
    work->pos.y += SPEED(work).y;
    work->pos.z += SPEED(work).z;
}

static void missile_move(struct task *task)
{
    struct task_work *work = task->work;

    work->pos.x += SPEED(work).x;
    work->pos.y += SPEED(work).y;
    work->pos.z += SPEED(work).z;
    missile_home(task);
}

/* while a lock holds the missile's target, turn the missile toward it by
 * MISSILE_TURN at most. send it SHOT_SPEED the way it then faces. once no
 * lock holds the target, the missile forgets it and flies on straight */
static void missile_home(struct task *task)
{
    NJS_VECTOR speed = {0.0F, 0.0F, SHOT_SPEED};
    struct task *target = task->extra;
    struct task_work *to;
    struct task_work *work = task->work;
    Float dx;
    Float dy;
    Float dz;
    Float flat;
    Angle ang_y;
    Angle ang_x;

    if (target != NULL) {
        if (lock_holds(target)) {
            to = target->work;
            dx = to->pos.x - work->pos.x;
            dy = to->pos.y - work->pos.y;
            dz = to->pos.z - work->pos.z;
            flat = njSqrt((dx * dx) + (dz * dz));
            ang_y = 0x4000 - (Angle)(atan2f(dz, dx) * ANGLE_PER_RADIAN);
            ang_x = -(Angle)(atan2f(dy, flat) * ANGLE_PER_RADIAN);
            work->ang.x = angle_approach(work->ang.x, ang_x, MISSILE_TURN);
            work->ang.y = angle_approach(work->ang.y, ang_y, MISSILE_TURN);
            njPushMatrix(NULL);
            njUnitMatrix(NULL);
            njRotateXYZ(NULL, work->ang.x, work->ang.y, work->ang.z);
            njCalcVector(NULL, &speed, &speed);
            SPEED(work).x = speed.x;
            SPEED(work).y = speed.y;
            SPEED(work).z = speed.z;
            njPopMatrix(1);
        } else {
            task->extra = NULL;
        }
    }
}

/* turn a new bullet toward target by BULLET_TURN at most. send it
 * SHOT_SPEED the way it then faces */
static void bullet_aim(struct task *target, struct task *task)
{
    NJS_VECTOR speed = {0.0F, 0.0F, SHOT_SPEED};
    struct task_work *work = task->work;
    struct task_work *to = target->work;
    Float dx;
    Float dy;
    Float dz;
    Float flat;
    Angle ang_y;
    Angle ang_x;

    dx = to->pos.x - work->pos.x;
    dy = to->pos.y - work->pos.y;
    dz = to->pos.z - work->pos.z;
    flat = njSqrt((dx * dx) + (dz * dz));
    ang_y = 0x4000 - (Angle)(atan2f(dz, dx) * ANGLE_PER_RADIAN);
    ang_x = -(Angle)(atan2f(dy, flat) * ANGLE_PER_RADIAN);
    work->ang.x = angle_approach(work->ang.x, ang_x, BULLET_TURN);
    work->ang.y = angle_approach(work->ang.y, ang_y, BULLET_TURN);
    njPushMatrix(NULL);
    njUnitMatrix(NULL);
    njRotateXYZ(NULL, work->ang.x, work->ang.y, work->ang.z);
    njCalcVector(NULL, &speed, &speed);
    SPEED(work).x = speed.x;
    SPEED(work).y = speed.y;
    SPEED(work).z = speed.z;
    njPopMatrix(1);
}

static Bool shot_hit(struct task *task)
{
    struct task_work *work = task->work;

    if (work->collision->flags & COLLISION_HIT) {
        return TRUE;
    }
    return FALSE;
}

/* shooting_shot_step, its one caller, expands it inline, and the image also
 * keeps a copy at 0x0c902d76 that nothing calls. it is external because SHC
 * drops a static function it expands at every call */
void missile_trail(struct task *task)
{
    missile_smoke.pos = task->work->pos;
    task_create_child(TASK_WORK, missile_trail_step, task);
}

/* a ray of a missile's trail. it stays where the missile was as the ray
 * was made. it draws for TRAIL_STEPS as its alpha falls */
static void missile_trail_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *missile = task->parent->work;

    switch (work->mode) {
    case SHOT_INIT:
        work->pos.x = missile->pos.x;
        work->pos.y = missile->pos.y;
        work->pos.z = missile->pos.z;
        work->ang.x = missile->ang.x;
        work->ang.y = missile->ang.y;
        work->ang.z = missile->ang.z;
        TASK_TIMER(work) = TRAIL_STEPS;
        TRAIL_ALPHA(work) = TRAIL_ALPHA_START;
        work->mode = SHOT_MISSILE;
        missile_draw(task);
        break;
    case SHOT_MISSILE:
        if (--TASK_TIMER(work) <= 0) {
            work->mode = SHOT_GONE;
        }
        draw_attr_save();
        draw_attr_set(0, NJD_FLAG_USE_ALPHA);
        draw_set_material(TRAIL_ALPHA(work), 0.0F, 0.0F, 0.0F);
        beam_draw_ray(&work->pos, &work->ang, missile_trail_widths,
                      missile_trail_colors, TRAIL_LENGTH);
        draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
        draw_attr_restore();
        TRAIL_ALPHA(work) -= TRAIL_FADE;
        break;
    case SHOT_GONE:
        task_kill(task);
        break;
    }
}
