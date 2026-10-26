/* O COLOR DOOR and O COLOR DOOR2 doors, and the C FAKE AMY boxes that warp
 * player 0 past them */
#ifndef STG10_COLOR_DOOR_H
#define STG10_COLOR_DOOR_H

#include "system/task.h"

/* the object list's steps for O COLOR DOOR, O COLOR DOOR2 and C FAKE AMY */
void color_door_step(struct task *task);
void color_door2_step(struct task *task);
void fake_amy_step(struct task *task);
/* pick at random the unit, 0 to 4, whose C FAKE AMY warps player 0. the O
 * COLOR DOOR2 doors show that unit's look. the stage calls it for Amy as act
 * 0 starts */
void color_door_pick(void);

#endif
