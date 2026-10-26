/* the stage's paths, and the lists of them */
#include "stg05/paths.h"
#include "path/path.h"
#include "path/steps.h" /* NOLINT(misc-include-cleaner) */
#include "stg05/pulley.h" /* NOLINT(misc-include-cleaner) */

/* each path's points are defined just before it. act 0's two paths step with
 * pulley_step. act 1's dat_0cad88e0 and dat_0cad9138 have no step, so they
 * make no task. the rest step with path_camera_n, which hands the camera a
 * stretch */
static struct path_point dat_0cad8028[8] = {
#include "stg05/paths/dat_0cad8028.inc"
};
static struct path dat_0cad80c8 = {
#include "stg05/paths/dat_0cad80c8.inc"
};
static struct path_point dat_0cad80d8[8] = {
#include "stg05/paths/dat_0cad80d8.inc"
};
static struct path dat_0cad8178 = {
#include "stg05/paths/dat_0cad8178.inc"
};
static struct path_point dat_0cad8188[94] = {
#include "stg05/paths/dat_0cad8188.inc"
};
static struct path dat_0cad88e0 = {
#include "stg05/paths/dat_0cad88e0.inc"
};
static struct path_point dat_0cad88f0[106] = {
#include "stg05/paths/dat_0cad88f0.inc"
};
static struct path dat_0cad9138 = {
#include "stg05/paths/dat_0cad9138.inc"
};
static struct path_point dat_0cad9148[28] = {
#include "stg05/paths/dat_0cad9148.inc"
};
static struct path dat_0cad9378 = {
#include "stg05/paths/dat_0cad9378.inc"
};
static struct path_point dat_0cad9388[67] = {
#include "stg05/paths/dat_0cad9388.inc"
};
static struct path dat_0cad98c4 = {
#include "stg05/paths/dat_0cad98c4.inc"
};
static struct path_point dat_0cad98d4[78] = {
#include "stg05/paths/dat_0cad98d4.inc"
};
static struct path dat_0cad9eec = {
#include "stg05/paths/dat_0cad9eec.inc"
};
static struct path_point dat_0cad9efc[40] = {
#include "stg05/paths/dat_0cad9efc.inc"
};
static struct path dat_0cada21c = {
#include "stg05/paths/dat_0cada21c.inc"
};
static struct path_point dat_0cada22c[43] = {
#include "stg05/paths/dat_0cada22c.inc"
};
static struct path dat_0cada588 = {
#include "stg05/paths/dat_0cada588.inc"
};
static struct path_point dat_0cada598[69] = {
#include "stg05/paths/dat_0cada598.inc"
};
static struct path dat_0cadaafc = {
#include "stg05/paths/dat_0cadaafc.inc"
};
static struct path_point dat_0cadab0c[70] = {
#include "stg05/paths/dat_0cadab0c.inc"
};
static struct path dat_0cadb084 = {
#include "stg05/paths/dat_0cadb084.inc"
};
struct path *mountain_act0_paths[3] = {
    &dat_0cad80c8,
    &dat_0cad8178,
    NULL,
};
struct path *mountain_act1_paths[10] = {
    &dat_0cad98c4, &dat_0cad9eec, &dat_0cada21c, &dat_0cada588, &dat_0cadaafc,
    &dat_0cadb084, &dat_0cad88e0, &dat_0cad9138, &dat_0cad9378, NULL,
};
