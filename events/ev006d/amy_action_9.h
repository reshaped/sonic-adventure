/* the names script.c reads in this object */
#ifndef EV006D_AMY_ACTION_9_H
#define EV006D_AMY_ACTION_9_H

#include <ninja.h>

extern NJS_ACTION ev006d_amy_action_9;
extern NJS_TEXLIST ev006d_bird_texlist_1;
extern NJS_TEXLIST ev006d_bird_texlist_2;
/* @hack type: ev006d_bird_action_0 declared as an array, which script.c passes
 * by its name. SHC then loads 340.01F into FR13 at 0x0cb80188 and 0.01F into
 * FR15 at 0x0cb8018c, the x and y of the script's first event_create_player,
 * event_set_pos and event_place_actor. as one NJS_ACTION passed as
 * &ev006d_bird_action_0, 340.01F goes to FR15 and 0.01F to FR13, and their pool
 * words at 0x0cb8029c and 0x0cb802a0 swap. 6 of ev006d_script's 1575 words then
 * differ */
extern NJS_ACTION ev006d_bird_action_0[];
extern NJS_ACTION ev006d_bird_action_1;
extern NJS_ACTION ev006d_bird_action_2;
extern NJS_ACTION ev006d_bird_action_3;
extern NJS_TEXLIST ev006d_picture_texlist;

#endif
