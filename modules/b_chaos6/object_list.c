/* the stage's object list: chaos6_object_kinds and their count */
#include "b_chaos6/object_list.h"
#include "b_chaos6/kinds.h" /* NOLINT(misc-include-cleaner) */
#include "object/placement.h"

#pragma section HEAD
struct object_list chaos6_object_list = {
#include "b_chaos6/object_list/chaos6_object_list.inc"
};
#pragma section
