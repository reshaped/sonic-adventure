/* the purple emerald's texture name, and a texlist that names it. no word in
 * this event or 1ST_READ.BIN points at the texlist. the material belongs to a
 * node of dat_0cba09b4, an emerald model in the data after this file */
#include <ninja.h>

#include "ev0007/dat_0cb9fbc0.h"

#define TEXT_1_LENGTH 15
#include "ev0007/dat_0cb9fbc0/text.inc"

static NJS_TEXNAME dat_0cb9fbc0[1] = {
    {TEXT_1, 0, 0},
};
static NJS_TEXLIST dat_0cb9fbcc = {
#include "ev0007/dat_0cb9fbc0/dat_0cb9fbcc.inc"
};
NJS_MATERIAL dat_0cb9fbd4[1] = {
#include "ev0007/dat_0cb9fbc0/dat_0cb9fbd4.inc"
};
