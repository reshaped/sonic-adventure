/* Sonic's action as he crashes while on a wall, and its motion. 1ST_READ.BIN's
 * sonic_actions points at the action, in its row ACTION_WALL_CRASH */
#include <ninja.h>

#include "stg04/dat_0ca72368.h"

extern NJS_MDATA2 dat_0ca71ed8[];
extern NJS_OBJECT sonic_object;

static NJS_MOTION dat_0ca72368 = {
#include "stg04/dat_0ca72368/dat_0ca72368.inc"
};
NJS_ACTION dat_0ca72374 = {
#include "stg04/dat_0ca72368/dat_0ca72374.inc"
};
