/* the kinds of object the stage places */
#ifndef STG08_OBJECT_LIST_H
#define STG08_OBJECT_LIST_H

#include "object/placement.h"

/* 1ST_READ.BIN's object_lists points at this list for each of the stage's
 * four acts. it reaches the list by its address, not by its name */
extern struct object_list icecap_object_list;

#endif
