/* the Egg Walker's bombs: lobbed from its body as it walks, or scattered
 * as it jumps */
#ifndef B_EGM2_BOMBS_H
#define B_EGM2_BOMBS_H
#include <ninja.h>
#include <sg_xpt.h>

#include "system/task.h"

/* a bomb's kind. a bounce keeps more of a lobbed bomb's speed than of a
 * scattered one's */
#define BOMB_LOBBED 0
#define BOMB_SCATTER 1

/* the boss's step in EGG_WALKER_BOMBS: it lobs five bombs, or fewer once
 * its walk stops. it then goes back to EGG_WALKER_WALK. task is the
 * boss's */
void egg_walker_bombs(struct task *task);
/* make a bomb of kind at *pos, moving at *speed, both copied. it makes
 * none when no task or memory is free. the bomb bounces until it comes to
 * rest or something hurts it, then blows up */
void egg_walker_bomb_create(NJS_POINT3 *pos, NJS_VECTOR *speed, int kind);

#endif
