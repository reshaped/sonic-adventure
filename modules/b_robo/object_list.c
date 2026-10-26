/* the stage's object list, which counts kinds.c's kinds and points at them */
#include "b_robo/object_list.h"
#include "b_robo/kinds.h" /* NOLINT(misc-include-cleaner) */
#include "object/placement.h"

#pragma section HEAD
struct object_list eggmanrobo_object_list = {
#include "b_robo/object_list/eggmanrobo_object_list.inc"
};
#pragma section
