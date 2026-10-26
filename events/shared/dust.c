/* an event's dust: a task that puffs dust where it is */
#include <ninja.h>

#include "effect/dust.h"
#include "shared/dust.h"
#include "system/task.h"

/* the params dust_create_from takes, filled from the puffing task's at its
 * place. a puff keeps a pointer to them, not a copy. so every puff of every
 * dust task steps by the last puff's growth, rate, drag and lift */
static struct dust_params event_dust_puff;

static void event_dust_destroy(struct task *task)
{
    heap_free(((struct event_dust_work *)task->work)->params);
}

static void event_dust_draw(struct task *task)
{
}

static void event_dust_step(struct task *task)
{
    struct event_dust_work *work = (struct event_dust_work *)task->work;

    if (--work->timer == 0) {
        struct dust_params *params = work->params;

        work->timer = work->interval;
        event_dust_puff.scale = params->scale;
        event_dust_puff.growth = params->growth;
        event_dust_puff.rate = params->rate;
        event_dust_puff.drag = params->drag;
        event_dust_puff.lift = params->lift;
        event_dust_puff.speed = params->speed;
        event_dust_puff.color = params->color;
        event_dust_puff.pos = work->pos;
        dust_create_from(&event_dust_puff);
    }
}

struct task *event_dust_create(void)
{
    struct task *task = task_create(TASK_WORK, 5, NULL);

    if (task != NULL) {
        struct event_dust_work *work;
        struct dust_params *params;

        task->step = event_dust_step;
        task->draw = event_dust_draw;
        task->destructor = event_dust_destroy;
        work = (struct event_dust_work *)task->work;
        work->timer = work->interval = 0;
        params = heap_alloc(sizeof(struct dust_params));
        work->params = params;
        params->scale = 1.0F;
        params->growth = 0.038F;
        params->rate = 0.19F;
        params->drag = 0.06F;
        params->lift = 0.025F;
        params->speed.x = 0.0F;
        params->speed.y = 0.0F;
        params->speed.z = 0.0F;
        params->color.a = 1.0F;
        params->color.r = 1.0F;
        params->color.g = 1.0F;
        params->color.b = 1.0F;
    }
    return task;
}
