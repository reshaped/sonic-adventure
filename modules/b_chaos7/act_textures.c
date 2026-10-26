/* the entry for Chaos 7's act 0 in 1ST_READ.BIN's act_list */
#include "b_chaos7/act_textures.h"
#include "b_chaos7/fight_pvms.h" /* NOLINT(misc-include-cleaner) */
#include "system/texture.h"

#pragma section HEAD
struct act_textures chaos7_act_textures = {
#include "b_chaos7/act_textures/chaos7_act_textures.inc"
};
#pragma section
