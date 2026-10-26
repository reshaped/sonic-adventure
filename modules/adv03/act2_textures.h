/* the texture names and the texlist of the past's act 2 land, and one
 * model's material */
#ifndef ADV03_ACT2_TEXTURES_H
#define ADV03_ACT2_TEXTURES_H

#include <ninja.h>

/* the texlist the act 2 land names. 1ST_READ.BIN's past_act_pvms names it
 * and PAST02 for act 2, which texture_load_act loads into it. the table
 * reaches the texlist by its address, not by its name */
extern NJS_TEXLIST past02_texlist;
/* the only material of the model that the act 2 land's first entry
 * names. the rest of that model, in the gen/ range after it, names it */
extern NJS_MATERIAL dat_0c999bd4[];

#endif
