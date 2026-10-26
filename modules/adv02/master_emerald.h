/* the emerald the object list names O MASTER EMERALD: the Master Emerald,
 * which turns and bobs under two glows, in the form FLAG_MASTER_EMERALD
 * picks */
#ifndef ADV02_MASTER_EMERALD_H
#define ADV02_MASTER_EMERALD_H

#include "system/task.h"

/* the event flag the emerald's form follows. for Sonic, Knuckles and E-102,
 * the emerald sets it from the story so far, to 0, MASTER_EMERALD_SHATTERED
 * or MASTER_EMERALD_INCOMPLETE. only events set MASTER_EMERALD_WHOLE */
#define FLAG_MASTER_EMERALD 37
/* FLAG_MASTER_EMERALD's values, each named for the form it picks. 0, the
 * flag clear, leaves the form as it is */
#define MASTER_EMERALD_SHATTERED 1
#define MASTER_EMERALD_INCOMPLETE 2
#define MASTER_EMERALD_WHOLE 3

/* the step the object list names for O MASTER EMERALD */
void mystic_ruins_master_emerald_step(struct task *task);

#endif
