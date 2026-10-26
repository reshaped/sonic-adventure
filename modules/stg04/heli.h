/* O HeliA and O HeliB, helicopters that hover with a searchlight, and
 * heli_route's helicopter, which carries player 0 */
#ifndef STG04_HELI_H
#define STG04_HELI_H

#include "system/task.h"

/* the object list's steps for O HeliA, whose searchlight sweeps, and O
 * HeliB, whose searchlight follows player 0 */
void heli_a_step(struct task *task);
void heli_b_step(struct task *task);
/* the first step of the task path_create_tasks makes for heli_route. for
 * Tails it does nothing, and the helicopter never appears */
void heli_route_step(struct task *task);

#endif
