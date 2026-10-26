/* Perfect Chaos's tornadoes, which move out from where he stands */
#ifndef B_CHAOS7_TORNADO_H
#define B_CHAOS7_TORNADO_H

#include "system/task.h"

/* the work->mode of a tornado that sways across its path. any other mode
 * keeps it on the path */
#define TORNADO_SWAYS 1

/* a tornado's first step, for a task with a work. its creator sets the work's
 * pos, ang.y and mode before the step runs. the tornado moves away from pos
 * in x and z, along (cos ang.y, sin ang.y). its cylinder hurts players and
 * objects. a player's touch knocks player 0 back. this step destroys the task
 * if it can't allocate the tornado's record. the tornado never destroys its
 * task after that */
void chaos7_tornado_start(struct task *task);

#endif
