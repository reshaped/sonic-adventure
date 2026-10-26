/* the avalanche's paths, act 1's one path, and the path lists of acts 1 and
 * 2 */
#ifndef STG08_AVALANCHE_PATHS_H
#define STG08_AVALANCHE_PATHS_H
#include "path/path.h"

/* the avalanche's paths. the first wave's pieces follow the odd ones, and
 * the second wave's the even ones. avalanche_path_1's task runs
 * avalanche_step, which starts the avalanche */
extern struct path avalanche_path_1;
extern struct path avalanche_path_2;
extern struct path avalanche_path_3;
extern struct path avalanche_path_4;
extern struct path avalanche_path_5;
extern struct path avalanche_path_6;
extern struct path avalanche_path_7;
extern struct path avalanche_path_8;
extern struct path avalanche_path_9;
extern struct path avalanche_path_10;
/* the path lists of Ice Cap's acts 1 and 2, which 1ST_READ.BIN's path_lists
 * points to */
extern struct path *icecap_act1_paths[];
extern struct path *icecap_act2_paths[];

#endif
