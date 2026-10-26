/* the doors, which the set editor names O DOOR, O DOOR B and O DOUBLE DOOR.
 * the first two are rows of panels. they rise for player 0 from one side,
 * and shut for good from the other. a double door's two leaves swing open */
#ifndef STG03_DOORS_H
#define STG03_DOORS_H

#include "system/task.h"

/* the steps twinkle_object_kinds names for O DOOR, O DOOR B and O DOUBLE
 * DOOR. each task needs a work, and player 0 must exist */
void door_step(struct task *task);
void door_b_step(struct task *task);
void double_door_step(struct task *task);

#endif
