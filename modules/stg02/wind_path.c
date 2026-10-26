/* Windy Valley path tasks: the wind path that carries a player along it, and an
 * unused spin path */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/polygon.h"
#include "path/path.h"
#include "player/player.h"
#include "sound/sound.h"
#include "stage/stage.h"
#include "stg02/wind.h"
#include "stg02/wind_path.h"
#include "system/task.h"

/* the steps after a path lets a player go before it can take that player
 * again */
#define PATH_WAIT 60

/* a path takes a player within these of its first point. each is squared, as
 * polygon_point_distance2's result is */
#define WIND_REACH (50.0F * 50.0F)
#define SPIN_REACH (20.0F * 20.0F)

#define SE_WIND 59

/* the players the path holds, as bit 1 << n for player n, in work->tick */
#define HELD_PLAYERS(work) ((work)->tick)

enum path_mode { PATH_SET_UP, PATH_RUN };

void windy_wind_path_step(struct task *task)
{
    struct task_work *work = task->work;
    NJS_POINT3 first;
    /* @hack redundant: nearest, a distance computed and never read, which
     * keeps its load, compare and moves in FR15, from 0x0c909206 to
     * 0x0c909368; without it, the function runs 32 bytes short, and 214 of
     * 228 words differ */
    Float nearest = 40000.0F;
    Float distance;
    Uint8 number;
    int bit;
    struct task_work *player;
    struct task *child;
    struct task_work *leaf;

    switch (work->mode) {
    case PATH_SET_UP:
        HELD_PLAYERS(work) = 0;
        PATH_VARS(work)->wait[0] = PATH_VARS(work)->wait[1] = PATH_WAIT;
        /* @hack redundant: first, copied and never read, from 0x0c909226 to
         * 0x0c909248; without it, the function runs 32 bytes short, and 195 of
         * 228 words differ */
        first.x = PATH_VARS(work)->path->points[0].pos.x;
        first.y = PATH_VARS(work)->path->points[0].pos.y;
        first.z = PATH_VARS(work)->path->points[0].pos.z;
        work->mode = PATH_RUN;
        break;
    case PATH_RUN:
        /* @hack redundant: first, copied and never read, since the loop
         * copies it again before each read, from 0x0c90924e to 0x0c909274;
         * without it, the function runs 32 bytes short, and 175 of 228 words
         * differ */
        first.x = PATH_VARS(work)->path->points[0].pos.x;
        first.y = PATH_VARS(work)->path->points[0].pos.y;
        first.z = PATH_VARS(work)->path->points[0].pos.z;
        if (stage_get_stage_act() != PATH_VARS(work)->stage_act) {
            task_kill(task);
            return;
        }
        for (number = 0; number < 2; number++) {
            if ((player = player_work[number]) == NULL) {
                continue;
            }
            bit = 1 << number;
            if (!(HELD_PLAYERS(work) & bit)) {
                if (++PATH_VARS(work)->wait[number] < PATH_WAIT) {
                    continue;
                }
                PATH_VARS(work)->wait[number] = PATH_WAIT;
                first.x = PATH_VARS(work)->path->points[0].pos.x;
                first.y = PATH_VARS(work)->path->points[0].pos.y;
                first.z = PATH_VARS(work)->path->points[0].pos.z;
                distance = polygon_point_distance2(&first, &player->pos);
                if (distance < WIND_REACH) {
                    sound_play_for(SE_WIND, (int)work, 1, 0, 60);
                    player_run_path(number, PATH_VARS(work)->path);
                    HELD_PLAYERS(work) |= bit;
                }
                if (distance < nearest) {
                    nearest = distance;
                }
            } else {
                nearest = 0.0F;
                if (!(player->flags & PLAYER_ON_PATH)) {
                    PATH_VARS(work)->wait[number] = 0;
                    HELD_PLAYERS(work) &= ~bit;
                }
            }
        }
        break;
    }
    /* every sixth step sends a leaf along the path, as work->timer counts. the
     * leaf takes work->submode as its phase, raised before each task_create.
     * while each create succeeds, the phases run 1 to 16 and start over. a
     * failed create still uses up its phase. the phase goes back to 0 only once
     * a leaf takes one of 16 or more */
    if (++TASK_TIMER(work) > 5) {
        work->timer = 0;
        work->submode++;
        if ((child = task_create(TASK_WORK, 6, windy_path_leaf_step)) != NULL) {
            leaf = child->work;
            PATH_LEAF(leaf)->path = PATH_VARS(work)->path;
            PATH_LEAF(leaf)->phase = work->submode;
            if (work->submode > 15) {
                work->submode = 0;
            }
        }
    }
}

void windy_spin_path_step(struct task *task)
{
    struct task_work *work = task->work;
    NJS_POINT3 first;
    Float distance;
    Uint8 number;
    int bit;
    struct task_work *player;

    switch (work->mode) {
    case PATH_SET_UP:
        HELD_PLAYERS(work) = 0;
        PATH_VARS(work)->wait[0] = PATH_VARS(work)->wait[1] = PATH_WAIT;
        work->mode = PATH_RUN;
        break;
    case PATH_RUN:
        if (stage_get_stage_act() != PATH_VARS(work)->stage_act) {
            task_kill(task);
            return;
        }
        player = player_work[0];
        /* @bug memory: the second pass reads the 64 bytes after player
         * 0's work, not player 1's work. player starts at player_work[0],
         * which MOV.L @R11,R11 at 0x0c90945e loads, and player++ steps it by
         * a task_work's 64 bytes, as ADD #64,R11 at 0x0c909518 does */
        for (number = 0; number < 2; number++, player++) {
            if (player == NULL) {
                continue;
            }
            bit = 1 << number;
            if (!(HELD_PLAYERS(work) & bit)) {
                if (++PATH_VARS(work)->wait[number] < PATH_WAIT) {
                    continue;
                }
                PATH_VARS(work)->wait[number] = PATH_WAIT;
                first.x = PATH_VARS(work)->path->points[0].pos.x;
                first.y = PATH_VARS(work)->path->points[0].pos.y;
                first.z = PATH_VARS(work)->path->points[0].pos.z;
                distance = polygon_point_distance2(&first, &player->pos);
                if (distance < SPIN_REACH) {
                    player_spin_path(number, PATH_VARS(work)->path);
                    HELD_PLAYERS(work) |= bit;
                }
            } else {
                if (!(player->flags & PLAYER_ON_PATH)) {
                    PATH_VARS(work)->wait[number] = 0;
                    HELD_PLAYERS(work) &= ~bit;
                }
            }
        }
        break;
    }
}
