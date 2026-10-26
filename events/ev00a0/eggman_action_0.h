/* ev00a0_eggman_action_0, which the script plays */
#ifndef EV00A0_EGGMAN_ACTION_0_H
#define EV00A0_EGGMAN_ACTION_0_H

#include <ninja.h>

/* @hack type: ev00a0_eggman_action_0 declared as an array, which script.c
 * passes by its name. SHC then puts event_set_pos's address in R11 and
 * event_play_action's in R8 (0x0cb80022, 0x0cb800da). as one NJS_ACTION passed
 * as &ev00a0_eggman_action_0, event_set_ang's and event_voice_play's take those
 * registers, and 16 of ev00a0_script's 550 words differ */
extern NJS_ACTION ev00a0_eggman_action_0[];

#endif
