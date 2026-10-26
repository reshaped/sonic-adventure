/* O FSAKU band between two posts that stretches and pulls a player back */
#ifndef STG10_ELASTIC_FENCE_H
#define STG10_ELASTIC_FENCE_H

#include "system/task.h"

/* the object list's step. scl.x, truncated, mod 3, picks the throw. 0 throws a
 * player back the way it came, reflected in the band. 1 throws the same way
 * from the player's heading masked to 0, 45, 90 or 135 degrees. any other
 * throws nothing.
 *
 * scl.y, truncated, mod 3, picks the pull. 1 pulls in proportion to the square
 * of the band's extra length. any other pulls in proportion to that length.
 *
 * scl.z, truncated, numbers the fence. a numbered band throws toward the fence
 * numbered one more. it does so only where that fence lies within 67.5 degrees
 * of the throw */
void elastic_fence_start(struct task *task);

#endif
