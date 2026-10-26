/* O Pin Lamp and O Side Lamp lamps with a turning glare */
#ifndef STG10_LAMP_H
#define STG10_LAMP_H

#include "system/task.h"

/* the object list's steps */
void pin_lamp_step(struct task *task);
void side_lamp_step(struct task *task);

#endif
