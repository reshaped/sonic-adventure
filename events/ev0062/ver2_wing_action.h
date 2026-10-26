/* ev0062_ver2_wing_action, which the script plays */
#ifndef EV0062_VER2_WING_ACTION_H
#define EV0062_VER2_WING_ACTION_H

#include <ninja.h>

/* @hack type: ev0062_ver2_wing_action declared as an array, which script.c
 * passes by its name. SHC then keeps event_camera_set_pos's address in R10
 * (0x0cb80026) and &ev0062_bird in R11 (0x0cb80138). it keeps 4.8 in FR12
 * (0x0cb8018e). as one NJS_ACTION passed as &ev0062_ver2_wing_action, R10 and
 * R11 trade values, FR12 takes 7.5, and 104 of ev0062_script's 1647 words
 * differ */
extern NJS_ACTION ev0062_ver2_wing_action[];

#endif
