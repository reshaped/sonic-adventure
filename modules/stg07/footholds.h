/* the footholds Ashiba01 to Ashiba03 and the foothold walls Ashikabe01 and
 * Ashikabe02 */
#ifndef STG07_FOOTHOLDS_H
#define STG07_FOOTHOLDS_H

#include "system/task.h"

/* the steps ruin_object_list names for Ashiba01 to Ashiba03, Ashikabe01 and
 * Ashikabe02, in that order */
void ruin_foothold_1_step(struct task *task);
void ruin_foothold_2_step(struct task *task);
void ruin_foothold_3_step(struct task *task);
void ruin_foothold_wall_1_step(struct task *task);
void ruin_foothold_wall_2_step(struct task *task);

#endif
