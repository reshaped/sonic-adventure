/* the kinds of object the stage places */
#ifndef STG03_KINDS_H
#define STG03_KINDS_H

#include "object/placement.h"

#define TWINKLE_KINDS 92

/* the kinds the stage's object list holds. a set entry's id & SET_KIND
 * picks one */
extern struct object_kind twinkle_object_kinds[TWINKLE_KINDS];

#endif
