/* the handle on Zero's task: the task zero_appear made, until zero_kill or
 * zero_init empties it */
#include <ninja.h>
#include <sg_xpt.h>

#include "enemy/zero.h"
#include "enemy/zero_handle.h"
#include "event/play.h"
#include "stage/stage.h"
#include "system/task.h"
#include "system/texture.h"

static struct task *zero_task = NULL;

void zero_init(void)
{
    zero_task = NULL;
    if (stage_get_stage_act() == STAGE_ACT(STAGE_FINAL, 0) ||
        (!IN_STAGE(STAGE_EGGMANROBO) && !event_in_control())) {
        texture_load_pvm("EGGROB", &zero_texlist);
    }
}

Bool zero_appear(struct zero_start *start)
{
    if (zero_task != NULL) {
        return FALSE;
    }
    /* @hack spelling: the task stored as it's tested, which loads zero_task's
     * address again after the call and tests R0 (0x8c1164f0, 0x8c1164f2); as
     * two statements, SHC keeps the address in R14 across the call and 18 of
     * 19 words differ */
    if ((zero_task = zero_create(start)) == NULL) {
        return FALSE;
    }
    return TRUE;
}

Bool zero_kill(void)
{
    /* @hack redundant: the else after the return, which loads task_kill's
     * address into R2 (0x8c116570), then zero_task's into R2 after the call
     * and the null into R3 (0x8c116576, 0x8c116578); without it, R2 and R3
     * trade and 5 of 18 words differ */
    if (zero_task == NULL) {
        return FALSE;
    } else {
        task_kill(zero_task);
        zero_task = NULL;
        return TRUE;
    }
}

Bool zero_leave(void)
{
    if (zero_task == NULL) {
        return FALSE;
    }
    zero_task->work->tick |= ZERO_LEAVING;
    return TRUE;
}

Bool zero_exists(void)
{
    if (zero_task == NULL) {
        return FALSE;
    }
    return TRUE;
}

Bool zero_get_pos(NJS_POINT3 *pos)
{
    if (zero_task == NULL || pos == NULL) {
        return FALSE;
    }
    *pos = zero_task->work->pos;
    return TRUE;
}
