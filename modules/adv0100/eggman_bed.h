/* O EGGMANBED Eggman's bed, which stands on end until switch 1 turns on, then
 * swings down flat */
#ifndef ADV0100_EGGMAN_BED_H
#define ADV0100_EGGMAN_BED_H

#include "system/task.h"

/* the object list's step */
void egg_carrier_eggman_bed_step(struct task *task);

#endif
