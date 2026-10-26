/* O TIKEIANIM, which animates the land of acts 0 and 1 */
#ifndef STG09_LAND_ANIM_H
#define STG09_LAND_ANIM_H

#include "system/task.h"

/* the object list's step. a task waits in this step while another runs, or
 * while the set editor is open. in act 0, the task puts the water in the
 * collision world, plays its sound and animates it. while player 0 isn't
 * Sonic, it also makes some of the land solid. that land is solid only while
 * player 0 is above y -110. player 0 must also be outside an upright cylinder
 * about (32.83, 24.6, -78.76), 160 in radius and 520 high. in act 1, the task
 * swaps the land's textures. it swaps fans 1 and 2's lamps too, while each fan
 * is idle. it also raises player 0's mud while player 0 is under water. the
 * task kills itself once the act changes */
void land_anim_step(struct task *task);

#endif
