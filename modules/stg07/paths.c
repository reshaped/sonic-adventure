/* the stage's paths, and the lists of them for acts 0 and 1 */
#include "stg07/paths.h"
#include "path/path.h"
#include "stg07/boulder.h" /* NOLINT(misc-include-cleaner) */
#include "stg07/path_camera.h" /* NOLINT(misc-include-cleaner) */
#include "stg07/roll_path.h" /* NOLINT(misc-include-cleaner) */
#include "stg07/slide.h" /* NOLINT(misc-include-cleaner) */
#include "stg07/snake.h" /* NOLINT(misc-include-cleaner) */

static struct path_point dat_0ca58d90[37] = {
#include "stg07/paths/dat_0ca58d90.inc"
};
/* no word in the image or in 1ST_READ.BIN points at this path, so no path
 * list holds it */
static struct path dat_0ca59074 = {
#include "stg07/paths/dat_0ca59074.inc"
};
static struct path_point dat_0ca59084[86] = {
#include "stg07/paths/dat_0ca59084.inc"
};
static struct path dat_0ca5973c = {
#include "stg07/paths/dat_0ca5973c.inc"
};
static struct path_point dat_0ca5974c[76] = {
#include "stg07/paths/dat_0ca5974c.inc"
};
static struct path dat_0ca59d3c = {
#include "stg07/paths/dat_0ca59d3c.inc"
};
static struct path_point dat_0ca59d4c[40] = {
#include "stg07/paths/dat_0ca59d4c.inc"
};
static struct path dat_0ca5a06c = {
#include "stg07/paths/dat_0ca5a06c.inc"
};
static struct path_point dat_0ca5a07c[63] = {
#include "stg07/paths/dat_0ca5a07c.inc"
};
struct path ruin_boulder_path = {
#include "stg07/paths/ruin_boulder_path.inc"
};
static struct path_point dat_0ca5a578[55] = {
#include "stg07/paths/dat_0ca5a578.inc"
};
static struct path dat_0ca5a9c4 = {
#include "stg07/paths/dat_0ca5a9c4.inc"
};
static struct path_point dat_0ca5a9d4[81] = {
#include "stg07/paths/dat_0ca5a9d4.inc"
};
struct path ruin_slide_path = {
#include "stg07/paths/ruin_slide_path.inc"
};
/* path_create_tasks stops at the null */
struct path *ruin_act0_paths[4] = {
    &dat_0ca5973c,
    &dat_0ca59d3c,
    &dat_0ca5a06c,
    NULL,
};
struct path *ruin_act1_paths[4] = {
    &ruin_boulder_path,
    &dat_0ca5a9c4,
    &ruin_slide_path,
    NULL,
};
