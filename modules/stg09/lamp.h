/* O SLB, O SLG, O SLR and O SLY lamps, which play their actions in a loop.
 * the four are blue, green, red and yellow */
#ifndef STG09_LAMP_H
#define STG09_LAMP_H

#include "system/task.h"

/* the object list's steps. scl.y is the action frames a lamp advances each
 * step */
void lamp_blue_step(struct task *task);
void lamp_green_step(struct task *task);
void lamp_red_step(struct task *task);
void lamp_yellow_step(struct task *task);

#endif
