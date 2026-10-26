/* the gravity panels: O TSWITCH and O TPANEL */
#ifndef STG07_GRAVITY_PANEL_H
#define STG07_GRAVITY_PANEL_H

#include "system/task.h"

/* the object list's steps */
void panel_switch_step(struct task *task);
void gravity_panel_step(struct task *task);

#endif
