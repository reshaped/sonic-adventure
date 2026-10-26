/* exponential without errno reporting */

extern float _in_ldexps(float x, int n);
extern const float _IFNANS;
extern const float _IFZEROS;
extern const float _IFINFS;
extern const float _IFONES;
extern const float _LOGE2S;
extern const float _NIFONES;

/* 2 to the f, for f in [-0.5, 0.5], is the ratio of two series in f log 2.
 * the two differ only in the signs of their odd terms. each array holds its
 * series' coefficients from the highest power */
static const float numerator[7] = {
    1.2648366e-6F, 5.3142125e-5F, 0.0010630326F, 0.012757532F,
    0.09568548F,   0.4210225F,    0.842045F,
};
static const float denominator[7] = {
    1.2648366e-6F, -5.3142125e-5F, 0.0010630326F, -0.012757532F,
    0.09568548F,   -0.4210225F,    0.842045F,
};

/* classify's flags for x. x's sign bit sets CLASS_NEGATIVE, even for -0 and
 * a NaN */
enum { CLASS_NAN = 1, CLASS_INFINITE = 2, CLASS_ZERO = 4, CLASS_NEGATIVE = 8 };

#pragma inline(classify)
#pragma inline(whole)
#pragma inline(series)

static int classify(float x)
{
    int kind = 0;
    union {
        float value;
        int bits;
    } word;

    word.value = x;
    if (word.bits < 0) {
        kind = CLASS_NEGATIVE;
    }
    if (x == 0.0F) {
        kind |= CLASS_ZERO;
    } else if ((word.bits & 0x7fffffff) == 0x7f800000) {
        kind |= CLASS_INFINITE;
    } else if (x != x) {
        kind |= CLASS_NAN;
    }
    return kind;
}

/* x truncated toward zero, with x's sign, so -0.5 gives -0. a magnitude from
 * 2 to the 31 up to 2 to the 32 saturates the conversion and gives 2 to the
 * 31 with x's sign. a larger magnitude, an infinity and a NaN come back as
 * they are */
static float whole(float x)
{
    union {
        float value;
        int bits;
    } word;
    int sign;

    word.value = x;
    sign = word.bits & 0x80000000;
    if ((word.bits & 0x7f800000) < 0x4f800000) {
        word.value = (float)(int)x;
    }
    word.bits |= sign;
    return word.value;
}

static float series(float x, const float *coefficient)
{
    float sum;
    int i;

    sum = *coefficient++;
    i = 6;
    do {
        sum = (sum * x) + *coefficient++;
    } while (--i > 0);
    return sum;
}

/* e to the x = 2 to the (x / log 2): the integral part goes into the exponent,
 * and the series gives 2 to the fraction */
float _in_exps(float x)
{
    /* @hack spelling: the constant declared in the function, which reads x into
     * FR1 and the constant into FR3 (0x8c123a66, 0x8c123a6a); declared at
     * file scope, the two trade and 7 of 160 words differ */
    static const float one_over_log_two = 1.4426951F;
    int kind = classify(x);
    int power;
    float reduced;
    float part;
    float fraction;

    /* @hack redundant: an else after a return, which puts the zero test's flag
     * in R3 (MOV #4,R3 at 0x8c123a20); without it, the flag takes R2 and 10 of
     * 160 words differ */
    if (kind & CLASS_NAN) {
        return _IFNANS;
    } else if (kind & CLASS_ZERO) {
        return _IFONES;
    }
    /* @hack type: x's exponent read through its address, from the frame slot x
     * is stored to at entry (FMOV.S FR4,@R3 at 0x8c1239dc, MOV.L @R3,R2 at
     * 0x8c123a36); as a union copy, 158 of 160 words differ */
    if (((*(int *)&x & 0x7f800000) >> 23) - 127 >= 15) {
        /* for |x| of 2 to the 15 or more, e to the x is infinite, or 0 for a
         * negative x */
        /* @hack redundant: an else after a return, which loads _IFINFS's
         * address into R0 (0x8c123a56); without it, the address takes R2 and 2
         * of 160 words differ */
        if (kind & CLASS_NEGATIVE) {
            return _IFZEROS;
        } else {
            return _IFINFS;
        }
    }
    reduced = x * one_over_log_two;
    part = whole(reduced);
    power = (int)part;
    fraction = reduced - part;
    /* a fraction of a half or more in magnitude moves 1, with its sign, into
     * the power, leaving the fraction in [-0.5, 0.5] */
    /* @hack type: fraction's exponent and sign read through its address, from
     * the frame slot it is stored to (FMOV.S FR3,@R15 and MOV.L @R3,R2 at
     * 0x8c123a9c and 0x8c123a9e) and the adjustment reloads it from; as a union
     * local, 154 of 160 words differ */
    if (((*(int *)&fraction & 0x7f800000) >> 23) - 127 >= -1) {
        if (*(int *)&fraction & 0x80000000) {
            fraction = _IFONES + fraction;
            power = power - 1;
        } else {
            fraction = _NIFONES + fraction;
            power = power + 1;
        }
    }
    reduced = _LOGE2S * fraction;
    part = series(reduced, numerator);
    part /= series(reduced, denominator);
    return _in_ldexps(part, power);
}
