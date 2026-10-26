/* ev0089_knuckles_action and its motion */
#include <ninja.h>

#include "ev0089/knuckles_action.h"

extern NJS_MDATA2 dat_0cb83178[];
extern NJS_OBJECT knuckles_object;

static NJS_MOTION ev0089_knuckles_motion = {
#include "ev0089/knuckles_action/ev0089_knuckles_motion.inc"
};
NJS_ACTION ev0089_knuckles_action = {
#include "ev0089/knuckles_action/ev0089_knuckles_action.inc"
};
