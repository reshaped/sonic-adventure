/* O KAZARI: an ornament whose textures scroll and whose light blinks, with
 * a row of ten lamps */
#ifndef B_EGM2_ORNAMENT_H
#define B_EGM2_ORNAMENT_H
#include <ninja.h>
#include <sg_xpt.h>

#include "system/task.h"

/* the first step of O KAZARI's task, which the stage's object list names.
 * create the task with TASK_WORK. its later steps scroll the ornament's
 * textures and blink its light. a child it starts lights the lamps in
 * patterns. the task ends once the camera and player 0 are both farther than
 * 1000 from it */
void ornament_step(struct task *task);
/* set bits in the attrflags of every material in the tree. nothing in
 * B_EGM2 calls it */
void tree_attr_or(NJS_OBJECT *object, Uint32 bits);

#endif
