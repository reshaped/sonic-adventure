/* the names of OBJ_BEACH's textures, and their texlist */
#ifndef STG01_OBJ_BEACH_TEXTURES_H
#define STG01_OBJ_BEACH_TEXTURES_H

#include <ninja.h>

/* OBJ_BEACH's texlist. beach_pvms names it first, so texture_load_stage
 * loads it into the stage's first slot. texture_set_stage makes that slot's
 * list current */
extern NJS_TEXLIST obj_beach_texlist;

#endif
