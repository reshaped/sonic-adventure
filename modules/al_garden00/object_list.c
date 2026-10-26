/* the list of the kinds of object the garden places */
#include "al_garden00/object_list.h"
#include "al_garden00/kinds.h" /* NOLINT(misc-include-cleaner) */
#include "object/placement.h"

struct object_list station_square_garden_object_list = {
#include "al_garden00/object_list/station_square_garden_object_list.inc"
};
