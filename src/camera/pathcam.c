/* pathcam_routes, and the paths and keys it leads to */
#include "camera/pathcam.h"

static struct pathcam_key pathcam_path0_keys[251] = {
#include "camera/pathcam/pathcam_path0_keys.inc"
};
static struct pathcam_path pathcam_path0 = {
#include "camera/pathcam/pathcam_path0.inc"
};
static struct pathcam_key pathcam_path1_keys[250] = {
#include "camera/pathcam/pathcam_path1_keys.inc"
};
static struct pathcam_path pathcam_path1 = {
#include "camera/pathcam/pathcam_path1.inc"
};
static struct pathcam_path *pathcam_route0_paths[1] = {
#include "camera/pathcam/pathcam_route0_paths.inc"
};
static struct pathcam_path *pathcam_route1_paths[1] = {
#include "camera/pathcam/pathcam_route1_paths.inc"
};
struct pathcam_route pathcam_routes[3] = {
#include "camera/pathcam/pathcam_routes.inc"
};
