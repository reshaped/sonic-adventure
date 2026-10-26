/* ZONE, a sphere a racing Chao can touch, drawn only in edit mode. the
 * collision checks ignore its shape for players and attacks, objects, pickups
 * and rings */
#ifndef AL_RACE_ZONE_H
#define AL_RACE_ZONE_H

#include "system/task.h"

/* the zone's step in the object list, which makes the task a zone */
void zone_init(struct task *task);
/* the zone's destructor, which does nothing. zone_get_work tells a zone's
 * task by it */
void zone_destroy(struct task *task);

#endif
