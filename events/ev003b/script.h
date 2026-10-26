/* EV003B.PRS, Tails' story: Tails and Sonic take off in the Tornado */
#ifndef EV003B_SCRIPT_H
#define EV003B_SCRIPT_H

#include "system/texture.h"

extern struct texture_pvm ev003b_textures[];

/* each puts the Tornado's Sonic or Tails node back in place of its stub.
 * before ev003b_script first hides the node, the call unlinks it and the
 * root's children after it */
void ev003b_show_sonic(void);
void ev003b_show_tails(void);

void ev003b_script(int phase);

#endif
