/* the dial, DIAL, whose lamps and faces show what cart_race_dial gives */
#ifndef MINICART_DIAL_H
#define MINICART_DIAL_H

#include "system/task.h"

/* the step cart_object_kinds names for DIAL. task needs a work, whose pos
 * and ang place the dial. once placement_try_leave finds the dial out of
 * range, the task destroys itself on its next step */
void dial_step(struct task *task);

#endif
