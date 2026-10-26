/* C SCENECHG, a cylinder that sends player 0 to PAST_CHAOS or PAST_KNUCKLES.
 * its radius is scl.x, and it reaches scl.y above and below its position. the
 * set editor shows it */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv03/past.h"
#include "adv03/scene_change.h"
#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "player/player.h"
#include "progress/progress.h"
#include "stage/stage.h"
#include "system/task.h"

/* the act a placement's ang.x sends player 0 to */
enum scene_change_to { TO_CHAOS, TO_KNUCKLES };

static void scene_change_go(int to);

void scene_change_step(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_try_leave(task)) {
        if (placement_editing(task)) {
            Float radius;
            Float height;

            njPushMatrix(NULL);
            radius = work->scl.x * MARKER_FIT;
            height = work->scl.y * MARKER_FIT;
            njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
            draw_set_material(0.1F, 1.0F, 0.8F, 0.2F);
            njScale(NULL, radius, height, radius);
            draw_tree(&marker_cylinder_object);
            njPopMatrix(1);
            draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
        } else if (player_in_cylinder(&work->pos, work->scl.x, work->scl.y) ==
                   1) {
            scene_change_go(work->ang.x);
            placement_kill(task);
        }
    }
}

static void scene_change_go(int to)
{
    switch (to) {
    case TO_CHAOS:
        stage_set_entrance(0);
        story_go_to(STAGE_PAST, PAST_CHAOS);
        break;
    case TO_KNUCKLES:
        stage_set_entrance(0);
        story_go_to(STAGE_PAST, PAST_KNUCKLES);
        break;
    }
}
