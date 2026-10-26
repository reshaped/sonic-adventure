/* a macroblock's blocks: decoded, then transformed by the inverse DCT */
#include <sg_xpt.h>

#include "movie/sdk/mpv.h"

/* the code goes in section PSGSFD00, beside the assembly it calls */
#pragma section SFD00

Sint32 MPVCDEC_IntraBlocks(struct mpv_obj *mpv)
{
    struct mpv_block_param *block;
    struct dct_fz_param *param;
    Uint8 *coded;

    MPVABDEC_EraseBlock6(mpv->coef[0]);
    param = &mpv->dct_param;
    block = &mpv->block;
    coded = param->coded;
    block->qscale = mpv->qscale;
    block->qmat = mpv->intra_qmat;
    block->nonintra = 0;
    block->dcsiz = mpvvlc_y_dcsiz;
    block->dc_pred = &mpv->dc_pred.y;
    block->coef = mpv->coef[0];
    coded[0] = MPVABDEC_IntraBlock(mpv, block);
    block->coef = mpv->coef[1];
    coded[1] = MPVABDEC_IntraBlock(mpv, block);
    block->coef = mpv->coef[2];
    coded[2] = MPVABDEC_IntraBlock(mpv, block);
    block->coef = mpv->coef[3];
    coded[3] = MPVABDEC_IntraBlock(mpv, block);
    block->dcsiz = mpvvlc_c_dcsiz;
    block->dc_pred = &mpv->dc_pred.cb;
    block->coef = mpv->coef[4];
    coded[4] = MPVABDEC_IntraBlock(mpv, block);
    block->dc_pred = &mpv->dc_pred.cr;
    block->coef = mpv->coef[5];
    coded[5] = MPVABDEC_IntraBlock(mpv, block);
    DCT_FzTrans6Blk(param);
    return 0;
}

Sint32 MPVCDEC_NintraBlocks(struct mpv_obj *mpv)
{
    struct dct_fz_param *param = &mpv->dct_param;
    Uint8 *coded = param->coded;
    struct mpv_block_param *block = &mpv->block;
    Uint32 cbp;
    int i;

    block->qscale = mpv->qscale;
    block->qmat = mpv->inter_qmat;
    block->nonintra = 1;
    /* the blocks in the stream's order, Y first, from bit 31 down */
    cbp = mpv->cbp << 2;
    param->cbp = cbp;
    for (i = 0; i < MPV_BLOCKS; i++) {
        if ((Sint32)cbp < 0) {
            block->coef = mpv->coef[i];
            coded[i] = MPVABDEC_NintraBlock(mpv, block);
        }
        cbp <<= 1;
    }
    DCT_FzTransCbp(param);
    return 0;
}

#pragma section
