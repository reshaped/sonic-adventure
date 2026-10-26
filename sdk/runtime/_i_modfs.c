/* integral and fractional split without errno reporting */

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

float _in_modfs(float value, float *iptr)
{
    float fraction;

    *iptr = whole(value);
    /* @hack spelling: the fraction through a local, which SHC computes in
     * FR4 and moves to FR0 last (FMOV FR4,FR0 at 0x8c12653a); returned as an
     * expression, it computes in FR0, whole's masks trade R5 and R6, and 13
     * of 26 words differ */
    fraction = value - *iptr;
    return fraction;
}
