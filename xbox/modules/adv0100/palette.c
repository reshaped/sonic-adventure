/* the Egg Carrier's palette routines: a gray ramp in the offset colors of
 * palettes 0 and 1, and copies of a palette's base or offset colors out and
 * back. make pc compiles this in place of modules/adv0100/palette.src,
 * whose comments give each. no C names them, so their declarations are
 * here */
#include <sg_xpt.h>

#include "draw/model.h"

void egg_carrier_palette_ramp(Uint8 level);
void egg_carrier_palette_read(int palette, int offsets, Uint32 *out);
void egg_carrier_palette_write(int palette, int offsets, Uint32 *in);

/* a gray of level: level in each of the low three bytes, 0xff in the top */
static Uint32 gray(Uint32 level)
{
    return 0xff000000u | level << 16 | level << 8 | level;
}

/* the ramp in 256 colors 8 bytes apart from out: 16 grays of level, then 32
 * that fall by step, in 16.16 fixed point, then 208 black */
static void ramp(Uint32 *out, Uint32 level, Uint32 step)
{
    Uint32 fixed = level << 16;
    int k;

    for (k = 0; k < 16; k++) {
        *out = gray(level);
        out += 2;
    }
    for (k = 0; k < 32; k++) {
        fixed -= step;
        *out = gray(fixed >> 16 & 0xff);
        out += 2;
    }
    for (k = 0; k < 208; k++) {
        *out = 0xff000000u;
        out += 2;
    }
}

void egg_carrier_palette_ramp(Uint8 level)
{
    /* a 32nd of level, in 16.16 fixed point */
    Uint32 step = (Uint32)level << 16 >> 5;

    ramp(draw_palettes[0] + 1, level, step);
    ramp(draw_palettes[1] + 1, level, step);
}

/* the first of draw_palettes[palette]'s base colors, or its offset colors */
static Uint32 *colors(int palette, int offsets)
{
    Uint8 *at = (Uint8 *)draw_palettes + (palette << 11);

    if (offsets != 0) {
        at += 4;
    }
    return (Uint32 *)at;
}

void egg_carrier_palette_read(int palette, int offsets, Uint32 *out)
{
    Uint32 *in = colors(palette, offsets);
    int k;

    for (k = 0; k < 256; k++) {
        out[k] = in[2 * k];
    }
}

void egg_carrier_palette_write(int palette, int offsets, Uint32 *in)
{
    Uint32 *out = colors(palette, offsets);
    int k;

    for (k = 0; k < 256; k++) {
        out[2 * k] = in[k];
    }
}
