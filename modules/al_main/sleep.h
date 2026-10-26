/* a Chao's sleep */
#ifndef AL_MAIN_SLEEP_H
#define AL_MAIN_SLEEP_H

#include "al_main/chao.h"
#include "system/task.h"

/* MODE_SLEEP's behavior. the Chao lies down and snores, and its record's
 * sleep falls by a point every 51 steps, the first in at most as many. at 0
 * the Chao gets up, and at the motion's end it goes back to MODE_IDLE, with
 * MODE_SLEEP its last_mode. each call shows eyes 4, and sets the Chao's
 * accel to -0.2 times its speed on x and z */
void alife_sleep(struct task_work *work, struct alife_chao *chao);

#endif
