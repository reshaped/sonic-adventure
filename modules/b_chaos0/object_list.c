/* the stage's object list, which counts kinds.c's kinds and points at them */
#include "b_chaos0/object_list.h"
#include "b_chaos0/kinds.h" /* NOLINT(misc-include-cleaner) */
#include "object/placement.h"

#pragma section HEAD
struct object_list chaos0_object_list = {
#include "b_chaos0/object_list/chaos0_object_list.inc"
};
#pragma section
