#include <multiprec.h>

int _lsfts(unsigned char *bytes, int n, short bits)
{
    short rest;
    int mask;
    int i;
    unsigned char carry;
    unsigned char high;

    rest = 8 - bits;
    if (rest != 0) {
        mask = 0xff << rest;
        carry = 0;
        for (i = n - 1; i >= 0; i--) {
            high = bytes[i] & mask;
            bytes[i] <<= bits;
            bytes[i] |= carry;
            carry = high >> rest;
        }
        if (carry != 0) {
            return 1;
        }
        return 0;
    }
    /* @bug uninitialized: a shift of 8 sets no result. BT at 0x8c12798e goes
     * to the epilogue, and RTS at 0x8c1279cc returns R0 as the caller left it
     * (0x8ce05ab6 and 0x8ce05af4 in LOADER.BIN). no caller passes 8 or reads
     * the result */
} /* NOLINT(clang-diagnostic-return-type) */
