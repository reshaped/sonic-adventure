/* O ITA02: a plank fence that is only drawn */
#include <ninja.h>

#include "draw/model.h"
#include "object/placement.h"
#include "stg08/plank_2.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT plank_2_object;

/* a plank fence's range for placement_try_leave_by, squared: about 1000
 * units */
#define PLANK_RANGE 1000100.0F

static void plank_2_destroy(struct task *task);
static void plank_2_run(struct task *task);
static void plank_2_draw(struct task *task);

void plank_2_step(struct task *task)
{
    if (!placement_try_leave_by(task, PLANK_RANGE)) {
        task->draw = plank_2_draw;
        task->step = plank_2_run;
        task->destructor = plank_2_destroy;
    }
}

static void plank_2_destroy(struct task *task)
{
}

static void plank_2_run(struct task *task)
{
    /* @hack spelling: planks, a copy of task, which keeps task in R14 across
     * the range check (MOV R4,R14 at 0x0c9090f0); through task, it goes to
     * the frame and 12 of 16 words differ */
    struct task *planks = task;

    if (!placement_try_leave_by(planks, PLANK_RANGE)) {
        plank_2_draw(planks);
    }
}

static void plank_2_draw(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave_by(task, PLANK_RANGE)) {
        return;
    }
    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateX, work->ang.x);
    TURN(njRotateY, work->ang.y);
    draw_model(plank_2_object.model);
    njPopMatrix(1);
}
