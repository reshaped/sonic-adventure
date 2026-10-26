/* O Bat, a bat that hangs in Red Mountain's caves. when player 0 comes near,
 * it flies about, then goes back to hang where it started */
#ifndef STG05_BAT_H
#define STG05_BAT_H

#include "system/task.h"

/* the object list's step */
void bat_step(struct task *task);

#endif
