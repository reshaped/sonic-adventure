/* the texlist act 2's land draws with */
#ifndef STG04_LAND2_TEXTURES_H
#define STG04_LAND2_TEXTURES_H
#include <ninja.h>

/* world_load_act loads this texlist from HIGHWAY03, the pvm act 2's land names.
 * 1ST_READ.BIN's highway_act_pvms reaches it too, by its address, 0x0c9a9c1c,
 * but texture_load_act refuses highway's acts */
extern NJS_TEXLIST highway_land2_texlist;

#endif
