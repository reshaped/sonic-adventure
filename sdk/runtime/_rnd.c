/* round the number at bytes, most significant byte first, to its first bits
 * bits. the rest rounds up when its first three bits are 101, 110 or 111, and
 * to even when they are 100. a bit of the three that falls past the byte the
 * rest starts in counts as 0. the rest itself stays as it was. it returns 1
 * for a carry out of the first byte, else 0 */
int _rnd(unsigned char *bytes, int bits)
{
    /* @hack spelling: the 8 as a variable, which SHC keeps in R6 and copies
     * to R0, the divisor, for each of the four calls to _divls and _modls
     * (MOV #8,R6 at 0x8c128070, MOV R6,R0 at 0x8c128076); as the constant 8,
     * each call loads #8 into R0 itself, and 25 of 102 words differ in each
     * image */
    int byte_bits = 8;
    int round_up = 0;
    unsigned char *cut;
    unsigned char *last;
    unsigned char cut_mask;
    unsigned char below_mask;
    unsigned char next_mask;
    unsigned char last_mask;

    cut = bytes + (bits / byte_bits);
    cut_mask = 0x80 >> (bits % byte_bits);
    below_mask = cut_mask >> 1;
    next_mask = below_mask >> 1;
    last = bytes + ((bits - 1) / byte_bits);
    last_mask = 0x80 >> ((bits - 1) % byte_bits);
    /* @hack spelling: the two empty arms, which keep the cut byte in R1 and the
     * mask in R3 (0x8c1280c2 to 0x8c1280ca); as nested tests without them, the
     * two trade and 17 of 102 words differ */
    if (!(*cut & cut_mask)) {
    } else if ((*cut & below_mask) || (*cut & next_mask)) {
        round_up = 1;
    } else if (*last & last_mask) {
        round_up = 1;
    } else {
    }
    if (round_up == 1) {
        /* @bug memory: a carry out of the first byte steps last to the
         * byte before bytes, which the loop's test reads (MOV.B @R5,R2 at
         * 0x8c128106 in 1ST_READ.BIN, 0x8ce0622e in LOADER.BIN) before it
         * tests last >= bytes (CMP/HS R4,R5 at 0x8c128110, 0x8ce06238) */
        while ((*last & last_mask) && last >= bytes) {
            *last &= ~last_mask;
            last_mask <<= 1;
            if (last_mask == 0) {
                last--;
                last_mask = 1;
            }
        }
        if (last >= bytes) {
            *last |= last_mask;
        } else {
            return 1;
        }
    }
    return 0;
}
