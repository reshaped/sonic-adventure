/* Emerald Coast's scenery: models in the collision world, solid models, a
 * scaled model, and a splash step */
#include <ninja.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "effect/splash.h"
#include "object/common.h"
#include "stg01/scenery.h"
#include "system/task.h"

extern NJS_OBJECT beach_float_object;
extern NJS_OBJECT bungalow_object;
extern NJS_OBJECT beach_rock_object;
extern NJS_OBJECT rock_pole_object;
extern NJS_OBJECT stone_pillar_object;
extern NJS_OBJECT komomo_object;
extern NJS_OBJECT beach_rock_2_object;

static struct collision_shape stone_pillar_shape = {SHAPE_CYLINDER,
                                                    0,
                                                    PUSH_ALL |
                                                        PUSH_FIRM(PUSH_ALL),
                                                    {0.0F, 20.0F, 0.0F},
                                                    10.0F,
                                                    20.0F,
                                                    0.0F,
                                                    {0, 0, 0},
                                                    0,
                                                    NULL};
static struct collision_shape beach_rock_2_shape = {SHAPE_SPHERE,
                                                    0,
                                                    PUSH_ALL |
                                                        PUSH_FIRM(PUSH_ALL),
                                                    {0.0F, 10.0F, 0.0F},
                                                    20.0F,
                                                    0.0F,
                                                    0.0F,
                                                    {0, 0, 0},
                                                    0,
                                                    NULL};

/* the splash step never enters SPLASH_STILL */
enum splash_mode { SPLASH_INIT, SPLASH_RUN, SPLASH_STILL };

/* the splash step throws a splash once tick, read before its increment,
 * passes SPLASH_WAIT: every 17 steps */
#define SPLASH_WAIT 15

void beach_float_step(struct task *task)
{
    struct task_work *work = task->work;

    WORLD_MODEL(work)->model = &beach_float_object;
    model_start_world(task);
}

void bungalow_step(struct task *task)
{
    struct task_work *work = task->work;

    WORLD_MODEL(work)->model = &bungalow_object;
    model_start_world(task);
}

void beach_rock_step(struct task *task)
{
    struct task_work *work = task->work;

    WORLD_MODEL(work)->model = &beach_rock_object;
    model_start_world(task);
}

void rock_pole_step(struct task *task)
{
    struct task_work *work = task->work;

    WORLD_MODEL(work)->model = &rock_pole_object;
    model_start_world(task);
}

void stone_pillar_step(struct task *task)
{
    struct task_work *work = task->work;

    MODEL(work)->object = &stone_pillar_object;
    if (!placement_editing(task)) {
        collision_init(task, &stone_pillar_shape, COLLISION_OBJECT, 1, 1);
    }
    model_start_solid(task);
}

void komomo_step(struct task *task)
{
    struct task_work *work = task->work;

    MODEL(work)->object = &komomo_object;
    model_start_scaled(task);
}

void beach_rock_2_step(struct task *task)
{
    struct task_work *work = task->work;

    MODEL(work)->object = &beach_rock_2_object;
    if (!placement_editing(task)) {
        collision_init(task, &beach_rock_2_shape, COLLISION_OBJECT, 1, 1);
    }
    model_start_solid(task);
}

void beach_splash_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case SPLASH_INIT:
        work->mode = SPLASH_RUN;
        break;
    case SPLASH_RUN:
        if (work->tick++ > SPLASH_WAIT) {
            NJS_VECTOR speed = {0.0F, 0.0F, 0.0F};

            splash_create(&work->pos, &speed, 0.1F);
            work->tick = 0;
        }
        break;
    case SPLASH_STILL:
        break;
    }
}
