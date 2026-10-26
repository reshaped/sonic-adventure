/* an event's light: Tikal's light, under a task that runs the paths its
 * event record queues */
#include <ninja.h>
#include <sg_xpt.h>

#include "event/event.h"
#include "object/tikal_light.h"
#include "shared/light.h"
#include "system/task.h"

/* the light task's work. pos, collision and event lie where task_work
 * keeps them, for the event calls that take it as one */
struct light_work {
    Uint8 unk_00[12];
    Float unk_0c; /* event_light_create clears it, and no code reads it */
    /* Tikal's light, a child task, or null once the light ends */
    union {
        struct task *task;
        Float size_add; /* task's bits, which the step reads as a Float */
    } light;
    /* the steps between flashes. it lies where task_work keeps ang.x, so a
     * path's step writes it */
    Sint32 flash_interval;
    /* the steps left to the next flash. it lies where task_work keeps ang.y,
     * so a path's step writes it. event_light_create sets it to 0. without a
     * path, it then runs down from -1, and reaches 0 only after 2^32 steps */
    Sint32 flash_timer;
    Uint8 unk_1c[4];
    NJS_POINT3 pos;
    Float alpha;
    /* added to alpha for the light each step, leaving alpha as it is */
    Float alpha_add;
    Float size;
    struct collision_work *collision;
    struct event_work *event;
};

static void event_light_destroy(struct task *task)
{
    task_destroy_children(task);
}

static void event_light_draw(struct task *task)
{
}

static void event_light_step(struct task *task)
{
    struct light_work *work = (struct light_work *)task->work;
    Float alpha;
    Float size;

    if (work->event->paths != NULL) {
        event_paths_step((struct task_work *)work);
    }
    alpha = work->alpha + work->alpha_add;
    /* @bug logic: adds the light's task pointer, read as a Float, to size
     * (FMOV.S @(R0,R14),FR3 at 0x0cb82f14 in EV0001, R0 16, the pointer's
     * offset). a heap address read so lies within 1e-30 of 0, and a null one
     * is 0 */
    size = work->size + work->light.size_add;
    if (alpha < 0.0F) {
        alpha = 0.0F;
    } else if (alpha > 1.0F) {
        alpha = 1.0F;
    }
    if (size < 0.0F) {
        size = 0.0F;
    }
    tikal_light_set(work->light.task, size, alpha);
    if (--work->flash_timer == 0) {
        tikal_light_flash(work->light.task);
        work->flash_timer = work->flash_interval;
    }
    task_step_children(task);
}

struct task *event_light_create(Float x, Float y, Float z)
{
    struct task *task;
    struct light_work *work;

    task = task_create(TASK_WORK, 5, NULL);
    if (task != NULL) {
        task->step = event_light_step;
        task->draw = event_light_draw;
        task->destructor = event_light_destroy;
        work = (struct light_work *)task->work;
        event_alloc((struct task_work *)work);
        work->pos.x = x;
        work->pos.y = y;
        work->pos.z = z;
        work->flash_interval = 0;
        work->flash_timer = 0;
        work->alpha = 1.0F;
        work->alpha_add = 0.0F;
        work->size = 4.0F;
        work->unk_0c = 0.0F;
        work->light.task =
            tikal_light_create(task, &work->light.task, &work->pos);
    }
    return task;
}
