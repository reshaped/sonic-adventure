/* the step of the sky deck's one path, which puts a player on it. it is
 * path_join_step with a smaller JOIN_REACH, and with a test that never holds */
#include <ninja.h>
#include <sg_xpt.h>

#include "path/path.h"
#include "player/player.h"
#include "stage/stage.h"
#include "stg06/path.h"
#include "system/task.h"

/* the steps the path waits after it lets a player go, before it can take
 * that player again. it can take either player as soon as it starts */
#define JOIN_WAIT 60
/* the path's box reaches this far past its points along each axis. the search
 * looks for a point this near the player */
#define JOIN_REACH 10.0F

/* the low and high corners of the box round the path's points, which the
 * work's pos and scl keep */
#define LOW(work) ((work)->pos)
#define HIGH(work) ((work)->scl)
/* the path's hold on players 0 and 1, a bit each: 1 << the player's
 * number */
#define HELD(work) ((work)->tick)

enum path_mode { PATH_SET_UP, PATH_RUN };

/* a player in the path's box joins the path where the player is nearest.
 * the path holds the player until PLAYER_ON_PATH clears, then waits
 * JOIN_WAIT steps before it can take that player again */
void sky_deck_path_join_step(struct task *task)
{
    struct task_work *work = task->work;
    struct path *path = PATH_VARS(work)->path;
    struct task_work *player;
    /* @hack redundant: 12 bytes of frame that no code reads, of a type unknown,
     * declared as a Uint32[3]. the frame takes 28 bytes (ADD #-28,R15 at
     * 0x0c900012); without them, 2 of 365 words differ */
    Uint32 unused[3];
    NJS_VECTOR to;
    Uint8 number;
    int i;
    int j;

    switch (work->mode) {
    case PATH_SET_UP:
        HELD(work) = 0;
        PATH_VARS(work)->wait[0] = PATH_VARS(work)->wait[1] = JOIN_WAIT;
        HIGH(work).x = LOW(work).x = path->points[0].pos.x;
        HIGH(work).y = LOW(work).y = path->points[0].pos.y;
        HIGH(work).z = LOW(work).z = path->points[0].pos.z;
        for (i = 0; i < path->count; i++) {
            if (path->points[i].pos.x > HIGH(work).x) {
                HIGH(work).x = path->points[i].pos.x;
            }
            if (path->points[i].pos.y > HIGH(work).y) {
                HIGH(work).y = path->points[i].pos.y;
            }
            if (path->points[i].pos.z > HIGH(work).z) {
                HIGH(work).z = path->points[i].pos.z;
            }
            if (path->points[i].pos.x < LOW(work).x) {
                LOW(work).x = path->points[i].pos.x;
            }
            if (path->points[i].pos.y < LOW(work).y) {
                LOW(work).y = path->points[i].pos.y;
            }
            if (path->points[i].pos.z < LOW(work).z) {
                LOW(work).z = path->points[i].pos.z;
            }
        }
        HIGH(work).x += JOIN_REACH;
        HIGH(work).y += JOIN_REACH;
        HIGH(work).z += JOIN_REACH;
        LOW(work).x -= JOIN_REACH;
        LOW(work).y -= JOIN_REACH;
        LOW(work).z -= JOIN_REACH;
        work->mode = PATH_RUN;
        break;
    case PATH_RUN:
        if (stage_get_stage_act() != PATH_VARS(work)->stage_act) {
            task_kill(task);
            break;
        }
        for (number = 0; number < 2; number++) {
            player = player_work[number];
            if (player == NULL) {
                continue;
            }
            if (!(HELD(work) & (1 << number))) {
                if (++PATH_VARS(work)->wait[number] < JOIN_WAIT) {
                    continue;
                }
                PATH_VARS(work)->wait[number] = JOIN_WAIT;
                if (player->pos.x > HIGH(work).x) {
                    continue;
                }
                if (player->pos.y > HIGH(work).y) {
                    continue;
                }
                if (player->pos.z > HIGH(work).z) {
                    continue;
                }
                if (player->pos.x < LOW(work).x) {
                    continue;
                }
                if (player->pos.y < LOW(work).y) {
                    continue;
                }
                if (player->pos.z < LOW(work).z) {
                    continue;
                }
                for (j = 0; j < path->count - 1; j++) {
                    to.x = path->points[j].pos.x;
                    to.y = path->points[j].pos.y;
                    to.z = path->points[j].pos.z;
                    njSubVector(&to, &player->pos);
                    if (njScalor(&to) < JOIN_REACH) {
                        break;
                    }
                }
                /* @bug logic: the search above stops one short of the
                 * last point, and this test counts to the last, so it never
                 * holds (CMP/GE with the count at 0x0c9002b8, where the
                 * loop's at 0x0c9002b0 takes the count less 1). a player
                 * anywhere in the box joins the path, near a point or not */
                if (j >= path->count) {
                    continue;
                }
                player_join_path(number, path);
                HELD(work) |= 1 << number;
            } else if (!(player->flags & PLAYER_ON_PATH)) {
                PATH_VARS(work)->wait[number] = 0;
                HELD(work) &= ~(1 << number);
            }
        }
        break;
    }
}
