/* the stage's paths, and the lists of them */
#ifndef STG09_PATHS_H
#define STG09_PATHS_H
#include "path/path.h"

/* the paths objects follow by name: JdaiL's and JdaiR's ramps and rails,
 * Path's rides and the eggs' routes */
extern struct path jdai_right_ramp_path;
extern struct path jdai_right_rail_path;
extern struct path jdai_left_ramp_path;
extern struct path jdai_left_rail_path;
extern struct path ride_path_2;
extern struct path ride_path_1;
extern struct path ride_path_3;
extern struct path ride_path_0;
extern struct path route_egg_lower_path;
extern struct path route_egg_upper_path;
/* the list 1ST_READ.BIN's path_lists names for the stage's act 1 */
extern struct path *casino_act1_paths[];

#endif
