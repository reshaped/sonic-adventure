/* power without errno reporting */

extern float _in_logs(float x);
extern float _in_exps(float x);
extern float _in_ldexps(float x, int n);
extern const float _IFNANS;
extern const float _IFZEROS;
extern const float _IFONES;

/* classify's flags for x. x's sign bit sets CLASS_NEGATIVE, even for -0 and
 * a NaN */
enum { CLASS_NAN = 1, CLASS_INFINITE = 2, CLASS_ZERO = 4, CLASS_NEGATIVE = 8 };

#pragma inline(classify)
#pragma inline(whole)

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

/* x to the y = e to the (y log x). a zero x gives 0, or a NaN for a zero or
 * negative y */
float _in_pows(float x, float y)
{
    int x_kind = classify(x);
    int y_kind = classify(y);
    float half;

    /* @hack redundant: an else after a return, here and in the zero x's test,
     * which has the zero x's 0 share the return of y's zero test (BRA at
     * 0x8c11c006 to 0x8c11c010); without them, it shares the negative x's
     * NaN return at 0x8c11c09e, and 9 of 170 words differ */
    if ((x_kind | y_kind) & CLASS_NAN) {
        return _IFNANS;
    } else if (x_kind & CLASS_ZERO) {
        if (y_kind & (CLASS_ZERO | CLASS_NEGATIVE)) {
            return _IFNANS;
        } else {
            return _IFZEROS;
        }
    }
    if (y_kind & CLASS_ZERO) {
        return _IFONES;
    }
    /* @hack redundant: an else after each return below, which likewise has the
     * zero x's 0 share the return of y's zero test (BRA at 0x8c11c006);
     * without them, it shares the negative x's NaN return, and 5 of 170 words
     * differ */
    if (x_kind & CLASS_NEGATIVE) {
        /* a negative x needs a finite integral y, or the result is a NaN.
         * an odd y negates the result */
        if (y == whole(y) && !(y_kind & CLASS_INFINITE)) {
            x = -x;
            x = _in_exps(_in_logs(x) * y);
            half = _in_ldexps(y, -1);
            if (half != whole(half)) {
                x = -x;
            }
            return x;
        } else {
            return _IFNANS;
        }
    } else {
        return _in_exps(_in_logs(x) * y);
    }
}
