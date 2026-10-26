/* the lens flare: a task that draws a light's flare as the camera sees it */
#include <ninja.h>

#include "effect/flare.h"
#include "effect/lens.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/task_desc.h"

static void flare_init(struct task *task, void *arg);
static void flare_step(struct task *task);

const struct task_desc flare_desc = {0,          TASK_WORK, flare_init,
                                     flare_step, NULL,      NULL};

struct task *flare_create(const NJS_POINT3 *pos)
{
    NJS_POINT3 at;

    at = *pos;
    return task_create_desc(4, &flare_desc, &at);
}

struct task *flare_setup(struct task *task)
{
    return task_apply_desc(task, &flare_desc, NULL);
}

static void flare_init(struct task *task, void *arg)
{
    NJS_POINT3 *pos = arg;

    if (pos != NULL) {
        task->work->pos = *pos;
    }
}

static void flare_step(struct task *task)
{
    if (frame_step == 0) {
        lens_draw(&task->work->pos);
    }
}
