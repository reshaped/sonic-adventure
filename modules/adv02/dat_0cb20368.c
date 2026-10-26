/* 8 zero bytes of data between kill_switch.c's and monkey_cage.c's. nothing in
 * this image, 1ST_READ.BIN or TIKAL_PROG.BIN points into them, so their type
 * and object are unknown */
#include <sg_xpt.h>

static Uint8 dat_0cb20368[8] = {
#include "adv02/dat_0cb20368/dat_0cb20368.inc"
};
