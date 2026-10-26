/* the kinds of object the stage places */
#ifndef STG01_KINDS_H
#define STG01_KINDS_H

#include "object/placement.h"

#define BEACH_KINDS 84

/* the kinds the stage's object list holds. a set entry's id & SET_KIND picks
 * one */
extern struct object_kind beach_object_kinds[BEACH_KINDS];

#endif
