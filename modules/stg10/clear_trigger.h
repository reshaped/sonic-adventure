/* C RELEASE trigger that clears the stage when player 0 comes within its
 * radius. once Sonic has cleared Final Egg, it places a capsule instead */
#ifndef STG10_CLEAR_TRIGGER_H
#define STG10_CLEAR_TRIGGER_H

#include "system/task.h"

/* the object list's step */
void final_clear_trigger_step(struct task *task);

#endif
