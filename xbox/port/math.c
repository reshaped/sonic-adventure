/* the C library's float sine and cosine, which make pc's objects call and
 * no library supplies: x87's FSIN and FCOS, which compute at the x87's own
 * precision on the float loaded to the stack's top, exact to a float's for
 * the angles the game passes, well inside their 2^63 range */
float sinf(float x)
{
    float r = x;

    __asm__("fsin" : "+t"(r));
    return r;
}

float cosf(float x)
{
    float r = x;

    __asm__("fcos" : "+t"(r));
    return r;
}

#include "xbox/include/sh4.h"

/* FSCA's sine and cosine of a 16-bit angle, 65536 to a turn, as the SH-4
 * gives them: Flycast's table of the console's own (fsca-table.h, GPL-2.0
 * as the port is), the sines of the first half turn, the second half their
 * negatives, and the cosine the sine a quarter turn on. because exact sines
 * differ from the console's in their last bits, the console's table is used */

static const unsigned fsca_sines[0x8000] = {
#include "xbox/third_party/flycast/core/hw/sh4/fsca-table.h"
};

static float table_sin(unsigned angle)
{
    union {
        unsigned u;
        float f;
    } bits;

    angle &= 0xffffu;
    bits.u = fsca_sines[angle & 0x7fffu];
    /* the second half's negated as Flycast negates it, so that 0x8000 is -0 */
    return angle < 0x8000u ? bits.f : -bits.f;
}

/* sh4_sse.h's FIPR and FTRV take Flycast's sums where this is 0, as the
 * game does; xbox/test/libm.c sets it for the tests' float sums */
int sh4_test_sums;

struct sh4_sincos port_sincos16(unsigned short angle)
{
    struct sh4_sincos pair;

    pair.sin = table_sin(angle);
    pair.cos = table_sin(angle + 0x4000u);
    return pair;
}
