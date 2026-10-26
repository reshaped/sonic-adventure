/* the characters' texlists and their effects', which texture_load_character
 * fills from their PVMs */
#ifndef PLAYER_TEXTURES_H
#define PLAYER_TEXTURES_H
#include <ninja.h>

#define E102BEAM_TEXTURES 14

extern NJS_TEXLIST sonic_texlist;
extern NJS_TEXLIST tails_texlist;
extern NJS_TEXLIST knuckles_texlist;
extern NJS_TEXLIST knu_eff_texlist;
extern NJS_TEXLIST e102_texlist;
/* the names e102beam_texlist holds. e102_sphere.c's texlist holds the last
 * alone, by its address */
extern NJS_TEXNAME e102beam_textures[E102BEAM_TEXTURES];
extern NJS_TEXLIST e102beam_texlist;
extern NJS_TEXLIST e102effect_texlist;
extern NJS_TEXLIST amy_texlist;
extern NJS_TEXLIST amy_eff_texlist;
extern NJS_TEXLIST big_texlist;
extern NJS_TEXLIST eggman_texlist;

#endif
