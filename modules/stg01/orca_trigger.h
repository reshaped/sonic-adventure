/* O AO SUMMON: a trigger that sends out the chasing orca once player 0
 * comes near */
#ifndef STG01_ORCA_TRIGGER_H
#define STG01_ORCA_TRIGGER_H

#include "system/task.h"

/* the first step, which beach_object_list names for O AO SUMMON */
void orca_trigger_init(struct task *task);

#endif
