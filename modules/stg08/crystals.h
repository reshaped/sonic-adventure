/* O SuiTaore: a crystal that topples once player 0 comes near. O SuisyouB,
 * O SuisyouBM and O SuisyouR: standing crystals that sparkle */
#ifndef STG08_CRYSTALS_H
#define STG08_CRYSTALS_H

#include "system/task.h"

/* the object list's steps */
void crystal_topple_step(struct task *task);
void crystal_b_step(struct task *task);
void crystal_bm_step(struct task *task);
void crystal_r_step(struct task *task);

#endif
