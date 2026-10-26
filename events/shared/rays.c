/* an event's rays: a task that starts count beams at one point each step */
#include <ninja.h>
#include <sg_xpt.h>

#include "effect/beam.h"
#include "shared/rays.h"
#include "system/task.h"

/* the rays' modes, in work->mode */
#define RAYS_IDLE 0
#define RAYS_RUNNING 1

/* the rays' record, from the heap, which the destructor frees */
struct rays {
    struct beam_params beam; /* the parameters each beam starts with */
    int count; /* beams started each step */
};

struct rays_vars {
    Sint32 steps; /* the steps run in RAYS_RUNNING, counted and never read */
    Uint8 unk_04[4];
    struct rays *rays;
};

#define VARS(work) ((struct rays_vars *)&(work)->vars)

/* the rays' task, from create until event_rays_kill */
static struct task *event_rays = NULL;

static void event_rays_init(struct task *task);
static void event_rays_step(struct task *task);
static void event_rays_destroy(struct task *task);

void event_rays_create(Float x, Float y, Float z, int count, int time,
                       Float width_foot, Float width_tip, Uint32 color_foot,
                       Uint32 color_tip)
{
    struct rays *rays;
    struct task_work *work;

    event_rays = task_create(TASK_WORK, 5, event_rays_init);
    rays = heap_alloc(sizeof(struct rays));
    work = event_rays->work;
    VARS(work)->rays = rays;
    work->mode = RAYS_IDLE;
    VARS(work)->steps = 0;
    work->pos.x = x;
    work->pos.y = y;
    work->pos.z = z;
    rays->beam.time = time;
    rays->beam.width[0] = width_foot;
    rays->beam.width[1] = width_tip;
    rays->beam.color[0] = color_foot;
    rays->beam.color[1] = color_tip;
    rays->beam.max_speed = 91;
    rays->beam.length = 100.0F;
    rays->beam.growth = 1.3F;
    rays->count = count;
    work->mode = RAYS_RUNNING;
}

void event_rays_kill(void)
{
    if (event_rays != NULL) {
        task_kill(event_rays);
        event_rays = NULL;
    }
}

static void event_rays_init(struct task *task)
{
    task->step = event_rays_step;
    task->draw = NULL;
    task->destructor = event_rays_destroy;
}

static void event_rays_step(struct task *task)
{
    struct task_work *work = task->work;
    struct rays *rays = VARS(work)->rays;
    int i;

    switch (work->mode) {
    case RAYS_IDLE:
        break;
    case RAYS_RUNNING:
        VARS(work)->steps++;
        for (i = 0; i < rays->count; i++) {
            /* @bug memory: a beam that outlives this task reads its foot from
             * freed memory. each beam keeps &work->pos and reads it each step
             * (JSR at 0x0cb80d7a in EV009D, MOV.L @(24,R14),R4 at 0x8c047a00
             * in beam_draw). event_rays_kill has the task destroy itself on
             * its next step, which frees work, and a beam runs on for about
             * time + min(time, 10) steps from its start */
            beam_create(&work->pos, &rays->beam);
        }
        break;
    }
}

static void event_rays_destroy(struct task *task)
{
    struct task_work *work = task->work;
    struct rays *rays = VARS(work)->rays;

    if (rays != NULL) {
        heap_free(rays);
    }
}
