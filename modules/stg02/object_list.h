/* the kinds of object the stage places */
#ifndef STG02_OBJECT_LIST_H
#define STG02_OBJECT_LIST_H

#include "object/placement.h"

/* 1ST_READ.BIN's object_lists points at this list for each of the stage's
 * three acts. it reaches the list by its address, 0x0c9ec3a0, not by its
 * name */
extern struct object_list windy_object_list;

#endif
