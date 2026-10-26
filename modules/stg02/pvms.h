/* the pvms the stage loads */
#ifndef STG02_PVMS_H
#define STG02_PVMS_H

#include "system/texture.h"

/* 1ST_READ.BIN's stage_pvms points at this list for the stage. it reaches
 * the list by its address, 0x0ca146b8, not by its name */
extern struct texture_pvm windy_pvms[];

#endif
