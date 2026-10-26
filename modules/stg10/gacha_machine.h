/* O GACHA and O GSHOOTER machines that launch E GACHA robots */
#ifndef STG10_GACHA_MACHINE_H
#define STG10_GACHA_MACHINE_H

#include "system/task.h"

/* the object list's steps */
void gacha_machine_step(struct task *task);
void gacha_shooter_step(struct task *task);

#endif
