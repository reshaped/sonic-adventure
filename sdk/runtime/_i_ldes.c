/* scaling by a power of 2 without errno reporting */

extern const float _IFNANS;
extern const float _IFZEROS;
extern const float _IFINFS;
extern const float _NIFZEROS;
extern const float _NIFINFS;

/* add n to x's exponent. past the exponent's range, the result is an infinity,
 * or a zero for a negative n, with x's sign */
float _in_ldexps(float x, int n)
{
    union {
        float value;
        int bits;
    } word;
    int exponent;

    word.value = x;
    exponent = word.bits & 0x7f800000;
    if (exponent == 0x7f800000) {
        if ((word.bits & 0x007fffff) == 0) {
            if ((word.bits & 0x80000000) == 0) {
                return _IFINFS;
            }
            return _NIFINFS;
        }
        return _IFNANS;
    }
    /* a zero or denormal x gives a zero with x's sign */
    if (exponent <= 0) {
        if ((word.bits & 0x80000000) == 0) {
            return _IFZEROS;
        }
        return _NIFZEROS;
    }
    exponent = (exponent >> 23) + n;
    if (exponent <= 0 || exponent >= 255) {
        if (n >= 0) {
            if ((word.bits & 0x80000000) == 0) {
                return _IFINFS;
            }
            return _NIFINFS;
        }
        if ((word.bits & 0x80000000) == 0) {
            return _IFZEROS;
        }
        return _NIFZEROS;
    }
    word.bits =
        (word.bits & 0x80000000) | (exponent << 23) | (word.bits & 0x007fffff);
    return word.value;
}
