/* the kinds of object the stage places */
#ifndef B_ROBO_KINDS_H
#define B_ROBO_KINDS_H

#include "object/placement.h"

#define EGGMANROBO_KINDS 21

/* the kinds the stage's object list holds. a set entry's id & SET_KIND picks
 * one */
extern struct object_kind eggmanrobo_object_kinds[EGGMANROBO_KINDS];

#endif
