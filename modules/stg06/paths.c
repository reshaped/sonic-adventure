/* the stage's path, and the list of it for act 0 */
#include "stg06/paths.h"
#include "path/path.h"
#include "stg06/path.h" /* NOLINT(misc-include-cleaner) */

static struct path_point dat_0cadb8e0[87] = {
#include "stg06/paths/dat_0cadb8e0.inc"
};
static struct path dat_0cadbfac = {
#include "stg06/paths/dat_0cadbfac.inc"
};
/* path_create_tasks stops at the null */
struct path *sky_deck_act0_paths[2] = {
    &dat_0cadbfac,
    NULL,
};
