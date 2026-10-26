/* the robots and props in the deck's two lanes: O ROBO A, O ROBO B, O BUKET,
 * O BOX and O UEKI. each slides as the deck tilts */
#ifndef STG06_ROBO_H
#define STG06_ROBO_H

#include "system/task.h"

/* the steps O ROBO A, O ROBO B, O BUKET, O BOX and O UEKI name */
void robo_a_step(struct task *task);
void robo_b_step(struct task *task);
void buket_step(struct task *task);
void box_step(struct task *task);
void ueki_step(struct task *task);

#endif
