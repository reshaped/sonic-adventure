/* O Dash Cart, the dash panel for carts. Twinkle Park's object list names it
 * O DASH PANEL 4 CART */
#ifndef MINICART_CART_DASH_PANEL_H
#define MINICART_CART_DASH_PANEL_H

#include "system/task.h"

/* the step cart_object_kinds names for O Dash Cart. task needs a work and an
 * extra. the panel pushes player 0, or a cart, that hits it. it pushes
 * toward its own -z at scl.x a step, and a scl.x below 0.1 becomes 10. a
 * panel that starts while the set editor holds it gets no collision record.
 * its later steps then read through null */
void cart_dash_panel_step(struct task *task);

#endif
