/* the stage's object list */
#ifndef B_ROBO_OBJECT_LIST_H
#define B_ROBO_OBJECT_LIST_H

#include "object/placement.h"

/* 1ST_READ.BIN's object_lists points at this list for act 0 alone. it
 * reaches the list by its address, 0x0c900110, not by its name */
extern struct object_list eggmanrobo_object_list;

#endif
