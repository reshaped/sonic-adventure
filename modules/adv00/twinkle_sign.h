/* O TWKANBAN Twinkle Park sign, whose lights cycle through three textures */
#ifndef ADV00_TWINKLE_SIGN_H
#define ADV00_TWINKLE_SIGN_H

#include "system/task.h"

/* the object list's step. in Twinkle Park's act it starts the lights at
 * their first texture and hands the task the sign's own step and draw. in
 * any other act it prints ACT TWINKLE ONLY ! and stays the task's step */
void twinkle_sign_init(struct task *task);

#endif
