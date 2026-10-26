/* the Egg Mobile's action: its model and a 32-frame motion. each event that
 * links them defines the motion's keys in event_eggmobile_mdata */
#include <ninja.h>

#include "shared/eggmobile_action.h"

extern NJS_OBJECT event_eggmobile_object;
extern NJS_MDATA2 event_eggmobile_mdata[];

static NJS_MOTION event_eggmobile_motion = {
#include "shared/eggmobile_action/event_eggmobile_motion.inc"
};
NJS_ACTION event_eggmobile_action = {
#include "shared/eggmobile_action/event_eggmobile_action.inc"
};
