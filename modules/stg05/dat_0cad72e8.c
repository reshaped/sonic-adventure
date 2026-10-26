/* 112 bytes of D, from 4 bytes past erupt.c's data to rock_burst.c's, that
 * this build keeps here. nothing in this image or 1ST_READ.BIN names them,
 * and their declaration and owner are unknown. the last 60 read as a struct
 * dust_params, as erupt.c's last datum and rock_burst.c's first are */
#include <sg_xpt.h>

static Uint8 dat_0cad72e8[112] = {
#include "stg05/dat_0cad72e8/dat_0cad72e8.inc"
};
