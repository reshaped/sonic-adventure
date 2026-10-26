/* O LAMP1 and O LAMP2, lamps whose glow faces the camera and flickers */
#ifndef STG05_LAMPS_H
#define STG05_LAMPS_H

#include "system/task.h"

/* task steps for O LAMP1 and O LAMP2, named in the object list */
void mountain_lamp_1_step(struct task *task);
void mountain_lamp_2_step(struct task *task);

#endif
