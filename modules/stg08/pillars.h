/* O_Hashra1B: a pillar whose top breaks into falling pieces once something
 * touches it. O_Hashra_C and O_Hashra10C: solid pillars */
#ifndef STG08_PILLARS_H
#define STG08_PILLARS_H

#include "system/task.h"

/* the object list's steps */
void icecap_pillar_1b_step(struct task *task);
void icecap_pillar_c_step(struct task *task);
void icecap_pillar_10c_step(struct task *task);

#endif
