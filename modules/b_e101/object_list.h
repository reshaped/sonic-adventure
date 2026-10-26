/* the kinds of object the stage places */
#ifndef B_E101_OBJECT_LIST_H
#define B_E101_OBJECT_LIST_H

#include "object/placement.h"

/* 1ST_READ.BIN's object_lists points at this list for the stage. it reaches
 * the list by its address, 0x0c900098, not by its name */
extern struct object_list e101_object_list;

#endif
