/* ev0032_emerald_action and its motion */
#include <ninja.h>

#include "ev0032/emerald_action.h"

extern NJS_OBJECT dat_0cb8631c;
extern NJS_MDATA2 dat_0cb86890[];

static NJS_MOTION ev0032_emerald_motion = {
#include "ev0032/emerald_action/ev0032_emerald_motion.inc"
};
NJS_ACTION ev0032_emerald_action = {
#include "ev0032/emerald_action/ev0032_emerald_action.inc"
};
