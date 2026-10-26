/* the kinds of object the race places */
#include "al_race/object_list.h"
/* only the full build's list names race_object_kinds */
#include "al_race/kinds.h" /* NOLINT(misc-include-cleaner) */
#include "object/placement.h"

#pragma section HEAD
struct object_list race_object_list = {
#include "al_race/object_list/race_object_list.inc"
};
#pragma section
