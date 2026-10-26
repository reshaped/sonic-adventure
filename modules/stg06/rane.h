/* the deck's sections, O RANE A to F and O ANA A and B. some break away and
 * sink */
#ifndef STG06_RANE_H
#define STG06_RANE_H

#include "system/task.h"

/* the steps O RANE A to F and O ANA A and B name */
void rane_a_step(struct task *task);
void rane_b_step(struct task *task);
void rane_c_step(struct task *task);
void rane_d_step(struct task *task);
void rane_e_step(struct task *task);
void rane_f_step(struct task *task);
void ana_a_step(struct task *task);
void ana_b_step(struct task *task);

#endif
