/* the lap: LAP and its number, at the top right of the screen */
#include <ninja.h>
#include <sg_xpt.h>

#include "minicart/lap.h"
#include "minicart/object_textures.h" /* NOLINT(misc-include-cleaner) */

/* the frames are obj_mini_cart_texlist's textures: cart_lap0 for the label,
 * and cart_lap1 to cart_lap3 for the digits. the stage loads them from
 * OBJ_MINI_CART */

static const NJS_TEXANIM lap_label_frame[1] = {
#include "minicart/lap/lap_label_frame.inc"
};
static const NJS_TEXANIM lap_digit_frames[3] = {
#include "minicart/lap/lap_digit_frames.inc"
};
static const NJS_SPRITE lap_label = {
#include "minicart/lap/lap_label.inc"
};
static const NJS_SPRITE lap_digit = {
#include "minicart/lap/lap_digit.inc"
};

/* the digit's depth is a little nearer than the label's, so the digit
 * draws over it */
void lap_draw(Sint8 lap)
{
    njDrawSprite2D((NJS_SPRITE *)&lap_label, 0, -2.001F, NJD_SPRITE_ALPHA);
    njDrawSprite2D((NJS_SPRITE *)&lap_digit, lap, -2.0F, NJD_SPRITE_ALPHA);
}
