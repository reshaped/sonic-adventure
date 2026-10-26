/* arc sine without errno reporting */
#include <mathf.h>

extern float _in_atans(float x);
extern float _in_ldexps(float x, int n);
extern const float _IFNANS;
extern const float _IFONES;
extern const float _HALFS;
extern const float _HLFPAIS;

/* classify's flags for x. x's sign bit sets CLASS_NEGATIVE, even for -0 and
 * a NaN */
enum { CLASS_NAN = 1, CLASS_INFINITE = 2, CLASS_ZERO = 4, CLASS_NEGATIVE = 8 };

#pragma inline(classify)

/* @hack type: an unsigned int result, for which SHC loads the negative flag
 * straight into R4 (MOV #8,R4 at 0x8c11be34); as int, the flag goes through
 * R7, kind takes R14, and 88 of _in_asins's 104 words differ */
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

/* asin x = 2 atan(x / (1 + sqrt(1 - x * x))) */
float _in_asins(float x)
{
    int kind = classify(x);
    float magnitude;
    float result;

    if (kind & CLASS_NAN) {
        /* @hack spelling: the NaN through result, for which SHC loads
         * classify's negative flag straight into R4 (MOV #8,R4 at
         * 0x8c11be34); returned as it is, the flag goes through R6, the
         * function is 4 bytes longer, and 97 of 104 words differ */
        result = _IFNANS;
        return result;
    }
    magnitude = x;
    /* @hack type: magnitude's sign bit read through its address, from the frame
     * slot it is stored to (FMOV.S FR4,@R15 and MOV.L @R3,R1 at 0x8c11be70 and
     * 0x8c11be72) and the tests below reload it from; as a union local, 97 of
     * 104 words differ and the function is 4 bytes longer */
    if (*(int *)&magnitude & 0x80000000) {
        magnitude = -magnitude;
    }
    if (magnitude > _IFONES) {
        result = _IFNANS;
    } else if (magnitude == _IFONES) {
        result = _HLFPAIS;
        if (kind & CLASS_NEGATIVE) {
            result = -result;
        }
    } else if (kind & CLASS_ZERO) {
        result = x;
    } else {
        /* tan(asin x / 2), in its second form when |x| is a half or more */
        if (_HALFS > magnitude) {
            result = x / (_IFONES + sqrtf(_IFONES - (x * x)));
        } else {
            result = (_IFONES - sqrtf(_IFONES - (x * x))) / x;
        }
        result = _in_ldexps(_in_atans(result), 1);
    }
    return result;
}
