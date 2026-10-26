/* act 0's upper pool, above the dam. it pours into the act's water once
 * Big has pulled its plug and that water is SHELTER_WATER_RISEN */
#ifndef STG12_UPPER_POOL_H
#define STG12_UPPER_POOL_H

#include <sg_xpt.h>

#include "system/task.h"

/* set in shelter_big_flags once Big has pulled the plug */
#define BIG_PULLED 0x1

/* Big's progress in act 0, BIG_PULLED or 0. the pool clears it as it
 * starts. after a retry, the stage task sets BIG_PULLED again. it does so
 * only if Big had pulled the plug and the dam switch had been on */
extern int shelter_big_flags;
/* the height of the upper pool's surface */
extern Float shelter_upper_pool_level;

/* the upper pool's first step. shelter_water_start creates the pool */
void shelter_upper_pool_start(struct task *task);

#endif
