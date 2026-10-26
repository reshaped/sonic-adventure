/* EV00FE.PRS, after the last fight, Chaos sees the Chao at peace and leaves
 * with Tikal, and Sonic sets off after Eggman */
#ifndef EV00FE_SCRIPT_H
#define EV00FE_SCRIPT_H

#include "system/texture.h"

/* empty: ev00fe_script loads its own textures at EVENT_LOAD, in place of
 * eff_regular_texlist's, and releases them at EVENT_UNLOAD */
extern struct texture_pvm ev00fe_textures[];

void ev00fe_script(int phase);

#endif
