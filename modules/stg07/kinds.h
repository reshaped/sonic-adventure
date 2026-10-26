/* the kinds of object the stage places */
#ifndef STG07_KINDS_H
#define STG07_KINDS_H

#include "object/placement.h"

#define RUIN_KINDS 86

/* the kinds the stage's object list holds. a set entry's id & SET_KIND picks
 * one */
extern struct object_kind ruin_object_kinds[RUIN_KINDS];

#endif
