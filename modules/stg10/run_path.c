/* path task that runs players along a path from its first point */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/polygon.h"
#include "path/path.h"
#include "player/player.h"
#include "stage/stage.h"
#include "stg10/run_path.h"
#include "system/task.h"

/* steps after a player leaves the path before the path can take that player
 * again */
#define PATH_WAIT 60
/* the path takes a player within 20 units of its first point. the distance is
 * squared, as polygon_point_distance2 gives it */
#define PATH_REACH (20.0F * 20.0F)

/* work->tick: a bit for each of players 0 and 1 that the path holds */
#define HELD(work) ((work)->tick)

enum path_mode { PATH_SET_UP, PATH_RUN };

void final_run_path_step(struct task *task)
{
    struct task_work *work = task->work;
    NJS_POINT3 first;
    /* @hack redundant: nearest, whose reads change nothing. it falls to the
     * least squared distance from the first point of a player the path
     * doesn't hold, and to 0 for one it holds. SHC keeps it in FR15
     * (0x0c900826, 0x0c9008e8, 0x0c900928); without it, the function runs 40
     * bytes short and 139 of 152 words differ */
    Float nearest = 40000.0F;
    Float distance;
    Uint8 number;
    int bit;
    struct task_work *player;

    switch (work->mode) {
    case PATH_SET_UP:
        HELD(work) = 0;
        PATH_VARS(work)->wait[0] = PATH_VARS(work)->wait[1] = PATH_WAIT;
        work->mode = PATH_RUN;
        break;
    case PATH_RUN:
        if (stage_get_stage_act() != PATH_VARS(work)->stage_act) {
            task_kill(task);
            return;
        }
        for (number = 0; number < 2; number++) {
            if ((player = player_work[number]) == NULL) {
                continue;
            }
            bit = 1 << number;
            if (!(HELD(work) & bit)) {
                if (++PATH_VARS(work)->wait[number] < PATH_WAIT) {
                    continue;
                }
                PATH_VARS(work)->wait[number] = PATH_WAIT;
                first.x = PATH_VARS(work)->path->points[0].pos.x;
                first.y = PATH_VARS(work)->path->points[0].pos.y;
                first.z = PATH_VARS(work)->path->points[0].pos.z;
                distance = polygon_point_distance2(&first, &player->pos);
                if (distance < PATH_REACH) {
                    player_run_path(number, PATH_VARS(work)->path);
                    HELD(work) |= bit;
                }
                if (distance < nearest) {
                    nearest = distance;
                }
            } else {
                nearest = 0.0F;
                if (!(player->flags & PLAYER_ON_PATH)) {
                    PATH_VARS(work)->wait[number] = 0;
                    HELD(work) &= ~bit;
                }
            }
        }
        break;
    }
}
