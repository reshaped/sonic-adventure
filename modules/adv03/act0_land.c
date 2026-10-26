/* the past's act 0 land */
#include "adv03/act0_land.h"
#include "adv03/act0_land_entries.h" /* NOLINT(misc-include-cleaner) */
#include "adv03/act0_textures.h" /* NOLINT(misc-include-cleaner) */
#include "collision/world.h"

#pragma section HEAD
struct land_table past_act0_land = {
#include "adv03/act0_land/past_act0_land.inc"
};
#pragma section
