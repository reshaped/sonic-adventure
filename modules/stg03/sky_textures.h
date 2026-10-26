/* the sky's object, the names of its textures, and their texlist */
#ifndef STG03_SKY_TEXTURES_H
#define STG03_SKY_TEXTURES_H
#include <ninja.h>

/* the sky, which twinkle.c's sky task draws round the camera */
extern NJS_OBJECT twinkle_sky_object;
/* the sky's texlist, which the PVM BG_SHAREOBJ fills */
extern NJS_TEXLIST bg_shareobj_texlist;

#endif
