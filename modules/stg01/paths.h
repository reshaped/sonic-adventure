/* the lists of the stage's paths for acts 0 and 1 */
#ifndef STG01_PATHS_H
#define STG01_PATHS_H

#include "path/path.h"

/* the lists 1ST_READ.BIN's path_lists names for the stage's acts 0 and 1.
 * each ends with a null, where path_create_tasks stops */
extern struct path *beach_act0_paths[];
extern struct path *beach_act1_paths[];

#endif
