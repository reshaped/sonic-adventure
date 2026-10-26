/* homing missiles that E-103 fires */
#ifndef STG02_MISSILE_H
#define STG02_MISSILE_H
#include <ninja.h>
#include <sg_xpt.h>

struct collision_work;

/* a count of the missiles in flight, which E-103 reads to limit how many it
 * fires. each missile adds 1 as it fires. as it is destroyed, it takes 1 off
 * a count above 0. once E-103 sets the count to -1, each missile bursts on
 * its next step */
extern int e103_missile_count;

/* fire a missile from pos, headed along dir's x and z. the missile flies
 * through source, the launcher's collision record, without bursting.
 *
 * when no task can be made, no missile fires. its other allocations go
 * unchecked:
 * - without its record, e103_missile_create writes through null;
 * - without its shapes' copy, the missile reads its collision record
 *   through null each step;
 * - without its lock-on record, enemy_alloc_record kills its task. but
 *   e103_missile_create then sets the task's step, so the missile flies on.
 *   a hit that hurts it then reads the lock-on record through null. so does
 *   a shot with a lock-on that touches it */
void e103_missile_create(NJS_POINT3 *pos, NJS_VECTOR *dir,
                         struct collision_work *source);

#endif
