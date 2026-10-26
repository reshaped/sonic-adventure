/* act 0's dam, a wall that sinks once DAM_SWITCH turns on. the water it holds
 * back then spills into the water below */
#ifndef STG12_DAM_H
#define STG12_DAM_H

#include <sg_xpt.h>

#include "system/task.h"

/* the switch that sinks the dam */
#define DAM_SWITCH 0

/* where the dam stands, on x and z */
#define DAM_X 750.0F
#define DAM_Z (-650.0F)

/* in shelter_dam_flags: the spill pours into the water below, so the
 * sinking dam raises that water each step */
#define DAM_SPILLING 0x1

extern int shelter_dam_flags;
extern Float shelter_dam_level; /* the height of the water the dam holds back */

/* the first step of a task with a work, which shelter_water_start makes. it
 * sets the dam up and makes the surface of its water as its child */
void shelter_dam_start(struct task *task);

#endif
