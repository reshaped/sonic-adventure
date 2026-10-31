/* the names of act 0's land's textures, their texlist, and the material of
 * the land's first model */
#ifndef STG01_LAND0_TEXTURES_H
#define STG01_LAND0_TEXTURES_H
#include <ninja.h>

/* the texlist of act 0's land, whose table points at it. 1ST_READ.BIN's
 * beach_act_pvms pairs it with BEACH01, the pvm texture_load_act loads into
 * it */
extern NJS_TEXLIST beach_land0_texlist;
/* the material of the land's first model, which lies in the generated data
 * after this file */
extern NJS_MATERIAL dat_0c9121cc[1];

#endif
