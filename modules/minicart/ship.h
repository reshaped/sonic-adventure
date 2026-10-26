/* O SHIP, a ship that cruises on, or plies to and fro, while player 0 is
 * near */
#ifndef MINICART_SHIP_H
#define MINICART_SHIP_H

#include "system/task.h"

/* task step for O SHIP, named in the object list. outside Twinkle Park's
 * act 0 and STAGE_CART, the ship neither moves nor draws */
void ship_step(struct task *task);

#endif
