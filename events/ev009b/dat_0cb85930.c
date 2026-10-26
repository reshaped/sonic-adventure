/* the red emerald's motion and action, then the sky emerald's texture name.
 * no word in EV009B or 1ST_READ.BIN points at the action. the data after
 * this file holds the sky emerald's texlist */
#include <ninja.h>

#define TEXT_1_LENGTH 14
#include "ev009b/dat_0cb85930/text.inc"

extern NJS_OBJECT dat_0cb85764;
extern NJS_MDATA3 dat_0cb858b8[];

static NJS_MOTION dat_0cb85930 = {
#include "ev009b/dat_0cb85930/dat_0cb85930.inc"
};
static NJS_ACTION dat_0cb8593c = {
#include "ev009b/dat_0cb85930/dat_0cb8593c.inc"
};
static NJS_TEXNAME ev009b_sky_emerald_textures[1] = {
    {TEXT_1, 0, 0},
};
