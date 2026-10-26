/* ARROW, Twinkle Park's O PANEL: a sign whose glow fades in and out */
#ifndef MINICART_PANEL_H
#define MINICART_PANEL_H

#include "system/task.h"

/* the step cart_object_kinds names for ARROW. task needs a work, whose pos
 * and ang place the panel. once placement_try_leave_by finds the panel out
 * of range, the task destroys itself on its next step. outside Twinkle
 * Park's act 0 and STAGE_CART, that check is all the step does */
void twinkle_panel_step(struct task *task);

#endif
