/* the pyramid in the jungle, which rises once Sonic's flag is set, opening the
 * way into the ruin */
#ifndef ADV02_PYRAMID_H
#define ADV02_PYRAMID_H

#include "system/task.h"

/* the step of the pyramid's task, which the field task creates as the jungle
 * act starts */
void pyramid_step(struct task *task);

#endif
