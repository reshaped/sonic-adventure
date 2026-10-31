/* shift the n bytes at bytes, most significant first, right by bits, from 0
 * to 7. it returns 1 if a set bit leaves the bottom, else 0. a shift of 8
 * changes nothing and returns no value */
int _rsfts(unsigned char *bytes, int n, short bits)
{
    int i;
    int mask;
    short rest;
    unsigned char carry;
    unsigned char low;

    rest = 8 - bits;
    if (rest != 0) {
        mask = 0xff >> rest;
        carry = 0;
        for (i = 0; i < n; ++i) {
            low = *bytes & mask;
            *bytes >>= bits;
            *bytes |= carry;
            carry = low << rest;
            /* @hack spelling: bytes = bytes + 1, which gives rest R13, the
             * mask R12 and i R14 (0x8c127a00 to 0x8c127a14 in 1ST_READ.BIN,
             * 0x8ce05b28 to 0x8ce05b3c in LOADER.BIN); as ++bytes, bytes++ or
             * bytes += 1, they take R14, R11 and R13, and 12 of 42 words differ
             * in each image */
            bytes = bytes + 1;
        }
        if (carry != 0) {
            return 1;
        }
        return 0;
    }
    /* @bug uninitialized: a shift of 8 sets no result. BT at 0x8c127a06 goes
     * to the epilogue, and RTS at 0x8c127a48 returns R0 as the caller left it
     * (0x8ce05b2e and 0x8ce05b70 in LOADER.BIN). no caller passes 8 or reads
     * the result */
#ifdef GCC
    /* repairs the missing result the tag names: GCC returns 0, as for a
     * shift that moves no set bit out */
    return 0;
#endif
} /* NOLINT(clang-diagnostic-return-type) */
