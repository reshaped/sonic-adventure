/* the list of the kinds of object the garden places */
#include "al_garden01/object_list.h"
#include "al_garden01/kinds.h" /* NOLINT(misc-include-cleaner) */
#include "object/placement.h"

struct object_list egg_carrier_garden_object_list = {
#include "al_garden01/object_list/egg_carrier_garden_object_list.inc"
};
