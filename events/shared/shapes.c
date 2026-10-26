/* an event's switch for whether a task's collision shapes stand firm */
#include "shared/shapes.h"
#include "collision/collision.h"
#include "system/task.h"

void event_shapes_set_firm(struct task *task)
{
    struct collision_work *collision = task->work->collision;
    int i;

    for (i = 0; i < collision->count; i++) {
        collision->shapes[i].push |= PUSH_FIRM(PUSH_PLAYERS | PUSH_ENEMIES);
    }
}

void event_shapes_clear_firm_players(struct task *task)
{
    struct collision_work *collision = task->work->collision;
    int i;

    for (i = 0; i < collision->count; i++) {
        collision->shapes[i].push &= ~PUSH_FIRM(PUSH_PLAYERS);
    }
}
