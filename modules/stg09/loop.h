/* Loop, a ring of animated sprites that shrinks when touched */
#ifndef STG09_LOOP_H
#define STG09_LOOP_H
#include "system/task.h"

/* the object list's step. each new touch plays SE_LOOP and shrinks the ring
 * to a quarter of its radius at rest. the ring then grows back */
void loop_step(struct task *task);

#endif
