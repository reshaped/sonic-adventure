/* the texture lists the stage loads, each with the PVM that fills it */
#ifndef B_EGM1_PVMS_H
#define B_EGM1_PVMS_H

#include "system/texture.h"

/* the stage's pvm list, whose address 1ST_READ.BIN's stage_pvms holds.
 * texture_load_stage loads from it */
extern struct texture_pvm eggmobile1_pvms[];

#endif
