/* ceiling without errno reporting */

extern const float _IFONES;

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

float _in_ceils(float x)
{
    float rounded;

    rounded = whole(x);
    /* @hack spelling: two nested tests, which move rounded to FR2 for the
     * add (FMOV FR4,FR2 and FADD FR2,FR4 at 0x8c123896 and 0x8c12389a); as
     * one test with &&, it moves to FR3, and 2 of 39 words differ */
    /* @hack type: x's sign bit read through its address, from the frame slot x
     * is stored to at entry (FMOV.S FR4,@R3 at 0x8c12385a, MOV.L @R2,R3 at
     * 0x8c123884); as a union copy, 38 of 39 words differ and the function is 8
     * bytes shorter */
    if (!(*(int *)&x & 0x80000000)) {
        if (x != rounded) {
            rounded = _IFONES + rounded;
        }
    }
    return rounded;
}
