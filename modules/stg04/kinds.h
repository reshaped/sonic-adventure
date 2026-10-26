/* the kinds of object the stage places */
#ifndef STG04_KINDS_H
#define STG04_KINDS_H

#include "object/placement.h"

#define HIGHWAY_KINDS 118

/* the kinds the stage's object list holds. a set entry's id & SET_KIND picks
 * one */
extern struct object_kind highway_object_kinds[HIGHWAY_KINDS];

#endif
