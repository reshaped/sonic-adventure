#include <multiprec.h>
#include <string.h>

/* 10 to the n, for n up to 27 */
extern void _pow10(int n, short *exponent, unsigned char *fraction);
/* round the number at bytes to its first bits bits, and return 1 for a carry
 * out of the top */
extern int _rnd(unsigned char *bytes, int bits);

/* the fractions of 10 to the 27 and of each square after it, up to 10 to the
 * 3456, then of their reciprocals. each is 8 bytes, most significant first,
 * with the top bit set */
static const unsigned char fractions[2][8][8] = {
    {
        {0xce, 0xcb, 0x8f, 0x27, 0xf4, 0x20, 0x0f, 0x3a}, /* 10^27 */
        {0xa7, 0x0c, 0x3c, 0x40, 0xa6, 0x4e, 0x6c, 0x52}, /* 10^54 */
        {0xda, 0x01, 0xee, 0x64, 0x1a, 0x70, 0x8d, 0xeb}, /* 10^108 */
        {0xb9, 0xa7, 0x4a, 0x06, 0x37, 0xce, 0x2e, 0xe3}, /* 10^216 */
        {0x86, 0xa3, 0x36, 0x4e, 0xa6, 0x2c, 0x67, 0x2f}, /* 10^432 */
        {0x8d, 0x9e, 0x89, 0xd1, 0x13, 0x46, 0xbd, 0xaa}, /* 10^864 */
        {0x9c, 0xb0, 0x0b, 0xfd, 0x6f, 0x02, 0x53, 0x43}, /* 10^1728 */
        {0xbf, 0xce, 0x0f, 0x5a, 0xb8, 0xa6, 0x76, 0x36}, /* 10^3456 */
    },
    {
        {0x9e, 0x74, 0xd1, 0xb7, 0x91, 0xe0, 0x7e, 0x48}, /* 10^-27 */
        {0xc4, 0x28, 0xd0, 0x5a, 0xa4, 0x75, 0x1e, 0x4c}, /* 10^-54 */
        {0x96, 0x4e, 0x85, 0x8c, 0x91, 0xba, 0x26, 0x54}, /* 10^-108 */
        {0xb0, 0x80, 0x39, 0x2c, 0xc4, 0x34, 0x9d, 0xea}, /* 10^-216 */
        {0xf3, 0x61, 0x1d, 0xad, 0x8e, 0xa3, 0x09, 0xe6}, /* 10^-432 */
        {0xe7, 0x61, 0x83, 0x2e, 0xfd, 0xc0, 0x64, 0x53}, /* 10^-864 */
        {0xd1, 0x21, 0x1f, 0xe3, 0x7a, 0xc6, 0xa1, 0x2d}, /* 10^-1728 */
        {0xaa, 0xd7, 0x1a, 0x5a, 0xab, 0x16, 0xdc, 0x40}, /* 10^-3456 */
    },
};
/* their binary exponents: the power of 2 each top bit stands for */
static const short exponents[2][8] = {
    {89, 179, 358, 717, 1435, 2870, 5740, 11480},
    {-90, -180, -359, -718, -1436, -2871, -5741, -11481},
};

/* the product of the table entry for each set bit of |n| / 27, and of 10 to
 * the remainder */
void _power(int n, short *exponent, unsigned char *fraction)
{
    int negative;
    int whole;
    int rest;
    int i;
    short scale;
    unsigned char product[9];
    /* @hack redundant: 12 bytes of frame that no instruction reads or writes.
     * without them, the frame is 28 bytes rather than 40 (ADD #-40,R15 at
     * 0x8c127f20 in 1ST_READ.BIN), and 3 of 147 words differ in each image */
    unsigned char unused[12];

    if (n >= 0) {
        negative = 0;
    } else {
        negative = 1;
        n = -n;
    }
    whole = n / 27;
    rest = n % 27;
    if (negative == 1 && rest != 0) {
        /* for a negative n, start from 10 to the -27, and multiply by 10 to the
         * (27 - rest) in place of 10 to the -rest */
        rest = 27 - rest;
        scale = exponents[1][0];
        memcpy(product, fractions[1][0], 8);
        product[8] = 0;
    } else {
        memset(product, 0, 9);
        product[0] |= 0x80;
        scale = 0;
    }
    for (i = 0; i < 8; i++) {
        if (whole & 1) {
            _mult64(&scale, product, exponents[negative][i],
                    fractions[negative][i]);
            if (_rnd(product, 64)) {
                scale++;
                product[0] |= 0x80;
            }
        }
        whole >>= 1;
    }
    if (rest != 0) {
        _pow10(rest, exponent, fraction);
        _mult64(&scale, product, *exponent, fraction);
        if (_rnd(product, 64) == 1) {
            scale++;
            product[0] |= 0x80;
        }
    }
    *exponent = scale;
    memcpy(fraction, product, 8);
}
