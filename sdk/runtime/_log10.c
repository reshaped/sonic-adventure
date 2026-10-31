/* an estimate of a double's decimal exponent, for the number conversions */
#include <errno.h>
#include <multiprec.h>

/* 0 for a finite double, 1 for a NaN, 2 for an infinity */
extern int _duchek(long double value);

static long double split_exponent(long double value, int *exp);
static long double split_whole(long double value, long double *whole);

/* the binary exponent frexp gives value, times 0.301 and truncated toward
 * zero. 0.301 is about log10(2). value must be finite: a NaN or an infinity
 * leaves the exponent unset */
int _log10(long double value)
{
    int exp;
    long double whole;

#ifdef GCC
    /* GCC can't see that the splits set both: _xti passes no NaN or infinity,
     * and exp's product is finite */
    exp = 0;
    whole = 0.0L;
#endif
    split_exponent(value, &exp);
    split_whole(exp * 0.301L, &whole);
    return (int)whole;
}

/* frexp on the double's bits: store the binary exponent in *exp and return
 * the fraction, in [0.5, 1). a NaN sets errno to ENUM and an infinity to
 * ERANGE. either returns all ones, with *exp unset */
static long double split_exponent(long double value, int *exp)
{
    unsigned char bytes[8];
    unsigned int keep = 0x800f;
    unsigned char *at;
    int kind;

    /* @hack spelling: kind assigned in the test, which SHC tests in R0 (TST
     * R0,R0 at 0x8c127714 in 1ST_READ.BIN, 0x8ce0583c in LOADER.BIN) and copies
     * to R4 in the branch's delay slot; assigned first, kind is copied to R4
     * and tested there, _mvito4b's address and the 16 of the subnormal loop
     * trade R12 and R9, and 14 of 167 words differ in each image */
    if ((kind = _duchek(value))) {
        if (kind == 1) {
            errno = ENUM;
        }
        if (kind == 2) {
            errno = ERANGE;
        }
        /* @hack type: the all-ones result stored as value's two words through
         * unsigned long lvalues, in value's own frame slot (MOV.L R4,@R3 and
         * MOV.L R4,@(4,R2) at 0x8c12773c and 0x8c127740 in 1ST_READ.BIN,
         * 0x8ce05864 and 0x8ce05868 in LOADER.BIN); as a union local's words,
         * 100 of 167 words differ in each image */
        ((unsigned long *)&value)[0] = -1;
        ((unsigned long *)&value)[1] = -1;
        return value;
    }
    if (value == 0.0) {
        *exp = 0;
        return 0.0;
    }
    at = (unsigned char *)&value;
    /* @hack type: value's exponent halfword read through an unsigned short
     * lvalue at byte 6 of its frame slot (MOV.W @R4,R4 at 0x8c127772 in
     * 1ST_READ.BIN, 0x8ce0589a in LOADER.BIN); as a union copy read as its
     * halfwords, 122 of 167 words differ in each image and the section is 12
     * bytes longer */
    kind = *((unsigned short *)at + 3);
    *exp = (((unsigned int)kind >> 4) & 0x7ff) - 1022;
    if (!(*exp + 1022)) {
        /* a subnormal: shift the fraction left until its leading bit reaches
         * the implicit bit's place */
        ++*exp;
        /* @hack type: the exponent halfword masked in place through an unsigned
         * short lvalue (MOV.W @R3,R0 to MOV.W R0,@R3 at 0x8c1277a4 to
         * 0x8c1277a8 in 1ST_READ.BIN, 0x8ce058cc to 0x8ce058d0 in LOADER.BIN);
         * in a union copy masked and written back, 156 of 167 words differ in
         * each image and the section is 36 bytes longer */
        *((unsigned short *)at + 3) &= keep;
        /* @hack type: value's words passed to _mvito4b, which reads them
         * through unsigned long lvalues, from value's frame slot (ADD #4,R5 at
         * 0x8c12779c in 1ST_READ.BIN, 0x8ce058c4 in LOADER.BIN); as a union
         * copy's words, 122 of 167 words differ in each image and the section
         * is 12 bytes longer */
        _mvito4b(bytes, (unsigned long *)at + 1);
        _mvito4b(bytes + 4, (unsigned long *)at);
        while (!(bytes[1] & 0x10)) {
            _lsft(bytes + 1, 7);
            --*exp;
        }
        /* @hack type: the byte vector's words passed to _mvito4b, which reads
         * them through unsigned long lvalues (JSR @R12 at 0x8c127826 and
         * 0x8c127836 in 1ST_READ.BIN, 0x8ce0594e and 0x8ce0595e in LOADER.BIN);
         * with the bytes in a union with two words, 124 of 167 words differ in
         * each image and the section is 12 bytes shorter */
        _mvito4b(at + 4, (unsigned long *)bytes);
        _mvito4b(at, (unsigned long *)bytes + 1);
    }
    /* @hack spelling: 0x3fe0 through kind, which loads it into R4 ahead of
     * the mask (MOV.W at 0x8c12783c in 1ST_READ.BIN, 0x8ce05964 in
     * LOADER.BIN) and keeps _mvito4b's address in R12 and the byte vector's
     * in R10 (0x8c127798 and 0x8c127792, 0x8ce058c0 and 0x8ce058ba); as a
     * constant in the OR, the two trade and 22 of 167 words differ in each
     * image */
    kind = 0x3fe0;
    /* @hack type: the exponent field set in place through unsigned short
     * lvalues (MOV.W R2,@R3 at 0x8c127842 and 0x8c12784a in 1ST_READ.BIN,
     * 0x8ce0596a and 0x8ce05972 in LOADER.BIN); in a union copy written back
     * and returned, 139 of 167 words differ in each image and the section is 16
     * bytes longer */
    *((unsigned short *)at + 3) &= keep;
    *((unsigned short *)at + 3) |= kind;
    return value;
}

/* split value on the double's bits, as modf does, but with other results.
 * *whole gets 0 for a magnitude below 1 and value itself otherwise, which
 * _log10 truncates. the fraction returned is not modf's, and _log10 doesn't
 * read it. a NaN sets errno to ENUM and an infinity to ERANGE. either returns
 * all ones, with *whole unset */
static long double split_whole(long double value, long double *whole)
{
    int kind;
    int exp;

    /* @hack spelling: kind assigned in the test, which SHC tests in R0 (TST
     * R0,R0 at 0x8c127882 in 1ST_READ.BIN, 0x8ce059aa in LOADER.BIN) and copies
     * to R4 in the branch's delay slot; assigned first, kind is copied to R4
     * and tested there, the function is 8 bytes shorter, and 114 of 117 words
     * differ */
    if ((kind = _duchek(value))) {
        if (kind == 1) {
            errno = ENUM;
        }
        if (kind == 2) {
            errno = ERANGE;
        }
        /* @hack type: the all-ones result stored as value's two words through
         * unsigned long lvalues, in value's own frame slot (MOV.L R13,@R1 and
         * MOV.L R13,@(4,R3) at 0x8c1278a4 and 0x8c1278a8 in 1ST_READ.BIN,
         * 0x8ce059cc and 0x8ce059d0 in LOADER.BIN); as a union local's words,
         * 112 of 117 words differ in each image and the section is 24 bytes
         * longer */
        ((unsigned long *)&value)[0] = -1;
        ((unsigned long *)&value)[1] = -1;
        return value;
    }
    /* @hack type: value's exponent halfword read through an unsigned short
     * lvalue at byte 6 of its frame slot (MOV.W @(6,R4),R0 at 0x8c1278ac in
     * 1ST_READ.BIN, 0x8ce059d4 in LOADER.BIN); as a union copy read as its
     * halfwords, 113 of 117 words differ in each image and the section is 36
     * bytes longer */
    exp = ((unsigned short *)((unsigned char *)&value + 6))[0];
    exp = ((unsigned int)exp >> 4 & 0x7ff) - 1023;
    if (exp < 0) {
        *whole = 0.0;
    } else {
        *whole = value;
        if (exp < 52) {
            /* meant to clear value's fraction bits, leaving its whole part */
            /* @hack type: value's fraction words cleared in place through
             * unsigned long lvalues (MOV.L R1,@R3 and MOV.L R2,@(4,R3) at
             * 0x8c127918 and 0x8c12792c in 1ST_READ.BIN, 0x8ce05a40 and
             * 0x8ce05a54 in LOADER.BIN); in a union copy written back, 113 of
             * 117 words differ in each image, the section 48 bytes longer */
            if ((52 - exp) / 32 > 0) {
                ((unsigned long *)&value)[0] = 0;
            }
            /* @bug logic: for an exponent from 21 to 51, the fraction
             * bits to clear lie in the low word, but the mask clears the low
             * bits of the high word (AND R4,R2 and MOV.L R2,@(4,R3) at
             * 0x8c12792a and 0x8c12792c in 1ST_READ.BIN, 0x8ce05a52 and
             * 0x8ce05a54 in LOADER.BIN). the fraction returned is then wrong,
             * but _log10 doesn't read it */
            exp = -1UL << ((52 - exp) % 32);
            ((unsigned long *)&value)[1] &= exp;
        }
    }
    value -= *whole;
    if (*whole < 0.0) {
        ((unsigned char *)&value)[7] |= 0x80;
    }
    return value;
}
