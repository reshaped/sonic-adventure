/* SIGNBOARD1 to 5, solid boards that each show two textures by turns */
#ifndef MINICART_SIGNBOARD_H
#define MINICART_SIGNBOARD_H

#include "system/task.h"

/* task steps for SIGNBOARD1 to 5, named in the object list */
void signboard_1_step(struct task *task);
void signboard_2_step(struct task *task);
void signboard_3_step(struct task *task);
void signboard_4_step(struct task *task);
void signboard_5_step(struct task *task);

#endif
