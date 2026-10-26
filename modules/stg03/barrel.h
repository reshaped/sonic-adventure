/* O BARREL powder keg whose fuse player 0 lights */
#ifndef STG03_BARREL_H
#define STG03_BARREL_H

#include "system/task.h"

/* the object list's step for O BARREL, whose task needs TASK_WORK's record.
 * player 0's work must exist. player 0 within 30 of the barrel lights its fuse.
 * the fuse burns 600 steps plus the set entry's scale x, rounded down. the
 * barrel blows 2 steps later, into five pieces. its blast hurts players,
 * enemies and objects. a failed copy of its shapes and a failed twinkle_reflect
 * go unchecked */
void barrel_step(struct task *task);

#endif
