/* the workshop's runway, its six trees and its four panels, which event flags
 * move out */
#ifndef ADV02_RUNWAY_H
#define ADV02_RUNWAY_H

#include "system/task.h"

/* event flags the scripts of EV000D and EV003B set and clear: one for the
 * runway and its trees, and one for the panels */
#define FLAG_RUNWAY 33
#define FLAG_RUNWAY_PANELS 34

/* the step of the runway's task, which the field task creates as the station
 * act starts */
void runway_step(struct task *task);

#endif
