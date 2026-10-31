/* the texture names and the texlist of the past's act 1 land, and one
 * model's material */
#ifndef ADV03_ACT1_TEXTURES_H
#define ADV03_ACT1_TEXTURES_H

#include <ninja.h>

/* the texlist the act 1 land names. 1ST_READ.BIN's past_act_pvms names it and
 * PAST01 for act 1, which texture_load_act loads into it */
extern NJS_TEXLIST past01_texlist;
/* the only material of the model that the act 1 land's first entry
 * names. the rest of that model, in the gen/ range after it, names it */
extern NJS_MATERIAL dat_0c9698e4[];

#endif
