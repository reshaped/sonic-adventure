#include <multiprec.h>

void _setsbit(unsigned char *bytes, int n, int bit)
{
    /* @hack spelling: the 8 as a variable, which SHC loads into R0, _divls's
     * divisor, ahead of the register saves (MOV #8,R0 at 0x8c12813e) and
     * copies the dividend in the call's delay slot (MOV R6,R1 at 0x8c128148);
     * as the constant 8, the two trade and 2 of 56 words differ in each
     * image */
    int byte_bits = 8;
    unsigned char mask;
    unsigned char *at;
    /* @hack redundant: register on cursor, which keeps it in R14 and the mask
     * in R13 (0x8c12814a, 0x8c128158); without it, the two trade and 11 of 56
     * words differ */
    register unsigned char *cursor;
    unsigned char below;
    unsigned char scan;

    at = bytes + (bit / byte_bits);
    cursor = at;
    mask = 0x80 >> (bit % byte_bits);
    if ((*cursor & mask) == 0) {
        scan = mask;
        for (below = 0; scan != 0; scan >>= 1) {
            below |= scan;
        }
        if ((*cursor & below) != 0) {
            *cursor |= mask;
        } else {
            for (bytes += n; ++cursor < bytes;) {
                if (*cursor != 0) {
                    *at |= mask;
                    break;
                }
            }
        }
    }
}
