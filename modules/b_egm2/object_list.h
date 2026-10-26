/* the kinds of object the stage places */
#ifndef B_EGM2_OBJECT_LIST_H
#define B_EGM2_OBJECT_LIST_H

#include "object/placement.h"

/* the stage's object list. 1ST_READ.BIN's object_lists reaches it by its
 * address, 0x0c900258, for every act */
extern struct object_list egm2_object_list;

#endif
