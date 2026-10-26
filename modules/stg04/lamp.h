/* O LAMP street lamp with two glows that face the camera, and O LAMP01 and
 * O LAMP02 lamps on arms */
#ifndef STG04_LAMP_H
#define STG04_LAMP_H

#include "system/task.h"

/* the object list's steps */
void highway_lamp_step(struct task *task);
void highway_lamp_1_step(struct task *task);
void highway_lamp_2_step(struct task *task);

#endif
