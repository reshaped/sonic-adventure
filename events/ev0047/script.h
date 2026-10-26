/* EV0047.PRS, Tails' story: Tails takes off in the Tornado 2 to join Sonic */
#ifndef EV0047_SCRIPT_H
#define EV0047_SCRIPT_H

#include "system/texture.h"

extern struct texture_pvm ev0047_textures[];

/* put the Tornado's Sonic node back in place of its stub. before
 * ev0047_script hides the node, the call unlinks the root's children from
 * the third on */
void ev0047_show_sonic(void);

void ev0047_script(int phase);

#endif
