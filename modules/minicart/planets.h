/* O PLANET 1 to 3 and O METEORITE 1 to 3, models that spin about y */
#ifndef MINICART_PLANETS_H
#define MINICART_PLANETS_H

#include "system/task.h"

/* the steps cart_object_kinds names for O PLANET 1 to 3 and O METEORITE 1
 * to 3. task needs a work, whose pos and ang place the object. its scl.x is
 * the spin, in degrees a step. once placement_try_leave_by finds the
 * object out of range, the task destroys itself on its next step. outside
 * Twinkle Park's act 0 and STAGE_CART, that check is all the step does */
void planet_1_step(struct task *task);
void planet_2_step(struct task *task);
void planet_3_step(struct task *task);
void meteorite_1_step(struct task *task);
void meteorite_2_step(struct task *task);
void meteorite_3_step(struct task *task);

#endif
