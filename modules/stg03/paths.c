/* act 1's paths, and their list */
#include "stg03/paths.h"
#include "path/path.h"

/* each path's points, then the path. no path here has a step, so
 * path_create_tasks makes no task for any */
static struct path_point dat_0c9c0f98[189] = {
#include "stg03/paths/dat_0c9c0f98.inc"
};
struct path twinkle_coaster_path = {
#include "stg03/paths/twinkle_coaster_path.inc"
};
static struct path_point dat_0c9c1e6c[67] = {
#include "stg03/paths/dat_0c9c1e6c.inc"
};
static struct path dat_0c9c23a8 = {
#include "stg03/paths/dat_0c9c23a8.inc"
};
static struct path_point dat_0c9c23b8[67] = {
#include "stg03/paths/dat_0c9c23b8.inc"
};
static struct path dat_0c9c28f4 = {
#include "stg03/paths/dat_0c9c28f4.inc"
};
/* the null ends the list for path_create_tasks */
struct path *twinkle_act1_paths[4] = {
    &twinkle_coaster_path,
    &dat_0c9c23a8,
    &dat_0c9c28f4,
    NULL,
};
