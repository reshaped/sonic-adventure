/* the kinds of object the stage places */
#ifndef B_E101_R_OBJECT_LIST_H
#define B_E101_R_OBJECT_LIST_H

#include "object/placement.h"

/* 1ST_READ.BIN's object_lists points at this list for the stage. it
 * reaches the list by its address, 0x0c9001b0, not by its name */
extern struct object_list e101_r_object_list;

#endif
