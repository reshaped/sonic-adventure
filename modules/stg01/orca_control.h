/* the controller of act 0's orcas, its sea and its floating bridge */
#ifndef STG01_ORCA_CONTROL_H
#define STG01_ORCA_CONTROL_H
#include <ninja.h>
#include <sg_xpt.h>

#include "system/task.h"

/* the controller's globals, which start at 0. bss holds them in the order
 * this header declares them, and the image fixes that order. only the
 * controller reads orca_task and sea_task. they stay global because as
 * statics they would leave that order */

/* the orca that is out, the chasing orca or the path orca, or NULL */
extern struct task *orca_task;
/* TRUE once orca_send_away sends the orca away. each summon clears it */
extern Bool orca_leaving;
extern struct task *sea_task;
/* the chasing orca's head, in world coordinates. it lies 100000 below the
 * origin while the orca rises. orca_control_start and orca_clear_task move
 * it to the origin */
extern NJS_POINT3 orca_head;
/* the chasing orca's move at its last step, by its speed and its sway. y
 * is 0, and a leaving orca's swim off and down isn't in it */
extern NJS_VECTOR orca_speed;
/* the pier sections the orca breaks in silence before the crash sound
 * plays again */
extern Sint32 orca_crash_wait;

/* forget the orca that is out, and move orca_head to the origin. then
 * create the floating bridge and the sea, and clear orca_crash_wait.
 * beach_act_start calls this as act 0 starts */
void orca_control_start(void);
/* remove the floating bridge, then kill the orca that is out and the sea.
 * beach_end calls this as act 0 ends */
void orca_control_end(void);
/* whether a summon must wait. it always returns FALSE. O AO SUMMON and
 * O PO SUMMON summon only when it returns FALSE */
Bool orca_summon_held(void);
/* kill the orca that is out, and clear orca_leaving. then create the
 * chasing orca as orca_task at x, y, z, heading ang. it returns TRUE. O AO
 * SUMMON calls this */
Bool orca_summon(Float x, Float y, Float z, Angle ang);
/* set orca_leaving, which sends the chasing orca away, and return TRUE.
 * with no orca out, return FALSE. O AO KILL calls this */
Bool orca_send_away(void);
/* forget the orca that is out if task is it, and move orca_head to the
 * origin. the chasing orca's destructor calls this */
void orca_clear_task(struct task *task);
/* clear orca_leaving, create the path orca as orca_task, and return TRUE.
 * with an orca out already, return FALSE. O PO SUMMON calls this */
Bool path_orca_summon(void);
/* forget the orca that is out if task is it. the path orca's destructor
 * calls this */
void path_orca_clear_task(struct task *task);
/* add a ripple to the sea, as beach_sea_add_ripple does. sea_task must be
 * alive */
void beach_sea_ripple(Float x, Float z, Float radius, Float width,
                      Float height);
/* the height of the sea's surface at x, z, or 0 while sea_task is NULL.
 * the floating bridge reads it */
Float beach_sea_height(Float x, Float z);

#endif
