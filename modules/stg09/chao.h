/* PianTama: a Chao egg on the tables, whose Chao flies out once a touch
 * springs its top off. PianPach, PianWalk, PianFish and PianSlee: Chao on
 * the tables, each looping an action */
#ifndef STG09_CHAO_H
#define STG09_CHAO_H

#include "system/task.h"

/* the object list's steps. task needs TASK_WORK's record */
void casino_chao_egg_step(struct task *task);
void casino_chao_clap_step(struct task *task);
void casino_chao_walk_step(struct task *task);
void casino_chao_fish_step(struct task *task);
void casino_chao_sleep_step(struct task *task);

#endif
