/* the names of the textures the lands of acts 1 and 3 draw with, and their
 * texlist. a material follows the texlist */
#ifndef STG08_ICECAP02_TEXTURES_H
#define STG08_ICECAP02_TEXTURES_H
#include <ninja.h>

/* 1ST_READ.BIN's icecap_act_pvms names this texlist and ICECAP02 for acts 1 and
 * 3, which texture_load_act loads into it */
extern NJS_TEXLIST icecap02_texlist;
/* the material after it, whose texture yura_yura.c steps, and which a
 * model in the generated data names */
extern NJS_MATERIAL dat_0c95cb4c[1];

#endif
