/* the Hedgehog Hammer's texlist, and the list of pvms the stage loads */
#ifndef STG00_TEXLISTS_H
#define STG00_TEXLISTS_H
#include <ninja.h>

#include "system/texture.h"

/* the stage's texlist, which the PVM ADV_EC32 fills. the land table names
 * it, and texture_load_stage loads it into the first stage slot */
extern NJS_TEXLIST practice_texlist;

/* the stage's pvm list, whose address 1ST_READ.BIN's stage_pvms holds.
 * texture_load_stage loads from it */
extern struct texture_pvm practice_pvms[];

#endif
