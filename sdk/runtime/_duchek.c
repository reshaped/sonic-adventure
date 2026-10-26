/* 0 for a finite double, 1 for a NaN, 2 for an infinity */
int _duchek(double value)
{
    /* the high word is the second in memory */
    /* @hack type: value's words read through an unsigned long cursor, in the
     * frame slot its FR5 and FR4 halves spill to (FMOV.S FR5,@R15 and FMOV.S
     * FR4,@R2 at 0x8c127df8 and 0x8c127dfa in 1ST_READ.BIN, 0x8ce05f20 and
     * 0x8ce05f22 in LOADER.BIN); as a union copy read as its words, 27 of 29
     * words differ in each image */
    unsigned long *word = (unsigned long *)&value + 1;

    if ((*word & 0x7ff00000) != 0x7ff00000) {
        return 0;
    }
    if (*word & 0x000fffff) {
        return 1;
    }
    if (*--word) {
        return 1;
    }
    return 2;
}
