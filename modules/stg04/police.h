/* E POLICE, a patrol car that hovers over the road. it chases player 0
 * near its home with its siren lit, then drifts back */
#ifndef STG04_POLICE_H
#define STG04_POLICE_H

#include <sg_xpt.h>

#include "system/task.h"

/* highway_act2_route cut into stretches, by distance along it. each row
 * holds a stretch's two ends, the farther first. the rows run back to the
 * route's start */
#define POLICE_STRETCHES 7
extern Float police_stretches[POLICE_STRETCHES][2];

/* the object list's init, which installs the step, draw and destructor */
void police_init(struct task *task);

#endif
