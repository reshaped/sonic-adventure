/* the stage's object list, which counts kinds.c's kinds and points at them */
#include "stg04/object_list.h"
#include "object/placement.h"
#include "stg04/kinds.h" /* NOLINT(misc-include-cleaner) */

struct object_list highway_object_list = {
#include "stg04/object_list/highway_object_list.inc"
};
