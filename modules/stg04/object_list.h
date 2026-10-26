/* the stage's object list */
#ifndef STG04_OBJECT_LIST_H
#define STG04_OBJECT_LIST_H

#include "object/placement.h"

/* 1ST_READ.BIN's object_lists points at this list for acts 0 to 2. it reaches
 * the list by its address, 0x0ca790b0, not by its name */
extern struct object_list highway_object_list;

#endif
