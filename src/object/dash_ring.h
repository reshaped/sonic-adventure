/* the dash ring, CMN_DRING or O DUSH RING in the object lists. a player
 * who passes through it is carried on toward its target */
#ifndef OBJECT_DASH_RING_H
#define OBJECT_DASH_RING_H

#include "system/task.h"

/* the object lists' step for CMN_DRING and O DUSH RING */
void dash_ring_step(struct task *task);

#endif
