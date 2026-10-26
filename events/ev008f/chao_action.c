/* ev008f_chao_action and its motion */
#include <ninja.h>

#include "ev008f/chao_action.h"

extern NJS_OBJECT event_chao_object;
extern NJS_MDATA2 dat_0cba87b8[];

static NJS_MOTION ev008f_chao_motion = {
#include "ev008f/chao_action/ev008f_chao_motion.inc"
};
NJS_ACTION ev008f_chao_action = {
#include "ev008f/chao_action/ev008f_chao_action.inc"
};
