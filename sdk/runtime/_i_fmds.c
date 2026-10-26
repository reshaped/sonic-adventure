/* floating-point remainder without errno reporting */

extern const float _IFNANS;

#pragma inline(whole)

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

/* return x less a whole number of y, with x's sign, and store that number's
 * magnitude in *quotient as an unsigned int. an infinite or NaN x, or a zero
 * or NaN y, returns a NaN. an infinite y returns x. either stores a quotient
 * of 0 */
float _in_fmods(float x, float y, int *quotient)
{
    union {
        float value;
        int bits;
    } dividend, divisor, result;
    int sign;

    dividend.value = x;
    divisor.value = y;
    if ((dividend.bits & 0x7f800000) == 0x7f800000 ||
        ((divisor.bits & 0x7f800000) == 0x7f800000 &&
         (divisor.bits & 0x007fffff) != 0) ||
        divisor.value == 0.0F) {
        *quotient = 0;
        result.value = _IFNANS;
    } else if ((divisor.bits & 0x7f800000) == 0x7f800000) {
        *quotient = 0;
        result.value = x;
    } else {
        sign = dividend.bits & 0x80000000;
        dividend.bits &= 0x7fffffff;
        divisor.bits &= 0x7fffffff;
        /* @hack type: the quotient stored through an unsigned int view, for
         * which SHC converts it with bounds it reaches by MOVA (0x8c1264a6,
         * 0x8c1264ae); stored to *quotient as an int, the bound comes through
         * FPUL, the function is 8 bytes shorter, and 46 of 114 words
         * differ */
        *(unsigned int *)quotient =
            (unsigned int)whole(dividend.value / divisor.value);
        result.value =
            dividend.value - ((unsigned int)*quotient * divisor.value);
        result.bits |= sign;
    }
    return result.value;
}
