/* O PANEL, a sign whose glow fades in and out */
#ifndef STG03_PANEL_H
#define STG03_PANEL_H

#include "system/task.h"

/* the step twinkle_object_kinds names for O PANEL. task needs a work, whose pos
 * and ang place the panel. once placement_try_leave_by finds the panel out of
 * range, the task destroys itself on its next step. outside Twinkle Park's
 * act 0 and STAGE_CART, that check is all the step does */
void twinkle_panel_step(struct task *task);

#endif
