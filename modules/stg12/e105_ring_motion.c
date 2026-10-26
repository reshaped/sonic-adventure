/* the motion E-105's third ring is drawn with, one frame long */
#include <ninja.h>

#include "stg12/e105_ring_motion.h"

extern NJS_MDATA2 dat_0caa0b48[];

NJS_MOTION e105_ring_motion = {
#include "stg12/e105_ring_motion/e105_ring_motion.inc"
};
