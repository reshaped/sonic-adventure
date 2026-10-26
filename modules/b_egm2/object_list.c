/* the kinds of object the stage places, their names, and the list that
 * counts them */
#include "b_egm2/object_list.h"
#include "b_egm2/ornament.h" /* NOLINT(misc-include-cleaner) */
#include "object/placement.h"
#include "object/ring.h" /* NOLINT(misc-include-cleaner) */

/* the kinds' names, as char arrays. egm2_object_kinds' initializer is a
 * body, data the build takes from the disc. a stub build, with no bodies,
 * would drop a string literal only a body points at. SHC aligns an array to
 * 1 and a literal to 4. egm2_ring_name's 8 bytes put egm2_ornament_name on
 * the 4-byte boundary its literal would take */
static const char egm2_ring_name[8] = {
#include "b_egm2/object_list/egm2_ring_name.inc"
};
static const char egm2_ornament_name[9] = {
#include "b_egm2/object_list/egm2_ornament_name.inc"
};
static struct object_kind egm2_object_kinds[2] = {
#include "b_egm2/object_list/egm2_object_kinds.inc"
};

#pragma section HEAD
struct object_list egm2_object_list = {
#include "b_egm2/object_list/egm2_object_list.inc"
};
#pragma section
