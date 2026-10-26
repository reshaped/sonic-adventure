/* the boss's effects: egm3spr's textures, and the sprites drawn from them */
#ifndef B_EGM3_EFFECTS_H
#define B_EGM3_EFFECTS_H

#include <ninja.h>

#include "b_egm3/egg_viper.h"

extern NJS_TEXLIST egg_viper_effect_texlist;
/* the six sprites, then a null. egg_viper.c reads the first five by its
 * SPRITE_ numbers */
extern struct egg_viper_sprite *egg_viper_sprites[];

#endif
