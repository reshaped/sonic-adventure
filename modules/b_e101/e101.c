/* B_E101's stage task and its sky's task. the stage task starts the fight
 * with E-101 once no event has control */
#include <sg_xpt.h>

#include "b_e101/boss.h"
#include "b_e101/e101.h"
#include "event/play.h"
#include "sound/stream.h"
#include "system/task.h"

enum e101_mode { E101_INIT, E101_WAIT, E101_START, E101_FIGHT };

static void e101_run(struct task *task);
static void e101_destroy(struct task *task);

/* #pragma section HEAD puts the step in PHEAD, which B_E101's link places
 * first, at 0x0c900000. 1ST_READ.BIN's stage_steps holds that address. the
 * switch of section after the step ends it on its own literal pool */
#pragma section HEAD
void e101_step(struct task *task)
{
    e101_run(task);
}
#pragma section

static void e101_run(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case E101_INIT:
        task->destructor = e101_destroy;
        work->mode++;
        /* fall through: the wait starts this step */
    case E101_WAIT:
        /* @hack spelling: event_in_control compared with TRUE, which SHC tests
         * with CMP/EQ #1 (0x0c900108). event_in_control returns only TRUE or
         * FALSE, so !event_in_control() is the same test; as that, 2 of 41
         * words differ */
        if (event_in_control() != TRUE) {
            work->mode++;
        }
        break;
    case E101_START:
        e101_create();
        stream_reset();
        stream_request_music(MUSIC_BOSSTRGT);
        work->mode++;
        break;
    }
}

static void e101_destroy(struct task *task)
{
}

/* 1ST_READ.BIN's sky_steps holds e101_sky_step's address, 0x0c900040. the sky's
 * task does nothing */
#pragma section HEAD
void e101_sky_step(struct task *task)
{
}
#pragma section
