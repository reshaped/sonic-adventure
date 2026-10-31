/* E ACLIFT, a lift that raises an aircraft through doors in the deck */
#ifndef STG06_ACLIFT_H
#define STG06_ACLIFT_H
#include <ninja.h>

#include "system/task.h"

/* the lift's texlist and its lamp's. 1ST_READ.BIN's sky_deck_pvms names
 * them, for E_AIRCRAFT's and AIR_SIGNAL's textures */
extern NJS_TEXLIST aclift_texlist;
extern NJS_TEXLIST aclift_lamp_texlist;

/* the step a lift's task starts with. the object list places lifts, and
 * rane.c's deck sections make lifts as children. scl.z picks the way the
 * aircraft leaves. above 0 it turns right, above -10 left, and otherwise it
 * rises straight ahead. a turning aircraft starts out heading the lift's ang.y
 * less 0x4000. one that rises heads ang.y plus 0x4000. a lift that is a child
 * goes with its parent, and any other once out of range */
void aclift_step(struct task *task);

#endif
