/* EV00F3.PRS, Knuckles finds Angel Island falling though the Master Emerald
 * is whole, and Eggman brings him word of Chaos */
#ifndef EV00F3_SCRIPT_H
#define EV00F3_SCRIPT_H

#include <sg_xpt.h>

#include "system/texture.h"

extern struct texture_pvm ev00f3_textures[];

/* move the puddle toward x, y, z for steps + 1 steps. each step covers
 * 1 / steps of the way, so the move ends one step past the point. the
 * puddle's y scale swings between 1 and 2 on the way, and then it wobbles.
 * the puddle must exist, and steps must be positive */
void ev00f3_puddle_move(Float x, Float y, Float z, int steps);
/* shrink and fade the puddle for steps + 1 steps. each step takes 1 / steps
 * off its scale factor, so the factor ends at -1 / steps. each step also
 * takes alpha / steps, truncated, off its Uint8 alpha. the alpha wraps past
 * 0 when (steps + 1) * (alpha / steps) exceeds it. the puddle must exist,
 * and steps must be positive */
void ev00f3_puddle_vanish(int steps);

void ev00f3_script(int phase);

#endif
