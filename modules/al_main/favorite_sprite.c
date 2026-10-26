/* the sprite express.c's favorite_draw draws, and its frames */
#include <ninja.h>

#include "al_main/models.h"

/* the textures the sprite draws from */
extern NJS_TEXLIST dat_0ca54fe8;

/* its frames: one for each character a Chao keeps a like for, and the last
 * for a Chao that likes none */
static NJS_TEXANIM dat_0ca54ff0[7] = {
#include "al_main/favorite_sprite/dat_0ca54ff0.inc"
};
NJS_SPRITE alife_favorite_sprite = {
#include "al_main/favorite_sprite/alife_favorite_sprite.inc"
};
