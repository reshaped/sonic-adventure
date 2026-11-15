/* a harness that runs the same calls on the SH-4, under qemu-sh4 against the
 * original assembly, and on x86 against make pc's C, and prints each result
 * as hex, so that run.sh can compare the two runs line by line */
#ifndef XBOX_TEST_HARNESS_H
#define XBOX_TEST_HARNESS_H

/* write len bytes of text to standard output */
static long harness_write(const char *text, unsigned long len)
{
#ifdef __sh__
    register long r3 __asm__("r3") = 4; /* write */
    register long r4 __asm__("r4") = 1;
    register const char *r5 __asm__("r5") = text;
    register unsigned long r6 __asm__("r6") = len;

    __asm__ volatile("trapa #0x13"
                     : "+r"(r3)
                     : "r"(r4), "r"(r5), "r"(r6)
                     : "memory");
    return r3;
#else
    long ret;

    __asm__ volatile("int $0x80"
                     : "=a"(ret)
                     : "0"(4), "b"(1), "c"(text), "d"(len)
                     : "memory");
    return ret;

#endif
}

/* set the x86 FPU as the console's: rounding toward zero, denormals flushed
 * and read as zero, every exception masked */
static void harness_fpu(void)
{
#ifndef __sh__
    unsigned int mxcsr = 0x1f80 | 0x6000 | 0x8000 | 0x40;

    __asm__ volatile("ldmxcsr %0" : : "m"(mxcsr));

#endif
}

static char harness_line[16384];
static unsigned harness_used;

static void harness_text(const char *text)
{
    while (*text != '\0' && harness_used < sizeof harness_line - 1) {
        harness_line[harness_used++] = *text++;
    }
}

static void harness_hex(unsigned value)
{
    static const char digits[] = "0123456789abcdef";
    char text[10];
    int i;

    text[0] = ' ';
    for (i = 0; i < 8; i++) {
        text[1 + i] = digits[(value >> (28 - 4 * i)) & 15];
    }
    text[9] = '\0';
    harness_text(text);
}

static void harness_float(float value)
{
    union {
        float f;
        unsigned u;
    } bits;

    bits.f = value;
    harness_hex(bits.u);
}

/* start a line naming the call and its case */
static void harness_begin(const char *name, unsigned index)
{
    harness_used = 0;
    harness_text(name);
    harness_hex(index);
}

static void harness_end(void)
{
    harness_line[harness_used++] = '\n';
    harness_write(harness_line, harness_used);
}

/* a deterministic stream of inputs */
static unsigned harness_seed = 12345;

static unsigned harness_random(void)
{
    harness_seed = harness_seed * 1103515245u + 12345u;
    return harness_seed;
}

/* a float from -range to range, with a few exact and special values mixed in */
static float harness_value(float range)
{
    unsigned pick = harness_random() >> 8;

    switch (pick % 16) {
    case 0:
        return 0.0F;
    case 1:
        return -0.0F;
    case 2:
        return 1.0F;
    case 3:
        return -1.0F;
    default:
        return ((float)(pick & 0xffff) / 32768.0F - 1.0F) * range;
    }
}


/* load XMTRX, XF0 to XF15, from m in order, and store it to m. the x86
 * run's are xmtrx.c's, over the port's XMTRX (xbox/include/sh4.h) */
#ifdef __sh__
static void harness_load_xmtrx(const float *m)
{
    __asm__ volatile("frchg\n\t"
                     "fmov.s @%0+,fr0\n\tfmov.s @%0+,fr1\n\t"
                     "fmov.s @%0+,fr2\n\tfmov.s @%0+,fr3\n\t"
                     "fmov.s @%0+,fr4\n\tfmov.s @%0+,fr5\n\t"
                     "fmov.s @%0+,fr6\n\tfmov.s @%0+,fr7\n\t"
                     "fmov.s @%0+,fr8\n\tfmov.s @%0+,fr9\n\t"
                     "fmov.s @%0+,fr10\n\tfmov.s @%0+,fr11\n\t"
                     "fmov.s @%0+,fr12\n\tfmov.s @%0+,fr13\n\t"
                     "fmov.s @%0+,fr14\n\tfmov.s @%0+,fr15\n\t"
                     "frchg"
                     : "+r"(m)
                     :
                     : "memory");
}

static void harness_store_xmtrx(float *m)
{
    float *end = m + 16;

    __asm__ volatile("frchg\n\t"
                     "fmov.s fr15,@-%0\n\tfmov.s fr14,@-%0\n\t"
                     "fmov.s fr13,@-%0\n\tfmov.s fr12,@-%0\n\t"
                     "fmov.s fr11,@-%0\n\tfmov.s fr10,@-%0\n\t"
                     "fmov.s fr9,@-%0\n\tfmov.s fr8,@-%0\n\t"
                     "fmov.s fr7,@-%0\n\tfmov.s fr6,@-%0\n\t"
                     "fmov.s fr5,@-%0\n\tfmov.s fr4,@-%0\n\t"
                     "fmov.s fr3,@-%0\n\tfmov.s fr2,@-%0\n\t"
                     "fmov.s fr1,@-%0\n\tfmov.s fr0,@-%0\n\t"
                     "frchg"
                     : "+r"(end)
                     :
                     : "memory");
}
#else
void harness_load_xmtrx(const float *m);
void harness_store_xmtrx(float *m);
#endif

#endif
