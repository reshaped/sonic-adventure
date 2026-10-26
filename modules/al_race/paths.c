/* the race's three paths, each a path and its points. race_paths lists the
 * two beside the course, then the course */
#include "al_race/course.h"
#include "path/path.h"

static struct path_point dat_0cbd3d08[41] = {
#include "al_race/paths/dat_0cbd3d08.inc"
};
static struct path dat_0cbd403c = {
#include "al_race/paths/dat_0cbd403c.inc"
};
static struct path_point dat_0cbd404c[46] = {
#include "al_race/paths/dat_0cbd404c.inc"
};
static struct path dat_0cbd43e4 = {
#include "al_race/paths/dat_0cbd43e4.inc"
};
static struct path_point dat_0cbd43f4[78] = {
#include "al_race/paths/dat_0cbd43f4.inc"
};
static struct path dat_0cbd4a0c = {
#include "al_race/paths/dat_0cbd4a0c.inc"
};
struct path *race_paths[3] = {&dat_0cbd403c, &dat_0cbd43e4, &dat_0cbd4a0c};
