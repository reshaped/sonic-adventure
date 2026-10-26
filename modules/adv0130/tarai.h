/* the washbasin puzzle the object list names O TARAI: six tiles to stand on
 * in the order of their faces */
#ifndef ADV0130_TARAI_H
#define ADV0130_TARAI_H

#include "system/task.h"

/* the event flag the board sets once all six tiles are pressed in order */
#define FLAG_TARAI_SOLVED 40

/* the object list's step */
void egg_carrier_tarai_step(struct task *task);
/* the step and draw of a ripple, a task no code creates */
void tarai_ripple_step(struct task *task);
void tarai_ripple_draw(struct task *task);

#endif
