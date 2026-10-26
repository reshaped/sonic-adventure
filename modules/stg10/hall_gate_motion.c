/* the hall gate's opening motion, and the action that plays it on
 * hall_gate_object */
#include <ninja.h>

#include "stg10/hall_gate_motion.h"

extern NJS_OBJECT hall_gate_object;
extern NJS_MDATA2 dat_0cb0e008[];

static NJS_MOTION hall_gate_motion = {
#include "stg10/hall_gate_motion/hall_gate_motion.inc"
};
NJS_ACTION hall_gate_action[1] = {
#include "stg10/hall_gate_motion/hall_gate_action.inc"
};
