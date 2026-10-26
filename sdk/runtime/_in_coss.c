/* cosine without errno reporting */

/* x less a whole number of y, with that number stored in *quotient */
extern float _in_fmods(float x, float y, int *quotient);
/* sine and cosine series for small arguments */
extern float _app_sins(float x);
extern float _app_coss(float x);
extern const float _IFNANS;
extern const float _QUOPAIS;
extern const float _NQUOPAIS;

/* classify's flags for x. x's sign bit sets CLASS_NEGATIVE, even for -0 and
 * a NaN */
enum { CLASS_NAN = 1, CLASS_INFINITE = 2, CLASS_ZERO = 4, CLASS_NEGATIVE = 8 };

#pragma inline(classify)

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

float _in_coss(float x)
{
    float result;
    int octant;
    int kind = classify(x);

    if (kind & (CLASS_NAN | CLASS_INFINITE)) {
        return _IFNANS;
    }
    if (kind & CLASS_NEGATIVE) {
        x = -x;
    }
    /* reduce x to [0, pi/4), keeping the octant it lay in */
    x = _in_fmods(x, _QUOPAIS, &octant);
    octant &= 7;
    if (octant & 1) {
        x = _NQUOPAIS + x;
    }
    switch (octant) {
    case 0:
    case 7:
        result = _app_coss(x);
        break;
    case 1:
    case 2:
        x = _app_sins(x);
        result = -x;
        break;
    case 3:
    case 4:
        x = _app_coss(x);
        result = -x;
        break;
    case 5:
    case 6:
        result = _app_sins(x);
        break;
    }
    return result;
}
