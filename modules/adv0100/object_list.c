/* the object list of the Egg Carrier outside */
#include "adv0100/object_list.h"
#include "adv0100/kinds.h" /* NOLINT(misc-include-cleaner) */
#include "object/placement.h"

#pragma section HEAD
struct object_list egg_carrier_object_list = {
#include "adv0100/object_list/egg_carrier_object_list.inc"
};
#pragma section
