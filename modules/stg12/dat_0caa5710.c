/* 8 bytes of bss between crash_tv.c's and waterfall.c's. nothing in the image
 * or 1ST_READ.BIN reads them */
#include <sg_xpt.h>

static Uint8 dat_0caa5710[8];
