/* ev0022_chao_action_2 and its motion. event_chao_object, the model the
 * action moves, lies in the data after this object. dat_0cba71b0, the
 * motion's NJS_MDATA2 table, lies in the data before this object */
#include <ninja.h>

#include "ev0022/chao_action_2.h"

extern NJS_MDATA2 dat_0cba71b0[];
extern NJS_OBJECT event_chao_object;

static NJS_MOTION ev0022_chao_motion_2 = {
#include "ev0022/chao_action_2/ev0022_chao_motion_2.inc"
};
NJS_ACTION ev0022_chao_action_2 = {
#include "ev0022/chao_action_2/ev0022_chao_action_2.inc"
};
