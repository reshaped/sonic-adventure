/* the motion of e102_stand_downhill_action, then the action. the action starts
 * 4 bytes past an 8-byte boundary, straight after the motion. an object that
 * SHC's R32 release compiles starts on an 8-byte boundary, so the two belong to
 * one object */
#include <ninja.h>

#include "player/e102_stand_downhill.h"

extern NJS_OBJECT e102_object;
extern NJS_MDATA2 dat_8c4d12c0[];

static NJS_MOTION e102_stand_downhill_motion = {
#include "player/e102_stand_downhill/e102_stand_downhill_motion.inc"
};
NJS_ACTION e102_stand_downhill_action = {
#include "player/e102_stand_downhill/e102_stand_downhill_action.inc"
};
