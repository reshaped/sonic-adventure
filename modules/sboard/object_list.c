/* the kinds of object the stage places */
#include "sboard/object_list.h"
#include "object/placement.h"
#include "sboard/sunaboard.h" /* NOLINT(misc-include-cleaner) */

#pragma section HEAD
struct object_list sunaboard_object_list = {
#include "sboard/object_list/sunaboard_object_list.inc"
};
#pragma section
