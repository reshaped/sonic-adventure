/* the SH-4's FIPR and FTRV for the Xbox's Pentium III, as Flycast computes
 * them: each sum's four products as doubles, summed in
 * order and rounded once, under the FPSCR's rounding toward zero. the
 * Pentium III's SSE has no doubles, so the sums run on the x87, which the
 * hosts' fpu_as_console sets to 53 bits and rounding toward zero (control
 * word 0x0e7f). sh4.h takes them for its sh4_fipr and sh4_ftrv, which the
 * port's Ninja, the translated assembly and the port's C run.
 *
 * each is an asm whose FMULs take their float operand from memory: the x87
 * widens a float exactly, so a product rounds once, to 53 bits, as the C
 * spelling (double)a * b rounds it, and each FADD as that C's sum. the
 * result's store to a float rounds it toward zero, as that C's conversion
 * did. so the asm gives the C's bits (xbox/test/sh4_sse.sh checks it), but
 * the C's strict floating point (-frounding-math) moved each operand
 * through a stack slot from an SSE register and waited (FWAIT) after each
 * load. the asm takes an operand where it lies.
 *
 * sh4_test_sums 1 gives float sums instead, the tests' model (below) */
#ifndef SH4_SSE_H
#define SH4_SSE_H
#ifdef XBOX

/* GCC's vector types, which clang compiles to SSE's MULPS and MOVUPS, and
 * not xmmintrin.h's: the tree's tools read each preprocessed source, and
 * tools/floats.py can't read that header. sh4_v4u loads and stores at
 * 4-byte alignment. a lane of MULPS rounds as MULSS does, under the same
 * MXCSR, so packed single arithmetic gives the scalar's bits */
typedef float sh4_v4 __attribute__((vector_size(16)));
typedef float sh4_v4u __attribute__((vector_size(16), aligned(4)));

#define SH4_SSE_INLINE static inline __attribute__((always_inline))

/* 0 in the game (xbox/port/math.c); 1 in xbox/test/run.sh's x86 side
 * (xbox/test/libm.c), whose SH-4 side's emulate.c sums FIPR's and FTRV's
 * products as floats, its single-only doubles unable to hold Flycast's: so
 * a test's difference is its translation's. a variable, not a macro, as
 * the camera and lights tests compile make pc's preprocessed text */
extern int sh4_test_sums;

/* Flycast's FIPR: the four products as doubles, summed in order, the sum
 * rounded once */
SH4_SSE_INLINE float sh4_sse_fipr(float a0, float a1, float a2, float a3, float b0, float b1, float b2, float b3)
{
    float sum;

    if (__builtin_expect(sh4_test_sums, 0)) {
        return a0 * b0 + a1 * b1 + a2 * b2 + a3 * b3;
    }
    __asm__("flds %1\n\t"
            "fmuls %2\n\t"
            "flds %3\n\t"
            "fmuls %4\n\t"
            "faddp\n\t"
            "flds %5\n\t"
            "fmuls %6\n\t"
            "faddp\n\t"
            "flds %7\n\t"
            "fmuls %8\n\t"
            "faddp"
            : "=t"(sum)
            : "m"(a0), "m"(b0), "m"(a1), "m"(b1), "m"(a2), "m"(b2), "m"(a3), "m"(b3)
            : "st(1)");
    return sum;
}

/* Flycast's FTRV of v by the matrix whose XF0 to XF15 are xf, at any
 * alignment: out[i] is the sum over j of xf[i + 4j] * v[j], each row's
 * products as FIPR's. all four sums stay on the x87's stack until each is
 * formed, so v and xf are read whole before out is written: out may be v,
 * or a column of xf */
SH4_SSE_INLINE void sh4_sse_ftrv(const float xf[16], const float v[4], float out[4])
{
    if (__builtin_expect(sh4_test_sums, 0)) {
        float in[4], sums[4];
        int i;

        for (i = 0; i < 4; i++) {
            in[i] = v[i];
        }
        for (i = 0; i < 4; i++) {
            sums[i] = xf[i] * in[0] + xf[4 + i] * in[1] + xf[8 + i] * in[2] + xf[12 + i] * in[3];
        }
        for (i = 0; i < 4; i++) {
            out[i] = sums[i];
        }
        return;
    }
#define SH4_SSE_ROW(i)                                                                                                 \
    "flds " #i "*4(%[xf])\n\t"                                                                                         \
    "fmuls (%[v])\n\t"                                                                                                 \
    "flds " #i "*4+16(%[xf])\n\t"                                                                                      \
    "fmuls 4(%[v])\n\t"                                                                                                \
    "faddp\n\t"                                                                                                        \
    "flds " #i "*4+32(%[xf])\n\t"                                                                                      \
    "fmuls 8(%[v])\n\t"                                                                                                \
    "faddp\n\t"                                                                                                        \
    "flds " #i "*4+48(%[xf])\n\t"                                                                                      \
    "fmuls 12(%[v])\n\t"                                                                                               \
    "faddp\n\t"
    __asm__(SH4_SSE_ROW(0) SH4_SSE_ROW(1) SH4_SSE_ROW(2) SH4_SSE_ROW(3)
            "fstps 12(%[out])\n\t"
            "fstps 8(%[out])\n\t"
            "fstps 4(%[out])\n\t"
            "fstps (%[out])"
            : "=m"(*(float(*)[4])out)
            : [xf] "r"(xf), [v] "r"(v), [out] "r"(out), "m"(*(const float(*)[16])xf), "m"(*(const float(*)[4])v)
            : "st", "st(1)", "st(2)", "st(3)", "st(4)", "st(5)", "st(6)", "st(7)");
#undef SH4_SSE_ROW
}
#endif
#endif
