/* O SHUTTER shutter in act 0 that rises out of the way */
#ifndef STG03_SHUTTER_H
#define STG03_SHUTTER_H

#include "system/task.h"

/* task step for O SHUTTER, named in the object list. outside act 0, the
 * shutter neither draws nor collides. its set entry's scale x offsets where it
 * draws, along the world's x. a scale x below 0 also makes it wait for player 0
 * to ride, not to come near */
void twinkle_shutter_step(struct task *task);

#endif
