/* an integer times a power of 10, as a float */
#include <errno.h>
#include <multiprec.h>
#include <string.h>

#include "shared/sdk/scan.h"

/* the third bit below a float's 24-bit fraction, counting from 0 at the top.
 * _setsbit sets it where any bit below it is set */
#define STICKY_BIT 26

float _calcfpow(unsigned char *integer, int n, char range_errno)
{
    short exponent;
    unsigned char scaled[9];
    /* @hack type: fraction sized 20 bytes, of which the work uses 8, which
     * makes the frame 44 bytes (ADD #-44,R15 at 0x0cb85edc in EV001E); at 8
     * bytes, 3 of 189 words differ */
    unsigned char fraction[20];
    int zero_bytes;
    int zero_bits;
    int top_exponent = 63;
    unsigned char *top;
    unsigned char mask;

    memset(fraction, 0, 8);
    /* fraction gets integer shifted up past its leading zero_bytes and
     * zero_bits, and top_exponent the power of 2 its top bit then stands for */
    zero_bytes = 0;
    zero_bits = 0;
    for (top = integer; *top == 0; top++, zero_bytes++, top_exponent -= 8) {
    }
    for (mask = 0x80; (*top & mask) == 0; mask >>= 1, zero_bits++) {
    }
    _lsfts(top, 8 - zero_bytes, zero_bits);
    top_exponent -= zero_bits;
    memcpy(fraction, top, 8 - zero_bytes);
    if (n < 0) {
        _power(-n, &exponent, scaled);
        _div64(&exponent, scaled, top_exponent, fraction);
    } else {
        _power(n, &exponent, scaled);
        _mult64(&exponent, scaled, top_exponent, fraction);
    }
    _setsbit(scaled, 9, STICKY_BIT);
    exponent += 127;
    while ((*scaled & 0x80) == 0) {
        _lsft(scaled, 4);
        exponent--;
    }
    if (exponent <= 0) {
        switch (range_errno) {
        case RANGE_ERRNO_FLOAT:
            errno = EFLOATU;
            break;
        case RANGE_ERRNO_DOUBLE:
            errno = EDBLU;
            break;
        case RANGE_ERRNO_LONG_DOUBLE:
            errno = ELDBLU;
            break;
        case RANGE_ERRNO_RANGE:
            errno = ERANGE;
            break;
        default:
            break;
        }
        memset(scaled, 0, 4);
        exponent = 0;
    }
    if (exponent >= 255) {
        switch (range_errno) {
        case RANGE_ERRNO_FLOAT:
            errno = EFLOATO;
            break;
        case RANGE_ERRNO_DOUBLE:
            errno = EDBLO;
            break;
        case RANGE_ERRNO_LONG_DOUBLE:
            errno = ELDBLO;
            break;
        case RANGE_ERRNO_RANGE:
            errno = ERANGE;
            break;
        default:
            break;
        }
        memset(scaled, 0, 4);
        exponent = 255;
    }
    return _packf(exponent, scaled);
}
