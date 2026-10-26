/* O PIT trapdoor that shakes, then swings open */
#ifndef STG03_PIT_H
#define STG03_PIT_H

#include "system/task.h"

/* task step for O PIT, named in the object list. its set entry's scale x adds
 * steps to the shake. scale z above 0 also draws the pit a half turn about y,
 * as its reflection */
void pit_step(struct task *task);

#endif
