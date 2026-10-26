/* O ROBO TV, a television that plays a picture once switch 4 is on */
#ifndef STG12_ROBO_TV_H
#define STG12_ROBO_TV_H

#include "system/task.h"

/* the step act 1's object list names for O ROBO TV. the crash TV runs it
 * too, for the robo TV it creates as its child */
void robo_tv_step(struct task *task);

#endif
