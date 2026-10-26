/* the gardens' warps, WARP SS, WARP EC, WARP MR and WARP EGG. a garden's
 * warp takes the player to that garden, and the egg warp out to the Egg
 * Carrier */
#ifndef AL_MAIN_WARP_H
#define AL_MAIN_WARP_H
#include <sg_xpt.h>

#include "system/task.h"

/* set alife_character's event flag for alife_garden, which marks the garden
 * it is in as visited. then return TRUE when the warp to garden is open to
 * alife_character: its warp flag is set, and it has visited garden. FALSE
 * otherwise. garden must be an enum garden */
Bool warp_mark_visit_check_open(int garden);
/* the warps' steps in the gardens' object lists. each clears the carried
 * Chao, sets its warp up and hands the task the warp's own step. a garden
 * warp's step counts its calls in its work's timer, and sets the warp up
 * once the timer passes 10. it opens the warp as warp_mark_visit_check_open
 * says. the egg warp's step sets the warp up at once, always open */
void warp_station_square_init(struct task *task);
void warp_egg_carrier_init(struct task *task);
void warp_mystic_ruins_init(struct task *task);
void warp_egg_init(struct task *task);

#endif
