/* act 0's upper pool, above the dam. it pours into the act's water once
 * Big has pulled its plug and that water is SHELTER_WATER_RISEN */
#include <sg_xpt.h>

#include "stage/stage.h"
#include "stg12/lid.h"
#include "stg12/plug.h"
#include "stg12/upper_pool.h"
#include "stg12/upper_water.h"
#include "stg12/water.h"
#include "stg12/waterfall.h"
#include "system/task.h"

#define POOL_X 750.0F
#define POOL_Y 65.0F
#define POOL_Z (-650.0F)

/* the pool's surface starts at FULL_LEVEL. as the pool pours, its surface
 * falls POUR_SPEED each step down to LOW_LEVEL */
#define FULL_LEVEL 155.0F
#define POUR_SPEED 0.15F
#define LOW_LEVEL 82.5F

/* the act's water rises FILL_SPEED each step the pool pours, about 62.1
 * units in all */
#define FILL_SPEED (62.1F / (FULL_LEVEL - LOW_LEVEL) * POUR_SPEED)

enum shelter_upper_pool_mode { POOL_FULL, POOL_POUR, POOL_LOW };

int shelter_big_flags = 0;
Float shelter_upper_pool_level;

static void shelter_upper_pool_step(struct task *task);
static void shelter_upper_pool_destroy(struct task *task);

void shelter_upper_pool_start(struct task *task)
{
    struct task_work *work = task->work;

    work->mode = POOL_FULL;
    /* the stage and the act the pool starts in. the step kills the pool
     * once either changes */
    TASK_TIMER(work) = stage_get_stage_act();
    work->pos.x = POOL_X;
    work->pos.y = POOL_Y;
    work->pos.z = POOL_Z;
    shelter_big_flags = 0;
    task_create_child(TASK_WORK | TASK_MOTION, shelter_plug_start, task);
    task_create_child(TASK_WORK, shelter_lid_start, task);
    shelter_upper_pool_level = FULL_LEVEL;
    task_create_child(TASK_WORK | TASK_FORCE, shelter_upper_water_start, task);
    task->step = shelter_upper_pool_step;
    task->destructor = shelter_upper_pool_destroy;
}

static void shelter_upper_pool_step(struct task *task)
{
    struct task_work *work = task->work;

    if (TASK_TIMER(work) != stage_get_stage_act()) {
        task_kill(task);
        return;
    }
    switch (work->mode) {
    case POOL_FULL:
        if ((shelter_big_flags & BIG_PULLED) &&
            shelter_water_state == SHELTER_WATER_RISEN) {
            work->mode = POOL_POUR;
            task_create_child(TASK_WORK, shelter_waterfall_start, task);
        }
        task_step_children(task);
        break;
    case POOL_POUR:
        shelter_upper_pool_level -= POUR_SPEED;
        if (shelter_upper_pool_level <= LOW_LEVEL) {
            shelter_upper_pool_level = LOW_LEVEL;
            shelter_water_state = SHELTER_WATER_FULL;
            work->mode = POOL_LOW;
        } else {
            shelter_water_level += FILL_SPEED;
        }
        task_step_children(task);
        break;
    case POOL_LOW:
        task_step_children(task);
        break;
    }
}

static void shelter_upper_pool_destroy(struct task *task)
{
    task_destroy_children(task);
}
