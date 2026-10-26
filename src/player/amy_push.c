/* amy_push_action and its motion, the last of Amy's data before big.c's.
 * the action starts 4 bytes past an 8-byte boundary, right after the
 * motion. R32, the SHC release that compiles this file, starts each
 * object's data on an 8-byte boundary, so the two belong to one object */
#include <ninja.h>

#include "player/amy_push.h"

extern NJS_OBJECT amy_object;
extern NJS_MDATA2 dat_8c53b970[];

static NJS_MOTION amy_push_motion = {
#include "player/amy_push/amy_push_motion.inc"
};
NJS_ACTION amy_push_action = {
#include "player/amy_push/amy_push_action.inc"
};
