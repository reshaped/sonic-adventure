/* a motion of dat_0cba09b4, the emerald model in the data before this file. an
 * action pairs the two. no word in this event or 1ST_READ.BIN points at the
 * action. the texture name table of event_chaos0_water_texlist follows. the
 * texlist, which events/shared/chaos0.c reads, starts the data after this
 * file */
#include <ninja.h>

#include "ev0007/dat_0cba1808.h"

#define TEXT_1_LENGTH 11
#include "ev0007/dat_0cba1808/text.inc"

extern NJS_OBJECT dat_0cba09b4;
extern NJS_MDATA2 dat_0cba17b8[];

static NJS_MOTION dat_0cba1808 = {
#include "ev0007/dat_0cba1808/dat_0cba1808.inc"
};
static NJS_ACTION dat_0cba1814 = {
#include "ev0007/dat_0cba1808/dat_0cba1814.inc"
};
NJS_TEXNAME event_chaos0_water_textures[1] = {
    {TEXT_1, 0, 0},
};
