/* O CORN, O SIGNAL and O TIRE: the Perfect Chaos fight's props, which are only
 * drawn, and five more that nothing places */
#ifndef B_CHAOS7_PROPS_H
#define B_CHAOS7_PROPS_H

#include "system/task.h"

/* each prop's step, for a placed task with a work. chaos7_object_kinds names
 * the cone's, the signal's and the tire's. nothing in this image refers to the
 * five numbered props' */
void chaos7_cone_step(struct task *task);
void chaos7_prop_1_step(struct task *task);
void chaos7_prop_2_step(struct task *task);
void chaos7_prop_3_step(struct task *task);
void chaos7_prop_4_step(struct task *task);
void chaos7_prop_5_step(struct task *task);
void chaos7_signal_step(struct task *task);
void chaos7_tire_step(struct task *task);

#endif
