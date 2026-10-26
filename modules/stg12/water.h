/* act 0's water: its level, and the start of its tasks */
#ifndef STG12_WATER_H
#define STG12_WATER_H

#include <sg_xpt.h>

/* how far act 0's water has risen */
enum shelter_water_state {
    SHELTER_WATER_LOW = 0, /* as the act starts */
    SHELTER_WATER_RISING = 1, /* once the dam starts to sink */
    SHELTER_WATER_RISEN = 2, /* once it reaches the sunk dam's fill level */
    SHELTER_WATER_FULL = 4 /* once the upper pool has poured into it too */
};

extern enum shelter_water_state shelter_water_state;
/* the height of act 0's water surface */
extern Float shelter_water_level;

/* set the water low. then start the lower and deep water, the dam, the
 * upper pool and the floats */
void shelter_water_start(void);

#endif
