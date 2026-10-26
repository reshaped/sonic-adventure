/* StarNeon, YajiNeon and Densyoku: a star, arrows and strings of lights in
 * neon, each flashing through its frames */
#ifndef STG09_NEON_H
#define STG09_NEON_H

#include "system/task.h"

/* the object list's steps. scl.x scales the neon. for an arrow or a string,
 * scl.y picks which of three. 0 picks the first, 1 the second, and any other
 * value the third. each of the neon's frames shows for scl.z + 2 steps,
 * rounded down */
void neon_star_step(struct task *task);
void neon_arrow_step(struct task *task);
void neon_lights_step(struct task *task);

#endif
