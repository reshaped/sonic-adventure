/* the palm the object list names O PALM, turned by all three of its set
 * entry's angles */
#include <ninja.h>

#include "adv02/palm.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "system/task.h"
#include "system/texture.h"

extern NJS_OBJECT mystic_ruins_palm_object;

enum palm_mode { PALM_INIT, PALM_RUN };

static void mystic_ruins_palm_draw(struct task *task);

void mystic_ruins_palm_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return;
    }
    switch (work->mode) {
    case PALM_INIT:
        task->draw = mystic_ruins_palm_draw;
        work->mode = PALM_RUN;
        break;
    /* @hack redundant: a case nothing needs, which SHC tests with CMP/EQ #1,R0
     * at 0x0c911998; without it, 17 of mystic_ruins_palm_step's 29 words differ
     */
    case PALM_RUN:
        break;
    }
    mystic_ruins_palm_draw(task);
}

static void mystic_ruins_palm_draw(struct task *task)
{
    struct task_work *work = task->work;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    njRotateZXY(NULL, work->ang.x, work->ang.y, work->ang.z);
    draw_tree(&mystic_ruins_palm_object);
    njPopMatrix(1);
}
