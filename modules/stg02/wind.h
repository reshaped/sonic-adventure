/* the object list's B LEAF, PU WIND and WC WIND, and the leaves they and the
 * wind path send out. B LEAF blows leaves out, and the winds lift players */
#ifndef STG02_WIND_H
#define STG02_WIND_H
#include <sg_xpt.h>

#include "path/path.h"
#include "system/task.h"

/* the object list's step for B LEAF. task needs TASK_WORK's record.
 *
 * on 2 of every 16 steps, the blower blows a leaf out along ang.x and ang.y,
 * at scl.x. each axis of the leaf's speed takes its own random share of that,
 * from 0.75 to 1.25. the leaf lives 100 steps. player 0 within 20 of the leaf
 * pushes it away and up. the push looks for player 1 past player 0's work. it
 * reads the 64 bytes after that work instead.
 *
 * while the set editor holds the blower, it blows no leaves. it then holds
 * scl.x between 0.1 and 10 */
void windy_leaf_blower_step(struct task *task);
/* the object list's step for PU WIND. task needs TASK_WORK's record.
 *
 * the wind is a box about work->pos, turned about y by -ang.y. scl holds its
 * sizes, each held between 10 and 200. the low byte of ang.x is its power,
 * held between 1 and 8. the box sends leaves up through itself.
 *
 * each step the box finds a player in it, it sets the player's updraft. it
 * may also send a leaf to swarm about the player. the updraft is 0 for the
 * first two steps in a row. after that, it is a tenth of the power less the
 * player's upward speed. the first time the box finds player 0, it plays its
 * sound at the box.
 *
 * while the set editor is open, the box draws itself each step the editor
 * holds it. it draws itself every other step the editor doesn't */
void windy_box_wind_step(struct task *task);
/* the object list's step for WC WIND. task needs TASK_WORK's record.
 *
 * the wind is a column about work->pos. scl.x is its radius, held between 1
 * and 100. it reaches from 10 below work->pos to scl.y above, scl.y held
 * between 10 and 100. scl.z is its power, held between 1 and 10.
 *
 * a column with ang.x even lifts no one. one with ang.x odd sets the updraft
 * of a player it finds in it, as PU WIND does. it also sends leaves up itself
 * and about that player. its model starts 50 below work->pos. while player 0 is
 * within scl.x + 30 of work->pos, the model rises 1 a step. otherwise it sinks
 * 0.5 a step, back to 50 below. once the model rises past 1 below work->pos,
 * the column stops lifting. it starts again once player 0 is out of that
 * reach.
 *
 * the column looks for player 1 past player 0's work and motion. it reads the
 * 64 bytes after each instead */
void windy_column_wind_step(struct task *task);

/* a path leaf's work->vars. phase picks the leaf's point on path:
 * points[path->count * phase / 20]. the leaf reads that point and the next, so
 * both must be among the path's points */
struct path_leaf_vars {
    struct path *path;
    Sint8 model; /* which of the three leaf models, 0 to 2 */
    Uint8 unk_05;
    Sint8 phase;
    Uint8 unk_07;
    Float distance; /* along the path, once it flies along it */
};

#define PATH_LEAF(work) ((struct path_leaf_vars *)&(work)->vars)

/* the step for a leaf on a wind path. task needs TASK_WORK's record, with
 * PATH_LEAF's path and phase set, and camera_work must not be null. the leaf
 * gathers on its point, then flies along the path. while it flies, it moves
 * only on a frame's first step, and counts the frame's later steps. it kills
 * itself once it has counted 101 of them, or passes the path's end. it also
 * kills itself beyond 1400 of the camera */
void windy_path_leaf_step(struct task *task);

#endif
