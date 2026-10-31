/* the names script.c reads in this object */
#ifndef EV0142_E102_ACTION_2_H
#define EV0142_E102_ACTION_2_H

#include <ninja.h>

extern NJS_ACTION ev0142_e102_action_2;
extern NJS_TEXLIST ev0142_e101_texlist;
/* @hack type: ev0142_e101_action declared as an array, which script.c passes by
 * its name. SHC then keeps event_message_show's address in R8 for its four
 * calls in ev0142_script (0x0cb8011e). as one NJS_ACTION passed as
 * &ev0142_e101_action, R8 holds the blend of 8 that five event_play_action
 * calls pass, and then event_sound_fade_volume's address. ev0142_script then
 * runs 4 bytes short, and 436 of 664 words differ */
extern NJS_ACTION ev0142_e101_action[];
/* the texture names of the generated texlist ev0142_memory_texlist */
extern NJS_TEXNAME ev0142_memory_textures[2];

#endif
