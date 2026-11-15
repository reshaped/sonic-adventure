/* modules/adv0100/palette.src against xbox/modules/adv0100/palette.c: the
 * ramp for every level, and the copies out and back */
#include "harness.h"

typedef unsigned Uint32;

#ifndef __sh__
/* under GCC the palettes open model.h's union draw_colors */
#define draw_palettes draw_colors
#endif
Uint32 draw_palettes[8][512];

void egg_carrier_palette_ramp(unsigned char level);
void egg_carrier_palette_read(int palette, int offsets, Uint32 *out);
void egg_carrier_palette_write(int palette, int offsets, Uint32 *in);

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
    static Uint32 buffer[300];
    unsigned i;
    int k;

    harness_fpu();
    for (i = 0; i < 256; i++) {
        for (k = 0; k < 8 * 512; k++) {
            draw_palettes[k / 512][k % 512] = harness_random();
        }
        egg_carrier_palette_ramp((unsigned char)i);
        put("ramp", i, draw_palettes[0], 1024);
        put("ramp2", i, draw_palettes[2], 8);
    }
    for (i = 0; i < 16; i++) {
        for (k = 0; k < 300; k++) {
            buffer[k] = 0xa5a5a5a5u;
        }
        egg_carrier_palette_read((int)(i / 2), (int)(i % 2), buffer);
        put("read", i, buffer, 300);
        for (k = 0; k < 256; k++) {
            buffer[k] = harness_random();
        }
        egg_carrier_palette_write((int)((i + 3) % 8), (int)(i % 2), buffer);
        put("write", i, draw_palettes[(i + 3) % 8], 512);
    }
    return 0;
}
