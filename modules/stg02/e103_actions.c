/* E-103's actions, and the motion the first plays */
#include <ninja.h>

#include "stg02/e103_actions.h"

/* E-103's model, which every action poses, and the MDATA2 entries of
 * e103_idle_motion */
extern NJS_OBJECT dat_0c9f453c;
extern NJS_MDATA2 dat_0ca12150[];
/* the other actions' motions: E-102's in 1ST_READ.BIN for the jump, the
 * descent and the landing, and E-103's own */
extern NJS_MOTION e102_jump_motion;
extern NJS_MOTION e102_fall_motion;
extern NJS_MOTION e102_hard_landing_motion;
extern NJS_MOTION dat_0ca10e24;
extern NJS_MOTION dat_0ca010a0;
extern NJS_MOTION dat_0c9fb3a4;
extern NJS_MOTION dat_0ca092cc;
extern NJS_MOTION dat_0ca0f548;

static NJS_MOTION e103_idle_motion = {
#include "stg02/e103_actions/e103_idle_motion.inc"
};
NJS_ACTION e103_idle_action = {
#include "stg02/e103_actions/e103_idle_action.inc"
};
NJS_ACTION e103_jump_action = {
#include "stg02/e103_actions/e103_jump_action.inc"
};
NJS_ACTION e103_descending_action = {
#include "stg02/e103_actions/e103_descending_action.inc"
};
NJS_ACTION e103_land_action = {
#include "stg02/e103_actions/e103_land_action.inc"
};
NJS_ACTION e103_fly_action = {
#include "stg02/e103_actions/e103_fly_action.inc"
};
NJS_ACTION e103_shoot_action = {
#include "stg02/e103_actions/e103_shoot_action.inc"
};
NJS_ACTION e103_action_6 = {
#include "stg02/e103_actions/e103_action_6.inc"
};
NJS_ACTION e103_hurt_action = {
#include "stg02/e103_actions/e103_hurt_action.inc"
};
NJS_ACTION e103_fall_action = {
#include "stg02/e103_actions/e103_fall_action.inc"
};
