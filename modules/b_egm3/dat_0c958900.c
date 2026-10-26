/* 16 zero bytes of DHEAD, the section #pragma section HEAD puts data in.
 * they lie between pvms.c's list and camera.c's tables. nothing in the image
 * points into them, so their type and owner are unknown */
#include <sg_xpt.h>

#pragma section HEAD
static Uint8 dat_0c958900[16] = {
#include "b_egm3/dat_0c958900/dat_0c958900.inc"
};
#pragma section
