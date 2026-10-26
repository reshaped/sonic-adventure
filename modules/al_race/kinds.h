/* the kinds of object the race places */
#ifndef AL_RACE_KINDS_H
#define AL_RACE_KINDS_H

#include "object/placement.h"

#define RACE_KINDS 19

/* the kinds the object list holds. a set entry's id & SET_KIND picks one */
extern struct object_kind race_object_kinds[RACE_KINDS];

#endif
