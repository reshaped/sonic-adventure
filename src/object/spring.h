/* springs: a touch launches a player along the spring's y axis, and the
 * spring bobs after. spring b also takes the player's control away for the
 * steps its scl.x gives */
#ifndef OBJECT_SPRING_H
#define OBJECT_SPRING_H

#include "system/task.h"

/* the steps of a spring and of spring b, as the object lists place them */
void spring_step(struct task *task);
void spring_b_step(struct task *task);

#endif
