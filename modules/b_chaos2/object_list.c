/* the kinds of object the stage places, their names, and the list that
 * counts them */
#include "b_chaos2/object_list.h"
#include "b_chaos2/chandelier.h" /* NOLINT(misc-include-cleaner) */
#include "b_chaos2/table.h" /* NOLINT(misc-include-cleaner) */
#include "object/placement.h"
#include "object/ring.h" /* NOLINT(misc-include-cleaner) */

/* the set editor's names for the kinds. each is a char array with a body
 * of its own: chaos2_object_kinds' body, the initializer generated from
 * the disc, can point only at a symbol. SHC aligns a string literal to 4
 * and an array to 1. each name is 8 bytes, so each array starts where its
 * literal would */
static const char chaos2_ring_name[8] = {
#include "b_chaos2/object_list/chaos2_ring_name.inc"
};
static const char chaos2_table_name[8] = {
#include "b_chaos2/object_list/chaos2_table_name.inc"
};
static const char chaos2_chandelier_name[8] = {
#include "b_chaos2/object_list/chaos2_chandelier_name.inc"
};
static struct object_kind chaos2_object_kinds[3] = {
#include "b_chaos2/object_list/chaos2_object_kinds.inc"
};

#pragma section HEAD
struct object_list chaos2_object_list = {
#include "b_chaos2/object_list/chaos2_object_list.inc"
};
#pragma section
