#include <multiprec.h>
#include <string.h>

/* _mvsto2b stores an unsigned short as two bytes, most significant first */
extern void _mvsto2b(unsigned char *bytes, const unsigned short *half);

/* store a times b in the 16 bytes at product. a and b are 8 bytes each, and
 * all three are most significant first */
void _mult(const unsigned char *a, const unsigned char *b,
           unsigned char *product)
{
    short i;
    short j;
    unsigned short partial;
    unsigned char row[16];
    unsigned short term;

    memset(product, 0, 16);
    for (i = 7; i >= 0; --i) {
        if (b[i] != 0) {
            /* row = a times b[i], at b[i]'s place, one byte of a at a time */
            memset(row, 0, 16);
            for (j = 7; j >= 0; --j) {
                term = a[j] * b[i];
                partial = *(row + i + j + 1);
                partial += term;
                _mvsto2b(row + i + j, &partial);
            }
            if (i == 7) {
                memcpy(product + i, row + i, 9);
            } else {
                _add(product + i, row + i, 9);
            }
        }
    }
}
