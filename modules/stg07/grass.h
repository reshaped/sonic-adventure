/* the grass: KusaA, KusaB, KusaC, Shitakusa and GakeKusa */
#ifndef STG07_GRASS_H
#define STG07_GRASS_H

#include "system/task.h"

/* the object list's steps */
void grass_a_step(struct task *task);
void grass_b_step(struct task *task);
void grass_c_step(struct task *task);
void undergrowth_step(struct task *task);
void cliff_grass_step(struct task *task);

#endif
