/* 8 zero bytes of the data section, between tp_ring.c's data and roof.c's.
 * nothing in this image or 1ST_READ.BIN points at them */
#include <sg_xpt.h>

static Uint8 dat_0c9c0f20[8] = {
#include "stg03/dat_0c9c0f20/dat_0c9c0f20.inc"
};
