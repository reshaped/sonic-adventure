/* the texlist act 0's land draws with */
#ifndef STG04_LAND0_TEXTURES_H
#define STG04_LAND0_TEXTURES_H
#include <ninja.h>

/* world_load_act loads this texlist from HIGHWAY01, the pvm act 0's land names.
 * 1ST_READ.BIN's highway_act_pvms reaches it too, by its address, 0x0c91aebc,
 * but texture_load_act refuses highway's acts */
extern NJS_TEXLIST highway_land0_texlist;

#endif
