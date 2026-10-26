/* the roads the traffic drives, and act 0's list of paths */
#ifndef ADV00_ROADS_H
#define ADV00_ROADS_H

#include "path/path.h"

/* the two roads the traffic drives, both loops */
extern struct path office_road;
extern struct path station_road;
/* the list 1ST_READ.BIN's path_lists names for act 0, ended by a null.
 * neither road has a step, so path_create_tasks makes no task for them */
extern struct path *station_square_act0_paths[];

#endif
