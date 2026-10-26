/* O FL: a spout that sends flames off at random, with a sphere that hurts
 * players */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "effect/dust.h"
#include "effect/flame.h"
#include "object/placement.h"
#include "stg09/casino_flame.h"
#include "system/task.h"
#include "system/util.h"

/* the task destroys itself beyond about 600 of the camera and player 0.
 * placement_try_leave_by takes the range squared */
#define RANGE ((600.0F * 600.0F) + 10.0F)

static Float casino_flame_scale = 5.0F;
static Float casino_flame_growth = 0.03F;
static Float casino_flame_rate = 0.13F;
static Float casino_flame_drag = 0.1F;
static Float casino_flame_lift = 0.08F;
/* the most a flame's speed varies either way, on each axis */
static Float casino_flame_spread = 0.1F;
static Float casino_flame_chance = 0.3F; /* of a flame, each step */
/* casino_flame_run overwrites every field but the color */
static struct dust_params casino_flame_params = {
    1.0F,
    0.038F,
    0.19F,
    0.06F,
    0.025F,
    {0.0F, 0.0F, 0.0F},
    {0.0F, 0.0F, 0.0F},
    {1.0F, 1.0F, 1.0F, 1.0F},
};
static struct collision_shape casino_flame_shape = {
    SHAPE_SPHERE | SHAPE_HURTABLE,
    SHAPE_HURTS_PLAYERS | GUARD(2) | ATTACK(3),
    PUSH_ALL,
    {0.0F, 0.0F, 0.0F},
    5.0F,
    0.0F,
    0.0F,
    {0},
    0,
    NULL,
};

static void casino_flame_init(struct task *task);
static void casino_flame_destroy(struct task *task);
static void casino_flame_run(struct task *task);

void casino_flame_step(struct task *task)
{
    /* @hack redundant: work, read and never used, which keeps task in R14
     * across the range check (MOV R4,R14 at 0x0c917028). without it, 12 of
     * 16 words differ */
    struct task_work *work = task->work;

    if (!placement_try_leave_by(task, RANGE)) {
        casino_flame_init(task);
    }
}

static void casino_flame_init(struct task *task)
{
    struct task_work *work = task->work;

    task->step = casino_flame_run;
    task->destructor = casino_flame_destroy;
    work->timer = 0;
    if (!placement_editing(task)) {
        collision_init(task, &casino_flame_shape, COLLISION_OBJECT, 1, 1);
    }
}

static void casino_flame_destroy(struct task *task)
{
}

/* on a step with chance casino_flame_chance, send a flame off along scl,
 * turned by ang.y */
static void casino_flame_run(struct task *task)
{
    struct task_work *work = task->work;
    NJS_VECTOR speed;
    Angle ang;
    Float spread;

    if (!placement_try_leave_by(task, RANGE)) {
        if (rand() * RAND_SCALE <= casino_flame_chance) {
            spread = casino_flame_spread;
            njPushMatrix(&_unit_matrix_);
            TURN_VIA(njRotateY, work->ang.y, ang);
            njCalcVector(NULL, &work->scl, &speed);
            njPopMatrix(1);
            speed.x += ((rand() * RAND_SCALE) - 0.5F) * spread * 2.0F;
            speed.y += ((rand() * RAND_SCALE) - 0.5F) * spread * 2.0F;
            speed.z += ((rand() * RAND_SCALE) - 0.5F) * spread * 2.0F;
            casino_flame_params.scale = casino_flame_scale;
            casino_flame_params.growth = casino_flame_growth;
            casino_flame_params.rate = casino_flame_rate;
            casino_flame_params.drag = casino_flame_drag;
            casino_flame_params.lift = casino_flame_lift;
            casino_flame_params.pos = work->pos;
            casino_flame_params.speed = speed;
            flame_create_from(&casino_flame_params);
        }
        work->timer++;
        if (!placement_editing(task)) {
            collision_register(work);
        }
    }
}
