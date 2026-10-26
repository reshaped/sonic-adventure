/* the texture name s_testhand and its texlist, then the three materials of a
 * model of Sonic's fingers. the model and its node lie at 0x0cb837e4 and
 * 0x0cb8380c, in the generated data after this object. no word in EV0023 or
 * 1ST_READ.BIN points at the texlist or the node. from this object's start
 * through the node, the bytes match EV001B's from dat_0cb821d0 through
 * ev001b_sonic_fingers. only the words that point into the image differ */
#include <ninja.h>

#define TEXT_1_LENGTH 10
#include "ev0023/dat_0cb83368/text.inc"

static NJS_TEXNAME dat_0cb83368[1] = {
    {TEXT_1, 0, 0},
};
static NJS_TEXLIST dat_0cb83374 = {
#include "ev0023/dat_0cb83368/dat_0cb83374.inc"
};
static NJS_MATERIAL dat_0cb8337c[3] = {
#include "ev0023/dat_0cb83368/dat_0cb8337c.inc"
};
