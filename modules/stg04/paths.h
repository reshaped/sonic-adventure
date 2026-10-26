/* the stage's paths, and the lists of them for acts 0 and 2 */
#ifndef STG04_PATHS_H
#define STG04_PATHS_H
#include "path/path.h"

/* the route the helicopter carries player 0 along. heli.c samples it by
 * name */
extern struct path heli_route;
/* act 2's route. it names no step, so path_create_tasks makes no task for
 * it. traffic.c and police.c find places along it by name */
extern struct path highway_act2_route;
/* the loop the cars drive. it names traffic_step, the step of the path task
 * that makes the cars. traffic.c's ROUTE_LENGTH is the loop's length */
extern struct path traffic_route;
/* the lists 1ST_READ.BIN's path_lists names for the stage's acts 0 and 2 */
extern struct path *highway_act0_paths[];
extern struct path *highway_act2_paths[];

#endif
