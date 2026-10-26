/* 104 bytes of D, from 4 bytes past kinds.c's data to D's end, that this
 * build keeps here: 96 bytes, which read as eight points of three Floats,
 * then a word that holds their address, then 4 bytes, which read as their
 * count. nothing in this image or 1ST_READ.BIN names any of them but that
 * word, and their declaration and owner are unknown */
#include <sg_xpt.h>

static Uint8 dat_0c9670d8[96] = {
#include "b_egm1/dat_0c9670d8/dat_0c9670d8.inc"
};
static Uint8 *dat_0c967138 = dat_0c9670d8;
static Uint8 dat_0c96713c[4] = {
#include "b_egm1/dat_0c9670d8/dat_0c96713c.inc"
};
