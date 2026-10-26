/* the texlists of the pvms OBJ_RUIN, OBJ_RUIN2 and BG_RUIN */
#ifndef STG07_OBJ_RUIN_TEXTURES_H
#define STG07_OBJ_RUIN_TEXTURES_H
#include <ninja.h>

/* OBJ_RUIN's texlist. 1ST_READ.BIN's ruin_pvms, the stage's pvm list, names
 * it and OBJ_RUIN2's */
extern NJS_TEXLIST obj_ruin_texlist;
/* runs of OBJ_RUIN2's textures, which the doors, the water and the gravity
 * panels draw with */
extern NJS_TEXLIST ruin_door_texlist;
extern NJS_TEXLIST ruin_water_texlist;
extern NJS_TEXLIST gravity_panel_texlist;
/* OBJ_RUIN2's texlist, which holds the three runs above */
extern NJS_TEXLIST obj_ruin2_texlist;
/* BG_RUIN's texlist, the sky's, which ruin.c loads for act 1 */
extern NJS_TEXLIST bg_ruin_texlist;

#endif
