/* O ICEJMP: an ice ramp the players stand on. a board that jumps off it goes
 * into a trick */
#include <ninja.h>

#include "object/common.h"
#include "player/player.h"
#include "stg08/ice_jump.h"
#include "system/task.h"

extern NJS_OBJECT ice_jump_object;

void ice_jump_step(struct task *task)
{
    struct task_work *work = task->work;

    work->id = RAMP_ID;
    WORLD_MODEL(work)->model = &ice_jump_object;
    model_start_world(task);
}
