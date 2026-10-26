#include <multiprec.h>
#include <string.h>

/* store a times b, 8 bytes each, in the 16 bytes at product, all most
 * significant first */
extern void _mult(const unsigned char *a, const unsigned char *b,
                  unsigned char *product);

void _mult64(short *a_exponent, unsigned char *a, short b_exponent,
             const unsigned char *b)
{
    unsigned char product[16];

    *a_exponent = *a_exponent + b_exponent;
    _mult(a, b, product);
    if (product[0] & 0x80) {
        *a_exponent = *a_exponent + 1;
    } else {
        _lsft(product, 16);
    }
    /* fold every bit below the ninth byte's top three into the sticky bit, bit
     * 0x20 of the ninth byte */
    _setsbit(product, 16, 66);
    product[8] &= 0xe0;
    memcpy(a, product, 9);
}
