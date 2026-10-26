/* a task that moves to the event camera's position each step, for another task
 * to look at */
#include "ev0061/look_camera.h"
#include "event/play.h"
#include "system/task.h"

static struct task *camera_mark = NULL;

static void camera_mark_step(struct task *task)
{
    task->work->pos.x = event_camera.pos.x;
    task->work->pos.y = event_camera.pos.y;
    task->work->pos.z = event_camera.pos.z;
}

void ev0061_look_at_camera(struct task *task)
{
    struct task *mark = task_create(TASK_WORK, 5, camera_mark_step);

    if (mark != NULL) {
        mark->work->pos.x = event_camera.pos.x;
        mark->work->pos.y = event_camera.pos.y;
        mark->work->pos.z = event_camera.pos.z;
        event_look_at_task(task, mark, 0.0F, 0.0F, 0.0F);
        camera_mark = mark;
    }
}

void ev0061_look_at_camera_stop(struct task *task)
{
    if (task != NULL) {
        event_look_clear(task);
    }
    if (camera_mark != NULL) {
        task_kill(camera_mark);
        camera_mark = NULL;
    }
}
