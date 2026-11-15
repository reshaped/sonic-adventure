/* the palette routines: copies, fills, a blend and a rotation of colors.
 * make pc compiles this in place of src/draw/palette.src, whose comments,
 * and palette.h's, give each routine.
 *
 * fun_8c0567dc's rotation of a palette onto itself saves its last shift
 * colors 8 bytes apart in dat_8c0568a0, which holds 128. as in palette.src,
 * a larger shift runs on over dat_8c056ca0, the pointer fun_8c0566a0 keeps,
 * which follows the buffer. palette.src's run goes on over main's code past
 * a shift of 132; here it goes into room that follows, which nothing reads */
#include <sg_xpt.h>

#include "draw/model.h"
#include "draw/palette.h"

/* dat_8c0568a0's colors, dat_8c056ca0 after them, as palette.src lays them
 * out, and room for the rest of a rotation's run past them */
static union {
    Uint32 words[512];
    struct {
        Uint32 dat_8c0568a0[256];
        void *dat_8c056ca0;
    } kept;
} palette_store;

void fun_8c0566a0(void *pointer)
{
    palette_store.kept.dat_8c056ca0 = pointer;
}

void *fun_8c0566ac(void)
{
    return palette_store.kept.dat_8c056ca0;
}

Uint32 *palette_get(int palette, int offsets)
{
    Uint8 *at = (Uint8 *)draw_palettes + (palette << 11);

    if (offsets != 0) {
        at += 4;
    }
    return (Uint32 *)at;
}

/* NOLINTBEGIN(readability-identifier-length) */
void fun_8c0566d0(Uint32 *out, Uint32 *in)
{
    int i;

    if (in == out) {
        return;
    }
    for (i = 0; i < 256; i++) {
        out[i] = in[i];
    }
}

void fun_8c0566ea(Uint32 *out, Uint32 *in)
{
    int i;

    if (in == out) {
        return;
    }
    for (i = 0; i < 512; i++) {
        out[i] = in[i];
    }
}

/* 32 passes, each down from 28 bytes on: the colors at 24, 16, 8 and 0 */
void fun_8c056704(Uint32 *out, Uint32 color)
{
    int pass;

    for (pass = 0; pass < 32; pass++) {
        out[6] = color;
        out[4] = color;
        out[2] = color;
        out[0] = color;
        out += 8;
    }
}

/* 32 passes of 8 colors, each from its last down */
void fun_8c056724(Uint32 *out, Uint32 color)
{
    int pass;
    int k;

    for (pass = 0; pass < 32; pass++) {
        for (k = 7; k >= 0; k--) {
            out[k] = color;
        }
        out += 8;
    }
}

/* MULS.W: the product of the low 16 bits of each, as signed numbers */
static Sint32 muls_w(Sint32 a, Sint32 b)
{
    return (Sint32)(Sint16)a * (Sint32)(Sint16)b;
}

/* FTRC, saturated as the SH-4's */
static Sint32 ftrc(Float f)
{
    if (f >= 2147483648.0F || f != f) {
        return 0x7fffffff;
    }
    if (f < -2147483648.0F) {
        return (Sint32)0x80000000;
    }
    return (Sint32)f;
}

void palette_mix(Uint32 *out, Uint32 *from, Uint32 *to, Float share)
{
    Sint32 w;
    Sint32 rest;
    int i;

    if (from == to) {
        return;
    }
    w = ftrc(256.0F * share);
    rest = 256 - w;
    for (i = 0; i < 256; i++) {
        Uint32 a = from[2 * i];
        Uint32 b = to[2 * i];
        Uint32 color = 0;
        int shift;

        for (shift = 16; shift >= 0; shift -= 8) {
            Sint32 sum = muls_w(w, (Sint32)(a >> shift & 0xff)) +
                         muls_w(rest, (Sint32)(b >> shift & 0xff));

            color = color << 8 | ((Uint32)sum >> 8 & 0xff);
        }
        out[2 * i] = color;
    }
}

void fun_8c0567dc(Uint32 *out, Uint32 *in, Uint8 shift)
{
    int count = shift;
    int k;

    if (count == 0) {
        return;
    }
    if (in != out) {
        for (k = 0; k < 256 - count; k++) {
            out[2 * (count + k)] = in[2 * k];
        }
        for (k = 0; k < count; k++) {
            out[2 * k] = in[2 * (256 - count + k)];
        }
        return;
    }
    /* save the last count colors, 8 bytes apart, run them onto themselves,
     * and put the saved ones back where they were */
    for (k = 0; k < count; k++) {
        palette_store.words[2 * k] = in[2 * (256 - count + k)];
    }
    for (k = 0; k < 256 - count; k++) {
        out[2 * k] = in[2 * k];
    }
    for (k = 0; k < count; k++) {
        out[2 * (256 - count + k)] = palette_store.words[2 * k];
    }
}

void fun_8c056860(Uint32 *out, Uint32 *in)
{
    int i;

    if (in == out) {
        return;
    }
    for (i = 0; i < 256; i++) {
        out[2 * i] = in[i];
    }
}

void fun_8c05687a(Uint32 *out, Uint32 *in)
{
    int i;

    if (in == out) {
        return;
    }
    for (i = 0; i < 256; i++) {
        out[i] = in[2 * i];
    }
}
/* NOLINTEND(readability-identifier-length) */

void fun_8c056896(void)
{
}
