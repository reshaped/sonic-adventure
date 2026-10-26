/* O CARDKAN: a card sign whose middle and upper parts turn, and whose faces
 * change in phases */
#ifndef STG09_CARD_SIGN_H
#define STG09_CARD_SIGN_H

#include "system/task.h"

/* the object list's step. task needs TASK_WORK's record. scl.x and scl.y
 * are the degrees the upper and middle parts turn each step. each phase
 * shows for scl.z + 2 steps, scl.z truncated */
void card_sign_step(struct task *task);

#endif
