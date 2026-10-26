/* the block decoders' tables: the zigzag scan and the default intra matrix in
 * the inverse DCT's order, and the bit masks */
#include <sg_xpt.h>
#include <sofdec.h>

#include "movie/sdk/mpv.h"

/* entry i masks the low 16 - i bits */
static const Uint16 mpvbdec_masks[16] = {
    0xffff, 0x7fff, 0x3fff, 0x1fff, 0x0fff, 0x07ff, 0x03ff, 0x01ff,
    0x00ff, 0x007f, 0x003f, 0x001f, 0x000f, 0x0007, 0x0003, 0x0001,
};

/* entry i is the raster position, row * 8 + column, of the zigzag scan's
 * coefficient i */
static const Sint8 mpvbdec_zigzag_raster[64] = {
    0,  1,  8,  16, 9,  2,  3,  10, 17, 24, 32, 25, 18, 11, 4,  5,
    12, 19, 26, 33, 40, 48, 41, 34, 27, 20, 13, 6,  7,  14, 21, 28,
    35, 42, 49, 56, 57, 50, 43, 36, 29, 22, 15, 23, 30, 37, 44, 51,
    58, 59, 52, 45, 38, 31, 39, 46, 53, 60, 61, 54, 47, 55, 62, 63,
};

/* entry r is raster position r's place in the order the inverse DCT reads
 * coefficients */
static const Sint8 mpvbdec_raster_ixa[64] = {
    0,  32, 2,  33, 1,  34, 3,  35, 16, 48, 18, 49, 17, 50, 19, 51,
    8,  40, 10, 41, 9,  42, 11, 43, 20, 52, 22, 53, 21, 54, 23, 55,
    4,  36, 6,  37, 5,  38, 7,  39, 24, 56, 26, 57, 25, 58, 27, 59,
    12, 44, 14, 45, 13, 46, 15, 47, 28, 60, 30, 61, 29, 62, 31, 63,
};

/* the MPEG default intra quantizer matrix, in raster order */
static const Sint8 mpvbdec_intra_matrix[64] = {
    8,  16, 19, 22, 26, 27, 29, 34, 16, 16, 22, 24, 27, 29, 34, 37,
    19, 22, 26, 27, 29, 34, 34, 38, 22, 22, 26, 27, 29, 34, 37, 40,
    22, 26, 27, 29, 32, 35, 40, 48, 26, 27, 29, 32, 35, 40, 48, 58,
    26, 27, 29, 34, 38, 46, 56, 69, 27, 29, 35, 38, 46, 56, 69, 83,
};

Sint8 mpvbdec_zigzag[64];
Sint8 mpvbdec_dfl_iqm[64];

static void mpvbdec_SetupIxa(struct mpv_ixa *ixa);

void MPVBDEC_Init(struct mpv_ixa *ixa)
{
    int i;

    for (i = 0; i < 64; i++) {
        mpvbdec_zigzag[i] = mpvbdec_raster_ixa[mpvbdec_zigzag_raster[i]];
        mpvbdec_dfl_iqm[mpvbdec_raster_ixa[i]] = mpvbdec_intra_matrix[i];
    }
    mpvbdec_SetupIxa(ixa);
}

/* copy mpvbdec_zigzag and the masks into ixa */
static void mpvbdec_SetupIxa(struct mpv_ixa *ixa)
{
    void *dst = ixa->zigzag;

    if (dst != NULL) {
        UTY_MemcpyDword(dst, mpvbdec_zigzag, MPV_NWORD(mpvbdec_zigzag));
    }
    dst = ixa->masks;
    if (dst != NULL) {
        UTY_MemcpyDword(dst, mpvbdec_masks, MPV_NWORD(mpvbdec_masks));
    }
}
