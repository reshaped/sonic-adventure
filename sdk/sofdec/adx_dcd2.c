/* ADX mono and stereo decoders, in integer arithmetic */
#include <sg_xpt.h>
#include <sofdec.h>

/* a block: a big-endian halfword scale, then 32 4-bit codes, two to a byte,
 * high code first */
#define BLOCK_CODES 32
/* the scale's top bit ends the stream, and its low 13 bits are the gain
 * less one */
#define SCALE_END 0x8000
#define SCALE_MASK 0x1fff
#define SAMPLE_MAX 32767
#define SAMPLE_MIN (-32768)

/* each byte's low code as a signed step; the high code is the signed byte
 * shifted right 4 */
static const int code_step[16] = {0,  1,  2,  3,  4,  5,  6,  7,
                                  -8, -7, -6, -5, -4, -3, -2, -1};

/* decode up to nblk blocks at ibuf into BLOCK_CODES samples each at obuf,
 * stopping at a block whose scale has SCALE_END. returns the blocks decoded.
 * delay holds the last two samples, newest first, and is updated unless
 * SCALE_END stops the decoder */
Sint32 ADX_DecodeMono2(Sint8 *ibuf, Sint32 nblk, Sint16 *obuf, Sint16 *delay,
                       Sint16 k0, Sint16 k1)
{
    Sint8 *in = ibuf;
    int newer = delay[0];
    int older = delay[1];
    int blk;
    int i;
    int scale;
    int code;
    int x;
    int step;
    /* @hack spelling: gain declared after the sample's locals, which gives
     * MUL.L its operands as R9,R13 (0x8c66d280, 0x8c66d2c0). declared before
     * code, x and step, the operands swap and 2 of 125 words differ */
    int gain;

    for (blk = 0; blk < nblk; blk++) {
        /* @hack spelling: the scale swapped through i, the loop's counter,
         * which swaps it in R10, i's register (MOV R13,R10 at 0x8c66d242, MOV
         * #0,R10 at 0x8c66d262). in one expression, as ADX_DecodeSte2 swaps
         * it, the function is 4 bytes shorter and 73 of 125 words differ */
        scale = i = *(Sint16 *)in;
        i = (i >> 8) & 0xff;
        i |= (scale << 8) & ~0xff;
        scale = (Sint16)i;
        if (scale & SCALE_END) {
            return blk;
        }
        in += 2;
        gain = (scale & SCALE_MASK) + 1;
        for (i = 0; i < BLOCK_CODES; i += 2) {
            /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
            x = *in++;
            code = x;
            x >>= 4;
            x = (gain * x) + (((k0 * newer) + (k1 * older)) >> 12);
            if (x > SAMPLE_MAX) {
                *obuf = SAMPLE_MAX;
            } else if (x < SAMPLE_MIN) {
                *obuf = SAMPLE_MIN;
            } else {
                *obuf = x;
            }
            older = newer;
            newer = *obuf++;

            step = code_step[code & 15];
            x = (((k0 * newer) + (k1 * older)) >> 12) + (gain * step);
            if (x > SAMPLE_MAX) {
                *obuf = SAMPLE_MAX;
            } else if (x < SAMPLE_MIN) {
                *obuf = SAMPLE_MIN;
            } else {
                *obuf = x;
            }
            older = newer;
            newer = *obuf++;
        }
    }
    delay[0] = newer;
    delay[1] = older;
    return nblk;
}

/* as ADX_DecodeMono2 for two channels, whose blocks alternate, left first.
 * nblk counts both channels' blocks. returns the blocks decoded before an end
 * scale, or else nblk. an odd nblk leaves its last block undecoded, though
 * the return counts it */
Sint32 ADX_DecodeSte2(Sint8 *ibuf, Sint32 nblk, Sint16 *obufl, Sint16 *delayl,
                      Sint16 *obufr, Sint16 *delayr, Sint16 k0, Sint16 k1)
{
    Sint8 *in = ibuf;
    int newer_l = delayl[0];
    int older_l = delayl[1];
    int newer_r = delayr[0];
    int older_r = delayr[1];
    int blk;
    int before_left;
    int before_right;
    int i;
    int j;
    int code;
    int x;
    int step;
    int gain_l;
    int gain_r;
    Sint16 scale;

    /* before_left and before_right count the blocks decoded before this
     * pair's left block, and before its right one */
    for (blk = 0, before_right = 1, before_left = 0; blk < nblk / 2;
         blk++, before_right += 2, before_left += 2) {
        scale = (Sint16)(((*(Sint16 *)in >> 8) & 0xff) |
                         ((*(Sint16 *)in << 8) & ~0xff));
        if (scale & SCALE_END) {
            return before_left;
        }
        gain_l = (scale & SCALE_MASK) + 1;
        in += 2;
        for (i = 0; i < BLOCK_CODES; i += 2) {
            /* @hack redundant: a store to delayl[0] that never runs, which
             * puts before_left at @(28,R15) and before_right at @(32,R15)
             * (0x8c66d372, 0x8c66d36e); without it, 8 of 270 words differ */
            if (0) {
                delayl[0] = 0;
            }
            /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
            x = (code = *in++) >> 4;
            x = (gain_l * x) + (((k0 * newer_l) + (k1 * older_l)) >> 12);
            if (x > SAMPLE_MAX) {
                *obufl = SAMPLE_MAX;
            } else if (x < SAMPLE_MIN) {
                *obufl = SAMPLE_MIN;
            } else {
                *obufl = x;
            }
            older_l = newer_l;
            newer_l = *obufl++;

            step = code_step[code & 15];
            x = (((k0 * newer_l) + (k1 * older_l)) >> 12) + (gain_l * step);
            if (x > SAMPLE_MAX) {
                *obufl = SAMPLE_MAX;
            } else if (x < SAMPLE_MIN) {
                *obufl = SAMPLE_MIN;
            } else {
                *obufl = x;
            }
            older_l = newer_l;
            /* @hack spelling: newer_l loaded before obufl steps, in two
             * statements, which gives the load R2 and the loop's count R3
             * (MOV.W @R6+,R2 at 0x8c66d444, MOV.L @(8,R15),R3 at 0x8c66d448).
             * as newer_l = *obufl++, the two swap and 6 of 270 words differ */
            newer_l = *obufl;
            obufl++;
        }

        scale = (Sint16)(((*(Sint16 *)in >> 8) & 0xff) |
                         ((*(Sint16 *)in << 8) & ~0xff));
        if (scale & SCALE_END) {
            return before_right;
        }
        gain_r = (scale & SCALE_MASK) + 1;
        in += 2;
        for (j = 0; j < BLOCK_CODES; j += 2) {
            /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
            code = x = *in++;
            x >>= 4;
            x = (gain_r * x) + (((k0 * newer_r) + (k1 * older_r)) >> 12);
            if (x > SAMPLE_MAX) {
                *obufr = SAMPLE_MAX;
            } else if (x < SAMPLE_MIN) {
                *obufr = SAMPLE_MIN;
            } else {
                *obufr = x;
            }
            older_r = newer_r;
            newer_r = *obufr++;

            step = code_step[code & 15];
            x = (((k0 * newer_r) + (k1 * older_r)) >> 12) + (gain_r * step);
            if (x > SAMPLE_MAX) {
                *obufr = SAMPLE_MAX;
            } else if (x < SAMPLE_MIN) {
                *obufr = SAMPLE_MIN;
            } else {
                *obufr = x;
            }
            older_r = newer_r;
            newer_r = *obufr++;
        }
    }
    delayl[0] = newer_l;
    delayl[1] = older_l;
    delayr[0] = newer_r;
    delayr[1] = older_r;
    return nblk;
}
