/* the Mystic Ruins' object list: the count of the kinds its acts place, and the
 * kinds */
#include "adv02/object_list.h"
#include "adv02/kinds.h" /* NOLINT(misc-include-cleaner) */
#include "object/placement.h"

/* in DHEAD, at the address 1ST_READ.BIN's object_lists holds */
#pragma section HEAD
struct object_list mystic_ruins_object_list = {
#include "adv02/object_list/mystic_ruins_object_list.inc"
};
#pragma section
