/* the gardens' areas: GD GRDUND, GD_WATER, GD ROAD, GD FLY and GD CLIFF.
 * each is a place entry of its own kind */
#include <ninja.h>

#include "al_main/area.h"
#include "al_main/entry.h"
#include "system/task.h"

/* how far past its work's scl.x an area's entry reaches */
#define AREA_REACH 10.0F

static void area_step(struct task *task)
{
}

static void area_destroy(struct task *task)
{
    alife_entry_remove(task->extra);
    /* @hack redundant: the return, which opens the frame with ADD #-4,R15 and
     * stores task at @R15 (0x0c908c44, 0x0c908c46); without it, SHC pushes
     * task, and 6 of 6 words differ */
    return; /* NOLINT(readability-redundant-control-flow) */
}

void garden_ground_init(struct task *task)
{
    struct task_work *work = task->work;

    work->submode = ALIFE_GROUND;
    alife_entry_add(task, ALIFE_PLACES, ALIFE_GROUND, work->scl.x + AREA_REACH);
    task->step = area_step;
    task->destructor = area_destroy;
}

void garden_water_init(struct task *task)
{
    struct task_work *work = task->work;

    work->submode = ALIFE_WATER;
    alife_entry_add(task, ALIFE_PLACES, ALIFE_WATER, work->scl.x + AREA_REACH);
    task->step = area_step;
    task->destructor = area_destroy;
}

void garden_road_init(struct task *task)
{
    struct task_work *work = task->work;

    work->submode = ALIFE_ROAD;
    alife_entry_add(task, ALIFE_PLACES, ALIFE_ROAD, work->scl.x + AREA_REACH);
    task->step = area_step;
    task->destructor = area_destroy;
}

void garden_fly_init(struct task *task)
{
    struct task_work *work = task->work;

    work->submode = ALIFE_FLY;
    alife_entry_add(task, ALIFE_PLACES, ALIFE_FLY, work->scl.x + AREA_REACH);
    task->step = area_step;
    task->destructor = area_destroy;
}

void garden_cliff_init(struct task *task)
{
    struct task_work *work = task->work;

    work->submode = ALIFE_CLIFF;
    alife_entry_add(task, ALIFE_PLACES, ALIFE_CLIFF, work->scl.x + AREA_REACH);
    task->step = area_step;
    task->destructor = area_destroy;
}
