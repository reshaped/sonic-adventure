/* checks xbox/include/sse/sh4_sse.h's x87 FIPR and FTRV against the C they
 * replaced, (double) products summed in order and rounded once, under the
 * modes the hosts set (MXCSR: rounding toward zero, flush to zero,
 * denormals as zero; x87: 53 bits, rounding toward zero), over random and
 * edge cases, then times both. run by xbox/test/sh4_sse.sh */
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "xbox/include/sse/sh4_sse.h"

int sh4_test_sums;

/* the C the asm replaced */
static float c_fipr(float a0, float a1, float a2, float a3, float b0, float b1, float b2, float b3)
{
    double sum = (double)a0 * b0;

    sum += (double)a1 * b1;
    sum += (double)a2 * b2;
    sum += (double)a3 * b3;
    return (float)sum;
}

static void c_ftrv(const float *xf, const float *v, float *out)
{
    float in[4], sums[4];
    int i;

    memcpy(in, v, sizeof in);
    for (i = 0; i < 4; i++) {
        double sum = (double)xf[i] * in[0];

        sum += (double)xf[4 + i] * in[1];
        sum += (double)xf[8 + i] * in[2];
        sum += (double)xf[12 + i] * in[3];
        sums[i] = (float)sum;
    }
    memcpy(out, sums, sizeof sums);
}

static unsigned seed = 12345;

static unsigned next(void)
{
    seed = seed * 1103515245u + 12345u;
    return seed >> 1;
}

/* mostly ordinary values across magnitudes, some edge cases */
static float value(void)
{
    static const unsigned edges[] = {0x00000000, 0x80000000, 0x7f800000, 0xff800000, 0x7fc00000, 0x00000001,
                                     0x807fffff, 0x7f7fffff, 0xff7fffff, 0x3f800000};
    unsigned pick = next();
    union {
        unsigned u;
        float f;
    } bits;

    if (pick % 50 == 0) {
        bits.u = edges[(pick / 50) % 10];
        return bits.f;
    }
    bits.u = (next() & 0x807fffffu) | ((100u + next() % 56u) << 23);
    return bits.f;
}

static unsigned bits_of(float f)
{
    unsigned u;

    memcpy(&u, &f, sizeof u);
    return u;
}

static int same(float a, float b)
{
    /* NaNs count as one */
    return bits_of(a) == bits_of(b) || (a != a && b != b);
}

int main(void)
{
    unsigned csr;
    unsigned short word = 0x0e7f;
    int round, k, bad = 0;
    float xf[16], v[4], o1[4], o2[4], sink = 0.0F;
    clock_t start;

    __asm__ volatile("stmxcsr %0" : "=m"(csr));
    csr |= 0x6000u | 0x8000u | 0x0040u;
    __asm__ volatile("ldmxcsr %0" : : "m"(csr));
    __asm__ volatile("fldcw %0" : : "m"(word));
    for (round = 0; round < 2000000 && bad < 10; round++) {
        float a[8];

        for (k = 0; k < 8; k++) {
            a[k] = value();
        }
        if (!same(sh4_sse_fipr(a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7]),
                  c_fipr(a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7]))) {
            printf("fipr differs at round %d\n", round);
            bad++;
        }
        for (k = 0; k < 16; k++) {
            xf[k] = value();
        }
        for (k = 0; k < 4; k++) {
            v[k] = value();
        }
        sh4_sse_ftrv(xf, v, o1);
        c_ftrv(xf, v, o2);
        for (k = 0; k < 4; k++) {
            if (!same(o1[k], o2[k])) {
                printf("ftrv differs at round %d, row %d\n", round, k);
                bad++;
            }
        }
        /* out over v, and over xf's last column */
        memcpy(o1, v, sizeof o1);
        sh4_sse_ftrv(xf, o1, o1);
        for (k = 0; k < 4; k++) {
            if (!same(o1[k], o2[k])) {
                printf("ftrv in place differs at round %d\n", round);
                bad++;
            }
        }
        {
            float m[16];

            memcpy(m, xf, sizeof m);
            sh4_sse_ftrv(m, v, &m[12]);
            for (k = 0; k < 4; k++) {
                if (!same(m[12 + k], o2[k])) {
                    printf("ftrv into its column differs at round %d\n", round);
                    bad++;
                }
            }
        }
    }
    printf("sh4_sse: %d rounds, %d differences from the C\n", round, bad);
    for (k = 0; k < 16; k++) {
        xf[k] = 0.5F + (float)k / 16.0F;
    }
    v[0] = 1.0F, v[1] = 2.0F, v[2] = 3.0F, v[3] = 1.0F;
    start = clock();
    for (round = 0; round < 10000000; round++) {
        c_ftrv(xf, v, v);
        __asm__ volatile("" : : "m"(v) : "memory");
    }
    printf("C ftrv:   %.1f ns\n", (double)(clock() - start) / CLOCKS_PER_SEC * 1e9 / round);
    start = clock();
    for (round = 0; round < 10000000; round++) {
        sh4_sse_ftrv(xf, v, v);
        __asm__ volatile("" : : "m"(v) : "memory");
    }
    printf("asm ftrv: %.1f ns\n", (double)(clock() - start) / CLOCKS_PER_SEC * 1e9 / round);
    start = clock();
    for (round = 0; round < 10000000; round++) {
        sink = c_fipr(sink, v[1], v[2], v[3], xf[0], xf[1], xf[2], xf[3]);
    }
    printf("C fipr:   %.1f ns\n", (double)(clock() - start) / CLOCKS_PER_SEC * 1e9 / round);
    start = clock();
    for (round = 0; round < 10000000; round++) {
        sink = sh4_sse_fipr(sink, v[1], v[2], v[3], xf[0], xf[1], xf[2], xf[3]);
    }
    printf("asm fipr: %.1f ns (%g)\n", (double)(clock() - start) / CLOCKS_PER_SEC * 1e9 / round, sink);
    return bad != 0;
}
