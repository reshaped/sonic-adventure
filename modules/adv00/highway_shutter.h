/* O HWBSHUTTER shutter across the way to the highway, which rolls up for Sonic
 * and breaks open for Tails */
#ifndef ADV00_HIGHWAY_SHUTTER_H
#define ADV00_HIGHWAY_SHUTTER_H

#include "system/task.h"

/* the step the object list names for O HWBSHUTTER. the shutter adds a wall to
 * the world to be solid. Sonic's rolls up once FLAG_SONIC_SS_ENTRANCE_HIGHWAY
 * is set, and is gone with its wall once it reaches a height of 50. Tails'
 * shutter shows broken open once FLAG_TAILS_SS_ENTRANCE_HIGHWAY is set, its
 * wall broken to match. for any other character the shutter stays shut */
void highway_shutter_init(struct task *task);

#endif
