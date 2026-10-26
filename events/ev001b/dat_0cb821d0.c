/* the texture name s_testhand and its texlist, then the three materials of
 * ev001b_sonic_fingers's model. that model lies in the generated data after
 * this object. no word in EV001B or 1ST_READ.BIN points at the texlist */
#include <ninja.h>

#define TEXT_1_LENGTH 10
#include "ev001b/dat_0cb821d0/text.inc"

static NJS_TEXNAME dat_0cb821d0[1] = {
    {TEXT_1, 0, 0},
};
static NJS_TEXLIST dat_0cb821dc = {
#include "ev001b/dat_0cb821d0/dat_0cb821dc.inc"
};
static NJS_MATERIAL dat_0cb821e4[3] = {
#include "ev001b/dat_0cb821d0/dat_0cb821e4.inc"
};
