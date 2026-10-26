/* the modes and the action Chaos 0's events ask the boss for */
#include <sg_xpt.h>

#include "b_chaos0/boss.h"
#include "b_chaos0/event_modes.h"
#include "chaos/chaos.h"
#include "draw/motion.h"
#include "system/task.h"

/* in the HEAD section, from 0x0c9000e0, at the addresses the events call */
#pragma section HEAD
void chaos0_event_melt(void)
{
    struct task_work *work = boss_work;

    if (work->mode != BOSS_MELT) {
        boss_next_mode = BOSS_MELT;
        boss_last_mode = work->mode;
    }
    boss_return_mode = BOSS_MODE_21;
}

void chaos0_event_form(void)
{
    struct task_work *work = boss_work;

    if (work->mode != BOSS_FORM) {
        boss_next_mode = BOSS_FORM;
        boss_last_mode = work->mode;
    }
    boss_return_mode = BOSS_WAIT;
}

void chaos0_event_action(Float speed)
{
    struct chaos *chaos = chaos_record;

    motion_flags &= ~MOTION_HOLDING;
    chaos->motion.wanted = ACTION_EVENT;
    chaos0_actions[ACTION_EVENT].speed = speed;
}
#pragma section
