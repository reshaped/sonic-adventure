/* 8 bytes of D, between strt2.c's data and card.c's, that this build keeps
 * here. nothing in this image or another names them, and their declaration
 * and owner are unknown: the data of an object linked between strt2.c and
 * card.c fits them, and card.c's own, starting 8 bytes early, fits them as
 * well */
#include <sg_xpt.h>

static Uint8 dat_8c1755e8[8] = {
#include "system/dat_8c1755e8/dat_8c1755e8.inc"
};
