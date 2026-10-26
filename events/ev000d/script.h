/* EV000D.PRS, Sonic's story: the runway at Tails' workshop moves out.
 * Sonic and Tails take off in the Tornado */
#ifndef EV000D_SCRIPT_H
#define EV000D_SCRIPT_H

#include "system/texture.h"

extern struct texture_pvm ev000d_textures[];

/* link Sonic back into the Tornado's model as its fourth child, with the
 * children that followed him. called before the script has unlinked them,
 * it unlinks them instead, and nothing keeps them to link back */
void ev000d_show_sonic(void);

void ev000d_script(int phase);

#endif
