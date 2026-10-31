/* arc tangent without errno reporting */

/* x less a whole number of y, with that number stored in *quotient */
extern float _in_fmods(float x, float y, int *quotient);
extern float _in_ldexps(float x, int n);
/* the arc tangent series for small arguments */
extern float _app_atans(float x);
extern const float _IFNANS;
extern const float _IFZEROS;
extern const float _IFONES;
extern const float _HLFPAIS;
extern const float _QUOPAIS;
extern const float _NIFONES;
extern const float _NHALFS;
extern const float _NQUOTS;

/* classify's flags for x. x's sign bit sets CLASS_NEGATIVE, even for -0 and
 * a NaN */
enum { CLASS_NAN = 1, CLASS_INFINITE = 2, CLASS_ZERO = 4, CLASS_NEGATIVE = 8 };

#pragma inline(classify)

/* @hack type: an unsigned int result, for which SHC loads the negative flag
 * straight into R4 (MOV #8,R4 at 0x8c123692); as int, the flag goes through
 * R7, _in_atans is 4 bytes longer, and 190 of its 200 words differ */
static unsigned int classify(float x)
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

/* an |x| of 1 or more is reduced below 1 by atan x = pi/4 + atan((x - 1) /
 * (x + 1)). the eighth of [0, 1) that the reduced x lies in picks the base for
 * the series */
float _in_atans(float x)
{
    float offset;
    float reduced;
    float result;
    float magnitude;
    int eighth;
    int kind;
    /* @hack spelling: the constants declared after the other locals, which
     * gives the 0.75 arm FMAC FR0,FR15,FR13 (0x8c1237c6); declared first, that
     * arm takes another FMOV, the function is 4 bytes longer, and 68 of 200
     * words differ */
    static const float atan_quarter = 0.24497867F;
    static const float atan_half = 0.46364761F;
    static const float atan_three_quarters = 0.64350111F;
    static const float three_quarters = 0.75F;
    static const float minus_three_quarters = -0.75F;
    static const float an_eighth = 0.125F;

    kind = classify(x);
    magnitude = x;
    /* @hack type: magnitude's sign bit, and its exponent below, read through
     * its address, from the frame slot it is stored to (FMOV.S FR4,@R15 and
     * MOV.L @R3,R1 at 0x8c1236be and 0x8c1236c0) and the reduction reloads it
     * from; as a union local, 148 of 200 words differ */
    if (*(int *)&magnitude & 0x80000000) {
        magnitude = -magnitude;
    }
    if (kind & CLASS_NAN) {
        result = _IFNANS;
    } else if (((*(int *)&magnitude & 0x7f800000) >> 23) - 127 >= 32) {
        /* for |x| of 2 to the 32 or more, pi over 2 with x's sign */
        result = _HLFPAIS;
        if (kind & CLASS_NEGATIVE) {
            result = -result;
        }
    } else if (kind & CLASS_ZERO) {
        result = x;
    } else {
        if (magnitude < _IFONES) {
            offset = _IFZEROS;
            reduced = magnitude;
        } else if (_IFONES == magnitude) {
            result = _QUOPAIS;
            if (kind & CLASS_NEGATIVE) {
                result = -result;
            }
            return result;
        } else {
            offset = _QUOPAIS;
            reduced = (_NIFONES + magnitude) / (_IFONES + magnitude);
        }
        _in_fmods(reduced, an_eighth, &eighth);
        /* atan r = atan b + atan((r - b) / (1 + r b)), for b the nearest of 0,
         * 0.25, 0.5, 0.75 and 1 */
        /* @bug uninitialized: under round to nearest, reduced rounds to 1 for
         * every magnitude from 33554432 to 4294967040, and for each multiple
         * of 4 from 16777220 to 33554428. eighth is then 8, which no case
         * takes, so result is never set: BRA at 0x8c123772 goes past every
         * case to the sign test at 0x8c1237ea, and FMOV FR5,FR0 at
         * 0x8c1237f0 returns FR5, still the 0.125 of an_eighth that
         * _in_fmods took as y, or its negation. the FPSCR the CPU resets
         * to (0x40001) rounds toward zero, which keeps reduced below 1 */
#ifdef GCC
        /* repairs the unset result the tag above names: for eighth 8, a
         * reduced of 1, GCC gives offset plus pi/4, the arc tangent of 1 */
        result = offset + _QUOPAIS;
#endif
        switch (eighth) {
        case 0:
            result = offset + _app_atans(reduced);
            break;
        case 1:
        case 2:
            result = offset + atan_quarter +
                     _app_atans((_NQUOTS + reduced) /
                                (_IFONES + _in_ldexps(reduced, -2)));
            break;
        case 3:
        case 4:
            result = offset + atan_half +
                     _app_atans((_NHALFS + reduced) /
                                (_IFONES + _in_ldexps(reduced, -1)));
            break;
        case 5:
        case 6:
            result = offset + atan_three_quarters +
                     _app_atans((reduced + minus_three_quarters) /
                                (_IFONES + (reduced * three_quarters)));
            break;
        case 7:
            result = offset + _QUOPAIS +
                     _app_atans((_NIFONES + reduced) / (_IFONES + reduced));
            break;
        }
        if (kind & CLASS_NEGATIVE) {
            result = -result;
        }
    }
    return result;
}
