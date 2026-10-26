/* O SIGNB poster board with three lamps that Knuckles breaks */
#ifndef STG04_BILLBOARD_H
#define STG04_BILLBOARD_H

#include "system/task.h"

/* task step for O SIGNB, named in the object list. its set entry's scale x
 * picks one of five posters, in steps of 10 */
void billboard_step(struct task *task);

#endif
