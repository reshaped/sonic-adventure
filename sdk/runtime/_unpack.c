#include <multiprec.h>
#include <string.h>

/* unpack value into its sign, 1 or -1, its biased exponent, and 7 bytes of
 * fraction with the leading bit at the top. a subnormal is normalized, with an
 * exponent of 0 or below. value must not be zero, for which the normalizing
 * loop does not end */
void _unpack(double value, signed char *sign, short *exponent,
             unsigned char *fraction)
{
    unsigned short *high;
    unsigned char word[4];

    /* the double's top halfword; the high word is the second in memory */
    /* @hack type: value's sign and exponent halfword read through an unsigned
     * short lvalue in value's own frame slot, whose address also bases the
     * words below (MOV.W @R12,R2 and MOV.W @R12,R1 at 0x8c127b22 and 0x8c127b38
     * in 1ST_READ.BIN, 0x8ce05c4a and 0x8ce05c60 in LOADER.BIN); as a union
     * copy read as its halfwords, 110 of 120 words differ in each image and the
     * section is 20 bytes longer */
    high = (unsigned short *)&value + 3;
    *sign = (*high & 0x8000) == 0x8000 ? -1 : 1;
    *exponent = *high;
    *exponent = (*exponent & 0x7ff0) >> 4;
    /* @hack type: value's words passed to _mvito4b, which reads them through
     * unsigned long lvalues, from high's address less 2 and less 6 (ADD #-2,R5
     * at 0x8c127b46 in 1ST_READ.BIN, 0x8ce05c6e in LOADER.BIN); as a union
     * copy's words, 104 of 120 words differ in each image and the section is 20
     * bytes longer */
    _mvito4b(word, (const unsigned long *)(high - 1));
    memcpy(fraction, word + 1, 3);
    _mvito4b(word, (const unsigned long *)(high - 3));
    memcpy(fraction + 3, word, 4);
    /* shift the 52 fraction bits up below the top bit */
    _lsfts(fraction, 7, 3);
    if (*exponent > 0) {
        fraction[0] |= 0x80;
    } else {
        ++*exponent;
        fraction[0] &= 0x7f;
        while (!(fraction[0] & 0x80)) {
            _lsfts(fraction, 7, 1);
            --*exponent;
        }
    }
}
