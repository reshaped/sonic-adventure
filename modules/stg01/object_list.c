/* the stage's object list: beach_object_kinds and their count */
#include "stg01/object_list.h"
#include "object/placement.h"
#include "stg01/kinds.h" /* NOLINT(misc-include-cleaner) */

struct object_list beach_object_list = {
#include "stg01/object_list/beach_object_list.inc"
};
