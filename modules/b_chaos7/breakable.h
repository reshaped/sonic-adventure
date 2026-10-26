/* the Perfect Chaos fight's cars and walls. the set editor names them
 * O RED CAR, O YELLOW CAR, O BLUE CAR and O WAll A to O WAll C. a car or
 * wall blows apart once hurt, but O WAll C can't be hurt */
#ifndef B_CHAOS7_BREAKABLE_H
#define B_CHAOS7_BREAKABLE_H

#include "system/task.h"

/* the steps chaos7_object_kinds names for them, in that order */
void chaos7_red_car_step(struct task *task);
void chaos7_yellow_car_step(struct task *task);
void chaos7_blue_car_step(struct task *task);
void chaos7_wall_a_step(struct task *task);
void chaos7_wall_b_step(struct task *task);
void chaos7_wall_c_step(struct task *task);

#endif
