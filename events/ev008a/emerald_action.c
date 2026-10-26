/* ev008a_emerald_action and its motion */
#include <ninja.h>

#include "ev008a/emerald_action.h"

extern NJS_OBJECT dat_0cb9f144;
extern NJS_MDATA3 dat_0cb9f298[];

static NJS_MOTION ev008a_emerald_motion = {
#include "ev008a/emerald_action/ev008a_emerald_motion.inc"
};
NJS_ACTION ev008a_emerald_action = {
#include "ev008a/emerald_action/ev008a_emerald_action.inc"
};
