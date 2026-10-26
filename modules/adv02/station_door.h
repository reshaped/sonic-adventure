/* door below the station, whose two panels slide apart and back together as
 * FLAG_STATION_DOOR says */
#ifndef ADV02_STATION_DOOR_H
#define ADV02_STATION_DOOR_H

#include "system/task.h"

/* the event flag that holds the door's station_door_state. event scripts such
 * as EV0047's set it */
#define FLAG_STATION_DOOR 35
enum station_door_state {
    STATION_DOOR_SHUT,
    STATION_DOOR_SHUTTING,
    STATION_DOOR_OPEN,
    STATION_DOOR_OPENING
};

/* task step the Mystic Ruins field task creates in the station act */
void station_door_step(struct task *task);

#endif
