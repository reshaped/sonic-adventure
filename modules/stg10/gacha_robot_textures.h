/* the names of the E GACHA robots' textures, their texlist, and the
 * materials, strips and uvs after it */
#ifndef STG10_GACHA_ROBOT_TEXTURES_H
#define STG10_GACHA_ROBOT_TEXTURES_H
#include <ninja.h>
#include <sg_xpt.h>

/* the texlist the E GACHA robots draw with, which 1ST_READ.BIN's final_pvms
 * loads from the pvm GACHAPON */
extern NJS_TEXLIST gacha_robot_texlist;
/* the materials, strips and colors that models and meshsets in the generated
 * data name */
extern NJS_MATERIAL dat_0cb012cc[4];
extern Sint16 dat_0cb0131c[11];
extern Sint16 dat_0cb01332[6];
extern Sint16 dat_0cb0133e[16];
extern Sint16 dat_0cb0135e[15];
extern NJS_COLOR dat_0cb0137c[9];

#endif
