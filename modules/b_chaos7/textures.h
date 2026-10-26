/* the textures of OBJ_CHAOS7 and EFF_WT_COLUMN with their texlists, and the
 * sky's model */
#ifndef B_CHAOS7_TEXTURES_H
#define B_CHAOS7_TEXTURES_H

#include <ninja.h>

/* the texlist chaos7_pvms loads OBJ_CHAOS7 into. it takes the stage's first
 * texlist slot, which texture_set_stage makes current */
extern NJS_TEXLIST obj_chaos7_texlist;
/* c4_column01 to c4_column05, the water column's frames. column.c gives each
 * a texlist of its own */
extern NJS_TEXNAME column_textures[];
/* the texlist over column_textures, which chaos7_pvms loads EFF_WT_COLUMN
 * into */
extern NJS_TEXLIST eff_wt_column_texlist;
/* the sky's model. chaos7.c's sky task draws it at the camera's x and z, and
 * at y 0 */
extern NJS_OBJECT chaos7_sky_object;

#endif
