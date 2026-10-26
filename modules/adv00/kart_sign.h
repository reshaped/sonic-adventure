/* O KARTKANBAN Twinkle Circuit sign, whose lights cycle through three
 * textures */
#ifndef ADV00_KART_SIGN_H
#define ADV00_KART_SIGN_H

#include "system/task.h"

/* the step the object list names for O KARTKANBAN. outside the Twinkle Park
 * act it prints ACT TWINKLE ONLY ! and stays the task's step, so the message
 * shows every step */
void kart_sign_init(struct task *task);

#endif
