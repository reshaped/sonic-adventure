/* the list of the kinds of object Chaos 4's stage places: one kind, the
 * ring, and the name the set editor gives it */
#include "b_chaos4/object_list.h"
#include "object/placement.h"
/* only the full build's kind points to ring_step */
#include "object/ring.h" /* NOLINT(misc-include-cleaner) */

/* the kinds' generated initializer can point only at a symbol, so the
 * ring's name is a named array, not a string literal. as the object's only
 * constant, it starts where the literal would */
static const char chaos4_ring_name[8] = {
#include "b_chaos4/object_list/chaos4_ring_name.inc"
};
static struct object_kind chaos4_object_kinds[1] = {
#include "b_chaos4/object_list/chaos4_object_kinds.inc"
};

#pragma section HEAD
struct object_list chaos4_object_list = {
#include "b_chaos4/object_list/chaos4_object_list.inc"
};
#pragma section
