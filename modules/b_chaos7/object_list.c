/* the kinds of object the stage places */
#include "b_chaos7/object_list.h"
#include "b_chaos7/kinds.h" /* NOLINT(misc-include-cleaner) */
#include "object/placement.h"

#pragma section HEAD
struct object_list chaos7_object_list = {
#include "b_chaos7/object_list/chaos7_object_list.inc"
};
#pragma section
