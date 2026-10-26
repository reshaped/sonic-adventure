/* the kinds of object the stage places */
#ifndef B_CHAOS0_KINDS_H
#define B_CHAOS0_KINDS_H

#include "object/placement.h"

#define CHAOS0_KINDS 9

/* the kinds the stage's object list holds. a set entry's id & SET_KIND picks
 * one */
extern struct object_kind chaos0_object_kinds[CHAOS0_KINDS];

#endif
