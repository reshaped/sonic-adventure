/* the past's object list: past_object_kinds and their count */
#include "adv03/object_list.h"
#include "adv03/kinds.h" /* NOLINT(misc-include-cleaner) */
#include "object/placement.h"

#pragma section HEAD
struct object_list past_object_list = {
#include "adv03/object_list/past_object_list.inc"
};
#pragma section
