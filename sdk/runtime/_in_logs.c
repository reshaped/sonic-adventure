/* natural logarithm without errno reporting */

extern const float _IFNANS;
extern const float _IFZEROS;
extern const float _IFINFS;
extern const float _IFONES;
extern const float _HALFS;
extern const float _LOGE2S;
extern const float _NIFINFS;

static const float root_two = 1.4142135623730951F;
static const float minus_root_two = -1.4142135623730951F;
/* the series' coefficients, by powers of reduced squared from the highest */
static const float coefficients[8] = {
    0.13237528502941132F, 0.153877392411232F,
    0.18181758F,          0.2222222238779068F,
    0.2857142984867096F,  0.4000000059604645F,
    0.6666666865348816F,  2.0F,
};

/* classify's flags for x. x's sign bit sets CLASS_NEGATIVE, even for -0 and
 * a NaN */
enum { CLASS_NAN = 1, CLASS_INFINITE = 2, CLASS_ZERO = 4, CLASS_NEGATIVE = 8 };

#pragma inline(classify)
#pragma inline(series)

static int classify(float x)
{
    union {
        float value;
        int bits;
    } word;
    int kind = 0;

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

static float series(float square)
{
    const float *coefficient = coefficients;
    float sum = *coefficient++;
    int i = 7;

    do {
        sum = (sum * square) + *coefficient++;
    } while (--i > 0);
    return sum;
}

float _in_logs(float x)
{
    float fraction;
    float power;
    float reduced;
    float result;
    int kind = classify(x);

    if (kind & CLASS_NAN) {
        return _IFNANS;
    } else if (kind & CLASS_ZERO) {
        return _NIFINFS;
    } else if (kind & CLASS_NEGATIVE) {
        return _IFNANS;
    } else if (kind & CLASS_INFINITE) {
        return _IFINFS;
    }
    if (x == _IFONES) {
        return _IFZEROS;
    }
    /* x is fraction times 2 to the e, with fraction in [1, 2). power is
     * (e + 0.5) log 2, the log of 2 to the e times root 2 */
    fraction = x;
    /* @hack type: fraction's bits rewritten through its address, which SHC
     * forms from R15 for each access (MOV R15,R3 at 0x8c123bcc and 0x8c123bde);
     * as a union local, 111 of 121 words differ */
    *(int *)&fraction &= 0x807fffff;
    *(int *)&fraction |= 0x3f800000;
    /* @hack type: x's exponent read through its address, from the frame slot x
     * is stored to at entry (FMOV.S FR4,@R3 at 0x8c123b52, MOV.L @R3,R0 at
     * 0x8c123bf0); as a union copy, 120 of 121 words differ */
    power = (((*(int *)&x & 0x7f800000) >> 23) - 127 + _HALFS) * _LOGE2S;
    /* log(fraction / root 2), by the series in (fraction - root 2) / (fraction
     * + root 2) */
    reduced = (fraction + minus_root_two) / (fraction + root_two);
    result = reduced * series(reduced * reduced);
    return power + result;
}
