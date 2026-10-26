/* the kinds of object the stage places */
#include "stg09/object_list.h"
#include "object/placement.h"
#include "stg09/kinds.h" /* NOLINT(misc-include-cleaner) */

struct object_list casino_object_list = {
#include "stg09/object_list/casino_object_list.inc"
};
