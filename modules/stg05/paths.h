/* the stage's paths, and the lists of them */
#ifndef STG05_PATHS_H
#define STG05_PATHS_H
#include "path/path.h"

/* the lists 1ST_READ.BIN's path_lists names for the stage's acts 0 and 1.
 * each ends with a null, where path_create_tasks stops */
extern struct path *mountain_act0_paths[];
extern struct path *mountain_act1_paths[];

#endif
