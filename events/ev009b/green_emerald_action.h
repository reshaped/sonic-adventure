/* the green emerald's action, which the script plays on the green and sky
 * emeralds */
#ifndef EV009B_GREEN_EMERALD_ACTION_H
#define EV009B_GREEN_EMERALD_ACTION_H

#include <ninja.h>

/* @hack type: ev009b_green_emerald_action declared as an array, which script.c
 * passes by its name. ev009b_script then keeps 0 in FR14 and the emeralds' z,
 * then 1, in FR15 (0x0cb80068, 0x0cb800b8, 0x0cb8012e). as one NJS_ACTION
 * passed as &ev009b_green_emerald_action, the two trade, and 78 of
 * ev009b_script's 690 words differ */
extern NJS_ACTION ev009b_green_emerald_action[];

#endif
