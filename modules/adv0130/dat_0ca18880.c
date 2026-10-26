/* 72 bytes of D, between jdoor.c's data and egg_door.c's, that this build
 * keeps here. nothing in this image, ADV01OBJ.BIN or 1ST_READ.BIN names
 * them, and their declaration and owner are unknown. jdoor.c is the likeliest
 * owner: its last datum before them, jdoor_soft_dust, is one its code doesn't
 * read either. they read as six points of three Floats */
#include <sg_xpt.h>

static Uint8 dat_0ca18880[72] = {
#include "adv0130/dat_0ca18880/dat_0ca18880.inc"
};
