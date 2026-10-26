/* the names of act 2's land's textures, and their texlist */
#ifndef STG01_LAND2_TEXTURES_H
#define STG01_LAND2_TEXTURES_H
#include <ninja.h>

/* the texlist of act 2's land, whose table points at it. 1ST_READ.BIN's
 * beach_act_pvms pairs it with BEACH03, the pvm texture_load_act loads into
 * it. beach_act_pvms reaches it by its address, not its name */
extern NJS_TEXLIST beach_land2_texlist;

#endif
