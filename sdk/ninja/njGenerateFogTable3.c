#include <kamui.h>
#include <math.h>
#include <ninja.h>
#include <sg_xpt.h>

/* with the fog density -f, fog table entry i stands for the distance
 * -f / ENTRY_DIVISOR(i) */
#define ENTRY_DIVISOR(i)                                                       \
    ((Float)(1 << ((i) >> 4)) * ((Float)((i) & 15) + 16.0F) / 16.0F)

void njGenerateFogTable3(NJS_FOG_TABLE fog, Float n, Float f)
{
    Uint32 density = 0;
    /* @hack type: exponent a byte, though frexp stores an int there. SHC gives
     * it the whole frame word at @R15, which frexp's MOV.L stores fill
     * (0x8c11c23c, 0x8c11c26e), and the code reads and writes its low byte with
     * MOV.B (0x8c616818, 0x8c616820, 0x8c6168d8); as an int, whose low byte the
     * OR takes, those three are MOV.L and 3 of 266 words differ */
    Uint8 exponent;
    Float mantissa;
    Float distance;
    Float range;
    Int i;
    Int count;

    if (n < -65535.0F) {
        density = FOG_DENSITY_65536;
    } else {
        /* the density -f, packed as njGenerateFogDensity packs it */
#ifdef GCC
        /* keeps what the tag on exponent describes: exponent takes the low
         * byte of frexp's int, which SHC's MOV.B reads. GCC gives exponent
         * one byte, which frexp's store would run past */
        int power;

        mantissa = frexp(-f, &power);
        exponent = power;
#else
        mantissa = frexp(-f, (int *)&exponent);
#endif
        exponent--;
        mantissa *= 2.0F;
        /* @hack spelling: each test takes its step from mantissa and compares
         * the difference with 0, by an FADD and an FCMP/GT (0x8c61682e,
         * 0x8c616830); as mantissa >= step, the function is 106 bytes shorter
         * and 238 of 266 words differ */
        if (mantissa - 1.0F >= 0.0F) {
            mantissa -= 1.0F;
            density |= 0x8000;
        }
        if (mantissa - 0.5F >= 0.0F) {
            mantissa -= 0.5F;
            density |= 0x4000;
        }
        if (mantissa - 0.25F >= 0.0F) {
            mantissa -= 0.25F;
            density |= 0x2000;
        }
        if (mantissa - 0.125F >= 0.0F) {
            mantissa -= 0.125F;
            density |= 0x1000;
        }
        if (mantissa - 0.0625F >= 0.0F) {
            mantissa -= 0.0625F;
            density |= 0x800;
        }
        if (mantissa - 0.03125F >= 0.0F) {
            mantissa -= 0.03125F;
            density |= 0x400;
        }
        if (mantissa - 0.015625F >= 0.0F) {
            mantissa -= 0.015625F;
            density |= 0x200;
        }
        if (mantissa - 0.0078125F >= 0.0F) {
            mantissa -= 0.0078125F;
            density |= 0x100;
        }
        density |= exponent;
    }
    kmSetFogDensity(density);

    /* where n is no farther than an entry past the last would stand, the table
     * is njGenerateFogTable's */
    if (-n <= -f / ENTRY_DIVISOR(FOG_TABLE_ENTRIES)) {
        for (i = 0; i < FOG_TABLE_ENTRIES; i++) {
            fog[i] =
                1.0F / ((Float)((1 << (i >> 4)) * ((i & 15) + 16)) / 16.0F);
        }
    } else {
        /* count the entries farther than n, which come first */
        count = 0;
        for (i = 0; i < FOG_TABLE_ENTRIES; i++) {
            distance = -f / ENTRY_DIVISOR(i);
            if (-n < distance) {
                count++;
            }
        }
        /* n becomes the depth of the first entry no farther than n, where the
         * fog reaches 0 */
        distance = -f / ENTRY_DIVISOR(count);
        /* @hack spelling: n set to the distance and then negated, which keeps
         * the loops' 15 in R4 and 1 in R5 (MOV #15,R4 at 0x8c6168fe, MOV #1,R5
         * at 0x8c6168e6); as n = -distance, the two trade and 10 of 266 words
         * differ */
        n = distance;
        n = -n;
        /* @hack spelling: i set before range, and the loop left without an
         * initializer, which keeps count in R14 and i in R7 (MOV #0,R14 at
         * 0x8c616996, MOV #0,R7 at 0x8c616998); as for (i = 0; ...), the two
         * trade, and so do 15 and 1, and 33 of 266 words differ */
        i = 0;
        range = n - f;
        for (; i < count; i++) {
            distance = -f / ENTRY_DIVISOR(i);
            fog[i] = 1.0F - ((-f - distance) / range);
        }
        for (i = count; i < FOG_TABLE_ENTRIES; i++) {
            fog[i] = 0.0F;
        }
    }
}
