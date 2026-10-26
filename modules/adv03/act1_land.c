/* the past's act 1 land */
#include "adv03/act1_land.h"
#include "adv03/act1_land_entries.h" /* NOLINT(misc-include-cleaner) */
#include "adv03/act1_textures.h" /* NOLINT(misc-include-cleaner) */
#include "collision/world.h"

#pragma section HEAD
struct land_table past_act1_land = {
#include "adv03/act1_land/past_act1_land.inc"
};
#pragma section
