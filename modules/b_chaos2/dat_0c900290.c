/* 8 zero bytes of DHEAD that nothing points at, between boss.c's head
 * data and chandelier.c's. their type is unknown */
#include <sg_xpt.h>

#pragma section HEAD
static Uint8 dat_0c900290[8] = {
#include "b_chaos2/dat_0c900290/dat_0c900290.inc"
};
#pragma section
