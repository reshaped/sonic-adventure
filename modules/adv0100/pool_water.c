/* O POOLWATER, the pool's water, whose textures ripple. the pool is drained
 * while FLAG_KNUCKLES_EC_POOL is set */
#include <ninja.h>

#include "adv0100/pool_water.h"
#include "adv0100/texlists.h"
#include "collision/world.h"
#include "draw/tree.h"
#include "player/player.h"
#include "progress/progress.h"
#include "system/task.h"

extern NJS_OBJECT pool_water_object;
/* the water's collision model, in which a player swims */
extern NJS_OBJECT pool_water_solid_object;

/* the flag Knuckles' story sets as it plays event 289 in act 2. it drains
 * the pool, and opens the pool door for Knuckles */
#define FLAG_KNUCKLES_EC_POOL 269

/* the ripple's frames, which are the first textures of ec_water_texlist, and
 * the steps each lasts. the model's first RIPPLE_MATERIALS materials show
 * it */
#define RIPPLE_FRAMES 14
#define RIPPLE_STEPS 4
#define RIPPLE_MATERIALS 3

struct pool_water_vars {
    NJS_OBJECT *solid; /* in the collision world */
    NJS_OBJECT *from; /* the object whose model, evalflags and pos it takes */
};

#define VARS(work) ((struct pool_water_vars *)&(work)->vars)
#define RIPPLE(work) ((work)->tick) /* the ripple's frame */

/* in work->flags, set while the pool is full */
#define WATER_FULL 0x400

enum pool_water_mode { WATER_INIT, WATER_RUN, WATER_DRAINED };

static void pool_water_destroy(struct task *task);
static void pool_water_draw(struct task *task);

/* add the water to the collision world as solid water, where its model
 * stands. while the pool is full, advance the ripple every RIPPLE_STEPS
 * steps. the world tests the water while player 0 is within 100 of it. while
 * the pool is drained, the step returns before that test, so WORLD_TESTED
 * keeps the value it had as the pool drained */
void egg_carrier_pool_water_step(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *solid;

    switch (work->mode) {
    case WATER_INIT:
        VARS(work)->from = &pool_water_solid_object;
        VARS(work)->solid = solid = world_object_alloc();
        solid->evalflags = VARS(work)->from->evalflags | NJD_EVAL_BREAK;
        solid->scl[0] = 1.0F;
        solid->scl[1] = 1.0F;
        solid->scl[2] = 1.0F;
        solid->ang[0] = 0;
        solid->ang[1] = 0;
        solid->ang[2] = 0;
        solid->pos[0] = VARS(work)->from->pos[0];
        solid->pos[1] = VARS(work)->from->pos[1];
        solid->pos[2] = VARS(work)->from->pos[2];
        solid->model = VARS(work)->from->model;
        solid->child = NULL;
        solid->sibling = NULL;
        world_add(WORLD_SOLID | WORLD_WATER, task, solid);
        work->flags |= WATER_FULL;
        RIPPLE(work) = 0;
        work->timer = RIPPLE_STEPS;
        task->destructor = pool_water_destroy;
        task->draw = pool_water_draw;
        work->mode = WATER_RUN;
        break;
    case WATER_RUN:
        if (!--TASK_TIMER(work)) {
            work->timer = RIPPLE_STEPS;
            RIPPLE(work)++;
            if (RIPPLE(work) == RIPPLE_FRAMES) {
                RIPPLE(work) = 0;
            }
        }
        if (event_flag_check(FLAG_KNUCKLES_EC_POOL)) {
            work->flags &= ~WATER_FULL;
            work->mode = WATER_DRAINED;
        }
        break;
    case WATER_DRAINED:
        if (!event_flag_check(FLAG_KNUCKLES_EC_POOL)) {
            work->flags |= WATER_FULL;
            work->mode = WATER_RUN;
        }
        return;
    }
    work->flags &= ~WORLD_TESTED;
    if (njDistanceP2P(&player_work[0]->pos,
                      (NJS_POINT3 *)VARS(work)->solid->pos) < 100.0F) {
        work->flags |= WORLD_TESTED;
    }
    pool_water_draw(task);
}

static void pool_water_destroy(struct task *task)
{
    struct task_work *work = task->work;

    if (VARS(work)->solid != NULL) {
        world_remove(task, VARS(work)->solid);
        world_object_free(VARS(work)->solid);
    }
}

/* draw the water at the ripple's frame while the pool is full */
static void pool_water_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object;
    NJS_MATERIAL *material;
    int i;

    if (!(work->flags & WATER_FULL)) {
        return;
    }
    object = &pool_water_object;
    material = object->model->mats;
    for (i = 0; i < RIPPLE_MATERIALS; i++, material++) {
        material->attr_texId = RIPPLE(work);
    }
    njSetTexture(&ec_water_texlist);
    njPushMatrix(NULL);
    draw_tree(object);
    njPopMatrix(1);
}
