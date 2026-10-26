/* O TREE tree, turned by all three of its set entry angles */
#include <ninja.h>

#include "adv02/tree.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "system/task.h"
#include "system/texture.h"

extern NJS_OBJECT mystic_ruins_tree_object;

enum mystic_ruins_tree_mode { TREE_INIT, TREE_RUN };

static void mystic_ruins_tree_draw(struct task *task);

void mystic_ruins_tree_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return;
    }
    switch (work->mode) {
    case TREE_INIT:
        task->draw = mystic_ruins_tree_draw;
        work->mode = TREE_RUN;
        break;
    case TREE_RUN:
        break;
    }
    mystic_ruins_tree_draw(task);
}

static void mystic_ruins_tree_draw(struct task *task)
{
    struct task_work *work = task->work;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    njRotateZXY(NULL, work->ang.x, work->ang.y, work->ang.z);
    draw_tree(&mystic_ruins_tree_object);
    njPopMatrix(1);
}
