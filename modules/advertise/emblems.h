/* the emblems of the menus' save, saves[save_slot]: how many it has, and
 * whether it has one */
#ifndef ADVERTISE_EMBLEMS_H
#define ADVERTISE_EMBLEMS_H

#include <sg_xpt.h>

/* emblems_won is how many emblems the menus' save has. emblem_won is TRUE
 * when the save has the emblem that emblem_mission_number numbers for story's
 * character, stage and mission */
Uint32 emblems_won(void);
Uint32 emblem_won(int story, int stage, int mission);

#endif
