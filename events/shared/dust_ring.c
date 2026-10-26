/* an event's ring of dust: glowing puffs in a ring about a point */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "effect/dust.h"
#include "shared/dust_ring.h"
#include "system/task.h"
#include "system/util.h"

#define DUST_RING_STEPS 16 /* the steps from one ring to the next */
/* a ring's puffs, DUST_RING_SPACING apart. 31 of them leave a gap where a
 * 32nd would close the ring */
#define DUST_RING_PUFFS 31
#define DUST_RING_SPACING 0x800 /* a 32nd of a turn */
#define DUST_RING_RADIUS 10.0F

/* the params of every puff in the ring. the ring sets each one's place,
 * scale and speed, which the puff copies */
static struct dust_params event_dust_ring_params = {
    1.0F,
    0.03F,
    0.19F,
    0.05F,
    0.025F,
    {0.0F, 0.0F, 0.0F},
    {0.0F, 0.0F, 0.0F},
    {0.3F, 0.8F, 0.8F, 1.0F},
};
static struct task *event_dust_ring_task = NULL;

static void event_dust_ring_destroy(struct task *task)
{
}

/* once work->tick reaches DUST_RING_STEPS, puff a ring about the task and set
 * work->tick back to 0. each puff is scattered a little, and drifts in x and z
 * at random */
static void event_dust_ring_puff(struct task_work *work)
{
    /* @hack type: i as an Sint8, which the image's loop, four bodies to a pass,
     * tests with EXTS.B (0x0cb819c6 in EV004C). as an int, the puff runs 708
     * bytes short and 445 of its 462 words differ */
    Sint8 i;
    Angle ang;

    if (work->tick >= DUST_RING_STEPS) {
        work->tick = 0;
        ang = 0;
        for (i = 0; i < DUST_RING_PUFFS; ++i) {
            event_dust_ring_params.pos.x = work->pos.x +
                                           (njCos(ang) * DUST_RING_RADIUS) +
                                           RANDOM(-3.0F, 3.0F);
            event_dust_ring_params.pos.y = work->pos.y + RANDOM(-2.0F, 2.0F);
            event_dust_ring_params.pos.z = work->pos.z +
                                           (njSin(ang) * DUST_RING_RADIUS) +
                                           RANDOM(-3.5F, 3.5F);
            event_dust_ring_params.scale = 0.5F;
            event_dust_ring_params.speed.x = RANDOM(-1.0F, 1.0F);
            event_dust_ring_params.speed.y = 0.0F;
            event_dust_ring_params.speed.z = RANDOM(-1.0F, 1.0F);
            dust_create_glow(&event_dust_ring_params);
            ang += DUST_RING_SPACING;
        }
    }
}

static void event_dust_ring_step(struct task *task)
{
    switch (task->work->mode) {
    case EVENT_DUST_RING_WAIT:
        break;
    case EVENT_DUST_RING_ON:
        event_dust_ring_puff(task->work);
        ++task->work->tick;
        break;
    case EVENT_DUST_RING_OFF:
        break;
    }
}

static void event_dust_ring_init(struct task *task)
{
    struct task_work *work = task->work;

    work->mode = EVENT_DUST_RING_WAIT;
    work->tick = 0;
    task->step = event_dust_ring_step;
    task->destructor = event_dust_ring_destroy;
}

void event_dust_ring_create(Float x, Float y, Float z)
{
    struct task_work *work;

    event_dust_ring_task = task_create(TASK_WORK, 5, event_dust_ring_init);
    if (event_dust_ring_task != NULL) {
        work = event_dust_ring_task->work;
        work->pos.x = x;
        work->pos.y = y;
        work->pos.z = z;
    }
}

void event_dust_ring_set_mode(enum event_dust_ring_mode mode)
{
    if (event_dust_ring_task != NULL) {
        event_dust_ring_task->work->mode = mode;
    }
}

void event_dust_ring_kill(void)
{
    if (event_dust_ring_task != NULL) {
        task_kill(event_dust_ring_task);
        event_dust_ring_task = NULL;
    }
}
