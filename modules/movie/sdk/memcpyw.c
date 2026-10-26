/* Sofdec halfword copy, 16 halfwords a pass */
#include <sg_xpt.h>

#include "movie/sdk/sfd.h"

/* copy n halfwords from src to dst. the n % 16 past a multiple of 16 go one at
 * a time, and the rest 16 a pass */
void UTY_MemcpyWord(Uint16 *dst, const Uint16 *src, Uint32 n)
{
    Uint32 rest;
    Uint32 nblk;
    /* @hack redundant: register on h0 to h3, which leaves R12 to nblk, saved
     * on the stack (MOV.L R12,@-R15 at 0x8ced9880, MOV R6,R12 at 0x8ced9898);
     * without it, nblk takes R7, and 62 of 64 words differ */
    register Uint16 h0;
    register Uint16 h1;
    register Uint16 h2;
    register Uint16 h3;

    /* @hack spelling: rest counted from one more, and decremented before its
     * test, which SHC writes as DT R7 (0x8ced9890); as a for loop that counts
     * down to 0, 6 of 64 words differ */
    rest = (n & 15) + 1;
    while (--rest != 0) {
        *dst++ = *src++;
    }
    /* @hack spelling: nblk counted from one more, and decremented before its
     * test, which SHC writes as DT R12 (0x8ced98f8); as a for loop that counts
     * down to 0, 38 of 64 words differ */
    nblk = (n >> 4) + 1;
    while (--nblk != 0) {
        h0 = *src++;
        h1 = *src++;
        h2 = *src++;
        h3 = *src++;
        dst[0] = h0;
        dst[1] = h1;
        dst[2] = h2;
        dst[3] = h3;
        h0 = *src++;
        h1 = *src++;
        h2 = *src++;
        h3 = *src++;
        dst[4] = h0;
        dst[5] = h1;
        dst[6] = h2;
        dst[7] = h3;
        h0 = *src++;
        h1 = *src++;
        h2 = *src++;
        h3 = *src++;
        dst[8] = h0;
        dst[9] = h1;
        dst[10] = h2;
        dst[11] = h3;
        h0 = *src++;
        h1 = *src++;
        h2 = *src++;
        h3 = *src++;
        dst[12] = h0;
        dst[13] = h1;
        dst[14] = h2;
        dst[15] = h3;
        dst += 16;
    }
}
