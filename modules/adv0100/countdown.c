/* countdown task step that no object list or code refers to */
#include <ninja.h>

#include "adv0100/countdown.h"
#include "object/placement.h"
#include "system/task.h"

/* the timer each wrap of scl.x sets, which nothing reads */
#define WRAP_TIMER 20

void fun_0c92d2a0(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return;
    }
    switch (work->mode) {
    case 0:
        work->timer = 0;
        work->mode = 1;
        break;
    case 1:
        work->scl.x -= work->scl.y;
        if (work->scl.x > 0.0F) {
            break;
        }
        work->scl.x += work->scl.z;
        work->timer = WRAP_TIMER;
        break;
    }
}
