/* EV0088.PRS, Knuckles' story: after the fight with Chaos 2, Chaos's
 * puddle flows to Eggman. Eggman tells Knuckles that Sonic is after the
 * Master Emerald too */
#ifndef EV0088_SCRIPT_H
#define EV0088_SCRIPT_H

#include "system/texture.h"

extern struct texture_pvm ev0088_textures[];

/* shrink the puddle and fade it over steps + 1 steps. its scale starts at 1
 * and loses 1 / steps a step. it ends at -1 / steps. its alpha loses a
 * truncated 1/steps share of its start a step. the Uint8 alpha wraps past 0
 * when (steps + 1) * (alpha / steps) exceeds alpha. steps must be positive,
 * and the puddle task must exist */
void ev0088_puddle_vanish(int steps);

void ev0088_script(int phase);

#endif
