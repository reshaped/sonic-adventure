/* SAND JUMP: a sand ramp the players stand on. a board that jumps off it
 * goes into a trick */
#include <ninja.h>

#include "object/common.h"
#include "player/player.h"
#include "sboard/sand_jump.h"
#include "system/task.h"

extern NJS_OBJECT sand_jump_object;

void sand_jump_step(struct task *task)
{
    struct task_work *work = task->work;

    work->id = RAMP_ID;
    WORLD_MODEL(work)->model = &sand_jump_object;
    model_start_world(task);
}
