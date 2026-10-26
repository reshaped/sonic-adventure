/* a Chao's motions: which one in its table it plays, and how far in */
#ifndef AL_MAIN_MOTION_H
#define AL_MAIN_MOTION_H

#include "al_main/chao.h"
#include "system/task.h"

/* start the Chao on its table's first motion, at a random frame of its
 * stretch. its rate becomes 1, and its six lowest flags clear */
void alife_motion_init(struct task_work *work, struct alife_chao *chao);
/* play the Chao's motion one step on. MOTION_NEW starts a blend into the
 * motion id names. while a blend runs, or once a motion has stopped at its
 * end, the frame stays where it is. at the end of its stretch, a motion
 * loops, blends, changes or stops as its entry's end says. it then sets
 * MOTION_DONE for the step, and its rate goes back to 1 */
void alife_motion_step(struct task_work *work, struct alife_chao *chao);

#endif
