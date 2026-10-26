/* the stage's land, and the name of the pvm its textures load from */
#include "b_chaos7/land.h"
#include "b_chaos7/land_textures.h" /* NOLINT(misc-include-cleaner) */
#include "collision/world.h"

extern struct land_entry chaos7_land_entries[];

#pragma section HEAD
/* the pvm's name as an array, since chaos7_land's generated initializer can
 * point only at a symbol. it is this file's only constant, so it lies where
 * the string literal would */
static const char chaos7_land_pvm[12] = {
#include "b_chaos7/land/chaos7_land_pvm.inc"
};
struct land_table chaos7_land = {
#include "b_chaos7/land/chaos7_land.inc"
};
#pragma section
