/* E-101, and the texlist its missiles share */
#ifndef B_E101_BOSS_H
#define B_E101_BOSS_H
#include <ninja.h>

/* make E-101's task at level 2, unless no task is free. it starts with
 * its intro, and its defeat clears the stage. player 0's task must exist.
 * B_E101 must stay loaded until the boss and every task it makes are
 * destroyed.
 *
 * the allocations of its lock-on record, its motion record and its shapes'
 * copy go unchecked. without the lock-on record, enemy_alloc_record kills
 * the task, whose destructor then ends the intro. that leaves the time
 * stopped and the pads disabled. without the motion record,
 * e101_create writes through null. without the shapes' copy, the boss
 * reads through null as it first guards */
void e101_create(void);
extern NJS_TEXLIST e101_texlist;

#endif
