/* the kinds of object the Hedgehog Hammer places, their names, and the
 * list that counts them */
#include "stg00/object_list.h"
/* only the full build's kinds name invisible_box_step, ring_step and
 * mole_step. the full build compiles their initializer as extracted from the
 * image, where the stub build compiles zeros */
#include "collision/invisible.h" /* NOLINT(misc-include-cleaner) */
#include "object/placement.h"
#include "object/ring.h" /* NOLINT(misc-include-cleaner) */
#include "stg00/mole.h" /* NOLINT(misc-include-cleaner) */

/* the kinds' names. the kinds' initializer comes from the disc, and can
 * point only at a symbol. each name is thus a named array. the first two
 * names are 8 and 16 bytes long. so each name starts on a multiple of 4,
 * where SHC would place a string literal */
static const char practice_ring_name[8] = {
#include "stg00/object_list/practice_ring_name.inc"
};
static const char practice_mole_name[16] = {
#include "stg00/object_list/practice_mole_name.inc"
};
static const char practice_box_name[7] = {
#include "stg00/object_list/practice_box_name.inc"
};
static struct object_kind practice_object_kinds[3] = {
#include "stg00/object_list/practice_object_kinds.inc"
};

#pragma section HEAD
struct object_list practice_object_list = {
#include "stg00/object_list/practice_object_list.inc"
};
#pragma section
