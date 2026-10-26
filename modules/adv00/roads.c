/* the roads the traffic drives, each a path and its points, and act 0's list
 * of paths, which 1ST_READ.BIN's path_lists names */
#include "adv00/roads.h"
#include "path/path.h"

static struct path_point office_road_points[92] = {
#include "adv00/roads/office_road_points.inc"
};
struct path office_road = {
#include "adv00/roads/office_road.inc"
};
static struct path_point station_road_points[170] = {
#include "adv00/roads/station_road_points.inc"
};
struct path station_road = {
#include "adv00/roads/station_road.inc"
};
struct path *station_square_act0_paths[3] = {&office_road, &station_road, NULL};
