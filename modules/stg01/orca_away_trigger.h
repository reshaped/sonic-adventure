/* O AO KILL: a trigger that sends the chasing orca away while player 0 or
 * 1 is near */
#ifndef STG01_ORCA_AWAY_TRIGGER_H
#define STG01_ORCA_AWAY_TRIGGER_H

#include "system/task.h"

/* the first step, which beach_object_list names for O AO KILL */
void orca_away_trigger_init(struct task *task);

#endif
