/* the controller of act 0's orcas, its sea and its floating bridge */
#include <ninja.h>
#include <sg_xpt.h>

#include "stg01/bridge.h"
#include "stg01/orca.h"
#include "stg01/orca_control.h"
#include "stg01/path_orca.h"
#include "stg01/sea.h"
#include "system/task.h"

struct task *orca_task;
Bool orca_leaving;
struct task *sea_task;
NJS_POINT3 orca_head;
NJS_VECTOR orca_speed;
Sint32 orca_crash_wait;

void orca_control_start(void)
{
    orca_task = NULL;
    orca_head.x = 0.0F;
    orca_head.y = 0.0F;
    orca_head.z = 0.0F;
    beach_bridge_create();
    sea_task = task_create(TASK_WORK | TASK_FORCE, 3, beach_sea_init);
    orca_crash_wait = 0;
}

void orca_control_end(void)
{
    beach_bridge_remove();
    if (orca_task != NULL) {
        task_kill(orca_task);
    }
    if (sea_task != NULL) {
        task_kill(sea_task);
    }
}

Bool orca_summon_held(void)
{
    return FALSE;
}

Bool orca_summon(Float x, Float y, Float z, Angle ang)
{
    struct task_work *work;

    if (orca_task != NULL) {
        task_kill(orca_task);
    }
    orca_leaving = FALSE;
    orca_task = task_create(TASK_MOTION | TASK_WORK, 3, orca_init);
    work = orca_task->work;
    work->pos.x = x;
    work->pos.y = y;
    work->pos.z = z;
    work->ang.y = ang;
    return TRUE;
}

Bool orca_send_away(void)
{
    /* @hack redundant: the else after the return, which has SHC load
     * orca_leaving's address into R1 at 0x0c908b26. without it, R3 takes
     * the address, and 2 of 12 words differ */
    if (orca_task == NULL) {
        return FALSE;
    } else {
        orca_leaving = TRUE;
        return TRUE;
    }
}

void orca_clear_task(struct task *task)
{
    if (orca_task == task) {
        orca_task = NULL;
        orca_head.x = 0.0F;
        orca_head.y = 0.0F;
        orca_head.z = 0.0F;
    }
}

Bool path_orca_summon(void)
{
    /* @hack redundant: the else after the return, which has SHC hold
     * orca_leaving's address in R3 and the 0 it stores in R1 at 0x0c908ba0.
     * without it, the two trade, and 7 of 22 words differ */
    if (orca_task != NULL) {
        return FALSE;
    } else {
        orca_leaving = FALSE;
        orca_task = task_create(TASK_MOTION | TASK_WORK, 3, path_orca_init);
        return TRUE;
    }
}

void path_orca_clear_task(struct task *task)
{
    if (orca_task == task) {
        orca_task = NULL;
    }
}

void beach_sea_ripple(Float x, Float z, Float radius, Float width, Float height)
{
    beach_sea_add_ripple(sea_task, x, z, radius, width, height);
}

Float beach_sea_height(Float x, Float z)
{
    Float height = 0.0F;

    if (sea_task != NULL) {
        height = beach_sea_get_height(sea_task, x, z);
    }
    return height;
}
