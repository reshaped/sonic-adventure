/* path steps: drawing a path, putting a player on it, or handing over the
 * camera near a stretch of it */
#include <ninja.h>
#include <sg_xpt.h>

#include "camera/camera.h"
#include "collision/polygon.h"
#include "path/path.h"
#include "path/steps.h"
#include "player/player.h"
#include "stage/stage.h"
#include "system/task.h"

/* the path draws while its first point is within 800 of the camera. the
 * test compares squared distances */
#define DRAW_RANGE (800.0F * 800.0F)

/* the steps a path waits after it lets a player go, before it can take that
 * player again. a path can take either player as soon as it starts */
#define JOIN_WAIT 60
#define HANG_WAIT 30
#define LEDGE_WAIT 60
#define CAMERA_WAIT 60

/* a path takes hold of a player within this distance of one of its
 * points. the distance also grows the path's box, which a player must be
 * in first */
#define JOIN_REACH 40.0F
#define HANG_REACH 10.0F
#define LEDGE_REACH 5.0F
/* a stretch takes the camera when a player comes within CAMERA_REACH of
 * one of its points. it can hand the camera back once the player is out of
 * its box grown by CAMERA_LEAVE. it can also do so once any of its points
 * is farther than CAMERA_LEAVE from the player */
#define CAMERA_REACH 50.0F
#define CAMERA_LEAVE 90.0F
/* after this many steps with camera_step_path's done bits clear, any done
 * bit lets the stretch hand the camera back. until then, only
 * CAMERA_PATH_FAR does. the count starts when the stretch takes the
 * camera */
#define CAMERA_CLEAR_STEPS 3

/* the low and high corners of the box round the path's points, or round
 * the stretch's, which the work's pos and scl keep */
#define LOW(work) ((work)->pos)
#define HIGH(work) ((work)->scl)
/* the path's hold on players 0 and 1, a bit each: 1 << the player's
 * number */
#define HELD(work) ((work)->tick)

enum path_mode { PATH_SET_UP, PATH_RUN };

static void path_camera_step(struct task *task);
static void path_camera_init(struct task *task);

/* the line out the way up grows from UP_LENGTH by UP_GROWTH a notch.
 * UP_NOTCHES gives the notches at a step: one every 4 steps, from 0 again
 * every 64 */
#define UP_LENGTH 2.0F
#define UP_GROWTH 0.3F
#define UP_NOTCHES(timer) (((timer) & 0x3f) >> 2)

/* the two lines path_draw_step draws at a point, two ends each: to the next
 * point, and out the way up */
static NJS_POINT3 path_draw_points[4] = {{0.0F, 0.0F, 0.0F}};
static NJS_COLOR path_draw_colors[4] = {
#include "path/steps/path_draw_colors.inc"
};
static NJS_POINT3COL path_draw_lines = {
#include "path/steps/path_draw_lines.inc"
};

#define STRETCH_SIGHTED                                                        \
    (CAMERA_STRETCH_WALLS | CAMERA_STRETCH_SIGHT | CAMERA_STRETCH_MEASURE |    \
     CAMERA_STRETCH_EASE)
#define STRETCH_WALLED                                                         \
    (CAMERA_STRETCH_WALLS | CAMERA_STRETCH_MEASURE | CAMERA_STRETCH_EASE)
#define STRETCH_OPEN (CAMERA_STRETCH_MEASURE | CAMERA_STRETCH_EASE)

#define PATH_CAMERAS 14
/* clang-format off */
static struct camera_path path_cameras[PATH_CAMERAS] = {
    {STRETCH_SIGHTED, NULL, 55.0F, 40.0F, 0.55F, 0.92F, 45.0F, 35.0F, 5.0F,
     0.0F, 20.0F, 0.5F, 100.0F, 0.0F, 0, 4, 20, {0.0F, 0.0F, 0.0F},
     {0.0F, 0.0F, 0.0F}, {0}, NJM_DEG_ANG(1), 0.1F},
    {STRETCH_OPEN, NULL, 52.0F, 40.0F, 0.55F, 0.93F, 48.0F, 35.0F, 6.0F,
     0.0F, 10.0F, 0.5F, 100.0F, 0.0F, 6, 8, 20, {0.0F, 0.0F, 0.0F},
     {0.0F, 0.0F, 0.0F}, {0}, NJM_DEG_ANG(1), 0.1F},
    {STRETCH_SIGHTED, NULL, 55.0F, 41.0F, 0.55F, 0.94F, 48.0F, 35.0F, 7.0F,
     1.0F, 20.0F, 0.5F, 150.0F, 0.0F, 6, 1, 20, {0.0F, 0.0F, 0.0F},
     {0.0F, 0.0F, 0.0F}, {0}, NJM_DEG_ANG(1), 0.1F},
    {STRETCH_WALLED, NULL, 60.0F, 40.0F, 0.6F, 0.9F, 55.0F, 35.0F, 7.0F,
     1.0F, 20.0F, 0.5F, 110.0F, 0.0F, 5, 0, 50, {10.0F, 0.0F, 0.0F},
     {0.0F, 0.0F, 0.0F}, {0}, NJM_DEG_ANG(1), 0.08F},
    {STRETCH_SIGHTED, NULL, 55.0F, 38.0F, 0.6F, 0.97F, 55.0F, 32.0F, 5.0F,
     0.0F, 20.0F, 0.5F, 100.0F, 0.0F, 0, 3, 20, {0.0F, 0.0F, 0.0F},
     {0.0F, 0.0F, 0.0F}, {0}, NJM_DEG_ANG(1), 0.08F},
    {STRETCH_OPEN, NULL, 52.0F, 40.0F, 0.65F, 0.9F, 48.0F, 35.0F, 15.0F,
     0.0F, 10.0F, 0.4F, 80.0F, 0.0F, 1, 5, 20, {0.0F, 0.0F, 0.0F},
     {0.0F, 0.0F, 0.0F}, {0}, NJM_DEG_ANG(1), 0.08F},
    {STRETCH_OPEN, NULL, 52.0F, 40.0F, 0.65F, 0.9F, 48.0F, 35.0F, 15.0F,
     0.0F, 10.0F, 0.4F, 80.0F, 0.0F, 1, 1, 20, {0.0F, 0.0F, 0.0F},
     {0.0F, 0.0F, 0.0F}, {0}, NJM_DEG_ANG(1), 0.08F},
    {STRETCH_OPEN, NULL, 52.0F, 40.0F, 0.65F, 0.9F, 48.0F, 35.0F, 15.0F,
     0.0F, 10.0F, 0.4F, 80.0F, 0.0F, 1, 1, 20, {0.0F, 0.0F, 0.0F},
     {0.0F, 0.0F, 0.0F}, {0}, NJM_DEG_ANG(1), 0.08F},
    {STRETCH_OPEN, NULL, 52.0F, 40.0F, 0.65F, 0.9F, 48.0F, 35.0F, 15.0F,
     0.0F, 10.0F, 0.4F, 80.0F, 0.0F, 1, 1, 20, {0.0F, 0.0F, 0.0F},
     {0.0F, 0.0F, 0.0F}, {0}, NJM_DEG_ANG(1), 0.08F},
    {STRETCH_OPEN, NULL, 52.0F, 40.0F, 0.65F, 0.9F, 48.0F, 35.0F, 15.0F,
     0.0F, 10.0F, 0.4F, 80.0F, 0.0F, 1, 1, 20, {0.0F, 0.0F, 0.0F},
     {0.0F, 0.0F, 0.0F}, {0}, NJM_DEG_ANG(1), 0.08F},
    {STRETCH_OPEN, NULL, 52.0F, 40.0F, 0.65F, 0.9F, 48.0F, 35.0F, 15.0F,
     0.0F, 10.0F, 0.4F, 80.0F, 0.0F, 1, 1, 20, {0.0F, 0.0F, 0.0F},
     {0.0F, 0.0F, 0.0F}, {0}, NJM_DEG_ANG(1), 0.08F},
    {STRETCH_OPEN, NULL, 52.0F, 60.0F, 0.65F, 0.9F, 48.0F, 35.0F, 15.0F,
     0.0F, 10.0F, 0.4F, 80.0F, 0.0F, 1, 1, 20, {0.0F, 0.0F, 0.0F},
     {0.0F, 0.0F, 0.0F}, {0}, NJM_DEG_ANG(1), 0.08F},
    {STRETCH_OPEN, NULL, 52.0F, 60.0F, 0.65F, 0.9F, 48.0F, 35.0F, 15.0F,
     0.0F, 10.0F, 0.4F, 80.0F, 0.0F, 1, 1, 20, {0.0F, 0.0F, 0.0F},
     {0.0F, 0.0F, 0.0F}, {0}, NJM_DEG_ANG(1), 0.08F},
    {STRETCH_OPEN, NULL, 52.0F, 60.0F, 0.65F, 0.9F, 48.0F, 35.0F, 15.0F,
     0.0F, 10.0F, 0.4F, 80.0F, 0.0F, 1, 1, 20, {0.0F, 0.0F, 0.0F},
     {0.0F, 0.0F, 0.0F}, {0}, NJM_DEG_ANG(1), 0.08F},
};
/* clang-format on */
static struct camera_path *path_camera_list[PATH_CAMERAS] = {
    &path_cameras[0],  &path_cameras[1],  &path_cameras[2],  &path_cameras[3],
    &path_cameras[4],  &path_cameras[5],  &path_cameras[6],  &path_cameras[7],
    &path_cameras[8],  &path_cameras[9],  &path_cameras[10], &path_cameras[11],
    &path_cameras[12], &path_cameras[13],
};

/* draw two lines at two of every four points, leaving out the last. one
 * runs to the next point, and one out the way up, growing from 2 to 6.5
 * long over 64 steps. the points that draw move on by one each step. the
 * first line fades from white to blue over 256 steps, and the second from
 * red to white. the task's timer counts only the steps the path is in
 * range */
void path_draw_step(struct task *task)
{
    struct task_work *work = task->work;
    struct path *path;
    struct path_point *point;
    NJS_VECTOR up;
    Float length;
    Uint16 rate;
    Uint16 rest;
    Uint8 red0;
    Uint8 green0;
    Uint8 blue0;
    Uint8 red1;
    Uint8 green1;
    Uint8 blue1;
    Uint16 i;

    if (stage_get_stage_act() != PATH_VARS(work)->stage_act) {
        task_kill(task);
        return;
    }
    if (polygon_point_distance2(&PATH_VARS(work)->path->points[0].pos,
                                &camera_work->pos) > DRAW_RANGE) {
        return;
    }
    TASK_TIMER(work)++;
    rate = (Uint8)TASK_TIMER(work);
    rest = 0x100 - rate;
    red0 = (rest * 255) >> 8;
    green0 = (rest * 255) >> 8;
    blue0 = ((rate * 255) + (rest * 255)) >> 8;
    red1 = ((rate * 255) + (rest * 255)) >> 8;
    green1 = (rate * 255) >> 8;
    blue1 = (rate * 255) >> 8;
    path_draw_colors[0].argb.r = red0;
    path_draw_colors[0].argb.g = green0;
    path_draw_colors[0].argb.b = blue0;
    path_draw_colors[1].argb.r = red0;
    path_draw_colors[1].argb.g = green0;
    path_draw_colors[1].argb.b = blue0;
    path_draw_colors[2].argb.r = red1;
    path_draw_colors[2].argb.g = green1;
    path_draw_colors[2].argb.b = blue1;
    path_draw_colors[3].argb.r = red1;
    path_draw_colors[3].argb.g = green1;
    path_draw_colors[3].argb.b = blue1;
    path = PATH_VARS(work)->path;
    for (i = 0, point = path->points; i < path->count - 1; i++, point++) {
        if ((TASK_TIMER(work) - i) & 2) {
            up.x = -(njSin(point->ang_z) * njCos(point->ang_x));
            up.y = njCos(point->ang_z) * njCos(point->ang_x);
            up.z = njSin(point->ang_x);
            length =
                UP_LENGTH + (UP_GROWTH * (Float)UP_NOTCHES(TASK_TIMER(work)));
            path_draw_points[0].x = point->pos.x;
            path_draw_points[0].y = point->pos.y;
            path_draw_points[0].z = point->pos.z;
            path_draw_points[1].x = point[1].pos.x;
            path_draw_points[1].y = point[1].pos.y;
            path_draw_points[1].z = point[1].pos.z;
            path_draw_points[2].x = point->pos.x + (length * up.x);
            path_draw_points[2].y = point->pos.y + (length * up.y);
            path_draw_points[2].z = point->pos.z + (length * up.z);
            path_draw_points[3].x = point->pos.x;
            path_draw_points[3].y = point->pos.y;
            path_draw_points[3].z = point->pos.z;
            njPushMatrix(NULL);
            njDrawLine3D(&path_draw_lines, 2, NJD_DRAW_NORMAL);
            njPopMatrix(1);
        }
    }
}

/* a player within JOIN_REACH of a point but the last joins the path where
 * the player is nearest. the path holds the player until PLAYER_ON_PATH
 * clears, then waits JOIN_WAIT steps before it can take that player again */
void path_join_step(struct task *task)
{
    struct task_work *work = task->work;
    struct path *path = PATH_VARS(work)->path;
    struct task_work *player;
    /* @hack redundant: 12 bytes of frame that no code reads, of a type unknown,
     * declared as a Uint32[3]. the image's ADD #-28,R15 reserves them
     * (0x8c06eb18); without them, 2 of 368 words differ */
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
                if (j >= path->count - 1) {
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

/* a player within HANG_REACH of a point hangs from the path there. the
 * path holds the player until PLAYER_ON_PATH clears, then waits HANG_WAIT
 * steps before it can take that player again */
void path_hang_step(struct task *task)
{
    struct task_work *work = task->work;
    struct path *path = PATH_VARS(work)->path;
    struct task_work *player;
    NJS_VECTOR to;
    int number;
    int i;
    int j;

    switch (work->mode) {
    case PATH_SET_UP:
        HELD(work) = 0;
        PATH_VARS(work)->wait[0] = PATH_VARS(work)->wait[1] = HANG_WAIT;
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
        HIGH(work).x += HANG_REACH;
        HIGH(work).y += HANG_REACH;
        HIGH(work).z += HANG_REACH;
        LOW(work).x -= HANG_REACH;
        LOW(work).y -= HANG_REACH;
        LOW(work).z -= HANG_REACH;
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
                if (++PATH_VARS(work)->wait[number] < HANG_WAIT) {
                    continue;
                }
                PATH_VARS(work)->wait[number] = HANG_WAIT;
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
                for (j = 0; j < path->count; j++) {
                    to.x = path->points[j].pos.x;
                    to.y = path->points[j].pos.y;
                    to.z = path->points[j].pos.z;
                    njSubVector(&to, &player->pos);
                    if (njScalor(&to) < HANG_REACH) {
                        break;
                    }
                }
                if (j >= path->count) {
                    continue;
                }
                player_hang_path(number, path, j);
                HELD(work) |= 1 << number;
            } else if (!(player->flags & PLAYER_ON_PATH)) {
                PATH_VARS(work)->wait[number] = 0;
                HELD(work) &= ~(1 << number);
            }
        }
        break;
    }
}

/* a player within LEDGE_REACH of a point holds on to a ledge there,
 * turned to the work's ang. the path holds the player until PLAYER_ON_PATH
 * clears, then waits LEDGE_WAIT steps before it can take that player again */
void path_ledge_step(struct task *task)
{
    struct task_work *work = task->work;
    struct path *path = PATH_VARS(work)->path;
    struct task_work *player;
    NJS_VECTOR to;
    int number;
    int i;
    int j;

    switch (work->mode) {
    case PATH_SET_UP:
        HELD(work) = 0;
        PATH_VARS(work)->wait[0] = PATH_VARS(work)->wait[1] = LEDGE_WAIT;
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
        HIGH(work).x += LEDGE_REACH;
        HIGH(work).y += LEDGE_REACH;
        HIGH(work).z += LEDGE_REACH;
        LOW(work).x -= LEDGE_REACH;
        LOW(work).y -= LEDGE_REACH;
        LOW(work).z -= LEDGE_REACH;
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
                if (++PATH_VARS(work)->wait[number] < LEDGE_WAIT) {
                    continue;
                }
                PATH_VARS(work)->wait[number] = LEDGE_WAIT;
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
                for (j = 0; j < path->count; j++) {
                    to.x = path->points[j].pos.x;
                    to.y = path->points[j].pos.y;
                    to.z = path->points[j].pos.z;
                    njSubVector(&to, &player->pos);
                    if (njScalor(&to) < LEDGE_REACH) {
                        break;
                    }
                }
                if (j >= path->count) {
                    continue;
                }
                player_ledge_path(number, path, j, &work->ang);
                HELD(work) |= 1 << number;
            } else if (!(player->flags & PLAYER_ON_PATH)) {
                PATH_VARS(work)->wait[number] = 0;
                HELD(work) &= ~(1 << number);
            }
        }
        break;
    }
}

/* a player within CAMERA_REACH of one of the stretch's points has the
 * stretch take the camera. it doesn't while the camera runs at the comp or
 * event level. the stretch hands the camera back with camera_restore once
 * camera_step_path sets a CAMERA_PATH_DONE bit and the player has left. the
 * player has left once out of the stretch's box grown by CAMERA_LEAVE, or
 * once any of the stretch's points is farther than CAMERA_LEAVE away. with
 * the camera at the comp level or off the event level, the stretch lets go
 * of the player without camera_restore and ends the step */
static void path_camera_step(struct task *task)
{
    struct task_work *work = task->work;
    struct path *path = PATH_VARS(work)->path;
    struct camera_path *camera = path_camera_list[work->submode];
    struct task_work *player;
    NJS_VECTOR to;
    /* @hack redundant: 12 bytes of frame that no code reads, of a type unknown,
     * declared as a Uint32[3]. the image's ADD #-32,R15 reserves them
     * (0x8c06f514); without them, 12 of 405 words differ */
    Uint32 unused[3];
    Float distance;
    Float away;
    int number;
    int i;

    if (stage_get_stage_act() != PATH_VARS(work)->stage_act) {
        task_kill(task);
        return;
    }
    for (number = 0; number < 2; number++) {
        if ((player = player_work[number]) == NULL) {
            continue;
        }
        if (!(HELD(work) & (1 << number))) {
            distance = 0.0F;
            if (++PATH_VARS(work)->wait[number] < CAMERA_WAIT) {
                continue;
            }
            if (camera_at_comp_level() || camera_at_event_level()) {
                continue;
            }
            PATH_VARS(work)->wait[number] = CAMERA_WAIT;
            if (player->pos.x > HIGH(work).x + CAMERA_REACH) {
                continue;
            }
            if (player->pos.y > HIGH(work).y + CAMERA_REACH) {
                continue;
            }
            if (player->pos.z > HIGH(work).z + CAMERA_REACH) {
                continue;
            }
            if (player->pos.x < LOW(work).x - CAMERA_REACH) {
                continue;
            }
            if (player->pos.y < LOW(work).y - CAMERA_REACH) {
                continue;
            }
            if (player->pos.z < LOW(work).z - CAMERA_REACH) {
                continue;
            }
            for (i = camera->first; i < path->count - camera->skip; i++) {
                to.x = path->points[i].pos.x;
                to.y = path->points[i].pos.y;
                to.z = path->points[i].pos.z;
                njSubVector(&to, &player->pos);
                if (distance > (away = njScalor(&to)) || i == camera->first) {
                    distance = away;
                }
            }
            if (distance > CAMERA_REACH) {
                continue;
            }
            camera->path = path;
            CAMERA_VARS(camera_work)->path = camera;
            camera_override(camera_step_path, 0, CAMERA_BY_STEP);
            PATH_VARS(work)->wait[number] = 0;
            HELD(work) |= 1 << number;
        } else {
            if (camera_at_comp_level() || !camera_at_event_level()) {
                PATH_VARS(work)->wait[number] = 0;
                HELD(work) &= ~(1 << number);
                return;
            }
            if (!(camera_work->submode & CAMERA_PATH_DONE)) {
                if (++PATH_VARS(work)->wait[number] > CAMERA_CLEAR_STEPS) {
                    PATH_VARS(work)->wait[number] = CAMERA_CLEAR_STEPS;
                }
                continue;
            }
            if (PATH_VARS(work)->wait[number] < CAMERA_CLEAR_STEPS &&
                !(camera_work->submode & CAMERA_PATH_FAR)) {
                continue;
            }
            if (player->pos.x <= HIGH(work).x + CAMERA_LEAVE &&
                player->pos.y <= HIGH(work).y + CAMERA_LEAVE &&
                player->pos.z <= HIGH(work).z + CAMERA_LEAVE &&
                player->pos.x >= LOW(work).x - CAMERA_LEAVE &&
                player->pos.y >= LOW(work).y - CAMERA_LEAVE &&
                player->pos.z >= LOW(work).z - CAMERA_LEAVE) {
                distance = 0.0F;
                for (i = camera->first; i < path->count - camera->skip; i++) {
                    to.x = path->points[i].pos.x;
                    to.y = path->points[i].pos.y;
                    to.z = path->points[i].pos.z;
                    njSubVector(&to, &player->pos);
                    away = njScalor(&to);
                    if (away > distance) {
                        distance = away;
                    }
                }
                if (distance <= CAMERA_LEAVE) {
                    continue;
                }
            }
            PATH_VARS(work)->wait[number] = 0;
            HELD(work) &= ~(1 << number);
            camera_restore();
        }
    }
}

/* box the stretch, grown by CAMERA_REACH, and hand the task on to
 * path_camera_step, whose test for a player grows the box by CAMERA_REACH
 * again */
static void path_camera_init(struct task *task)
{
    struct task_work *work = task->work;
    struct camera_path *camera = path_camera_list[work->submode];
    struct path *path = PATH_VARS(work)->path;
    int i;

    HELD(work) = 0;
    PATH_VARS(work)->wait[0] = PATH_VARS(work)->wait[1] = CAMERA_WAIT;
    HIGH(work).x = LOW(work).x = path->points[camera->first].pos.x;
    HIGH(work).y = LOW(work).y = path->points[camera->first].pos.y;
    HIGH(work).z = LOW(work).z = path->points[camera->first].pos.z;
    for (i = camera->first; i < path->count - camera->skip; i++) {
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
    HIGH(work).x += CAMERA_REACH;
    HIGH(work).y += CAMERA_REACH;
    HIGH(work).z += CAMERA_REACH;
    LOW(work).x -= CAMERA_REACH;
    LOW(work).y -= CAMERA_REACH;
    LOW(work).z -= CAMERA_REACH;
    task->step = path_camera_step;
}

/* SHC places functions in source order, so path_camera_4 stays before
 * path_camera_0, as the image has it (0x8c06fa22) */
void path_camera_4(struct task *task)
{
    task->work->submode = 4;
    path_camera_init(task);
}

void path_camera_0(struct task *task)
{
    task->work->submode = 0;
    path_camera_init(task);
}

void path_camera_1(struct task *task)
{
    task->work->submode = 1;
    path_camera_init(task);
}

void path_camera_2(struct task *task)
{
    task->work->submode = 2;
    path_camera_init(task);
}

void path_camera_3(struct task *task)
{
    task->work->submode = 3;
    path_camera_init(task);
}

void path_camera_5(struct task *task)
{
    task->work->submode = 5;
    path_camera_init(task);
}

void path_camera_6(struct task *task)
{
    task->work->submode = 6;
    path_camera_init(task);
}

void path_camera_7(struct task *task)
{
    task->work->submode = 7;
    path_camera_init(task);
}

void path_camera_8(struct task *task)
{
    task->work->submode = 8;
    path_camera_init(task);
}

void path_camera_9(struct task *task)
{
    task->work->submode = 9;
    path_camera_init(task);
}

void path_camera_10(struct task *task)
{
    task->work->submode = 10;
    path_camera_init(task);
}

void path_camera_11(struct task *task)
{
    task->work->submode = 11;
    path_camera_init(task);
}

void path_camera_12(struct task *task)
{
    task->work->submode = 12;
    path_camera_init(task);
}

void path_camera_13(struct task *task)
{
    task->work->submode = 13;
    path_camera_init(task);
}
