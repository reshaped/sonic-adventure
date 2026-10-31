/* the texlist act 0's land draws with */
#ifndef STG04_LAND0_TEXTURES_H
#define STG04_LAND0_TEXTURES_H
#include <ninja.h>

/* world_load_act loads this texlist from HIGHWAY01, the pvm act 0's land names.
 * 1ST_READ.BIN's highway_act_pvms names it too, but texture_load_act refuses
 * highway's acts */
extern NJS_TEXLIST highway_land0_texlist;
/* the materials that a model in the generated data names */
extern NJS_MATERIAL dat_0c91aec4[11];

#endif
