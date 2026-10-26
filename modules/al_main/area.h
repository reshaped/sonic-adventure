/* the gardens' areas: GD GRDUND, GD_WATER, GD ROAD, GD FLY and GD CLIFF.
 * alife_go_to_place can send a Chao to each */
#ifndef AL_MAIN_AREA_H
#define AL_MAIN_AREA_H

#include "system/task.h"

/* the steps each garden's object list names for the five kinds, in that
 * order. each gives its task an entry in ALIFE_PLACES, of kind
 * ALIFE_GROUND to ALIFE_CLIFF, with a radius 10 past its work's scl.x. the
 * task does nothing more until its destructor takes the entry out */
void garden_ground_init(struct task *task);
void garden_water_init(struct task *task);
void garden_road_init(struct task *task);
void garden_fly_init(struct task *task);
void garden_cliff_init(struct task *task);

#endif
