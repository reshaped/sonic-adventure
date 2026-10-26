#include <multiprec.h>
#include <string.h>

extern int _rsft(unsigned char *bytes, int n);
extern int _sub(int mode, unsigned char *to, const unsigned char *from, int n);

void _div64(short *divisor_exponent, unsigned char *divisor,
            short dividend_exponent, unsigned char *dividend)
{
    unsigned char rest[12];
    unsigned char by[12];
    unsigned char quotient[16];
    int i;

    memset(rest, 0, 4);
    memcpy(&rest[4], dividend, 8);
    memset(by, 0, 4);
    memcpy(&by[4], divisor, 8);
    memset(quotient, 0, 16);

    /* long division, one quotient bit at a time */
    for (i = 0; i < 66; i++) {
        _lsft(quotient, 12);
        if (_sub(0, rest, by, 12) >= 0) {
            quotient[11]++;
        } else {
            _add(rest, by, 12);
        }
        _lsft(rest, 12);
    }
    for (i = 0; i < 2; i++) {
        _rsft(quotient, 16);
    }

    /* a nonzero remainder sets the sticky bit, bit 0x20 of the quotient's ninth
     * byte */
    if (rest[0] || rest[1] || rest[2] || rest[3] || rest[4] || rest[5] ||
        rest[6] || rest[7] || rest[8] || rest[9] || rest[10] || rest[11]) {
        quotient[12] |= 0x20;
    }

    *divisor_exponent = dividend_exponent - *divisor_exponent;

    if (quotient[4] || quotient[5] || quotient[6] || quotient[7] ||
        quotient[8] || quotient[9] || quotient[10] || quotient[11] ||
        quotient[12]) {
        while (!(quotient[4] & 0x80)) {
            _lsft(&quotient[4], 9);
            --*divisor_exponent;
        }
    }
    memcpy(divisor, &quotient[4], 9);
}
