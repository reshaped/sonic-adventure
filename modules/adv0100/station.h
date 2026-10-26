/* O STATION, a monorail station, whose button calls the Egg Carrier's
 * monorail. modules/adv0130/station.c builds station.c again for the inside,
 * with that field's button textures */
#ifndef ADV0100_STATION_H
#define ADV0100_STATION_H

#include "system/task.h"

/* the event flag the station sets to call the monorail. the monorail clears
 * it once it has left empty, and each field task clears it as an act with a
 * station starts */
#define FLAG_MONORAIL_CALLED 39

/* the step egg_carrier_object_kinds and egg_carrier_inside_object_kinds name
 * for O STATION */
void egg_carrier_station_step(struct task *task);

#endif
