/* the two modes Chaos 6's events ask the boss for */
#include "b_chaos6/event_modes.h"
#include "b_chaos6/boss.h"
#include "draw/motion.h"
#include "system/task.h"

/* under section HEAD, SHC writes the two to section PHEAD, which the link
 * places after chaos6.c's. the events call them there, at 0x0c900040 and
 * 0x0c90005e */
#pragma section HEAD
void chaos6_event_melt(void)
{
    struct task_work *work = boss_work;

    if (work->mode != BOSS_MELT) {
        boss_next_mode = BOSS_MELT;
        boss_last_mode = work->mode;
    }
    boss_return_mode = BOSS_MELTED;
}

void chaos6_event_form(void)
{
    struct task_work *work = boss_work;

    if (work->mode != BOSS_FORM) {
        boss_next_mode = BOSS_FORM;
        boss_last_mode = work->mode;
    }
    boss_return_mode = BOSS_WAIT;
}
#pragma section
