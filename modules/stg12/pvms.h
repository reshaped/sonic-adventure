/* the pvm lists for the stage and for each act */
#ifndef STG12_PVMS_H
#define STG12_PVMS_H

#include "system/texture.h"

/* the list 1ST_READ.BIN's stage_pvms names for the stage */
extern struct texture_pvm shelter_pvms[];
/* the lists 1ST_READ.BIN's shelter_acts names for acts 0 to 3.
 * shelter_acts holds each list's length, so no list ends in a null entry */
extern struct texture_pvm shelter_act0_pvms[];
extern struct texture_pvm shelter_act1_pvms[];
extern struct texture_pvm shelter_act2_pvms[];
extern struct texture_pvm shelter_act3_pvms[];

#endif
