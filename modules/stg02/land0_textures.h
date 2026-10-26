/* the names of act 0's land's textures, and its texlist */
#ifndef STG02_LAND0_TEXTURES_H
#define STG02_LAND0_TEXTURES_H
#include <ninja.h>

/* act 0's land points at this texlist. 1ST_READ.BIN's windy_act_pvms
 * loads WINDY01 into it by its address, 0x0c90fbb0 */
extern NJS_TEXLIST windy_land0_texlist;

#endif
