/* the names of the textures act 0's land draws with, and their texlist */
#ifndef STG08_ICECAP01_TEXTURES_H
#define STG08_ICECAP01_TEXTURES_H
#include <ninja.h>

/* 1ST_READ.BIN's icecap_act_pvms names this texlist and ICECAP01 for act
 * 0, which texture_load_act loads into it. the table reaches the texlist by
 * its address, not by its name */
extern NJS_TEXLIST icecap01_texlist;

#endif
