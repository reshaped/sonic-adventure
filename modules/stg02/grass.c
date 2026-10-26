/* O GRASS: a tuft of grass, only drawn */
#include <ninja.h>

#include "debug/editor.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "stg02/grass.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT windy_grass_object;

/* the grass's modes, 0, 1 and 3 as the image stores them. in GRASS_END its
 * step does nothing, and no step here sets it */
enum grass_mode { GRASS_START = 0, GRASS_RUN = 1, GRASS_END = 3 };

static void windy_grass_draw(struct task *task);

static void windy_grass_idle(struct task *task)
{
    /* @hack redundant: work, read and never used, which keeps the MOV R14,R4 at
     * 0x0c90bc7c, all that is left of the call windy_grass_step inlines;
     * without it, 11 of 35 words differ */
    struct task_work *work = task->work;
}

void windy_grass_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return;
    }
    switch (work->mode) {
    case GRASS_START:
        task->draw = windy_grass_draw;
        work->mode = GRASS_RUN;
        break;
    case GRASS_RUN:
        windy_grass_draw(task);
        if (!placement_editing(task)) {
            windy_grass_idle(task);
        }
        break;
    case GRASS_END:
        break;
    }
}

static void windy_grass_draw(struct task *task)
{
    struct task_work *work = task->work;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
    TURN(njRotateZ, work->ang.z);
    TURN(njRotateX, work->ang.x);
    TURN(njRotateY, work->ang.y);
    draw_tree(&windy_grass_object);
    njPopMatrix(1);
}
