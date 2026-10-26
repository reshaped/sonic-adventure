/* 12 bytes of D, between the object list and elevator.c's data, that this
 * build keeps here. nothing in this image, AL_MAIN.BIN or 1ST_READ.BIN names
 * them, and their declaration and owner are unknown; garden.c is the
 * likeliest owner, as its constants follow the kinds' names and its data
 * would follow the list. they read as three Floats */
#include <sg_xpt.h>

static Uint8 dat_0cb931a8[12] = {
#include "al_garden00/dat_0cb931a8/dat_0cb931a8.inc"
};
