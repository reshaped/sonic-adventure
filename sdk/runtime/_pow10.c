#include <multiprec.h>
#include <string.h>

extern void _pow5(int n, unsigned char *bytes);

/* store 10 to the n, for n up to 27, as an 8-byte fraction and the power of
 * 2 its top bit stands for. the fraction is 5 to the n, shifted left until its
 * top bit is set, and the exponent n + 63 less the shift */
void _pow10(int n, short *exponent, unsigned char *fraction)
{
    unsigned char five[8];
    unsigned char *top;
    unsigned char mask;
    int bytes;
    int bits;

    *exponent = n + 63;
    _pow5(n, five);
    memset(fraction, 0, 8);
    bytes = 0;
    bits = 0;
    top = five;
    while (*top == 0) {
        *exponent -= 8;
        top++;
        bytes++;
    }
    mask = 0x80;
    while ((*top & mask) == 0) {
        mask >>= 1;
        bits++;
    }
    _lsfts(top, 8 - bytes, bits);
    *exponent -= bits;
    memcpy(fraction, top, 8 - bytes);
}
