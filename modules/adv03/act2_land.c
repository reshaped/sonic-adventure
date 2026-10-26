/* the past's act 2 land */
#include "adv03/act2_land.h"
#include "adv03/act2_textures.h" /* NOLINT(misc-include-cleaner) */
#include "collision/world.h"

/* the land's entries, which a gen/ range holds, so no header declares
 * them */
extern struct land_entry past_act2_land_entries[];

#pragma section HEAD
struct land_table past_act2_land = {
#include "adv03/act2_land/past_act2_land.inc"
};
#pragma section
