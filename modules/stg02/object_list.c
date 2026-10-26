/* the stage's object list: windy_object_kinds and its count */
#include "stg02/object_list.h"
#include "object/placement.h"
#include "stg02/kinds.h" /* NOLINT(misc-include-cleaner) */

struct object_list windy_object_list = {
#include "stg02/object_list/windy_object_list.inc"
};
