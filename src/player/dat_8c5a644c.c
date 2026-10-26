/* an action of Eggman's: its motion, then the action that plays the motion
 * on eggman_object */
#include <ninja.h>

#include "player/dat_8c5a644c.h"

extern NJS_OBJECT eggman_object;
extern NJS_MDATA2 dat_8c5a62c0[];

static NJS_MOTION dat_8c5a6440 = {
#include "player/dat_8c5a644c/dat_8c5a6440.inc"
};
NJS_ACTION dat_8c5a644c = {
#include "player/dat_8c5a644c/dat_8c5a644c.inc"
};
