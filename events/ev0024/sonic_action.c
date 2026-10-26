/* ev0024_sonic_action and its motion */
#include <ninja.h>

#include "ev0024/sonic_action.h"

extern NJS_MDATA2 dat_0cb83c40[];
extern NJS_OBJECT sonic_object;

static NJS_MOTION ev0024_sonic_motion = {
#include "ev0024/sonic_action/ev0024_sonic_motion.inc"
};
NJS_ACTION ev0024_sonic_action = {
#include "ev0024/sonic_action/ev0024_sonic_action.inc"
};
