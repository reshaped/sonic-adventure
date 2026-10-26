/* the pvms the stage loads */
#ifndef B_EGM3_PVMS_H
#define B_EGM3_PVMS_H

#include "system/texture.h"

/* 1ST_READ.BIN's stage_pvms reaches this list by its address, 0x0c9588d8,
 * not by its name. texture_load_stage loads it */
extern struct texture_pvm eggmobile3_pvms[];

#endif
