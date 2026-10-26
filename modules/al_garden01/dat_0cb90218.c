/* 12 bytes of D, between the object list and death_zones.c's data, that
 * this build keeps here, as AL_GARDEN00's dat_0cb931a8.c keeps 12 after its
 * list. nothing in this image, AL_MAIN.BIN or 1ST_READ.BIN names them, and
 * their declaration and owner are unknown; garden.c is the likeliest owner,
 * as its constants follow the kinds' names and its data would follow the
 * list. they read as three Floats */
#include <sg_xpt.h>

static Uint8 dat_0cb90218[12] = {
#include "al_garden01/dat_0cb90218/dat_0cb90218.inc"
};
