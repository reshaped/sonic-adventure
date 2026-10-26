/* the stage's paths, and the lists of them */

#include "stg09/paths.h"
#include "path/path.h"
#include "stg09/path_camera.h" /* NOLINT(misc-include-cleaner) */

static struct path_point dat_0caec400[72] = {
#include "stg09/paths/dat_0caec400.inc"
};
static struct path dat_0caec9a0 = {
#include "stg09/paths/dat_0caec9a0.inc"
};
static struct path_point dat_0caec9b0[75] = {
#include "stg09/paths/dat_0caec9b0.inc"
};
static struct path dat_0caecf8c = {
#include "stg09/paths/dat_0caecf8c.inc"
};
static struct path_point dat_0caecf9c[25] = {
#include "stg09/paths/dat_0caecf9c.inc"
};
static struct path dat_0caed190 = {
#include "stg09/paths/dat_0caed190.inc"
};
static struct path_point dat_0caed1a0[37] = {
#include "stg09/paths/dat_0caed1a0.inc"
};
static struct path dat_0caed484 = {
#include "stg09/paths/dat_0caed484.inc"
};
static struct path_point dat_0caed494[3] = {
#include "stg09/paths/dat_0caed494.inc"
};
static struct path dat_0caed4d0 = {
#include "stg09/paths/dat_0caed4d0.inc"
};
static struct path_point dat_0caed4e0[23] = {
#include "stg09/paths/dat_0caed4e0.inc"
};
static struct path dat_0caed6ac = {
#include "stg09/paths/dat_0caed6ac.inc"
};
static struct path_point dat_0caed6bc[45] = {
#include "stg09/paths/dat_0caed6bc.inc"
};
static struct path dat_0caeda40 = {
#include "stg09/paths/dat_0caeda40.inc"
};
static struct path_point dat_0caeda50[78] = {
#include "stg09/paths/dat_0caeda50.inc"
};
static struct path dat_0caee068 = {
#include "stg09/paths/dat_0caee068.inc"
};
static struct path_point dat_0caee078[9] = {
#include "stg09/paths/dat_0caee078.inc"
};
struct path jdai_right_ramp_path = {
#include "stg09/paths/jdai_right_ramp_path.inc"
};
static struct path_point dat_0caee13c[114] = {
#include "stg09/paths/dat_0caee13c.inc"
};
struct path jdai_right_rail_path = {
#include "stg09/paths/jdai_right_rail_path.inc"
};
static struct path_point dat_0caeea34[9] = {
#include "stg09/paths/dat_0caeea34.inc"
};
struct path jdai_left_ramp_path = {
#include "stg09/paths/jdai_left_ramp_path.inc"
};
static struct path_point dat_0caeeaf8[109] = {
#include "stg09/paths/dat_0caeeaf8.inc"
};
struct path jdai_left_rail_path = {
#include "stg09/paths/jdai_left_rail_path.inc"
};
static struct path_point dat_0caef38c[201] = {
#include "stg09/paths/dat_0caef38c.inc"
};
struct path ride_path_2 = {
#include "stg09/paths/ride_path_2.inc"
};
static struct path_point dat_0caf0350[97] = {
#include "stg09/paths/dat_0caf0350.inc"
};
struct path ride_path_1 = {
#include "stg09/paths/ride_path_1.inc"
};
static struct path_point dat_0caf0af4[20] = {
#include "stg09/paths/dat_0caf0af4.inc"
};
struct path ride_path_3 = {
#include "stg09/paths/ride_path_3.inc"
};
static struct path_point dat_0caf0c94[95] = {
#include "stg09/paths/dat_0caf0c94.inc"
};
struct path ride_path_0 = {
#include "stg09/paths/ride_path_0.inc"
};
static struct path_point dat_0caf1410[32] = {
#include "stg09/paths/dat_0caf1410.inc"
};
struct path route_egg_lower_path = {
#include "stg09/paths/route_egg_lower_path.inc"
};
static struct path_point dat_0caf16a0[40] = {
#include "stg09/paths/dat_0caf16a0.inc"
};
struct path route_egg_upper_path = {
#include "stg09/paths/route_egg_upper_path.inc"
};
/* path_create_tasks stops at the null */
struct path *casino_act1_paths[9] = {
    &dat_0caec9a0, &dat_0caecf8c, &dat_0caed190, &dat_0caed484, &dat_0caed4d0,
    &dat_0caed6ac, &dat_0caeda40, &dat_0caee068, NULL,
};
/* lists of the paths objects follow by name, laid out as act 1's is. no word
 * in the image or in 1ST_READ.BIN points at either */
static struct path *casino_jdai_paths[5] = {
    &jdai_right_ramp_path,
    &jdai_right_rail_path,
    &jdai_left_ramp_path,
    &jdai_left_rail_path,
    NULL,
};
static struct path *casino_ride_paths[7] = {
    &ride_path_0,          &ride_path_1,          &ride_path_2, &ride_path_3,
    &route_egg_lower_path, &route_egg_upper_path, NULL,
};
