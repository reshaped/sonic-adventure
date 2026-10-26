/* the Mystic Ruins' texture lists, and the pvms the stage loads */
#ifndef ADV02_TEXLISTS_H
#define ADV02_TEXLISTS_H
#include <ninja.h>

#include "system/texture.h"

/* they live in ADV02: use them only while it is loaded.
 * texture_load_stage and texture_load_act fill all but tornado2_texlist */
extern NJS_TEXLIST adv_mr00_texlist;
extern NJS_TEXLIST adv_mr01_texlist;
extern NJS_TEXLIST adv_mr02_texlist;
extern NJS_TEXLIST adv_mr03_texlist;
extern NJS_TEXLIST mrobj_texlist;
extern NJS_TEXLIST mr_finalegg_texlist;
extern NJS_TEXLIST mr_train_texlist;
extern NJS_TEXLIST mr_pyramid_texlist;
extern NJS_TEXLIST tornado2_texlist;
extern NJS_TEXLIST mr_sky00_texlist;
extern NJS_TEXLIST mr_sky01_texlist;
extern NJS_TEXLIST mr_sky02_texlist;

/* the pvms texture_load_stage loads, from 1ST_READ.BIN's stage_pvms */
extern struct texture_pvm mystic_ruins_pvms[];

#endif
