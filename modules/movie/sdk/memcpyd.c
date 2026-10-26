/* Sofdec word copy, nword % 16 words first, then 16 words a pass */
#include <machine.h>
#include <sg_xpt.h>

/* @hack type: dst and src typed Uint32 *, where sofdec.h declares void *,
 * which keeps them in R4 and R5 (MOV.L @R5+,R3 and MOV.L R3,@R4 at 0x8ced51bc,
 * 0x8ced51be); as void * parameters copied to typed locals, they move to R14
 * and R1, and 54 of 54 words differ */
void UTY_MemcpyDword(Uint32 *dst, const Uint32 *src, Uint32 nword)
{
    Uint32 rest;
    Uint32 nblk;
    /* @hack redundant: register on w0 to w3, which gives nblk R2 (MOV R6,R2 at
     * 0x8ced51ca); without it, nblk takes R7, and 25 of 54 words differ */
    register Uint32 w0;
    register Uint32 w1;
    register Uint32 w2;
    register Uint32 w3;

    /* @hack spelling: rest counted from one more, and decremented before its
     * test, which SHC writes as DT R7 (0x8ced51c2); as a for loop that counts
     * down to 0, 5 of 54 words differ */
    rest = (nword & 15) + 1;
    while (--rest != 0) {
        *dst++ = *src++;
    }
    /* @hack spelling: nblk counted from one more, and decremented before its
     * test, which SHC writes as DT R2 (0x8ced5218); as a for loop that counts
     * down to 0, 37 of 54 words differ */
    nblk = (nword >> 4) + 1;
    while (--nblk != 0) {
        w0 = *src++;
        w1 = *src++;
        w2 = *src++;
        w3 = *src++;
        dst[0] = w0;
        dst[1] = w1;
        dst[2] = w2;
        dst[3] = w3;
        w0 = *src++;
        w1 = *src++;
        w2 = *src++;
        w3 = *src++;
        dst[4] = w0;
        dst[5] = w1;
        dst[6] = w2;
        dst[7] = w3;
        /* fetch the cache line that holds the next pass's first word */
        prefetch((void *)(src + 8));
        w0 = *src++;
        w1 = *src++;
        w2 = *src++;
        w3 = *src++;
        dst[8] = w0;
        dst[9] = w1;
        dst[10] = w2;
        dst[11] = w3;
        w0 = *src++;
        w1 = *src++;
        w2 = *src++;
        w3 = *src++;
        dst[12] = w0;
        dst[13] = w1;
        dst[14] = w2;
        dst[15] = w3;
        dst += 16;
    }
}
