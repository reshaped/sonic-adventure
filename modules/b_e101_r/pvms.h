/* the pvms the stage loads */
#ifndef B_E101_R_PVMS_H
#define B_E101_R_PVMS_H

#include "system/texture.h"

/* the pvms texture_load_stage loads for the stage. 1ST_READ.BIN's
 * stage_pvms reaches the list by its address, 0x0c900150, not by its
 * name */
extern struct texture_pvm e101_r_pvms[];

#endif
