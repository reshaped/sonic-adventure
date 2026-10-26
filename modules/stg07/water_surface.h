/* the water's surface: O Suimen */
#ifndef STG07_WATER_SURFACE_H
#define STG07_WATER_SURFACE_H
#include <sg_xpt.h>

#include "system/task.h"

/* the height the water rises or sinks to, as its switches set it */
extern Float ruin_water_level;

/* the object list's step */
void water_surface_step(struct task *task);

#endif
