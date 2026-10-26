/* EV00BD.PRS's factory task, which draws the factory 500 up */
#include <ninja.h>

#include "ev00bd/factory.h"
#include "ev00bd/factory_task.h"
#include "system/task.h"

static struct task *ev00bd_factory = NULL;

static void ev00bd_factory_step(struct task *task)
{
    njPushMatrix(NULL);
    njTranslate(NULL, 0.0F, 500.0F, 0.0F);
    ev00bd_factory_draw();
    njPopMatrix(1);
}

void ev00bd_factory_create(void)
{
    if (ev00bd_factory == NULL) {
        ev00bd_factory = task_create(0, 5, ev00bd_factory_step);
    }
}

void ev00bd_factory_kill(void)
{
    if (ev00bd_factory != NULL) {
        task_kill(ev00bd_factory);
        ev00bd_factory = NULL;
    }
}
