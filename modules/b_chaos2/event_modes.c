/* the modes and the action Chaos 2's event, EV0087.PRS, asks the boss
 * for */
#include <sg_xpt.h>

#include "b_chaos2/boss.h"
#include "b_chaos2/event_modes.h"
#include "chaos/chaos.h"
#include "draw/motion.h"
#include "system/task.h"

/* in the HEAD section, from the module's first address, 0x0c900000, at
 * the addresses the event calls */
#pragma section HEAD
void chaos2_event_melt(void)
{
    struct task_work *work = boss_work;

    if (work->mode != BOSS_MELT) {
        boss_next_mode = BOSS_MELT;
        boss_last_mode = work->mode;
    }
    boss_return_mode = BOSS_MODE_19;
}

void chaos2_event_form(void)
{
    struct task_work *work = boss_work;

    if (work->mode != BOSS_FORM) {
        boss_next_mode = BOSS_FORM;
        boss_last_mode = work->mode;
    }
    boss_return_mode = BOSS_WAIT;
}

void chaos2_event_action(Float speed)
{
    struct chaos *chaos = chaos_record;

    motion_flags &= ~MOTION_HOLDING;
    chaos->motion.wanted = ACTION_EVENT;
    chaos2_actions[ACTION_EVENT].speed = speed;
}
#pragma section
