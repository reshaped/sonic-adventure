/* an event's stopwatch: a count of steps printed on the screen, with a lap
 * held beside it */
#include <ninja.h>
#include <sg_xpt.h>
#include <shinobi.h>
#include <stdio.h>

#include "shared/stopwatch.h"
#include "system/pad.h"
#include "system/task.h"

/* bits of work->mode: the count runs, and a lap is held */
#define STOPWATCH_RUNNING 0x1
#define STOPWATCH_LAP_HELD 0x2

struct stopwatch_vars {
    Uint8 unk_00[4];
    Sint32 steps;
    Sint32 lap; /* steps, when the lap was taken */
};

#define VARS(work) ((struct stopwatch_vars *)&(work)->vars)

/* the stopwatch's task, from create until event_stopwatch_kill */
static struct task *event_stopwatch = NULL;

static void event_stopwatch_destroy(struct task *task)
{
}

static void event_stopwatch_draw(struct task *task)
{
    char text[64];
    struct task_work *work = task->work;

    sprintf(text, "STOPWATCH %s",
            (work->mode & STOPWATCH_RUNNING) ? "RUN" : "STOP");
    njPrintC(NJM_LOCATION(1, 2), text);
    njPrintD(NJM_LOCATION(18, 2), VARS(work)->steps, 4);
    if (work->mode & STOPWATCH_LAP_HELD) {
        njPrintD(NJM_LOCATION(24, 2), VARS(work)->lap, 4);
    }
}

static void event_stopwatch_take_lap(struct task *task)
{
    struct task_work *work = task->work;

    work->mode |= STOPWATCH_LAP_HELD;
    VARS(work)->lap = VARS(work)->steps;
}

static void event_stopwatch_clear(struct task *task)
{
    struct task_work *work = task->work;

    work->mode = 0;
    VARS(work)->steps = 0;
}

static void event_stopwatch_run(struct task *task)
{
    struct task_work *work = task->work;

    work->mode |= STOPWATCH_RUNNING;
}

static void event_stopwatch_halt(struct task *task)
{
    struct task_work *work = task->work;

    work->mode &= ~STOPWATCH_RUNNING;
}

static void event_stopwatch_step(struct task *task)
{
    struct task_work *work = task->work;

    if (work->mode & STOPWATCH_RUNNING) {
        VARS(work)->steps++;
        if (per_info[1]->press & PDD_DGT_TR) {
            event_stopwatch_halt(task);
        }
        if (per_info[1]->press & PDD_DGT_TL) {
            event_stopwatch_take_lap(task);
        }
    } else {
        if (per_info[1]->press & PDD_DGT_TR) {
            event_stopwatch_run(task);
        }
        if (per_info[1]->press & PDD_DGT_TL) {
            event_stopwatch_clear(task);
        }
    }
    event_stopwatch_draw(task);
}

static void event_stopwatch_init(struct task *task)
{
    task->step = event_stopwatch_step;
    task->draw = event_stopwatch_draw;
    task->destructor = event_stopwatch_destroy;
    event_stopwatch_clear(task);
}

void event_stopwatch_create(void)
{
    event_stopwatch = task_create(TASK_WORK, 5, event_stopwatch_init);
}

void event_stopwatch_kill(void)
{
    if (event_stopwatch != NULL) {
        task_kill(event_stopwatch);
        event_stopwatch = NULL;
    }
}

void event_stopwatch_reset(void)
{
    event_stopwatch_clear(event_stopwatch);
}

void event_stopwatch_start(void)
{
    event_stopwatch_run(event_stopwatch);
}

void event_stopwatch_stop(void)
{
    event_stopwatch_halt(event_stopwatch);
}

void event_stopwatch_lap(void)
{
    event_stopwatch_take_lap(event_stopwatch);
}
