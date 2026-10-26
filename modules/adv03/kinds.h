/* the kinds of object the past places, each with its step and the set
 * editor's name for it, such as O FIRE */
#ifndef ADV03_KINDS_H
#define ADV03_KINDS_H

#include "object/placement.h"

#define PAST_KINDS 40

/* the kinds past_object_list holds. the SET_KIND bits of a set entry's id
 * give its kind's index here */
extern struct object_kind past_object_kinds[PAST_KINDS];

#endif
