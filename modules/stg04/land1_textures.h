/* the texlist act 1's land draws with */
#ifndef STG04_LAND1_TEXTURES_H
#define STG04_LAND1_TEXTURES_H
#include <ninja.h>

/* world_load_act loads this texlist from HIGHWAY02, the pvm act 1's land names.
 * 1ST_READ.BIN's highway_act_pvms reaches it too, by its address, 0x0c99cf48,
 * but texture_load_act refuses highway's acts */
extern NJS_TEXLIST highway_land1_texlist;

#endif
