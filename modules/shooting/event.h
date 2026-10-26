/* the sky chase's events: act 1's, as its course nears its end, and act
 * 2's, half way */
#ifndef SHOOTING_EVENT_H
#define SHOOTING_EVENT_H

#include "system/task.h"

/* the steps of act 1's and act 2's events. the stage task creates them as it
 * enters SHOOTING_EVENT1 and SHOOTING_EVENT2. each plays its act's camera
 * actions, then kills its task */
void shooting_event1_step(struct task *task);
void shooting_event2_step(struct task *task);

#endif
