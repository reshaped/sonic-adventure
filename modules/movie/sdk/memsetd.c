/* Sofdec word fill, 16 words a pass, from the end down */
#include <sg_xpt.h>
#include <sofdec.h>

/* fill nword words at dst with value, from the end down. the nword % 16 at the
 * end go one at a time, and the rest 16 a pass */
void UTY_MemsetDword(void *dst, Uint32 value, Uint32 nword)
{
    Uint32 *end = (Uint32 *)dst + nword;
    Uint32 rest;

    /* @hack spelling: rest counted from one more, and decremented before its
     * test, which SHC writes as DT R7 (0x8ced5230); as a for loop that counts
     * down to 0, 29 of 35 words differ */
    rest = (nword & 15) + 1;
    while (--rest != 0) {
        *--end = value;
    }
    /* @hack spelling: nword, reused as the pass count, which counts in R0 (MOV
     * R6,R0 and DT R0 at 0x8ced5238, 0x8ced525e); with a count of its own, it
     * counts in R7, and 3 of 35 words differ */
    /* @hack spelling: nword counted from one more, and decremented before its
     * test, which SHC writes as DT R0 (0x8ced525e); as a for loop that counts
     * down to 0, 9 of 35 words differ */
    nword = (nword >> 4) + 1;
    while (--nword != 0) {
        *--end = value;
        *--end = value;
        *--end = value;
        *--end = value;
        *--end = value;
        *--end = value;
        *--end = value;
        *--end = value;
        *--end = value;
        *--end = value;
        *--end = value;
        *--end = value;
        *--end = value;
        *--end = value;
        *--end = value;
        *--end = value;
    }
}
