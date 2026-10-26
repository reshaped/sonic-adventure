/* O Jump Cart, the jump panel for carts. Twinkle Park's object list names it
 * O JUMP PANEL 4 CART */
#ifndef MINICART_CART_JUMP_PANEL_H
#define MINICART_CART_JUMP_PANEL_H

#include "system/task.h"

/* the step cart_object_kinds names for O Jump Cart. task needs a work and an
 * extra. the panel throws player 0, or a cart, that hits its ramp. it throws
 * toward its own -z at scl.x a step, and a scl.x below 0.1 becomes 10. it
 * throws up along its y at scl.y times that, and a scl.y below 0.1 becomes
 * 0.7. a panel that starts while the set editor holds it gets no collision
 * record. its later steps then read through null */
void cart_jump_panel_step(struct task *task);

#endif
