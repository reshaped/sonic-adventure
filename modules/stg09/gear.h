/* O HAGURUMA: a gear that turns about y, carrying player 0 round */
#ifndef STG09_GEAR_H
#define STG09_GEAR_H

#include <ninja.h>

#include "system/task.h"

/* O HAGURUMA and the O TUTU tubes turn casino_gear_turn times
 * casino_gear_scale a step, for each unit of speed. each scales only the
 * low halfword of casino_gear_turn, and keeps its high halfword */
extern Angle casino_gear_turn;
extern Float casino_gear_scale;

/* the object list's step. task needs TASK_WORK's and TASK_FORCE's records.
 * the gear turns to the right for an odd scl.x, and its speed is scl.y, each
 * truncated */
void gear_step(struct task *task);

#endif
