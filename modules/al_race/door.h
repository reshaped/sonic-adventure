/* the race hall's door, which takes a player through it to the Station
 * Square garden */
#ifndef AL_RACE_DOOR_H
#define AL_RACE_DOOR_H
#include <sg_xpt.h>

/* add the door to the hall's scenery */
void door_create(void);
/* TRUE from the door's step that starts it opening from shut until its next
 * step. a door that opens again as it shuts leaves it FALSE */
Bool door_opening(void);

#endif
