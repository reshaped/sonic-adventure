/* the red emerald's texture name and texlist, then the first material of a
 * node of the emerald's model. the data after this file holds the node. no
 * word in EV009B or 1ST_READ.BIN points at the texlist */
#include <ninja.h>

#define TEXT_1_LENGTH 14
#include "ev009b/dat_0cb84970/text.inc"

static NJS_TEXNAME dat_0cb84970[1] = {
    {TEXT_1, 0, 0},
};
static NJS_TEXLIST dat_0cb8497c = {
#include "ev009b/dat_0cb84970/dat_0cb8497c.inc"
};
static NJS_MATERIAL dat_0cb84984[1] = {
#include "ev009b/dat_0cb84970/dat_0cb84984.inc"
};
