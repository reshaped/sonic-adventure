/* O BALLOON1 to O BALLOON4: hot air balloons that rise and turn */
#ifndef STG08_HOT_AIR_BALLOON_H
#define STG08_HOT_AIR_BALLOON_H

#include "system/task.h"

/* the object list's steps */
void hot_air_balloon_1_step(struct task *task);
void hot_air_balloon_2_step(struct task *task);
void hot_air_balloon_3_step(struct task *task);
void hot_air_balloon_4_step(struct task *task);

#endif
