/* 16 bytes of constants, between rocker.c's and lamps.c's, that this build
 * keeps here. nothing in this image or 1ST_READ.BIN names them, and their
 * declaration and owner are unknown. they read as eight Sint16 */
#include <sg_xpt.h>

/* NOLINTNEXTLINE(clang-diagnostic-unused-const-variable) */
static const Uint8 dat_0cadcb38[16] = {
#include "stg05/dat_0cadcb38/dat_0cadcb38.inc"
};
