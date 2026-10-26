/* the handle on Zero's task: the task zero_appear made, until zero_kill or
 * zero_init empties it */
#ifndef ENEMY_ZERO_HANDLE_H
#define ENEMY_ZERO_HANDLE_H
#include <ninja.h>
#include <sg_xpt.h>

struct zero_start;

/* empty the handle without ending any Zero it holds, then load EGGROB into
 * zero_texlist. in act 0 of STAGE_FINAL it always loads it. elsewhere, in
 * any stage but STAGE_EGGMANROBO, it loads it only while no event is in
 * control */
void zero_init(void);
/* make Zero as start says, and hold its task. returns TRUE, or FALSE when
 * the handle already holds a Zero or zero_create returns null */
Bool zero_appear(struct zero_start *start);
/* end the held Zero on its task's next step, as task_kill does, and empty
 * the handle. returns TRUE, or FALSE when the handle holds no Zero */
Bool zero_kill(void);
/* set ZERO_LEAVING in the held Zero's work->tick, so Zero rises away and
 * ends. returns TRUE, or FALSE when the handle holds no Zero */
Bool zero_leave(void);
/* TRUE while the handle holds a Zero, and otherwise FALSE */
Bool zero_exists(void);
/* store the held Zero's position in *pos. returns TRUE, or FALSE, storing
 * nothing, when the handle holds no Zero or pos is null */
Bool zero_get_pos(NJS_POINT3 *pos);

#endif
