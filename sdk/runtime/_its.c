#include <string.h>

/* multiple-precision arithmetic on numbers of n bytes, most significant
 * first */
extern int _sub(int mode, unsigned char *to, const unsigned char *from, int n);
extern int _rsfts(unsigned char *bytes, int n, short bits);
extern int _umemcmp(const unsigned char *s1, const unsigned char *s2, int n);

/* 8 times 10 to each place from 0 to 16, most significant byte first */
static const unsigned char eights[17][8] = {
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08}, /* 8 * 10^0 */
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x50}, /* 8 * 10^1 */
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x20}, /* 8 * 10^2 */
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0x40}, /* 8 * 10^3 */
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x38, 0x80}, /* 8 * 10^4 */
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x0c, 0x35, 0x00}, /* 8 * 10^5 */
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x7a, 0x12, 0x00}, /* 8 * 10^6 */
    {0x00, 0x00, 0x00, 0x00, 0x04, 0xc4, 0xb4, 0x00}, /* 8 * 10^7 */
    {0x00, 0x00, 0x00, 0x00, 0x2f, 0xaf, 0x08, 0x00}, /* 8 * 10^8 */
    {0x00, 0x00, 0x00, 0x01, 0xdc, 0xd6, 0x50, 0x00}, /* 8 * 10^9 */
    {0x00, 0x00, 0x00, 0x12, 0xa0, 0x5f, 0x20, 0x00}, /* 8 * 10^10 */
    {0x00, 0x00, 0x00, 0xba, 0x43, 0xb7, 0x40, 0x00}, /* 8 * 10^11 */
    {0x00, 0x00, 0x07, 0x46, 0xa5, 0x28, 0x80, 0x00}, /* 8 * 10^12 */
    {0x00, 0x00, 0x48, 0xc2, 0x73, 0x95, 0x00, 0x00}, /* 8 * 10^13 */
    {0x00, 0x02, 0xd7, 0x98, 0x83, 0xd2, 0x00, 0x00}, /* 8 * 10^14 */
    {0x00, 0x1c, 0x6b, 0xf5, 0x26, 0x34, 0x00, 0x00}, /* 8 * 10^15 */
    {0x01, 0x1c, 0x37, 0x93, 0x7e, 0x08, 0x00, 0x00}, /* 8 * 10^16 */
};

/* write value as n decimal digits, the least significant first, and a null
 * after them. value is 8 bytes, most significant first, below 10 to the n,
 * and n is at most 17 */
void _its(unsigned char *value, char *digits, int n)
{
    int bit;
    int place;
    int digit;
    int remains;
    int weight;
    int skip;
    char *out;
    unsigned char rest[8];
    unsigned char step[8];

    remains = 1;
    out = digits + n;
    *out = 0;
    memcpy(rest, value, 8);
    for (place = n - 1; place >= 0; place--) {
        /* skip leading bytes known to be zero in 8 times 10 to the place */
        skip = 8 - ((((place + 1) / 5) + 1) * 2);
        memcpy(step + skip, eights[place] + skip, 8 - skip);
        /* find the digit a bit at a time, by subtracting 8, 4, 2 and 1 times 10
         * to the place */
        digit = 0;
        weight = 8;
        for (bit = 0; bit < 4; bit++) {
            if (_umemcmp(rest + skip, step + skip, 8 - skip) >= 0) {
                remains = _sub(0, rest + skip, step + skip, 8 - skip);
                digit += weight;
            }
            weight /= 2;
            if (weight == 0) {
                break;
            }
            _rsfts(step + skip, 8 - skip, 1);
        }
        if (remains == 0) {
            /* the last subtraction left zero, so every remaining digit is 0 */
            *--out = digit + '0';
            while (--out >= digits) {
                *out = '0';
            }
            return;
        }
        *--out = digit + '0';
    }
}
