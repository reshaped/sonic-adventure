/* the kinds of object the stage places */
#include "minicart/object_list.h"
#include "minicart/cart.h" /* NOLINT(misc-include-cleaner) */
#include "object/placement.h"

#pragma section HEAD
struct object_list cart_object_list = {
#include "minicart/object_list/cart_object_list.inc"
};
#pragma section
