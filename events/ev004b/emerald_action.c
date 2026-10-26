/* ev004b_emerald_action and its motion */
#include <ninja.h>

#include "ev004b/emerald_action.h"

extern NJS_OBJECT ev004b_emerald_object;
extern NJS_MDATA2 ev004b_emerald_motion_mdata[];

static NJS_MOTION ev004b_emerald_motion = {
#include "ev004b/emerald_action/ev004b_emerald_motion.inc"
};
NJS_ACTION ev004b_emerald_action = {
#include "ev004b/emerald_action/ev004b_emerald_action.inc"
};
