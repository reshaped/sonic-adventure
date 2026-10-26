/* ev0042_emerald_action and its motion */
#include <ninja.h>

#include "ev0042/emerald_action.h"

extern NJS_OBJECT dat_0cb99b84;
extern NJS_MDATA3 dat_0cb99cd8[];

static NJS_MOTION ev0042_emerald_motion = {
#include "ev0042/emerald_action/ev0042_emerald_motion.inc"
};
NJS_ACTION ev0042_emerald_action = {
#include "ev0042/emerald_action/ev0042_emerald_action.inc"
};
