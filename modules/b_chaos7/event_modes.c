/* the Perfect Chaos modes that the last story's events ask for */
#include <sg_xpt.h>

#include "b_chaos7/boss.h"
#include "b_chaos7/event_modes.h"
#include "draw/motion.h"
#include "system/task.h"

/* under section HEAD, SHC writes these functions to section PHEAD, which the
 * module's link places first. the events call them by their addresses there */
#pragma section HEAD
void chaos7_event_mode_1(void)
{
    struct task_work *work = boss_work;

    if (work->mode != BOSS_READY) {
        boss_next_mode = BOSS_READY;
        boss_last_mode = work->mode;
    }
}

void chaos7_event_mode_2(void)
{
    struct task_work *work = boss_work;

    if (work->mode != BOSS_MODE_2) {
        boss_next_mode = BOSS_MODE_2;
        boss_last_mode = work->mode;
    }
}

void chaos7_event_mode_13(void)
{
    struct task_work *work = boss_work;

    if (work->mode != BOSS_MODE_13) {
        boss_next_mode = BOSS_MODE_13;
        boss_last_mode = work->mode;
    }
}

void chaos7_event_mode_14(void)
{
    struct task_work *work = boss_work;

    if (work->mode != BOSS_MODE_14) {
        boss_next_mode = BOSS_MODE_14;
        boss_last_mode = work->mode;
    }
}

void chaos7_event_ray(Float length)
{
    struct task_work *work = boss_work;

    if (work->mode == BOSS_EVENT_RAY) {
        return;
    }
    /* @hack redundant: the mode tested again, though the return above makes
     * this test always pass. SHC then reads and compares it a second time
     * (CMP/EQ #16,R0 at 0x0c900454); without it, the function runs 6 bytes
     * short, and 14 of 18 words differ */
    if (work->mode != BOSS_EVENT_RAY) {
        boss_next_mode = BOSS_EVENT_RAY;
        boss_last_mode = work->mode;
    }
    work->vars.f[0] = length;
}
#pragma section
