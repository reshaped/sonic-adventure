/* the texlist the script draws Tails' plane and Tails with. the event loads it
 * from EV_S_T2C_BODY */
#ifndef EV0003_PLANE_TEXTURES_H
#define EV0003_PLANE_TEXTURES_H

#include <ninja.h>
#include <sg_xpt.h>

extern NJS_TEXLIST ev0003_plane_texlist;
/* the materials of a node of the plane's model. the data after this object
 * holds the rest of the node */
extern NJS_MATERIAL dat_0cb9171c[4];
/* the strips of the four meshsets of that node's model, and the first
 * meshset's texture coordinates */
extern Sint16 dat_0cb9176c[40];
extern Sint16 dat_0cb917bc[32];
extern Sint16 dat_0cb917fc[39];
extern Sint16 dat_0cb9184a[13];
extern NJS_COLOR dat_0cb91864[31];

#endif
