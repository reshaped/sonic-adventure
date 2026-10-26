/* the stage's object list, its one kind of object and the kind's name */
#include "b_e101_r/object_list.h"
#include "object/placement.h"
#include "object/ring.h" /* NOLINT(misc-include-cleaner) */

/* the kind's name, which e101_r_object_kinds points at. the kinds'
 * initializer is generated from the disc, and names each pointer's target
 * by its symbol. so the name is an array, not a string literal. SHC aligns
 * the array to 1 and a literal to 4. as the object's only constant, the
 * array starts where a literal would */
static const char e101_r_ring_name[8] = {
#include "b_e101_r/object_list/e101_r_ring_name.inc"
};
static struct object_kind e101_r_object_kinds[1] = {
#include "b_e101_r/object_list/e101_r_object_kinds.inc"
};

#pragma section HEAD
struct object_list e101_r_object_list = {
#include "b_e101_r/object_list/e101_r_object_list.inc"
};
#pragma section
