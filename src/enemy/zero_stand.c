/* Zero's stand: its motion, and the action that plays the motion on
 * zero_bob_object. zero.c's zero_actions holds the action at ZERO_ACT_STAND */
#include <ninja.h>

#include "enemy/zero_stand.h"

extern NJS_OBJECT zero_bob_object;
extern NJS_MDATA2 dat_8c612008[];

static NJS_MOTION zero_stand_motion = {
#include "enemy/zero_stand/zero_stand_motion.inc"
};
NJS_ACTION zero_stand_action = {
#include "enemy/zero_stand/zero_stand_action.inc"
};
