/* the kinds of object the stage places */
#ifndef STG02_KINDS_H
#define STG02_KINDS_H

#include "object/placement.h"

#define WINDY_KINDS 80

/* the kinds the stage's object list holds. a set entry's id & SET_KIND picks
 * one */
extern struct object_kind windy_object_kinds[WINDY_KINDS];

#endif
