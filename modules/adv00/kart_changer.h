/* O KARTCHANGER invisible box that sends player 0 to the Twinkle Circuit cart
 * race */
#ifndef ADV00_KART_CHANGER_H
#define ADV00_KART_CHANGER_H

#include "system/task.h"

/* the step the object list names for O KARTCHANGER. the set entry's scale
 * sizes the box. once player 0 is inside it, the story goes to the cart stage
 * at entrance 0, and the placement makes no more boxes */
void kart_changer_init(struct task *task);

#endif
