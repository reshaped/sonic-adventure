/* the names of the textures the casino's objects load, and their texlists */
#ifndef STG09_OBJ_CASINO_TEXTURES_H
#define STG09_OBJ_CASINO_TEXTURES_H
#include <ninja.h>

/* casino.c loads obj_casino_texlist from OBJ_CASINO, and obj_casino2_texlist
 * from OBJ_CASINO2. each of the other texlists names the stretch of
 * obj_casino2_texlist's textures that its object or sprite draws with */
extern NJS_TEXLIST obj_casino_texlist;
extern NJS_TEXLIST card_sparkle_texlist;
extern NJS_TEXLIST big_star_texlist;
extern NJS_TEXLIST loop_texlist;
/* card_face_sprite's and card_white_sprite's */
extern NJS_TEXLIST card_face_texlist;
extern NJS_TEXLIST card_white_texlist;
/* the star's 2 frames, then each arrow's 2. then each string's 3 frames, for
 * pink, orange and yellow. a string's frame takes 2 textures, one for each of
 * its model's materials */
extern NJS_TEXLIST neon_texlist;
extern NJS_TEXLIST obj_casino2_texlist;

#endif
