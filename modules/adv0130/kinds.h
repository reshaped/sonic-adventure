/* the kinds of object the Egg Carrier's inside stage places */
#ifndef ADV0130_KINDS_H
#define ADV0130_KINDS_H

#include "object/placement.h"

#define EGG_CARRIER_INSIDE_KINDS 40

/* the kinds the stage's object list holds. a set entry's id & SET_KIND picks
 * one */
extern struct object_kind
    egg_carrier_inside_object_kinds[EGG_CARRIER_INSIDE_KINDS];

#endif
