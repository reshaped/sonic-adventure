#include <multiprec.h>
#include <string.h>

extern int _rsfts(unsigned char *bytes, int n, short bits);
extern int _rnd(unsigned char *bytes, int bits);

/* divide the number exponent and fraction give by 10 to the n. integer gets
 * the quotient rounded to a whole number, in 8 bytes, most significant
 * first */
void _calcnpw(short exponent, unsigned char *fraction, int n,
              unsigned char *integer)
{
    short power;
    unsigned char scaled[9];
    short shift;
    short bytes;
    short i;
    /* @hack spelling: the 8 as a variable, which SHC keeps in the frame and
     * divides by with a call to _divws (JSR @R2 at 0x8c127602); as the
     * constant 8, the division is a shift, the function is 4 bytes shorter,
     * and 105 of 111 words differ in each image */
    short byte_bits = 8;

    if (n > 0) {
        _power(n, &power, scaled);
        _div64(&power, scaled, exponent, fraction);
    } else {
        _power(-n, &power, scaled);
        _mult64(&power, scaled, exponent, fraction);
    }
    /* shift the number down to a whole number. the bits it shifts out fold
     * into the sticky bit first; the shift then moves whole bytes, and then
     * the remaining bits */
    shift = 63 - power;
    bytes = shift / byte_bits;
    _setsbit(scaled, 9, 66 - shift);
    for (i = 8; i >= bytes; --i) {
        scaled[i] = scaled[i - bytes];
    }
    for (; i >= 0; --i) {
        scaled[i] = 0;
    }
    _rsfts(scaled, 9, shift - (bytes * 8));
    /* @hack redundant: power, never read again, which keeps exponent at
     * @(4,R15) and fraction at @(8,R15) (0x8c1275b0, 0x8c1275ba); without it,
     * the two trade and 31 of 111 words differ */
    power = 63;
    _rnd(scaled, 64);
    memcpy(integer, scaled, 8);
}
