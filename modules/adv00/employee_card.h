/* O CARD employee card that Sonic carries to the station's reader to open the
 * way to the highway */
#ifndef ADV00_EMPLOYEE_CARD_H
#define ADV00_EMPLOYEE_CARD_H

#include "system/task.h"

/* the step the object list names for O CARD. the card lies only for Sonic, from
 * FLAG_SONIC_SS_CARD until FLAG_SONIC_SS_ENTRANCE_HIGHWAY is set. picking it up
 * shows Sonic's line, and a dropped card goes once it has lain 1801 steps.
 * carried to the station act's reader, it sets FLAG_SONIC_SS_ENTRANCE_HIGHWAY,
 * shows the reader's message and plays MUSIC_ITEM1 */
void employee_card_init(struct task *task);

#endif
