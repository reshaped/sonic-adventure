/* Sofdec byte copy, 32 bytes a pass */
#include <sg_xpt.h>

#include "movie/sdk/sfd.h"

/* copy n bytes from src to dst. the n % 32 past a multiple of 32 go one at a
 * time, and the rest 32 a pass */
void UTY_MemcpyByte(Uint8 *dst, const Uint8 *src, Uint32 n)
{
    Uint32 rest;
    Uint32 nblk;
    /* @hack redundant: register on b0 to b3, which leaves R12 to nblk, saved
     * on the stack (MOV.L R12,@-R15 at 0x8ced97a4, MOV R6,R12 at 0x8ced97be);
     * without it, nblk takes R7, and 107 of 109 words differ */
    register Uint8 b0;
    register Uint8 b1;
    register Uint8 b2;
    register Uint8 b3;

    /* @hack spelling: rest counted from one more, and decremented before its
     * test, which SHC writes as DT R7 (0x8ced97b4); as a for loop that counts
     * down to 0, 6 of 109 words differ */
    rest = (n & 31) + 1;
    while (--rest != 0) {
        *dst++ = *src++;
    }
    /* @hack spelling: nblk counted from one more, and decremented before its
     * test, which SHC writes as DT R12 (0x8ced9876); as a for loop that counts
     * down to 0, 71 of 109 words differ */
    nblk = (n >> 5) + 1;
    while (--nblk != 0) {
        b0 = *src++;
        b1 = *src++;
        b2 = *src++;
        b3 = *src++;
        dst[0] = b0;
        dst[1] = b1;
        dst[2] = b2;
        dst[3] = b3;
        b0 = *src++;
        b1 = *src++;
        b2 = *src++;
        b3 = *src++;
        dst[4] = b0;
        dst[5] = b1;
        dst[6] = b2;
        dst[7] = b3;
        b0 = *src++;
        b1 = *src++;
        b2 = *src++;
        b3 = *src++;
        dst[8] = b0;
        dst[9] = b1;
        dst[10] = b2;
        dst[11] = b3;
        b0 = *src++;
        b1 = *src++;
        b2 = *src++;
        b3 = *src++;
        dst[12] = b0;
        dst[13] = b1;
        dst[14] = b2;
        dst[15] = b3;
        /* an SH-4 byte store reaches at most 15 bytes past its base register,
         * so dst moves on 16 halfway through the pass */
        dst += 16;
        b0 = *src++;
        b1 = *src++;
        b2 = *src++;
        b3 = *src++;
        dst[0] = b0;
        dst[1] = b1;
        dst[2] = b2;
        dst[3] = b3;
        b0 = *src++;
        b1 = *src++;
        b2 = *src++;
        b3 = *src++;
        dst[4] = b0;
        dst[5] = b1;
        dst[6] = b2;
        dst[7] = b3;
        b0 = *src++;
        b1 = *src++;
        b2 = *src++;
        b3 = *src++;
        dst[8] = b0;
        dst[9] = b1;
        dst[10] = b2;
        dst[11] = b3;
        b0 = *src++;
        b1 = *src++;
        b2 = *src++;
        b3 = *src++;
        dst[12] = b0;
        dst[13] = b1;
        dst[14] = b2;
        dst[15] = b3;
        dst += 16;
    }
}
