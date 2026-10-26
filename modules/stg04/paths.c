/* the stage's paths, and the lists of them for acts 0 and 2 */
#include "stg04/paths.h"
#include "path/path.h"
#include "path/steps.h" /* NOLINT(misc-include-cleaner) */
#include "stg04/heli.h" /* NOLINT(misc-include-cleaner) */
#include "stg04/path_camera.h" /* NOLINT(misc-include-cleaner) */
#include "stg04/traffic.h" /* NOLINT(misc-include-cleaner) */

/* each path's points, then the path */
static struct path_point dat_0ca73af8[41] = {
#include "stg04/paths/dat_0ca73af8.inc"
};
static struct path dat_0ca73e2c = {
#include "stg04/paths/dat_0ca73e2c.inc"
};
static struct path_point dat_0ca73e3c[70] = {
#include "stg04/paths/dat_0ca73e3c.inc"
};
static struct path dat_0ca743b4 = {
#include "stg04/paths/dat_0ca743b4.inc"
};
static struct path_point dat_0ca743c4[15] = {
#include "stg04/paths/dat_0ca743c4.inc"
};
static struct path dat_0ca744f0 = {
#include "stg04/paths/dat_0ca744f0.inc"
};
static struct path_point dat_0ca74500[181] = {
#include "stg04/paths/dat_0ca74500.inc"
};
struct path heli_route = {
#include "stg04/paths/heli_route.inc"
};
static struct path_point dat_0ca75334[37] = {
#include "stg04/paths/dat_0ca75334.inc"
};
static struct path dat_0ca75618 = {
#include "stg04/paths/dat_0ca75618.inc"
};
static struct path_point dat_0ca75628[89] = {
#include "stg04/paths/dat_0ca75628.inc"
};
static struct path dat_0ca75d1c = {
#include "stg04/paths/dat_0ca75d1c.inc"
};
static struct path_point dat_0ca75d2c[91] = {
#include "stg04/paths/dat_0ca75d2c.inc"
};
static struct path dat_0ca76448 = {
#include "stg04/paths/dat_0ca76448.inc"
};
static struct path_point dat_0ca76458[37] = {
#include "stg04/paths/dat_0ca76458.inc"
};
static struct path dat_0ca7673c = {
#include "stg04/paths/dat_0ca7673c.inc"
};
static struct path_point dat_0ca7674c[163] = {
#include "stg04/paths/dat_0ca7674c.inc"
};
static struct path dat_0ca77408 = {
#include "stg04/paths/dat_0ca77408.inc"
};
static struct path_point dat_0ca77418[18] = {
#include "stg04/paths/dat_0ca77418.inc"
};
struct path highway_act2_route = {
#include "stg04/paths/highway_act2_route.inc"
};
static struct path_point dat_0ca77590[200] = {
#include "stg04/paths/dat_0ca77590.inc"
};
struct path traffic_route = {
#include "stg04/paths/traffic_route.inc"
};
/* path_create_tasks stops at the null */
struct path *highway_act0_paths[10] = {
    &dat_0ca73e2c, &dat_0ca743b4, &dat_0ca744f0, &heli_route,   &dat_0ca75618,
    &dat_0ca75d1c, &dat_0ca76448, &dat_0ca7673c, &dat_0ca77408, NULL,
};
struct path *highway_act2_paths[3] = {
    &highway_act2_route,
    &traffic_route,
    NULL,
};
