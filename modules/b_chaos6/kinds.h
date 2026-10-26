/* the kinds of object the stage places */
#ifndef B_CHAOS6_KINDS_H
#define B_CHAOS6_KINDS_H

#include "object/placement.h"

#define CHAOS6_KINDS 7

/* the kinds chaos6_object_list holds. a set entry's id & SET_KIND picks
 * one */
extern struct object_kind chaos6_object_kinds[CHAOS6_KINDS];

#endif
