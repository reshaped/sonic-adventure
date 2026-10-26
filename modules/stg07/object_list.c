/* the kinds of object the stage places */
#include "stg07/object_list.h"
#include "object/placement.h"
#include "stg07/kinds.h" /* NOLINT(misc-include-cleaner) */

struct object_list ruin_object_list = {
#include "stg07/object_list/ruin_object_list.inc"
};
