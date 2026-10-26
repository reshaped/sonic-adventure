/* Chaos 6's jets: forty jets of water, and the glow on the boss's tail */
#ifndef B_CHAOS6_JETS_H
#define B_CHAOS6_JETS_H

#include "system/task.h"

/* in the jets' task's work->flags */
#define JETS_HIT 0x100 /* a jet has knocked player 0 back */
#define JETS_END 0x200 /* set by the boss to draw the jets back in */

/* the step of the jets' task, which the boss creates with a work. its
 * first step makes the forty jets round him, as its children. it then
 * fires them ten at a time, every 45 steps. once JETS_END is set, a step
 * draws them all back in. the task kills itself on the 100th step, counting
 * that one */
void chaos6_jets_step(struct task *task);
/* the first step of the tail glow's task, which needs a work. it makes the
 * task the glow on the boss's tail, which fades in and holds until
 * chaos6_tail_glow_end. the glow then fades out, and its task kills itself.
 * it draws in chaos6_tail_matrix's frame, which the boss's step must have
 * set */
void chaos6_tail_glow_start(struct task *task);
/* let the tail glow fade out */
void chaos6_tail_glow_end(void);

#endif
