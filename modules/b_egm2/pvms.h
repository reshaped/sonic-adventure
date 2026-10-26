/* the texture lists the stage loads, each with the PVM that fills it */
#ifndef B_EGM2_PVMS_H
#define B_EGM2_PVMS_H

#include "system/texture.h"

/* the list texture_load_stage loads for the stage. 1ST_READ.BIN's
 * stage_pvms reaches it by its address, 0x0c9001b0 */
extern struct texture_pvm egm2_pvms[];

#endif
