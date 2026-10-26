/* START MARK, SMALL SCREEN and ISLET, props that are only drawn */
#ifndef AL_RACE_PROPS_H
#define AL_RACE_PROPS_H

#include "system/task.h"

/* the object list's steps */
void start_mark_init(struct task *task);
void small_screen_init(struct task *task);
void islet_init(struct task *task);

#endif
