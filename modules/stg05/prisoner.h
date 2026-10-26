/* Syuujin b, Syuujin r and Syuujin g: prisoners who stir where they sit.
 * Syuujin b also swells along x, and Syuujin r along y */
#ifndef STG05_PRISONER_H
#define STG05_PRISONER_H

#include "system/task.h"

/* the steps mountain_object_list names for Syuujin b, Syuujin r and
 * Syuujin g. a scale of 0 in the set entry stands for 1 */
void prisoner_b_step(struct task *task);
void prisoner_r_step(struct task *task);
void prisoner_g_step(struct task *task);

#endif
