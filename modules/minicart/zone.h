/* GOAL ZONE, PREVIOUS ZONE and NEXT ZONE, boxes that tell the race when a
 * player is in one */
#ifndef MINICART_ZONE_H
#define MINICART_ZONE_H

#include "system/task.h"

/* task steps for GOAL ZONE, PREVIOUS ZONE and NEXT ZONE, named in the
 * object list */
void goal_zone_step(struct task *task);
void previous_zone_step(struct task *task);
void next_zone_step(struct task *task);

#endif
