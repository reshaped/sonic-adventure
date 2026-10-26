/* the kinds of object the Egg Carrier inside places */
#include "adv0130/object_list.h"
#include "adv0130/kinds.h" /* NOLINT(misc-include-cleaner) */
#include "object/placement.h"

#pragma section HEAD
struct object_list egg_carrier_inside_object_list = {
#include "adv0130/object_list/egg_carrier_inside_object_list.inc"
};
#pragma section
