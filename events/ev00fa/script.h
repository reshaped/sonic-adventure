/* EV00FA.PRS, Sonic and Tails find the crashed Tornado 2, and its emerald is
 * taken as water bursts up beside it */
#ifndef EV00FA_SCRIPT_H
#define EV00FA_SCRIPT_H

#include "system/texture.h"

extern struct texture_pvm ev00fa_textures[];

/* put the Tornado 2's emerald back as the root's third child. before
 * ev00fa_emerald_hide takes it off, this sets that child to null. nothing
 * calls it */
void ev00fa_emerald_show(void);

void ev00fa_script(int phase);

#endif
