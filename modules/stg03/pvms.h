/* the pvms the stage loads */
#ifndef STG03_PVMS_H
#define STG03_PVMS_H

#include "system/texture.h"

/* 1ST_READ.BIN's stage_pvms points at this list for the stage */
extern struct texture_pvm twinkle_pvms[];
/* each act's pvms in turn. 1ST_READ.BIN's twinkle_acts points at each
 * act's first, and holds its count */
extern struct texture_pvm twinkle_act_pvms[];

#endif
