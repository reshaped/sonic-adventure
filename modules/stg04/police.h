/* E POLICE, a patrol car that hovers over the road. it chases player 0
 * near its home with its siren lit, then drifts back */
#ifndef STG04_POLICE_H
#define STG04_POLICE_H

#include <ninja.h>
#include <sg_xpt.h>

#include "system/task.h"

/* highway_act2_route cut into stretches, by distance along it. each row
 * holds a stretch's two ends, the farther first. the rows run back to the
 * route's start */
#define POLICE_STRETCHES 7
extern Float police_stretches[POLICE_STRETCHES][2];

/* the texlist of the car, its driver and the siren. 1ST_READ.BIN's
 * highway_pvms names it, for NISEPAT's textures */
extern NJS_TEXLIST police_texlist;

/* the object list's init, which installs the step, draw and destructor */
void police_init(struct task *task);

#endif
