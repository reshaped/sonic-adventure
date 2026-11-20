/* fast.c against math.c, the reference it stands in for, in the build this
 * test links. every FSCA angle runs first, then every float of [1, 4)
 * through FSRRA and FSQRT. those floats hold each mantissa under an even
 * exponent and an odd one. seeded operands follow, of each kind in
 * operand's table, in each of the FPU's four rounding modes. on x86 each
 * mode runs again with results flushed to zero, as engine/native/runner.c
 * sets MXCSR. each runs a third time with operands flushed too. FSQRT and
 * the default build's FSRRA take a first guess from the FPU. no mode may
 * move their bits. given a count, the twelve seeded passes share that many
 * cases, and FTRV takes half as many. given "every", the test runs all 2^32
 * floats through FSRRA and FSQRT */
#include "play_math.h"

#include <fenv.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef __SSE__
#include <xmmintrin.h>

/* MXCSR's flush bits: none, results (FTZ), and results and operands (DAZ) */
static const unsigned int flushes[3] = {0, 0x8000U, 0x8040U};
#endif

float ref_fsqrt(float a);
float ref_fmac(float f0, float fm, float fn);
float ref_fipr(const float a[4], const float b[4]);
void ref_ftrv(const float m[16], const float v[4], float out[4]);
void ref_fsca(uint32_t angle, float out[2]);
float ref_fsrra(float a);

static uint64_t seed = 0x9e3779b97f4a7c15ULL;
static unsigned long failed;

static uint64_t next(void)
{
    seed ^= seed << 13;
    seed ^= seed >> 7;
    seed ^= seed << 17;
    return seed;
}

static uint32_t bits_of(float value)
{
    uint32_t bits;

    memcpy(&bits, &value, sizeof bits);
    return bits;
}

static float float_of(uint32_t bits)
{
    float value;

    memcpy(&value, &bits, sizeof value);
    return value;
}

/* a float of one kind of operand: an exponent's range, and how much of the
 * mantissa it keeps */
static float operand(unsigned int kind)
{
    static const struct {
        uint32_t base, range, mantissa;
    } kinds[] = {
        {0, 256, 0x7fffff}, /* any bits */
        {1, 254, 0x7fffff}, /* any normal */
        {112, 32, 0x7fffff}, /* the ranges play holds */
        {120, 16, 0x7fffff}, /* close together, so sums cancel */
        {127, 4, 0x700000}, /* few bits, so sums land exact */
        {60, 12, 0x7fffff}, /* products at flush */
        {190, 8, 0x7fffff}, /* products at overflow */
        {120, 16, 0x7fffff}, /* zeros, below */
    };
    uint64_t random = next();
    const uint32_t mantissa = (uint32_t)random & kinds[kind].mantissa;
    uint32_t exponent =
        kinds[kind].base + (uint32_t)(random >> 32) % kinds[kind].range;

    if (kind == 7 && random >> 61 < 3) {
        return float_of((uint32_t)random & 0x80000000U);
    }
    return float_of(((uint32_t)random & 0x80000000U) | exponent << 23 |
                    mantissa);
}

static void differ(const char *name, const float *in, int ins, uint32_t fast,
                   uint32_t reference)
{
    int i;

    if (failed++ < 8) {
        printf("%-5s", name);
        for (i = 0; i < ins; i++) {
            printf(" %08x", bits_of(in[i]));
        }
        printf(" gave %08x, the reference's %08x\n", fast, reference);
    }
}

static void unary(float a)
{
    uint32_t fast = bits_of(sa_fsrra(a));
    uint32_t reference = bits_of(ref_fsrra(a));

    if (fast != reference) {
        differ("FSRRA", &a, 1, fast, reference);
    }
    fast = bits_of(sa_fsqrt(a));
    reference = bits_of(ref_fsqrt(a));
    if (fast != reference) {
        differ("FSQRT", &a, 1, fast, reference);
    }
}

/* each of FSCA's 65536 angles twice, under two high halves it ignores */
static void every_angle(void)
{
    uint32_t angle;

    for (angle = 0; angle < 0x20000; angle++) {
        uint32_t high = (angle * 0x9e3779b9U) & 0xffff0000U;
        uint32_t word = (angle & 0xffff) | high;
        float in = float_of(word);
        float fast[2];
        float reference[2];
        int i;

        sa_fsca(word, fast);
        ref_fsca(word, reference);
        for (i = 0; i < 2; i++) {
            if (bits_of(fast[i]) != bits_of(reference[i])) {
                differ("FSCA", &in, 1, bits_of(fast[i]), bits_of(reference[i]));
            }
        }
    }
}

/* make the first two products all but cancel. a[1] and b[1] become a[0]
 * and -b[0], or -a[0] and b[0], by chance. b[1] then moves by -3 to 4 units
 * in its last place */
static void cancel(float a[4], float b[4])
{
    uint32_t x = bits_of(a[0]);
    uint32_t y = bits_of(b[0]);
    uint32_t sign = (uint32_t)(next() & 1) << 31;

    a[1] = float_of(x ^ sign);
    b[1] = float_of(((y ^ 0x80000000U ^ sign) + (uint32_t)(next() & 7)) - 3);
}

/* a matrix turned about z and moved within the ranges play holds. v holds a
 * second turn's cosine and sine, zeros elsewhere, as Ninja's rotations give
 * FTRV */
static void turned(float m[16], float v[4])
{
    float one[2];
    float two[2];
    int at = (int)(next() % 3);
    int i;

    sa_fsca((uint32_t)next(), one);
    sa_fsca((uint32_t)next(), two);
    memset(m, 0, 16 * sizeof m[0]);
    m[0] = one[1];
    m[1] = one[0];
    m[4] = -one[0];
    m[5] = one[1];
    m[10] = 1.0F;
    m[15] = 1.0F;
    for (i = 12; i < 15; i++) {
        m[i] = operand(2);
    }
    memset(v, 0, 4 * sizeof v[0]);
    v[at] = two[1];
    v[(at + 1) % 3] = two[0];
}

static void compare_ftrv(const float *m, const float *v, const float *in)
{
    float fast[4];
    float reference[4];
    int k;

    sa_ftrv(m, v, fast);
    ref_ftrv(m, v, reference);
    for (k = 0; k < 4; k++) {
        if (bits_of(fast[k]) != bits_of(reference[k])) {
            differ("FTRV", in, 24, bits_of(fast[k]), bits_of(reference[k]));
        }
    }
}

static void products(unsigned long count)
{
    unsigned long i;

    for (i = 0; i < count; i++) {
        unsigned int kind = (unsigned int)(i & 7);
        float in[8 + 16];
        float *a = in;
        float *b = in + 4;
        float *m = in + 8;
        float fast;
        float reference;
        int k;

        for (k = 0; k < 8; k++) {
            in[k] = operand(kind);
        }
        if (i >> 3 & 1) {
            cancel(a, b);
        }
        fast = sa_fipr(a, b);
        reference = ref_fipr(a, b);
        if (bits_of(fast) != bits_of(reference)) {
            differ("FIPR", in, 8, bits_of(fast), bits_of(reference));
        }
        fast = sa_fmac(a[0], a[1], a[2]);
        reference = ref_fmac(a[0], a[1], a[2]);
        if (bits_of(fast) != bits_of(reference)) {
            differ("FMAC", in, 3, bits_of(fast), bits_of(reference));
        }
        unary(a[3]);
        if (i & 3) {
            continue;
        }
        /* FTRV takes one case in four, and its operands' kind steps through
         * all eight over those */
        kind = (unsigned int)(i >> 2 & 7);
        for (k = 0; k < 4; k++) {
            a[k] = operand(kind);
        }
        for (k = 0; k < 16; k++) {
            m[k] = operand(kind);
        }
        compare_ftrv(m, a, in);
        turned(m, a);
        compare_ftrv(m, a, in);
    }
}

int main(int argc, char **argv)
{
    static const int modes[] = {FE_TOWARDZERO, FE_TONEAREST, FE_UPWARD,
                                FE_DOWNWARD};
    /* the seeded passes: each of the four rounding modes, under each of
     * MXCSR's three flush settings */
    enum { PASSES = 12 };
    unsigned long count = 1UL << 23;
    uint32_t bits;
    unsigned int i;

    if (argc > 1 && strcmp(argv[1], "every") == 0) {
        bits = 0;
        do {
            unary(float_of(bits));
        } while (++bits != 0);
        printf("%lu differ of 2^32\n", failed);
        return failed != 0;
    }
    if (argc > 1) {
        count = strtoul(argv[1], NULL, 0);
    }
    every_angle();
    for (bits = 0x3f800000U; bits < 0x40800000U; bits++) {
        unary(float_of(bits));
    }
    for (i = 0; i < PASSES; i++) {
        fesetround(modes[i % 4]);
#ifdef __SSE__
        _mm_setcsr((_mm_getcsr() & ~0x8040U) | flushes[i / 4]);
#endif
        products(count / PASSES);
    }
    if (failed != 0) {
        printf("%lu differ\n", failed);
    }
    return failed != 0;
}
