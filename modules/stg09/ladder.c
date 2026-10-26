/* O HASGO ladder, which players climb on either face */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "path/path.h"
#include "player/player.h"
#include "stage/stage.h"
#include "stg09/casino.h"
#include "stg09/ladder.h"
#include "system/task.h"
#include "system/util.h"

/* the points of the ladder's two faces, and the path its child task holds
 * players on. ladder_step allocates the three from the heap, and
 * ladder_destroy frees them */
struct ladder_vars {
    struct path_point *back_points; /* on the ladder's +z face */
    struct path_point *points; /* on its -z face */
    struct path *path; /* its points are those of the face on player 0's side */
};

#define VARS(work) ((struct ladder_vars *)&(work)->vars)
/* TRUE once the ladder has requested act 0, after which no one climbs it */
#define REQUESTED(work) ((work)->mode)

extern NJS_OBJECT ladder_object;

/* destroyed beyond about 600 units of the camera and player 0 */
#define RANGE ((600.0F * 600.0F) + 10.0F)

/* each face's RUNGS points, one at each rung, lie RUNG_SPACING apart along
 * the ladder's y. the lowest is a rung above FOOT_Y, and each lies FACE_Z
 * from the ladder's middle along its z */
#define RUNGS 48
#define RUNG_SPACING 3.0F
#define FOOT_Y 6.0F
#define FACE_Z 2.9F

/* the ladder's top is TOP_Y above its position. in act 1, player 0 within
 * TOP_REACH of the top has the ladder request act 0 */
#define TOP_Y 120.0F
#define TOP_REACH 20.0F

/* a player within LADDER_REACH of one of the path's points holds on to the
 * ladder there. the distance also grows the path's box, which a player must
 * be in first */
#define LADDER_REACH 8.0F
/* the steps the path waits after it lets a player go, before it can take
 * that player again */
#define LADDER_WAIT 60

/* the path task's work. LOW and HIGH are the corners of the box about the
 * path's points. HELD has a bit for each player the path holds, 1 << the
 * player's number. HELD_EITHER has both bits */
#define LOW(work) ((work)->pos)
#define HIGH(work) ((work)->scl)
#define HELD(work) ((work)->tick)
#define HELD_EITHER (1 << 0 | 1 << 1)

enum ladder_path_mode { PATH_SET_UP, PATH_RUN };

/* the ladder's body, and its rails at either side. ladder_step scales their
 * x by scl.x */
static struct collision_shape ladder_shapes[3] = {
    {SHAPE_RECTANGLE,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 75.0F, 0.9F},
     13.5F,
     75.0F,
     0.9F,
     {0},
     0,
     NULL},
    {SHAPE_CYLINDER,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {13.0F, 75.0F, 0.0F},
     0.5F,
     75.0F,
     0.0F,
     {0},
     0,
     NULL},
    {SHAPE_CYLINDER,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {-13.0F, 75.0F, 0.0F},
     0.5F,
     75.0F,
     0.0F,
     {0},
     0,
     NULL},
};

static void ladder_run(struct task *task);
static void ladder_draw(struct task *task);
static void ladder_destroy(struct task *task);
static void ladder_path_step(struct task *task);

void ladder_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task *child;
    struct path_point *points;
    struct path_point *back_points;
    struct path *path;
    Angle angle;
    NJS_POINT3 world;
    NJS_POINT3 local;
    int i;
    int count;

    if (placement_try_leave_by(task, RANGE)) {
        return;
    }
    task->step = ladder_run;
    task->draw = ladder_draw;
    task->destructor = ladder_destroy;
    REQUESTED(work) = FALSE;
    if (placement_editing(task)) {
        return;
    }
    collision_init(task, ladder_shapes, COLLISION_OBJECT, 1, 3);
    work->collision->shapes[0].center.x *= work->scl.x;
    work->collision->shapes[0].radius *= work->scl.x;
    work->collision->shapes[1].center.x *= work->scl.x;
    work->collision->shapes[2].center.x *= work->scl.x;
    child = task_create_child(TASK_WORK, ladder_path_step, task);
    PATH_VARS(child->work)->stage_act = stage_get_stage_act();
    child->work->ang.x = 0;
    child->work->ang.y = work->ang.y;
    child->work->ang.z = work->ang.z;

    count = RUNGS;
    local.x = 0.0F;
    local.y = FOOT_Y;
    local.z = -FACE_Z;
    njPushMatrix(&_unit_matrix_);
    TURN_VIA(njRotateZ, work->ang.z, angle);
    TURN_VIA(njRotateY, work->ang.y, angle);
    points = heap_alloc(count * sizeof(struct path_point));
    VARS(work)->points = points;
    for (i = 0; i < count; i++) {
        local.y += RUNG_SPACING;
        points[i].ang_x = work->ang.x;
        points[i].ang_z = work->ang.z;
        points[i].length = RUNG_SPACING;
        njCalcPoint(NULL, &local, &world);
        points[i].pos.x = work->pos.x + world.x;
        points[i].pos.y = work->pos.y + world.y;
        points[i].pos.z = work->pos.z + world.z;
    }
    njPopMatrix(1);

    local.x = 0.0F;
    local.y = FOOT_Y;
    local.z = -FACE_Z;
    njPushMatrix(&_unit_matrix_);
    TURN_VIA(njRotateZ, work->ang.z, angle);
    TURN_VIA(njRotateY, work->ang.y + NJM_DEG_ANG(180), angle);
    back_points = heap_alloc(count * sizeof(struct path_point));
    VARS(work)->back_points = back_points;
    for (i = 0; i < count; i++) {
        local.y += RUNG_SPACING;
        back_points[i].ang_x = work->ang.x;
        back_points[i].ang_z = work->ang.z;
        back_points[i].length = RUNG_SPACING;
        njCalcPoint(NULL, &local, &world);
        back_points[i].pos.x = work->pos.x + world.x;
        back_points[i].pos.y = work->pos.y + world.y;
        back_points[i].pos.z = work->pos.z + world.z;
    }
    njPopMatrix(1);

    path = heap_alloc(sizeof(struct path));
    VARS(work)->path = path;
    path->ends = PATH_STOP;
    path->count = count;
    path->length = (count - 1) * RUNG_SPACING;
    path->points = points;
    path->step = ladder_path_step;
    PATH_VARS(child->work)->path = path;
}

static void ladder_run(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *player = player_work[0];
    struct path *path;
    NJS_POINT3 top;
    NJS_VECTOR delta;
    Sint16 bearing;
    Float distance;

    if (placement_try_leave_by(task, RANGE)) {
        return;
    }
    ladder_draw(task);
    if (task->child) {
        path = VARS(work)->path;
        {
            /* @hack spelling: byte cursors over delta and the two positions.
             * SHC reads the positions with postincrement loads (FMOV.S @R6+,FR3
             * at 0x0c918ff8). through the fields, SHC saves R10 as well, the
             * function runs 2 bytes longer, and 78 of ladder_run's 160 words
             * differ */
            char *out = (char *)&delta;
            const char *to = (const char *)&player->pos;
            const char *from = (const char *)&work->pos;

            /* @hack redundant: a test of player that never runs, a
             * reference SHC counts when it assigns registers; without it, the
             * three component subtractions change and 16 of 160 words differ */
            if (0) {
                if (player) {
                }
            }
            *(Float *)out = *(const Float *)to - *(const Float *)from;
            out += sizeof(Float);
            to += sizeof(Float);
            from += sizeof(Float);
            *(Float *)out = *(const Float *)to - *(const Float *)from;
            out += sizeof(Float);
            to += sizeof(Float);
            from += sizeof(Float);
            *(Float *)out = *(const Float *)to - *(const Float *)from;
        }
        /* player 0's bearing from the ladder's +z. past a quarter turn either
         * way, player 0 is on the ladder's -z side */
        bearing =
            (Angle)(atan2f(delta.x, delta.z) * ANGLE_PER_RADIAN) - work->ang.y;
        if (bearing > NJM_DEG_ANG(90) || bearing < -NJM_DEG_ANG(90)) {
            task->child->work->ang.y = work->ang.y;
            path->points = VARS(work)->points;
            work->collision->shapes[0].center.z = ladder_shapes[0].center.z;
        } else {
            task->child->work->ang.y = work->ang.y + NJM_DEG_ANG(180);
            path->points = VARS(work)->back_points;
            work->collision->shapes[0].center.z = -ladder_shapes[0].center.z;
        }
        task_step_children(task);
        collision_register(work);
    }
    top = work->pos;
    top.y += TOP_Y;
    if (!REQUESTED(work) && (Uint8)stage_get_stage_act() == 1 && task->child &&
        (HELD(task->child->work) & HELD_EITHER)) {
        distance = njDistanceP2P(&top, &player->pos);
        if (distance < TOP_REACH) {
            casino_request = CASINO_REQUEST_ACT0;
            player_set_mode(0, PLAYER_MODE_RESET);
            task_destroy_children(task);
            REQUESTED(work) = TRUE;
        }
    }
}

static void ladder_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle angle;

    casino_set_texture(CASINO_TEXTURES_ACTS_12);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateZ, work->ang.z, angle);
    TURN_VIA(njRotateY, work->ang.y, angle);
    njScale(NULL, work->scl.x, 1.0F, 1.0F);
    draw_tree(&ladder_object);
    njPopMatrix(1);
}

/* destroy the path task, then free the path and the points it reads */
static void ladder_destroy(struct task *task)
{
    struct task_work *work = task->work;
    int i;

    if (task->child) {
        for (i = 0; i < 2; i++) {
            if (!player_work[i]) {
                continue;
            }
            /* @bug logic: the test reads the ladder's own tick, not the
             * path task's HELD, which holds the players' bits (MOV.B
             * @(3,R13),R0 at 0x0c9191b6, R13 the ladder's work). nothing sets
             * the ladder's tick, so the destructor resets no player. a player
             * still climbing stays on the path that ladder_destroy frees */
            if (work->tick & (1 << i)) {
                player_set_mode(i, PLAYER_MODE_RESET);
            }
        }
        task_destroy_children(task);
    }
    heap_free(VARS(work)->path);
    heap_free(VARS(work)->back_points);
    heap_free(VARS(work)->points);
}

/* path_ledge_step, but with LADDER_REACH and LADDER_WAIT for its reach and
 * wait. a player within reach of a point holds on to the ladder there,
 * turned to the work's ang. the path holds that player until PLAYER_ON_PATH
 * clears */
static void ladder_path_step(struct task *task)
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
        PATH_VARS(work)->wait[0] = PATH_VARS(work)->wait[1] = LADDER_WAIT;
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
        HIGH(work).x += LADDER_REACH;
        HIGH(work).y += LADDER_REACH;
        HIGH(work).z += LADDER_REACH;
        LOW(work).x -= LADDER_REACH;
        LOW(work).y -= LADDER_REACH;
        LOW(work).z -= LADDER_REACH;
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
                if (++PATH_VARS(work)->wait[number] < LADDER_WAIT) {
                    continue;
                }
                PATH_VARS(work)->wait[number] = LADDER_WAIT;
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
                    if (njScalor(&to) < LADDER_REACH) {
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
