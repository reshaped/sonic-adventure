/* E-104's actions, and the motion e104_stand_action plays */
#include <ninja.h>

#include "stg05/e104_actions.h"

/* E-104's model, and e104_stand_motion's keys */
extern NJS_OBJECT dat_0cab77dc;
extern NJS_MDATA2 dat_0cad5380[];
/* the other actions' motions: three of E-102's, in 1ST_READ.BIN, and five
 * of E-104's own */
extern NJS_MOTION e102_jump_motion;
extern NJS_MOTION e102_fall_motion;
extern NJS_MOTION e102_hard_landing_motion;
extern NJS_MOTION dat_0cad4054;
extern NJS_MOTION dat_0cac42d0;
extern NJS_MOTION dat_0cabe5d4;
extern NJS_MOTION dat_0cacc4fc;
extern NJS_MOTION dat_0cad2778;

static NJS_MOTION e104_stand_motion = {
#include "stg05/e104_actions/e104_stand_motion.inc"
};
NJS_ACTION e104_stand_action = {
#include "stg05/e104_actions/e104_stand_action.inc"
};
NJS_ACTION e104_jump_action = {
#include "stg05/e104_actions/e104_jump_action.inc"
};
NJS_ACTION e104_fall_action = {
#include "stg05/e104_actions/e104_fall_action.inc"
};
NJS_ACTION e104_landing_action = {
#include "stg05/e104_actions/e104_landing_action.inc"
};
NJS_ACTION e104_fly_action = {
#include "stg05/e104_actions/e104_fly_action.inc"
};
NJS_ACTION e104_fire_right_action = {
#include "stg05/e104_actions/e104_fire_right_action.inc"
};
NJS_ACTION e104_fire_left_action = {
#include "stg05/e104_actions/e104_fire_left_action.inc"
};
NJS_ACTION e104_hurt_action = {
#include "stg05/e104_actions/e104_hurt_action.inc"
};
NJS_ACTION e104_defeat_action = {
#include "stg05/e104_actions/e104_defeat_action.inc"
};
