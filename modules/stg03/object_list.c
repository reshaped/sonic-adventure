/* the kinds of object the stage places */
#include "stg03/object_list.h"
#include "object/placement.h"
#include "stg03/kinds.h" /* NOLINT(misc-include-cleaner) */

struct object_list twinkle_object_list = {
#include "stg03/object_list/twinkle_object_list.inc"
};
