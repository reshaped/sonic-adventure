/* the stage's object list: eggmobile1_object_kinds and their count */
#include "b_egm1/object_list.h"
/* only the full build's list points to eggmobile1_object_kinds */
#include "b_egm1/kinds.h" /* NOLINT(misc-include-cleaner) */
#include "object/placement.h"

#pragma section HEAD
struct object_list eggmobile1_object_list = {
#include "b_egm1/object_list/eggmobile1_object_list.inc"
};
#pragma section
