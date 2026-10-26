/* the avalanche's paths, act 1's one path, and the path lists of acts 1 and
 * 2 */
#include "stg08/avalanche_paths.h"
#include "path/path.h"
#include "path/steps.h" /* NOLINT(misc-include-cleaner) */
#include "stg08/avalanche.h" /* NOLINT(misc-include-cleaner) */

static struct path_point dat_0cab1ca0[27] = {
#include "stg08/avalanche_paths/dat_0cab1ca0.inc"
};
/* act 1's one path, which path_join_step puts a player on as they come
 * near it */
static struct path dat_0cab1ebc = {
#include "stg08/avalanche_paths/dat_0cab1ebc.inc"
};
static struct path_point dat_0cab1ecc[241] = {
#include "stg08/avalanche_paths/dat_0cab1ecc.inc"
};
struct path avalanche_path_1 = {
#include "stg08/avalanche_paths/avalanche_path_1.inc"
};
static struct path_point dat_0cab31b0[241] = {
#include "stg08/avalanche_paths/dat_0cab31b0.inc"
};
struct path avalanche_path_2 = {
#include "stg08/avalanche_paths/avalanche_path_2.inc"
};
static struct path_point dat_0cab4494[241] = {
#include "stg08/avalanche_paths/dat_0cab4494.inc"
};
struct path avalanche_path_3 = {
#include "stg08/avalanche_paths/avalanche_path_3.inc"
};
static struct path_point dat_0cab5778[241] = {
#include "stg08/avalanche_paths/dat_0cab5778.inc"
};
struct path avalanche_path_4 = {
#include "stg08/avalanche_paths/avalanche_path_4.inc"
};
static struct path_point dat_0cab6a5c[241] = {
#include "stg08/avalanche_paths/dat_0cab6a5c.inc"
};
struct path avalanche_path_5 = {
#include "stg08/avalanche_paths/avalanche_path_5.inc"
};
static struct path_point dat_0cab7d40[241] = {
#include "stg08/avalanche_paths/dat_0cab7d40.inc"
};
struct path avalanche_path_6 = {
#include "stg08/avalanche_paths/avalanche_path_6.inc"
};
static struct path_point dat_0cab9024[241] = {
#include "stg08/avalanche_paths/dat_0cab9024.inc"
};
struct path avalanche_path_7 = {
#include "stg08/avalanche_paths/avalanche_path_7.inc"
};
static struct path_point dat_0caba308[241] = {
#include "stg08/avalanche_paths/dat_0caba308.inc"
};
struct path avalanche_path_8 = {
#include "stg08/avalanche_paths/avalanche_path_8.inc"
};
static struct path_point dat_0cabb5ec[241] = {
#include "stg08/avalanche_paths/dat_0cabb5ec.inc"
};
struct path avalanche_path_9 = {
#include "stg08/avalanche_paths/avalanche_path_9.inc"
};
static struct path_point dat_0cabc8d0[241] = {
#include "stg08/avalanche_paths/dat_0cabc8d0.inc"
};
struct path avalanche_path_10 = {
#include "stg08/avalanche_paths/avalanche_path_10.inc"
};
/* each list ends with a null, where path_create_tasks stops */
struct path *icecap_act1_paths[2] = {
    &dat_0cab1ebc,
    NULL,
};
struct path *icecap_act2_paths[11] = {
    &avalanche_path_1,
    &avalanche_path_2,
    &avalanche_path_3,
    &avalanche_path_4,
    &avalanche_path_5,
    &avalanche_path_6,
    &avalanche_path_7,
    &avalanche_path_8,
    &avalanche_path_9,
    &avalanche_path_10,
    NULL,
};
