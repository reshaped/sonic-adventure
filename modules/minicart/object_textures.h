/* the objects' texture names and texlist, then materials and strips of
 * signboard_object's model */
#ifndef MINICART_OBJECT_TEXTURES_H
#define MINICART_OBJECT_TEXTURES_H

#include <ninja.h>
#include <sg_xpt.h>

extern NJS_TEXLIST obj_mini_cart_texlist;
/* the eight materials of signboard_object's model. the fifth shows the
 * board's picture. the model lies in the generated data after them */
extern NJS_MATERIAL signboard_materials[];
/* the strips of that model's first two meshsets */
extern Sint16 dat_0ca380e4[];
extern Sint16 dat_0ca38124[];

#endif
