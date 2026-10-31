/* the Egg Carrier's sky, and the puffs of cloud on its deck. the sky is a
 * dome over a sheet of cloud in flight, and a dome over the sea once the Egg
 * Carrier has sunk */
#ifndef ADV0100_SKY_H
#define ADV0100_SKY_H

#include "system/task.h"

/* the sky's step, at the module's first byte, whose address 1ST_READ.BIN's
 * sky_steps holds */
void egg_carrier_sky_step(struct task *task);
/* the clouds' step. the field task creates its task in acts 0 to 2, on the
 * deck, while the Egg Carrier flies */
void egg_carrier_clouds_step(struct task *task);

#endif
