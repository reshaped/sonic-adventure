/* Ninja's built-in font, which njInitPrint loads when texture is null: 128 by
 * 128 ARGB1555 texels, twiddled, so that a texel's index interleaves the bits
 * of its row and column */
#include <ninja.h>
#include <sg_xpt.h>

#define FONT_SIZE 128

Uint8 _nj_debug_char_[FONT_SIZE * FONT_SIZE * sizeof(Uint16)] = {
#include "ninja/debug/_nj_debug_char_.inc"
};
