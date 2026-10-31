#include <math.h>
#include <ninja.h>
#include <sg_xpt.h>

Uint32 njGenerateFogDensity(Float f)
{
    Uint32 density = 0;
    /* @hack type: exponent a byte, though frexp stores an int there. SHC gives
     * it the whole frame word at @R15, which frexp's MOV.L stores fill
     * (0x8c11c23c, 0x8c11c26e), and the code reads and writes its low byte with
     * MOV.B (0x8c616a7c, 0x8c616a84, 0x8c616b3c); as an int, whose low byte the
     * OR takes, those three are MOV.L and 3 of 125 words differ */
    Uint8 exponent;
    Float mantissa;

    if (f < -65536.0F) {
        density = FOG_DENSITY_65536;
    } else {
        /* for f below 0, frexp gives a mantissa in [0.5, 1), and the fog
         * density register takes one in [1, 2) */
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
         * the difference with 0, by an FADD and an FCMP/GT (0x8c616a90,
         * 0x8c616a92); as mantissa >= step, the function is 100 bytes shorter
         * and 104 of 125 words differ */
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
    return density;
}
