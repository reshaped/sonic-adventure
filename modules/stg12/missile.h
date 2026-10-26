/* homing missiles that E-105 fires */
#ifndef STG12_MISSILE_H
#define STG12_MISSILE_H
#include <ninja.h>
#include <sg_xpt.h>

#include "system/task.h"

/* the missiles alive, held or flying. E-105 arms a launcher only while
 * fewer than 8 are. its intro sets the count to 0, and its defeat to -1.
 * while the count is negative, every homing missile bursts on its next
 * step */
extern int e105_missile_count;

/* create a missile that sits at anchor and points along dir's horizontal
 * part until e105_missile_release launches it. it reads both at once and
 * again on each step it is held. both must outlive the hold. NULL if
 * task_create fails. a failed heap_alloc of the missile's own record goes
 * uncaught, and create then writes through NULL. E-105's launchers release
 * the result unchecked */
struct task *e105_missile_create(NJS_POINT3 *anchor, NJS_VECTOR *dir);
/* launch a held missile from e105_missile_create. it flies along its
 * heading, then homes on player 0 */
void e105_missile_release(struct task *task);

#endif
