/* MPEG video inverse DCT: its matrices and parameters, and the drivers that
 * hand each block to a kernel */
#include <sg_xpt.h>
#include <sofdec.h>

#include "movie/sdk/mpv.h"

/* Wk is cos(k pi / 16) / 2, an 8-point inverse DCT's weight. W4, 1 / (2
 * sqrt 2), also weighs coefficient 0. SHC rounds a float literal toward 0,
 * so W5 and W7 come out an ulp below the nearest floats, as 0x3e8e39d9 and
 * 0x3dc7c5c1 */
#define W1 0.4903926402F
#define W2 0.4619397663F
#define W3 0.4157348062F
#define W4 0.3535533906F
#define W5 0.2777851165F
#define W6 0.1913417162F
#define W7 0.09754516101F

/* the dense kernel stores each result x as (int)(x + BIAS + 0.5) - BIAS. the
 * cast truncates toward 0, which floors a sum of 0 or more. so for x from
 * -512.5 up, the result is x rounded to the nearest integer, halves up */
#define BIAS 512

/* each row holds one coefficient's weights. the even matrix's rows are those
 * of coefficients 0, 4, 2 and 6. the odd matrix's are those of 1, 3, 5 and 7.
 * entry r is the weight in point 2r of the 8-point line. point 7 - 2r takes
 * the same weight, negated for an odd coefficient */
static const Float dctfz_mtx_even_tbl[4][4] = {
    {W4, W4, W4, W4},
    {W4, -W4, W4, -W4},
    {W2, -W6, -W2, W6},
    {W6, W2, -W6, -W2},
};
static const Float dctfz_mtx_odd_tbl[4][4] = {
    {W1, W5, -W7, -W3},
    {W3, -W1, W5, W7},
    {W5, W7, -W3, W1},
    {W7, W3, W1, W5},
};

/* the matrices' alignment in DCT_FzInit's own storage, in bytes */
#define MTX_ALIGN 32

/* both matrices, with room to align them */
static Uint8 dctfz_mtx_area[(2 * sizeof dctfz_mtx_even_tbl) + MTX_ALIGN];
/* these are declared here, not in mpv.h. SHC places bss in the order it
 * first sees names, so mpv.h would put them ahead of dctfz_mtx_area */
Float *dctfz_p_mtx_even;
Float *dctfz_p_mtx_odd;

void DCT_FzInit(Uint8 *dct)
{
    /* @hack spelling: dctfz_p_mtx_odd and dctfz_p_mtx_even reached through
     * their addresses, which SHC loads into R13 and R14 at entry (0x8ced96e4,
     * 0x8ced96e6). through the names, SHC saves neither register, the
     * function is 6 bytes shorter, and 25 of its 26 words differ */
    Float **odd = &dctfz_p_mtx_odd;
    Float **even = &dctfz_p_mtx_even;

    if (dct == NULL) {
        dct = (Uint8 *)(((Uint32)dctfz_mtx_area + MTX_ALIGN - 1) &
                        ~(Uint32)(MTX_ALIGN - 1));
    }
    *even = (Float *)dct;
    *odd = (Float *)(dct + sizeof dctfz_mtx_even_tbl);
    /* @hack spelling: each table passed as the address of its first element, so
     * that SHC loads UTY_MemcpyDword's address into R3 for each call
     * (0x8ced96f8, 0x8ced9708); with either table passed whole, 2 of 26 words
     * differ */
    UTY_MemcpyDword(*even, &dctfz_mtx_even_tbl[0][0],
                    MPV_NWORD(dctfz_mtx_even_tbl));
    UTY_MemcpyDword(*odd, &dctfz_mtx_odd_tbl[0][0],
                    MPV_NWORD(dctfz_mtx_odd_tbl));
}

void DCT_FzInitPa(struct dct_fz_param *param)
{
    param->coded[0] = 0;
    param->coded[1] = 0;
    param->coded[2] = 0;
    param->coded[3] = 0;
    param->coded[4] = 0;
    param->coded[5] = 0;
    param->coded[6] = 0;
    param->coded[7] = 0;
    param->sparse = DCTAFZ_SparseDc;
    param->dense = DCTAFZ_DenseCbp;
    param->first_bit = 0x80000000;
    param->first_keep = 0x7fffffff;
    param->count[0] = 0;
    param->count[1] = 0;
    param->round_bias = BIAS + 0.5F;
    param->stride = 64 * sizeof(Float);
    param->cbp = 0;
    param->coef = NULL;
    param->blocks = NULL;
    param->cursor = NULL;
    param->output_bias = BIAS;
    param->unk_3c = 128;
    param->even = dctfz_p_mtx_even;
    param->odd = dctfz_p_mtx_odd;
    param->scratch = NULL;
    param->left = 0;
    param->scratch_at = NULL;
}

#pragma section SFD00

void DCT_FzTrans6Blk(struct dct_fz_param *param)
{
    Float *coef = param->coef;
    Sint16 **block = param->blocks;
    Uint32 bit = param->first_bit;
    Uint32 cbp = 0;
    Uint8 *coded = param->coded;
    Sint32 i;

    for (i = 0; i < MPV_BLOCKS; i++) {
        if (*coded == 0) {
            param->sparse(coef, *block);
        } else {
            cbp |= bit;
        }
        block++;
        bit >>= 1;
        coef = (Float *)((Uint8 *)coef + param->stride);
        coded++;
    }
    if (cbp != 0) {
        param->cbp = cbp;
        param->cursor = param->blocks;
        DCTAFZ_DenseCbp(param);
    }
}

void DCT_FzTransCbp(struct dct_fz_param *param)
{
    Float *coef = param->coef;
    Sint16 **block = param->blocks;
    Uint32 cbp = param->cbp;
    Uint32 keep = param->first_keep;
    Uint8 *coded = param->coded;
    Sint32 i;

    for (i = 0; i < MPV_BLOCKS; i++) {
        if ((Sint32)cbp < 0) {
            if (*coded == 0) {
                param->sparse(coef, *block);
                param->cbp &= keep;
            } else {
                /* @hack redundant: an empty else, which puts the stride in R3
                 * and keep in R2, where SHC shifts it (0x8ceb0c88, 0x8ceb0c92).
                 * without it, R2 and R3 trade there and in the test of
                 * param->cbp after the loop, and 7 of 63 words differ */
            }
        }
        cbp <<= 1;
        coef = (Float *)((Uint8 *)coef + param->stride);
        block++;
        coded++;
        keep = (keep >> 1) | param->first_bit;
    }
    if (param->cbp != 0) {
        param->cursor = param->blocks;
        DCTAFZ_DenseCbp(param);
    }
}

#pragma section
