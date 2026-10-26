/* O SHWR, a shower that holds player 0 and washes off the mud */
#ifndef STG09_SHOWER_H
#define STG09_SHOWER_H

#include "system/task.h"

/* the object list's step. the shower pours while player 0 stands under it,
 * and washes his mud off. on his 36th step there it holds him, for about 500
 * steps, once. a jump button lets him go */
void shower_step(struct task *task);

#endif
