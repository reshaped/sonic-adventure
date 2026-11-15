/* draw/palette.src against xbox/src/draw/palette.c: each routine's output
 * over random colors. the rotation of a palette onto itself runs to a shift
 * of 129, whose last saved color lands on the kept pointer, as on the
 * console: past that, palette.src's run reaches what follows it in memory */
#include "harness.h"

typedef unsigned Uint32;

#ifndef __sh__
/* under GCC the palettes open model.h's union draw_colors */
#define draw_palettes draw_colors
#endif
Uint32 draw_palettes[8][512];
static Uint32 scratch[1024];

void fun_8c0566a0(void *pointer);
void *fun_8c0566ac(void);
Uint32 *palette_get(int palette, int offsets);
void fun_8c0566d0(Uint32 *out, Uint32 *in);
void fun_8c0566ea(Uint32 *out, Uint32 *in);
void fun_8c056704(Uint32 *out, Uint32 color);
void fun_8c056724(Uint32 *out, Uint32 color);
void palette_mix(Uint32 *out, Uint32 *from, Uint32 *to, float share);
void fun_8c0567dc(Uint32 *out, Uint32 *in, unsigned char shift);
void fun_8c056860(Uint32 *out, Uint32 *in);
void fun_8c05687a(Uint32 *out, Uint32 *in);
void fun_8c056896(void);

static void fill(void)
{
    int p;
    int k;

    for (p = 0; p < 8; p++) {
        for (k = 0; k < 512; k++) {
            draw_palettes[p][k] = harness_random();
        }
    }
    for (k = 0; k < 1024; k++) {
        scratch[k] = 0xa5a5a5a5u;
    }
}

static void put(const char *name, unsigned index, const Uint32 *at, int count)
{
    int k;

    harness_begin(name, index);
    for (k = 0; k < count; k++) {
        harness_hex(at[k]);
    }
    harness_end();
}

int main(void)
{
    static const float shares[] = {0.0F, 1.0F, 0.5F, 0.25F, 0.999F, 0.004F,
                                   -0.5F, 1.5F, 3.0F, -200.0F, 300.0F};
    unsigned i;

    harness_fpu();
    fill();
    harness_begin("get", 0);
    for (i = 0; i < 16; i++) {
        harness_hex((unsigned)((char *)palette_get((int)(i / 2), (int)(i % 2)) -
                               (char *)draw_palettes));
    }
    harness_end();
    fun_8c0566a0(&scratch[3]);
    harness_begin("kept", 0);
    harness_hex((unsigned)((char *)fun_8c0566ac() - (char *)scratch));
    harness_end();

    fun_8c0566d0(scratch, draw_palettes[1]);
    put("copy256", 0, scratch, 300);
    fun_8c0566ea(scratch, draw_palettes[2]);
    put("copy512", 0, scratch, 600);
    fun_8c0566d0(draw_palettes[3], draw_palettes[3]);
    fun_8c056704(scratch + 1, 0x12345678u);
    put("fill8", 0, scratch, 300);
    fun_8c056724(scratch + 2, 0x9abcdef0u);
    put("fill4", 0, scratch, 300);
    fun_8c056860(scratch, draw_palettes[4]);
    put("spread", 0, scratch, 520);
    fun_8c05687a(scratch, draw_palettes[5] + 1);
    put("gather", 0, scratch, 300);
    fun_8c056896();

    for (i = 0; i < sizeof shares / sizeof shares[0]; i++) {
        fill();
        palette_mix(draw_palettes[0] + 1, draw_palettes[1], draw_palettes[2] + 1,
                    shares[i]);
        put("mix", i, draw_palettes[0], 512);
    }
    palette_mix(draw_palettes[0], draw_palettes[1], draw_palettes[1], 0.5F);
    put("mix_same", 0, draw_palettes[0], 512);

    for (i = 0; i < 256; i++) {
        fill();
        fun_8c0567dc(draw_palettes[0], draw_palettes[1] + 1, (unsigned char)i);
        put("rotate", i, draw_palettes[0], 512);
    }
    for (i = 0; i <= 129; i++) {
        fill();
        fun_8c0566a0(&scratch[7]);
        fun_8c0567dc(draw_palettes[2], draw_palettes[2], (unsigned char)i);
        put("rotate_same", i, draw_palettes[2], 512);
        harness_begin("kept_after", i);
        if (i == 129) {
            /* the saved color 128, from palette entry 255 */
            harness_hex((unsigned)(unsigned long)fun_8c0566ac() ==
                        draw_palettes[2][510]);
        } else {
            harness_hex((unsigned)((char *)fun_8c0566ac() - (char *)scratch));
        }
        harness_end();
    }
    return 0;
}
