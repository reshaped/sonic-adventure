/* O EGGSTAND stand for the shop's egg, which sinks under an egg's weight */
#ifndef ADV00_EGG_STAND_H
#define ADV00_EGG_STAND_H

#include "system/task.h"

/* set while an egg rests on the stand. the second shop's fence comes down
 * while it is clear */
#define FLAG_EGG_ON_STAND 3
/* the step the object list names for O EGGSTAND. once FLAG_GOLD_EGG_IN is
 * set, the stand stays down with the fake egg on it */
void station_square_egg_stand_init(struct task *task);

#endif
