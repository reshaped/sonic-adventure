/* the race's clock on screen: its label, and times in glyphs from texture
 * 42 of obj_mini_cart_texlist */
#include <ninja.h>
#include <sg_xpt.h>

#include "minicart/cart_time.h"
#include "minicart/object_textures.h" /* NOLINT(misc-include-cleaner) */
#include "save/minigame.h"
#include "system/texlists.h" /* NOLINT(misc-include-cleaner) */

/* the texture is a strip of glyphs, each GLYPH_TEXELS wide. the digits 0
 * to 9 come first, then a colon, a plus and a minus */
#define GLYPH_COLON 10
#define GLYPH_PLUS 11
#define GLYPH_MINUS 12
#define GLYPH_TEXELS 16

/* where on screen the clock's time and the gap start */
#define CLOCK_X 64.0F
#define CLOCK_Y 33.0F
#define GAP_X 248.0F
#define GAP_Y 152.0F

static NJS_TEXANIM cart_time_label_frame[1] = {
#include "minicart/cart_time_draw/cart_time_label_frame.inc"
};
static NJS_SPRITE cart_time_label = {
#include "minicart/cart_time_draw/cart_time_label.inc"
};
/* the glyph's region of the strip. cart_time_draw_glyph sets its u1 and u2
 * for each glyph */
/* @hack type: volatile on the region, which has cart_time_draw_glyph
 * load u1's and u2's addresses from pool words of their own (MOV.L at
 * 0x0c90b954 and 0x0c90b966); without it, SHC stores both at offsets from
 * one address and 17 of 29 words differ */
static volatile NJS_TEXANIM cart_time_glyph_frame = {
#include "minicart/cart_time_draw/cart_time_glyph_frame.inc"
};
static NJS_SPRITE cart_time_glyph = {
#include "minicart/cart_time_draw/cart_time_glyph.inc"
};

static Float cart_time_draw_pair(Float x, Float y, const Float *depths,
                                 int number);
static Float cart_time_draw_glyph(Float x, Float y, Float depth, int glyph);

/* @hack spelling: the colon through a function of its own, which SHC
 * expands, copying the colon's width to FR5 before it adds it (FMOV FR0,FR5
 * at 0x0c90b78a); called directly, summed over cart_time_draw and
 * cart_time_draw_gap, 136 of 158 words differ */
static Float cart_time_draw_colon(Float x, Float y, Float depth)
{
    return cart_time_draw_glyph(x, y, depth, GLYPH_COLON);
}

void cart_time_draw_label(void)
{
#ifdef GCC
    /* the port's 16:9 squeezes the label about the screen's left edge, as
     * cart_time_draw squeezes the clock */
    port_wide_anchor = WIDE_LEFT;
#endif
    njDrawSprite2D(&cart_time_label, 0, -2.0F, NJD_SPRITE_ALPHA);
#ifdef GCC
    port_wide_anchor = WIDE_MIDDLE;
#endif
}

void cart_time_draw(int hundredths)
{
    /* the glyphs take the two depths by turns, and no glyph shares its
     * neighbor's. the first comes again last, so a pair can start on
     * either */
    static const Float depths[3] = {-2.001F, -2.0F, -2.001F};
    struct cart_time time;
    Float x;
    Float y;
    const Float *depth;

    time = cart_time_of(hundredths);
    x = CLOCK_X;
    y = CLOCK_Y;
#ifdef GCC
    port_wide_anchor = WIDE_LEFT;
#endif
    /* @hack redundant: depths + 0, which SHC builds in R4 and saves from
     * there (MOV.L R4,@(4,R15) at 0x0c90b774); without it, SHC saves R13
     * and 2 of 62 words differ */
    x += cart_time_draw_pair(x, y, depth = depths + 0, time.minutes);
    x += cart_time_draw_colon(x, y, *depth);
    x += cart_time_draw_pair(x, y, depth = depths + 1, time.seconds);
    x += cart_time_draw_colon(x, y, *depth);
    cart_time_draw_pair(x, y, depths, time.hundredths);
#ifdef GCC
    port_wide_anchor = WIDE_MIDDLE;
#endif
}

void cart_time_draw_final(int hundredths, int best)
{
    static const NJS_ARGB yellow = {1.0F, 1.0F, 1.0F, 0.0F};
    static const NJS_ARGB white = {1.0F, 1.0F, 1.0F, 1.0F};

    if (hundredths < best) {
        njSetConstantMaterial((NJS_ARGB *)&yellow);
    }
    cart_time_draw(hundredths);
    njSetConstantMaterial((NJS_ARGB *)&white);
}

void cart_time_draw_gap(int hundredths, int best)
{
    static const Float depths[3] = {-2.001F, -2.0F, -2.001F};
    static const NJS_ARGB yellow = {1.0F, 1.0F, 1.0F, 0.0F};
    static const NJS_ARGB white = {1.0F, 1.0F, 1.0F, 1.0F};
    struct cart_time time;
    Float x;
    Float y;
    const Float *depth;

    time = cart_time_of(hundredths - best);
    x = GAP_X;
    y = GAP_Y;
    if (time.sign < 0) {
        njSetConstantMaterial((NJS_ARGB *)&yellow);
        cart_time_draw_glyph(x, y, depths[0], GLYPH_MINUS);
    } else if (time.sign > 0) {
        cart_time_draw_glyph(x, y, depths[0], GLYPH_PLUS);
    }
    x = GAP_X + GLYPH_TEXELS;
    x += cart_time_draw_pair(x, y, depth = depths + 1, time.minutes);
    x += cart_time_draw_colon(x, y, *depth);
    /* @hack redundant: depths + 0, as in cart_time_draw (MOV.L R4,@R15 at
     * 0x0c90b86c); without it, 2 of 96 words differ */
    x += cart_time_draw_pair(x, y, depth = depths + 0, time.seconds);
    x += cart_time_draw_colon(x, y, *depth);
    cart_time_draw_pair(x, y, depths + 1, time.hundredths);
    njSetConstantMaterial((NJS_ARGB *)&white);
}

/* draw number's tens at depths[0] and its units at depths[1], and return
 * their width */
static Float cart_time_draw_pair(Float x, Float y, const Float *depths,
                                 int number)
{
    cart_time_draw_glyph(x, y, depths[0], number / 10);
    /* @hack redundant: a goto to the next statement, which puts __modls's
     * address in R3 (MOV.L at 0x0c90b924); without it, R2 takes it and 2 of 39
     * words differ */
    goto units;
units:
    cart_time_draw_glyph(x + GLYPH_TEXELS, y, depths[1], number % 10);
    return 2 * GLYPH_TEXELS;
}

static Float cart_time_draw_glyph(Float x, Float y, Float depth, int glyph)
{
    /* @hack type: u1 and u2 written through casts that drop volatile, which
     * let SHC schedule them as plain stores: u1's store follows u2's first
     * shift (MOV.W R3,@R2 at 0x0c90b962). as volatile stores, 10 of 29 words
     * differ */
    *(Sint16 *)&cart_time_glyph_frame.u1 = glyph * GLYPH_TEXELS;
    *(Sint16 *)&cart_time_glyph_frame.u2 = ((glyph + 1) * GLYPH_TEXELS) - 1;
    cart_time_glyph.p.x = x;
    cart_time_glyph.p.y = y;
    njDrawSprite2D(&cart_time_glyph, 0, depth,
                   NJD_SPRITE_ALPHA | NJD_SPRITE_COLOR);
    return GLYPH_TEXELS;
}
