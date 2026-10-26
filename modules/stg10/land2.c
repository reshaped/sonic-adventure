/* act 2's land table, which 1ST_READ.BIN's world_lands names */
#include "stg10/land2.h"
#include "collision/world.h"
#include "stg10/finalegg3_pvm.h" /* NOLINT(misc-include-cleaner) */
#include "stg10/finalegg3_textures.h" /* NOLINT(misc-include-cleaner) */

extern struct land_entry final_act2_land_entries[];
extern struct land_motion final_act2_land_motions[];

struct land_table final_act2_land = {
#include "stg10/land2/final_act2_land.inc"
};
