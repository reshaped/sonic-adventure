/* ADX stereo decoder, in floating point */
#include <sg_xpt.h>
#include <sofdec.h>

/* a block: a big-endian halfword scale, then 32 4-bit codes, two to a byte,
 * high code first */
#define BLOCK_CODES 32
/* the scale's top bit ends the stream, and its low 13 bits are the gain
 * less one */
#define SCALE_END 0x8000
#define SCALE_MASK 0x1fff

/* a halfword byte swap, for a block's big-endian scale */
#define BIG_HALF(w) ((((w) >> 8) & 0xff) | (((w) << 8) & ~0xff))

/* decode as ADX_DecodeSte2 does, in single precision, on samples scaled by
 * 65536 and with each step from AdxQtblFloat0 and AdxQtblFloat1. a sample
 * past 16 bits comes out as 32767 or -32768, since FTRC, the conversion to
 * int, saturates (0x8c66d63c). the history keeps the unclamped value, where
 * ADX_DecodeSte2's keeps the clamped one */
Sint32 ADX_DecodeSteFloat(Sint8 *ibuf, Sint32 nblk, Sint16 *obufl,
                          Sint16 *delayl, Sint16 *obufr, Sint16 *delayr,
                          Sint16 k0, Sint16 k1)
{
    /* @hack spelling: the locals declared in this order, which puts c0 in FR7
     * and c1 in FR8 (FDIV FR4,FR7 at 0x8c66d5d4, FDIV FR4,FR8 at 0x8c66d5d8).
     * declared in the order they are first set, the function is 4 bytes
     * shorter and 144 of 240 words differ */
    int blk;
    int before_right;
    int before_left;
    Float step;
    Uint8 *in;
    Float x;
    Float newer_l;
    Float c1;
    Float gain;
    Float newer_r;
    Float older_l;
    int code;
    Float c0;
    int scale;
    int i;
    Float older_r;
    /* @hack redundant: register on sample, which keeps it and the code out of
     * the frame; without it, both go to the frame, the function is 24 bytes
     * longer and 237 of 240 words differ */
    register int sample;

    in = (Uint8 *)ibuf;
    newer_l = (Float)(delayl[0] << 16);
    older_l = (Float)(delayl[1] << 16);
    newer_r = (Float)(delayr[0] << 16);
    older_r = (Float)(delayr[1] << 16);
    c0 = (Float)(k0 << 4) / 65536.0F;
    c1 = (Float)(k1 << 4) / 65536.0F;

    /* before_left and before_right count the blocks decoded before this
     * pair's left block, and before its right one */
    /* @hack spelling: before_right = before_right + 2, which keeps blk at
     * @R15, before_left at @(4,R15) and before_right at @(8,R15) (0x8c66d5be,
     * 0x8c66d5ce, 0x8c66d5ca). as before_right += 2, the three take other
     * slots and 12 of 240 words differ */
    for (blk = 0, before_right = 1, before_left = 0; blk < nblk / 2;
         blk++, before_right = before_right + 2, before_left += 2) {
        scale = BIG_HALF(*(Sint16 *)in);
        if ((Sint16)scale & SCALE_END) {
            return before_left;
        }
        gain = (Float)(((Sint16)scale & SCALE_MASK) + 1);
        in += 2;
        for (i = 0; i < BLOCK_CODES; i += 2) {
            code = *in++;

            /* @hack spelling: each step through step, a local, which keeps
             * newer_l in FR9 and older_l in FR2 (FMOV FR3,FR9 at 0x8c66d58c,
             * FMOV FR3,FR2 at 0x8c66d59c). read in the sum, 80 of 240 words
             * differ */
            step = AdxQtblFloat0[code];
            x = (c0 * newer_l) + (c1 * older_l) + (gain * step);
            older_l = newer_l;
            newer_l = x;
            sample = (int)x;
            *obufl++ = sample >> 16;

            step = AdxQtblFloat1[code];
            x = (c0 * newer_l) + (c1 * older_l) + (gain * step);
            older_l = newer_l;
            newer_l = x;
            sample = (int)x;
            *obufl++ = sample >> 16;
        }

        scale = BIG_HALF(*(Sint16 *)in);
        if ((Sint16)scale & SCALE_END) {
            return before_right;
        }
        gain = (Float)(((Sint16)scale & SCALE_MASK) + 1);
        in += 2;
        for (i = 0; i < BLOCK_CODES; i += 2) {
            code = *in++;

            step = AdxQtblFloat0[code];
            x = (c0 * newer_r) + (c1 * older_r) + (gain * step);
            older_r = newer_r;
            newer_r = x;
            sample = (int)x;
            *obufr++ = sample >> 16;

            step = AdxQtblFloat1[code];
            x = (c0 * newer_r) + (c1 * older_r) + (gain * step);
            older_r = newer_r;
            newer_r = x;
            sample = (int)x;
            *obufr++ = sample >> 16;
        }
    }
    delayl[0] = (int)newer_l >> 16;
    delayl[1] = (int)older_l >> 16;
    delayr[0] = (int)newer_r >> 16;
    delayr[1] = (int)older_r >> 16;

    return nblk;
}
