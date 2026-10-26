/* the Final Egg base in the jungle, which the object list names O FINALEGG,
 * and O FINALWAY, the solid way into it. both are there only once the
 * character's FLAG_*_MR_APPEAR_FINAL_EGG is set */
#ifndef ADV02_FINAL_EGG_H
#define ADV02_FINAL_EGG_H

#include "system/task.h"

/* the steps the object list names for O FINALEGG and O FINALWAY */
void mystic_ruins_final_egg_step(struct task *task);
void mystic_ruins_final_way_step(struct task *task);

#endif
