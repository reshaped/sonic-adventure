/* the object list's B LEAF, PU WIND and WC WIND, and the leaves they and the
 * wind path send out. B LEAF blows leaves out, and the winds lift players */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "camera/camera.h"
#include "collision/polygon.h"
#include "debug/editor.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "path/path.h"
#include "player/player.h"
#include "sound/sound.h"
#include "stg02/wind.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

/* an eighth of an octagonal column of height 1 about the y axis. it is five
 * lines, as pairs of points. a spoke from the axis and a side lie at the
 * bottom, and again at the top. an upright joins the spokes' ends */
static NJS_POINT3 wind_wedge_points[10] = {
#include "stg02/wind/wind_wedge_points.inc"
};
static NJS_COLOR wind_wedge_colors[10] = {
#include "stg02/wind/wind_wedge_colors.inc"
};
/* @hack type: an array of one, whose address windy_column_wind_step loads
 * once, into R9 at 0x0c908ba2, for all its calls; as a scalar, it loads the
 * address for each, and 364 of that function's 898 words differ */
static NJS_POINT3COL wind_wedge[1] = {
#include "stg02/wind/wind_wedge.inc"
};
/* the twelve edges of a unit cube about the origin, as pairs of points. the set
 * editor draws a box wind's box with them, and marks each kind's position with
 * them */
static NJS_POINT3 wind_cube_points[24] = {
#include "stg02/wind/wind_cube_points.inc"
};
static NJS_COLOR wind_box_colors[24] = {
#include "stg02/wind/wind_box_colors.inc"
};
/* @hack type: an array of one, whose address windy_box_wind_step loads once,
 * into R9 at 0x0c908768, for the box and its slices; as a scalar, it loads
 * the address for each call, that function is 32 bytes shorter, and 107 of
 * its 633 words differ */
static NJS_POINT3COL wind_box[1] = {
#include "stg02/wind/wind_box.inc"
};
static NJS_COLOR wind_origin_colors[24] = {
#include "stg02/wind/wind_origin_colors.inc"
};
static NJS_POINT3COL wind_origin = {
#include "stg02/wind/wind_origin.inc"
};

/* the three models a leaf is drawn with */
extern NJS_OBJECT dat_0c9bfa2c;
extern NJS_OBJECT dat_0c9bfb30;
extern NJS_OBJECT dat_0c9bfc34;

/* a leaf's work->mode */
enum leaf_mode {
    LEAF_START = 0,
    LEAF_FLY = 2, /* blown out, or flying along its path */
    LEAF_DRIFT = 4, /* rising on a wind, or swarming about a player */
    LEAF_FALL = 7,
    LEAF_GATHER = 8 /* closing on its point of the path */
};

#define LEAF_SIZE(work) ((work)->tick) /* in sixteenths of its model's */
#define LEAF_LIFE(work) TASK_TIMER(work) /* the steps it has left */
#define LEAF_SPEED(work) ((work)->scl)

/* the work->vars of the leaves leaf_draw draws: blown, rising and swarming
 * ones */
struct leaf_vars {
    Uint16 spin_x; /* added to ang.x, as spin_y is to ang.y */
    Uint16 spin_y;
    Sint8 model; /* which of the three models, 0 to 2 */
    Uint8 unk_05[3];
    union {
        Float top; /* a rising leaf's height, above which it falls */
        struct task_work *player; /* a swarming leaf's: its player's work */
    } around;
};

#define LEAF(work) ((struct leaf_vars *)&(work)->vars)

/* the steps a blown or a rising leaf lives, and a swarming leaf */
#define LEAF_STEPS 100
#define SWARM_STEPS 300
/* a blown leaf's upward speed falls BLOWN_FALL a step. a rising or a
 * swarming leaf drifts LEAF_ACCEL a step faster, and falls LEAF_ACCEL a step
 * faster. no leaf falls faster than LEAF_FALL_MAX */
#define BLOWN_FALL 0.05F
#define LEAF_ACCEL 0.1F
#define LEAF_FALL_MAX 1.0F
/* a swarming leaf starts within half SWARM_SPREAD of its player on x and z.
 * it starts up to SWARM_SPREAD below the player. within SWARM_NEAR of the
 * player, each axis of its speed gains SWARM_PULL away from the player. from
 * there to SWARM_FAR, each gains SWARM_PULL toward it. once the player is more
 * than SWARM_FAR above it, it falls for good */
#define SWARM_SPREAD 20.0F
#define SWARM_NEAR 10.0F
#define SWARM_FAR 30.0F
#define SWARM_PULL 0.2F

static void leaf_draw(struct task *task);
static Float leaf_push(struct task_work *work, NJS_VECTOR *push);

/* the step of a leaf B LEAF blows out. the leaf lives LEAF_STEPS steps, and
 * player 0 pushes it. while it rises, it tumbles with its upward speed. while
 * it falls, it rocks to and fro, and slides the way it rocks */
static void leaf_blown_step(struct task *task)
{
    struct task_work *work = task->work;
    NJS_VECTOR push;
    Angle rock;

    if (placement_try_leave(task)) {
        return;
    }
    switch (work->mode) {
    case LEAF_START:
        LEAF_LIFE(work) = LEAF_STEPS;
        work->ang.y = RANDOM_ANGLE();
        work->ang.x = RANDOM_ANGLE();
        LEAF(work)->spin_x = 0;
        LEAF(work)->spin_y = (Sint16)(RANDOM(-0.5F, 0.5F) * 512.0F);
        LEAF_SIZE(work) = (Uint8)(RANDOM(1.0F, 3.0F) * 16.0F);
        work->mode = LEAF_FLY;
        LEAF(work)->model = (int)(rand() * (10.0F * RAND_SCALE)) % 3;
        break;
    case LEAF_FLY:
        if (--LEAF_LIFE(work) == 0) {
            task_kill(task);
            return;
        }
        leaf_push(work, &push);
        LEAF_SPEED(work).x += push.x;
        LEAF_SPEED(work).y += push.y - BLOWN_FALL;
        LEAF_SPEED(work).z += push.z;
        VECTOR_ADD(&work->pos, &LEAF_SPEED(work));
        work->ang.y += LEAF(work)->spin_y;
        if (LEAF_SPEED(work).y > 0.0F) {
            work->ang.x += (Angle)(LEAF_SPEED(work).y * 2048.0F);
            LEAF(work)->spin_x = 0;
        } else {
            if (!(work->ang.x & 0x4000)) {
                LEAF(work)->spin_x -= 384;
            } else {
                LEAF(work)->spin_x += 384;
            }
            /* @bug arithmetic: spin_x is unsigned (EXTU.W at 0x0c90707a), so
             * a turn back by n adds 65536 - n to ang.x, not -n. ang.x gathers
             * whole turns that way, and the test against 0x8000 below (CMP/GT
             * at 0x0c907082) compares it with those turns in it */
            work->ang.x += LEAF(work)->spin_x;
            rock = work->ang.x;
            if (work->ang.x > 0x8000) {
                rock += 0x8000;
            }
            LEAF_SPEED(work).x += njSin(rock) * njSin(work->ang.y) * 0.5F;
            LEAF_SPEED(work).z += njSin(rock) * njCos(work->ang.y) * 0.5F;
            if (LEAF_SPEED(work).y < -LEAF_FALL_MAX) {
                LEAF_SPEED(work).y = -LEAF_FALL_MAX;
            }
        }
        break;
    }
    leaf_draw(task);
}

static void leaf_draw(struct task *task)
{
    struct task_work *work = task->work;
    Float scale = LEAF_SIZE(work) * 0.0625F;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateZ, work->ang.z);
    TURN(njRotateX, work->ang.x);
    TURN(njRotateY, work->ang.y);
    njScale(NULL, scale, scale, scale);
    switch (LEAF(work)->model) {
    case 0:
        draw_tree(&dat_0c9bfa2c);
        break;
    /* @hack spelling: case 2 ahead of case 1, which puts model 2's call ahead
     * of model 1's (0x0c9071a6, 0x0c9071ac); in order, 3 of 84 words
     * differ */
    case 2:
        draw_tree(&dat_0c9bfc34);
        break;
    /* @hack redundant: a case for model 1, the default's own call, with its
     * CMP/EQ #1 at 0x0c907194. without it, the function is 10 bytes shorter,
     * the functions after it move, and 36 of 84 words differ */
    case 1:
        draw_tree(&dat_0c9bfb30);
        break;
    default:
        draw_tree(&dat_0c9bfb30);
        break;
    }
    njPopMatrix(1);
}

/* the step of a leaf a wind sends up. the leaf starts up at the speed the wind
 * gives it, and lives LEAF_STEPS steps. below top, it drifts on each frame's
 * first step. it gains LEAF_ACCEL along its heading in x and z. it also gains
 * LEAF_ACCEL times the sine of twice ang.x upward. above top, it falls until
 * it is below top again */
static void leaf_rise_step(struct task *task)
{
    struct task_work *work = task->work;
    Float accel;
    Float choice;
    Float random_scale;

    if (placement_try_leave(task)) {
        return;
    }
    if (--LEAF_LIFE(work) == 0) {
        task_kill(task);
        return;
    }
    accel = LEAF_ACCEL;
    switch (work->mode) {
    case LEAF_START:
        LEAF_LIFE(work) = LEAF_STEPS;
        random_scale = RAND_SCALE;
        choice = rand() * random_scale;
        LEAF_SPEED(work).x = -0.5F + choice;
        LEAF_SPEED(work).z = -0.5F + (rand() * random_scale);
        work->ang.y = RANDOM_ANGLE();
        work->ang.x = RANDOM_ANGLE();
        LEAF(work)->spin_x =
            (Sint16)((-0.5F + (rand() * random_scale)) * 6553.6F);
        LEAF(work)->spin_y =
            (Sint16)((-0.5F + (rand() * random_scale)) * 6553.6F);
        LEAF_SIZE(work) = (Uint8)(RANDOM(1.0F, 3.0F) * 16.0F);
        work->mode = LEAF_DRIFT;
        LEAF(work)->model = (int)(choice * 10.0F) % 3;
        break;
    case LEAF_DRIFT:
        if (frame_step) {
            break;
        }
        LEAF_SPEED(work).x += njSin(work->ang.y) * accel;
        LEAF_SPEED(work).z += njCos(work->ang.y) * accel;
        LEAF_SPEED(work).y += njSin(work->ang.x * 2) * accel;
        if (work->pos.y > LEAF(work)->around.top) {
            work->mode = LEAF_FALL;
        }
        break;
    case LEAF_FALL:
        if ((LEAF_SPEED(work).y -= accel) < -LEAF_FALL_MAX) {
            LEAF_SPEED(work).y = -LEAF_FALL_MAX;
        }
        if (work->pos.y < LEAF(work)->around.top) {
            work->mode = LEAF_DRIFT;
        }
        break;
    }
    work->ang.x += LEAF(work)->spin_x;
    work->ang.y += LEAF(work)->spin_y;
    VECTOR_ADD(&work->pos, &LEAF_SPEED(work));
    leaf_draw(task);
}

/* the step of a leaf a wind sends about a player it lifts. the leaf lives
 * SWARM_STEPS steps, and drifts as a rising leaf does, but on every step. the
 * player's pull keeps it near until the player rises away */
static void leaf_swarm_step(struct task *task)
{
    struct task_work *work = task->work;
    Float accel;
    Float choice;
    Float dx;
    Float dy;
    Float dz;
    Float distance_squared;

    if (placement_try_leave(task)) {
        return;
    }
    accel = LEAF_ACCEL;
    switch (work->mode) {
    case LEAF_START:
        LEAF_LIFE(work) = SWARM_STEPS;
        choice = rand() * RAND_SCALE;
        work->pos.x =
            LEAF(work)->around.player->pos.x + ((choice - 0.5F) * SWARM_SPREAD);
        work->pos.y = -(rand() * (SWARM_SPREAD * RAND_SCALE)) +
                      LEAF(work)->around.player->pos.y;
        work->pos.z = (RANDOM(-0.5F, 0.5F) * SWARM_SPREAD) +
                      LEAF(work)->around.player->pos.z;
        LEAF_SPEED(work).x = RANDOM(-0.5F, 0.5F);
        LEAF_SPEED(work).y = RANDOM(-0.5F, 0.5F);
        LEAF_SPEED(work).z = RANDOM(-0.5F, 0.5F);
        work->ang.y = RANDOM_ANGLE();
        work->ang.x = RANDOM_ANGLE();
        LEAF(work)->spin_x = (Sint16)(RANDOM(-0.5F, 0.5F) * 6553.6F);
        LEAF(work)->spin_y = (Sint16)(RANDOM(-0.5F, 0.5F) * 6553.6F);
        LEAF_SIZE(work) = (Uint8)(RANDOM(1.0F, 3.0F) * 16.0F);
        work->mode = LEAF_DRIFT;
        LEAF(work)->model = (int)(choice * 10.0F) % 3;
        break;
    case LEAF_DRIFT:
        LEAF_SPEED(work).x += njSin(work->ang.y) * accel;
        LEAF_SPEED(work).z += njCos(work->ang.y) * accel;
        LEAF_SPEED(work).y += njSin(work->ang.x * 2) * accel;
        dy = LEAF(work)->around.player->pos.y - work->pos.y;
        if (dy > SWARM_FAR) {
            work->mode = LEAF_FALL;
        }
        dx = LEAF(work)->around.player->pos.x - work->pos.x;
        dz = LEAF(work)->around.player->pos.z - work->pos.z;
        distance_squared = (dx * dx) + (dy * dy) + (dz * dz);
        if (distance_squared < SQUARE(SWARM_NEAR)) {
            if (dx < 0.0F) {
                LEAF_SPEED(work).x -= -SWARM_PULL;
            } else {
                LEAF_SPEED(work).x += -SWARM_PULL;
            }
            if (dy < 0.0F) {
                LEAF_SPEED(work).y -= -SWARM_PULL;
            } else {
                LEAF_SPEED(work).y += -SWARM_PULL;
            }
            if (dz < 0.0F) {
                LEAF_SPEED(work).z -= -SWARM_PULL;
            } else {
                LEAF_SPEED(work).z += -SWARM_PULL;
            }
        } else if (distance_squared < SQUARE(SWARM_FAR)) {
            if (dx < 0.0F) {
                LEAF_SPEED(work).x -= SWARM_PULL;
            } else {
                LEAF_SPEED(work).x += SWARM_PULL;
            }
            if (dy < 0.0F) {
                LEAF_SPEED(work).y -= SWARM_PULL;
            } else {
                LEAF_SPEED(work).y += SWARM_PULL;
            }
            if (dz < 0.0F) {
                LEAF_SPEED(work).z -= SWARM_PULL;
            } else {
                LEAF_SPEED(work).z += SWARM_PULL;
            }
        }
        break;
    case LEAF_FALL:
        if ((LEAF_SPEED(work).y -= accel) < -LEAF_FALL_MAX) {
            LEAF_SPEED(work).y = -LEAF_FALL_MAX;
        }
        break;
    }
    work->ang.x += LEAF(work)->spin_x;
    work->ang.y += LEAF(work)->spin_y;
    VECTOR_ADD(&work->pos, &LEAF_SPEED(work));
    leaf_draw(task);
    if (--LEAF_LIFE(work) == 0) {
        task_kill(task);
    }
}

#define GATHER_SETBACK 120.0F
#define GATHER_STEPS 60
#define CLOSE_ACCEL 0.89F
#define FLY_SPREAD 10.0F
#define FLY_SPEED 1.5F
#define STRAY_SPREAD 50.0F
#define STRAY_SPEED 5.0F
/* a flying leaf kills itself once its count of a frame's later steps passes
 * LATE_STEPS */
#define LATE_STEPS 100
/* past the root of DRAW_RANGE from the camera, a path leaf isn't drawn. past
 * the root of EXPIRE_RANGE, it kills itself */
#define DRAW_RANGE (700.0F * 700.0F)
#define EXPIRE_RANGE (1400.0F * 1400.0F)

/* a path leaf first gathers on its point, then flies along the path. it
 * starts behind the point, set back along the unit vector to the next point.
 * each axis takes GATHER_SETBACK times its own roll from 0.1 to 1.1. for
 * GATHER_STEPS steps, the leaf closes on the point. its speed gains
 * CLOSE_ACCEL a step toward it on each axis. it then starts again within half
 * FLY_SPREAD of the point on each axis. it flies at 1 to 1 + FLY_SPEED a
 * frame. one leaf in 20 takes STRAY_SPREAD and STRAY_SPEED instead. flying,
 * it closes on its place on the path the same way.
 *
 * while the leaf gathers, ang.z sets how fast it tumbles. while it flies,
 * ang.z holds its speed along the path in 256ths. submode then counts a
 * frame's later steps */
void windy_path_leaf_step(struct task *task)
{
    struct task_work *work = task->work;
    struct path_sample sample;
    NJS_POINT3 origin_data;
    NJS_VECTOR direction_data;
    NJS_POINT3 *origin;
    NJS_VECTOR *direction;
    Float magnitude;
    Float choice;
    Float spread;
    Float speed;
    Float distance;
    Float scale;
    Float dy;
    Float dz;
    int index;
    void (*expire)(struct task *task);

    /* @bug memory: the start reads points[index] and points[index + 1]
     * (0x0c9078b0, 0x0c9078b6), and index reaches the path's count at phase 20.
     * a leaf takes that phase once windy_wind_path_step's task_create fails at
     * phases 16 to 19 in a row. that step raises the phase before each try
     * (0x0c90939e). it sets the phase back to 0 only after a leaf takes a phase
     * past 15 (0x0c9093a8 to 0x0c9093c0) */
    if (PATH_LEAF(work)->phase) {
        int count = PATH_LEAF(work)->path->count;
        index = count * PATH_LEAF(work)->phase / 20;
    } else {
        index = 0;
    }
    /* @hack redundant: six never-run tests of task, which keep task in R11
     * and the origin's address in R10 (0x0c90781e, 0x0c9078a4); without them,
     * 17 of 789 words differ */
    if (0) {
        if (task) {
        }
        if (task) {
        }
        if (task) {
        }
        if (task) {
        }
        if (task) {
        }
        if (task) {
        }
    }
    expire = task_kill;
    /* @hack redundant: a never-run test of expire, through which the step
     * calls task_kill, which loads task_kill's address once, into R9 at
     * 0x0c907854, with the scaled index in R8 (0x0c907856), whether the calls
     * go through expire or by name; without it, 22 of 692 words differ */
    if (0) {
        if (expire) {
        }
    }
    switch (work->mode) {
    case LEAF_START:
        /* @hack spelling: origin and direction reached through pointers, which
         * keep their addresses in R10 and R12 (0x0c9078a4, 0x0c9078aa); with
         * origin by name, R8 goes unsaved (0x0c907810) and the function is 2
         * bytes shorter, and with direction by name, 40 of 789 words differ */
        origin = &origin_data;
        direction = &direction_data;
        direction->x = PATH_LEAF(work)->path->points[index + 1].pos.x -
                       (origin->x = PATH_LEAF(work)->path->points[index].pos.x);
        direction->y = PATH_LEAF(work)->path->points[index + 1].pos.y -
                       (origin->y = PATH_LEAF(work)->path->points[index].pos.y);
        direction->z = PATH_LEAF(work)->path->points[index + 1].pos.z -
                       (origin->z = PATH_LEAF(work)->path->points[index].pos.z);
        dy = direction->y;
        dz = direction->z;
        magnitude = (direction->x * direction->x) + (dy * dy) + (dz * dz);
        magnitude = njSqrt(magnitude);
        direction->x /= magnitude;
        direction->y /= magnitude;
        direction->z /= magnitude;
        work->pos.x = origin->x - (((rand() * RAND_SCALE) + 0.1F) *
                                   direction->x * GATHER_SETBACK);
        work->pos.y = origin->y - (((rand() * RAND_SCALE) + 0.1F) *
                                   direction->y * GATHER_SETBACK);
        work->pos.z = origin->z - (((rand() * RAND_SCALE) + 0.1F) *
                                   direction->z * GATHER_SETBACK);
        LEAF_SPEED(work).x = RANDOM(-0.5F, 0.5F);
        LEAF_SPEED(work).y = RANDOM(-0.5F, 0.5F);
        LEAF_SPEED(work).z = RANDOM(-0.5F, 0.5F);
        choice = rand() * RAND_SCALE;
        work->ang.x = (Angle)(choice * 65536.0F);
        work->ang.y = RANDOM_ANGLE();
        work->ang.z = RANDOM_ANGLE();
        LEAF_SIZE(work) = (Uint8)(RANDOM(1.0F, 3.0F) * 16.0F);
        LEAF_LIFE(work) = GATHER_STEPS;
        work->mode = LEAF_GATHER;
        PATH_LEAF(work)->model = (int)(choice * 10.0F) % 3;
        break;
    case LEAF_GATHER:
        /* @hack spelling: origin reached through a pointer, which keeps its
         * address in R4 (0x0c907a24) for its stores and loads; by name, 26 of
         * 789 words differ */
        origin = &origin_data;
        origin->x = PATH_LEAF(work)->path->points[index].pos.x;
        origin->y = PATH_LEAF(work)->path->points[index].pos.y;
        origin->z = PATH_LEAF(work)->path->points[index].pos.z;
        if (work->pos.x < origin->x) {
            LEAF_SPEED(work).x += CLOSE_ACCEL;
        } else {
            LEAF_SPEED(work).x -= CLOSE_ACCEL;
        }
        if (work->pos.y < origin->y) {
            LEAF_SPEED(work).y += CLOSE_ACCEL;
        } else {
            LEAF_SPEED(work).y -= CLOSE_ACCEL;
        }
        if (work->pos.z < origin->z) {
            LEAF_SPEED(work).z += CLOSE_ACCEL;
        } else {
            LEAF_SPEED(work).z -= CLOSE_ACCEL;
        }
        VECTOR_ADD(&work->pos, &LEAF_SPEED(work));
        work->ang.x += work->ang.z >> 5;
        work->ang.y += (work->ang.z * 4) >> 7;
        if (--LEAF_LIFE(work) == 0) {
            spread = FLY_SPREAD;
            speed = FLY_SPEED;
            if (rand() * RAND_SCALE < 0.05F) {
                spread = STRAY_SPREAD;
                speed = STRAY_SPEED;
            }
            work->pos.x = PATH_LEAF(work)->path->points[index].pos.x +
                          (RANDOM(-0.5F, 0.5F) * spread);
            work->pos.y = PATH_LEAF(work)->path->points[index].pos.y +
                          (RANDOM(-0.5F, 0.5F) * spread);
            work->pos.z = PATH_LEAF(work)->path->points[index].pos.z +
                          (RANDOM(-0.5F, 0.5F) * spread);
            path_get_distance(PATH_LEAF(work)->path, index,
                              &PATH_LEAF(work)->distance);
            LEAF_SPEED(work).x = RANDOM(-0.5F, 0.5F);
            LEAF_SPEED(work).y = RANDOM(-0.5F, 0.5F);
            LEAF_SPEED(work).z = RANDOM(-0.5F, 0.5F);
            work->ang.z =
                (Sint16)(((rand() * speed * RAND_SCALE) + 1.0F) * 256.0F);
            work->mode = LEAF_FLY;
        }
        break;
    case LEAF_FLY:
        if (frame_step) {
            work->submode++;
            break;
        }
        if (work->submode > LATE_STEPS) {
            expire(task);
            return;
        }
        PATH_LEAF(work)->distance += work->ang.z * (1.0F / 256.0F);
        sample.distance = PATH_LEAF(work)->distance;
        if (path_sample(PATH_LEAF(work)->path, &sample) != TRUE) {
            expire(task);
            return;
        }
        if (work->pos.x < sample.pos.x) {
            LEAF_SPEED(work).x += CLOSE_ACCEL;
        } else {
            LEAF_SPEED(work).x -= CLOSE_ACCEL;
        }
        if (work->pos.y < sample.pos.y) {
            LEAF_SPEED(work).y += CLOSE_ACCEL;
        } else {
            LEAF_SPEED(work).y -= CLOSE_ACCEL;
        }
        if (work->pos.z < sample.pos.z) {
            LEAF_SPEED(work).z += CLOSE_ACCEL;
        } else {
            LEAF_SPEED(work).z -= CLOSE_ACCEL;
        }
        VECTOR_ADD(&work->pos, &LEAF_SPEED(work));
        work->ang.x += (Angle)((LEAF_SPEED(work).x + LEAF_SPEED(work).y +
                                LEAF_SPEED(work).z) *
                               1024.0F);
        work->ang.y +=
            (Angle)((LEAF_SPEED(work).x + LEAF_SPEED(work).z) * 1024.0F);
        break;
    }
    distance = polygon_point_distance2(&camera_work->pos, &work->pos);
    if (distance > EXPIRE_RANGE) {
        expire(task);
    } else if (distance > DRAW_RANGE) {
        /* @hack spelling: an arm for 700 to 1400 away that returns where
         * control returns anyway, which loads texture_set_stage's and
         * njPushMatrix's addresses into R3 and R2 (0x0c907d8e, 0x0c907d94);
         * with the draw under distance <= DRAW_RANGE instead, 9 of 692 words
         * differ */
        return;
    } else {
        texture_set_stage();
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN(njRotateX, work->ang.x);
        TURN(njRotateY, work->ang.y);
        scale = LEAF_SIZE(work) * 0.0625F;
        njScale(NULL, scale, scale, scale);
        switch (PATH_LEAF(work)->model) {
        case 0:
            draw_tree(&dat_0c9bfa2c);
            break;
        /* @hack spelling: case 2 ahead of case 1, which puts model 2's call
         * ahead of model 1's (0x0c907dfa, 0x0c907e00); in order, 3 of 789
         * words differ */
        case 2:
            draw_tree(&dat_0c9bfc34);
            break;
        /* @hack redundant: a case for model 1, the default's own call, with its
         * CMP/EQ #1 at 0x0c907de8. without it, the function is 10 bytes
         * shorter, leaf_push after it moves, and 38 of 692 words differ */
        case 1:
            draw_tree(&dat_0c9bfb30);
            break;
        default:
            draw_tree(&dat_0c9bfb30);
            break;
        }
        njPopMatrix(1);
    }
}

#define PUSH_REACH 20.0F
#define PUSH_SCALE 0.02F
#define PUSH_LIFT 0.2F

/* return the scale of the push on a leaf from the nearest work within
 * PUSH_REACH of it. the scale is PUSH_REACH less the distance, times
 * PUSH_SCALE. *push gets the unit vector from that work to the leaf. its x
 * and z are scaled, and its y is PUSH_LIFT. with no work that near, the scale
 * and *push are 0. the works are player 0's and the 64 bytes after it */
static Float leaf_push(struct task_work *work, NJS_VECTOR *push)
{
    Float nearest = SQUARE(PUSH_REACH);
    struct task_work *player;
    NJS_VECTOR delta;
    Float distance;
    Uint8 i;

    /* @bug memory: the loop steps player from player 0's work by its own
     * size, so the second pass reads the 64 bytes after that work, not player
     * 1's: MOV.L @R6,R6 at 0x0c907e40 loads player_work[0], and ADD #64,R6 at
     * 0x0c907f0a steps it */
    for (i = 0, player = player_work[0]; i < 2; i++, player++) {
        if (!player) {
            continue;
        }
        /* @hack implicit: each difference stored and read back in the product
         * that squares it. C89 leaves such a read undefined (section 3.3), and
         * SHC evaluates it as the image does: the x store at 0x0c907ece and its
         * reload at 0x0c907ee6, y and z forwarded at 0x0c907ed8 and 0x0c907eec.
         * stored in statements of their own, the function is 8 bytes longer,
         * windy_leaf_blower_step after it moves, and 89 of 97 words differ */
        distance = ((delta.x = work->pos.x - player->pos.x) * delta.x) +
                   ((delta.y = work->pos.y - player->pos.y) * delta.y) +
                   ((delta.z = work->pos.z - player->pos.z) * delta.z);
        if (distance < nearest) {
            nearest = distance;
            push->x = delta.x;
            push->y = delta.y;
            push->z = delta.z;
        }
    }
    if (nearest < SQUARE(PUSH_REACH)) {
        distance = njSqrt(nearest);
        nearest = PUSH_REACH - distance;
        nearest *= PUSH_SCALE;
        njUnitVector(push);
        push->x *= nearest;
        push->y = PUSH_LIFT;
        push->z *= nearest;
    } else {
        nearest = 0.0F;
        push->x = push->y = push->z = nearest;
    }
    return nearest;
}

/* the work->mode of B LEAF and each wind */
enum wind_mode {
    WIND_START = 0,
    WIND_BLOW = 2,
    WIND_LIFT = 4, /* a column's, while it lifts */
    WIND_REST = 5, /* a column's, while it doesn't */
    /* while the set editor holds the task. a box also takes it every other step
     * while the editor is open */
    WIND_EDIT = 9
};

struct blower_vars {
    Uint8 unk_00[4];
    Sint8 model; /* the leaf the set editor shows along its aim */
};

#define BLOWER(work) ((struct blower_vars *)&(work)->vars)

void windy_leaf_blower_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task *child;
    struct task_work *leaf;
    NJS_POINT3 pos;
    Float offset;
    Float start;
    Float random_scale;
    Float bias;
    Float center;
    Float scatter;
    int (*random)(void);
    Bool (*editing)(struct task *task);
    Bool (*push)(NJS_MATRIX *matrix);
    Bool (*pop)(Uint32 count);

    if (placement_try_leave(task)) {
        return;
    }
    /* @hack spelling: placement_editing called through editing, which loads its
     * address once, into R8 at 0x0c907f96, with rand's in R11 (0x0c907f98);
     * called by name, 23 of 434 words differ */
    editing = placement_editing;
    /* @hack spelling: rand called through random, which loads its address once,
     * into R11 at 0x0c907f98, and leaves the leaf's work in R9 (0x0c907ffe);
     * called by name, without the call below, 33 of 434 words differ */
    random = rand;
    /* @hack redundant: a never-run call of random, which keeps rand's address
     * in R11 and the leaf's work in R9 (0x0c907f98, 0x0c907ffe); without it,
     * 12 of 434 words differ */
    if (0) {
        random();
    }
    switch (work->mode) {
    case WIND_START:
        work->mode = WIND_BLOW;
        BLOWER(work)->model = (int)(random() * (10.0F * RAND_SCALE)) % 3;
        break;
    case WIND_BLOW:
        if (editing(task)) {
            break;
        }
        if (++TASK_TIMER(work) & 13) {
            return;
        }
        if ((child = task_create(TASK_WORK, 6, leaf_blown_step)) != NULL) {
            leaf = child->work;
            leaf->pos = work->pos;
            bias = 1.0F;
            center = -0.5F;
            scatter = 0.5F;
            random_scale = RAND_SCALE;
            /* @hack redundant: a never-run store of random_scale, which keeps
             * task in R13 (0x0c907f80) and the leaf's work in R9
             * (0x0c907ffe); without it, 38 of 434 words differ */
            if (0) {
                LEAF_SPEED(leaf).x = random_scale;
            }
            /* @hack redundant: a never-run store of scatter, which keeps bias
             * in FR13 and scatter in FR14 (0x0c908000, 0x0c908010); without
             * it, 9 of 434 words differ */
            if (0) {
                scatter = 0.0F;
            }
            LEAF_SPEED(leaf).x =
                ((((random() * random_scale) + center) * scatter) + bias) *
                njCos(work->ang.x) * njSin(work->ang.y) * work->scl.x;
            LEAF_SPEED(leaf).y =
                ((((random() * random_scale) + center) * scatter) + bias) *
                njSin(work->ang.x) * work->scl.x;
            LEAF_SPEED(leaf).z =
                ((((random() * random_scale) + center) * scatter) + bias) *
                njCos(work->ang.x) * njCos(work->ang.y) * work->scl.x;
        }
        break;
    case WIND_EDIT:
        /* while the set editor holds the blower, draw its leaf every 4 along
         * its aim, out to 30. the first sits 0 to 3 out, one further each step.
         * then draw a unit cube at the position, and label the fields */
        if (work->scl.x < 0.1F) {
            work->scl.x = 0.1F;
        } else if (work->scl.x > 10.0F) {
            work->scl.x = 10.0F;
        }
        if (!editing(task)) {
            work->mode = WIND_START;
        }
        start = ++TASK_TIMER(work) & 3;
        /* @hack spelling: njPopMatrix and njPushMatrix called through pop and
         * push, which SHC loads once, into R8 and R9 (0x0c908130,
         * 0x0c908138), and which keep placement_editing's address in R8 and
         * the leaf's work in R9 (0x0c907f96, 0x0c907ffe); called by name,
         * without the test below, both pairs trade registers and 17 of 434
         * words differ */
        /* @hack spelling: pop set before push, and both after the timer; set
         * push first, the pair trade registers and 4 of 434 words differ, and
         * set before the timer, placement_editing's address and the leaf's
         * work trade and 13 differ */
        pop = njPopMatrix;
        push = njPushMatrix;
        /* @hack redundant: a never-run test of push, which keeps both pairs
         * in those registers; without it, 17 of 434 words differ */
        if (0) {
            if (push) {
            }
        }
        /* NOLINTNEXTLINE(bugprone-float-loop-counter) */
        for (offset = start; offset < 30.0F; offset += 4.0F) {
            pos.x = work->pos.x +
                    (njCos(work->ang.x) * njSin(work->ang.y) * offset);
            pos.y = work->pos.y + (njSin(work->ang.x) * offset);
            pos.z = work->pos.z +
                    (njCos(work->ang.x) * njCos(work->ang.y) * offset);
            texture_set_stage();
            push(NULL);
            njTranslate(NULL, pos.x, pos.y, pos.z);
            njScale(NULL, 2.0F, 2.0F, 2.0F);
            switch (BLOWER(work)->model) {
            case 0:
                draw_tree(&dat_0c9bfa2c);
                break;
            /* @hack spelling: case 2 ahead of case 1, which puts model 2's call
             * ahead of model 1's (0x0c9081d4, 0x0c9081da); in order, 5 of
             * 434 words differ */
            case 2:
                draw_tree(&dat_0c9bfc34);
                break;
            /* @hack redundant: a case for model 1, the default's own call, with
             * its CMP/EQ #1 at 0x0c9081c2. without it, the function is 8 bytes
             * shorter, the functions after it move, and 101 of 322 words
             * differ */
            case 1:
                draw_tree(&dat_0c9bfb30);
                break;
            default:
                draw_tree(&dat_0c9bfb30);
                break;
            }
            pop(1);
            /* @hack redundant: a never-run store of 2.0 to pos.x, which keeps
             * 2.0 in FR15 and offset in FR14 (0x0c908148, 0x0c90814a);
             * without it, 28 of 434 words differ */
            if (0) {
                pos.x = 2.0F;
            }
        }
        push(NULL);
        njTranslateV(NULL, &work->pos);
        njDrawLine3D(&wind_origin, 12, 0);
        pop(1);
        njPrintC(EDITOR_LABEL(EDITOR_ROW_ANG_X), "<- RotX");
        njPrintC(EDITOR_LABEL(EDITOR_ROW_ANG_Y), "<- RotY");
        njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_X), "<- WindSpeed{0.1-10.0}");
        return;
    /* @hack redundant: a default that only breaks, which sets WIND_EDIT through
     * R2 (0x0c9082a8); without it, 2 of 434 words differ */
    default:
        break;
    }
    if (editing(task)) {
        work->mode = WIND_EDIT;
    }
}

#define SE_BOX_WIND 56

/* a box's sizes in scl, clamped between 10 and 200 */
#define BOX_MIN 10.0F
#define BOX_MAX 200.0F
/* a box's power, in the low byte of ang.x. it is clamped between 1 and 8,
 * though the set editor's label says 1 to 10 */
#define POWER(work) ((Uint8)(work)->ang.x)
#define POWER_MIN 1
#define POWER_MAX 8

/* a wind's dwell counts the steps in a row it has found a player in it. it
 * counts no further than DWELL_MAX. the wind's updraft lifts the player once
 * the dwell passes DWELL_WAIT. the updraft is then LIFT_SCALE of the wind's
 * power less the player's upward speed */
#define DWELL_MAX 32
#define DWELL_WAIT 2
#define LIFT_SCALE 0.1F

/* a wind's roll for a rising leaf passes below LEAF_ODDS plus its size times
 * its odds. a box's size is its volume, and its odds BOX_ODDS. a column's size
 * is its radius squared times its height, and its odds COLUMN_ODDS. a box
 * sends a leaf up on a pass that follows more than LEAF_PASSES since its
 * last */
#define LEAF_ODDS 0.3F
#define BOX_ODDS 0.000000125F
#define COLUMN_ODDS 0.000001F
#define LEAF_PASSES 5
/* the odds that a wind sends a leaf to swarm about a player it lifts, each
 * step */
#define SWARM_ODDS 0.5F

struct box_wind_vars {
    Uint8 dwell[4];
    Uint8 unk_04[2];
    /* how far above the box's bottom the set editor's slices start, mod 128.
     * the offset rises by the power each step */
    Sint8 slice_offset;
    Uint8 unk_07;
    /* @hack type: passes as the first of four bytes, so its stores and its
     * increment go through work + 16 in a register (0x0c908394, 0x0c908414,
     * 0x0c908428); as a Sint8 of its own, 295 of 633 words differ */
    Sint8 passes[4]; /* the rolls passed since the last rising leaf */
};

#define BOX(work) ((struct box_wind_vars *)&(work)->vars)
/* TRUE once the box has played its sound since it started */
#define PLAYED(work) ((work)->submode)

static void box_wind_destroy(struct task *task);

/* each step, the box rolls for a rising leaf. on every seventh roll that
 * passes, it sends a leaf up from a random point in it. the point ignores the
 * box's turn. the box also lifts each player in it, and may send a leaf to
 * swarm about the player */
void windy_box_wind_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *player;
    struct player *record;
    struct task *child;
    struct task_work *leaf;
    Uint8 i;
    Float top;
    Float bottom;
    Float y;
    Float lift;
    Float density;
    Float ax;
    Float az;
    Float bx;
    Float bz;
    Float half_ax;
    Float half_az;
    Float half_bx;
    Float half_bz;
    Float edge_x;
    Float edge_z;
    Float dx;
    Float dz;

    work->tick = 1;
    if (placement_try_leave(task)) {
        return;
    }
    top = work->pos.y + (work->scl.y * 0.5F);
    bottom = work->pos.y - (work->scl.y * 0.5F);
    if (work->mode == WIND_START || work->mode == WIND_EDIT) {
        if (work->scl.x < BOX_MIN) {
            work->scl.x = BOX_MIN;
        } else if (work->scl.x > BOX_MAX) {
            work->scl.x = BOX_MAX;
        }
        if (work->scl.y < BOX_MIN) {
            work->scl.y = BOX_MIN;
        } else if (work->scl.y > BOX_MAX) {
            work->scl.y = BOX_MAX;
        }
        if (work->scl.z < BOX_MIN) {
            work->scl.z = BOX_MIN;
        } else if (work->scl.z > BOX_MAX) {
            work->scl.z = BOX_MAX;
        }
        if (POWER(work) < POWER_MIN) {
            work->ang.x = POWER_MIN;
        } else if (POWER(work) > POWER_MAX) {
            work->ang.x = POWER_MAX;
        }
    }
    switch (work->mode) {
    case WIND_START:
        /* @hack type: the four dwells cleared as one word, vars.l[0], with one
         * MOV.L at 0x0c90839a; without it, 187 of 633 words differ */
        work->vars.l[0] = 0;
        work->mode = WIND_BLOW;
        BOX(work)->passes[0] = 0;
        PLAYED(work) = FALSE;
        task->destructor = box_wind_destroy;
        break;
    case WIND_BLOW:
        density =
            LEAF_ODDS + (work->scl.x * work->scl.y * work->scl.z * BOX_ODDS);
        if (rand() * RAND_SCALE < density &&
            BOX(work)->passes[0]++ > LEAF_PASSES) {
            BOX(work)->passes[0] = 0;
            if ((child = task_create(TASK_WORK, 6, leaf_rise_step)) != NULL) {
                leaf = child->work;
                leaf->pos.x = (work->scl.x * RANDOM(-0.5F, 0.5F)) + work->pos.x;
                leaf->pos.y = (work->scl.y * RANDOM(-0.5F, 0.5F)) + work->pos.y;
                leaf->pos.z = (work->scl.z * RANDOM(-0.5F, 0.5F)) + work->pos.z;
                LEAF(leaf)->around.top = top;
                LEAF_SPEED(leaf).y = POWER(work);
            }
        }
        for (i = 0; i < 2; i++) {
            if (player_get_task(i)) {
                player = player_get_task(i)->work;
                if (player->pos.y > top || player->pos.y < bottom) {
                    BOX(work)->dwell[i] = 0;
                    continue;
                }
                /* the box's edges along its own x and z, turned with it, and
                 * their halves */
                ax = njCos(work->ang.y) * work->scl.x;
                az = njSin(work->ang.y) * work->scl.x;
                bx = -(njSin(work->ang.y) * work->scl.z);
                bz = njCos(work->ang.y) * work->scl.z;
                half_ax = ax * 0.5F;
                half_az = az * 0.5F;
                half_bx = bx * 0.5F;
                half_bz = bz * 0.5F;
                /* a player outside any of the four sides is out of the box */
                edge_x = -bx;
                edge_z = -bz;
                dx = player->pos.x - work->pos.x - half_ax - half_bx;
                dz = player->pos.z - work->pos.z - half_az - half_bz;
                if ((edge_z * dx) - (edge_x * dz) < 0.0F) {
                    BOX(work)->dwell[i] = 0;
                    continue;
                }
                edge_x = -ax;
                edge_z = -az;
                dx = player->pos.x - work->pos.x - half_ax + half_bx;
                dz = player->pos.z - work->pos.z - half_az + half_bz;
                if ((edge_z * dx) - (edge_x * dz) < 0.0F) {
                    BOX(work)->dwell[i] = 0;
                    continue;
                }
                edge_x = bx;
                edge_z = bz;
                dx = player->pos.x - work->pos.x + half_ax + half_bx;
                dz = player->pos.z - work->pos.z + half_az + half_bz;
                if ((edge_z * dx) - (edge_x * dz) < 0.0F) {
                    BOX(work)->dwell[i] = 0;
                    continue;
                }
                edge_x = ax;
                edge_z = az;
                dx = player->pos.x - work->pos.x + half_ax - half_bx;
                dz = player->pos.z - work->pos.z + half_az - half_bz;
                if ((edge_z * dx) - (edge_x * dz) < 0.0F) {
                    BOX(work)->dwell[i] = 0;
                    continue;
                }
                if (++BOX(work)->dwell[i] > DWELL_MAX) {
                    BOX(work)->dwell[i] = DWELL_MAX;
                }
                record = player_get(i);
                if (BOX(work)->dwell[i] > DWELL_WAIT) {
                    lift = (POWER(work) - record->speed.y) * LIFT_SCALE;
                } else {
                    lift = 0.0F;
                }
                if (i == 0 && !PLAYED(work)) {
                    PLAYED(work) = TRUE;
                    sound_play_at_for(SE_BOX_WIND, (int)task, 1, 0, work->pos.x,
                                      work->pos.y, work->pos.z, 2);
                }
                player_updraft(i, 0.0F, lift, 0.0F);
                if (rand() * RAND_SCALE < SWARM_ODDS) {
                    if ((child = task_create(TASK_WORK, 6, leaf_swarm_step)) !=
                        NULL) {
                        leaf = child->work;
                        LEAF(leaf)->around.player = player;
                        LEAF_SPEED(leaf).y = POWER(work);
                    }
                }
            }
        }
        break;
    case WIND_EDIT:
        /* draw a unit cube at the position, the box, and level slices of the
         * box 128 apart. the slices rise by the power each step. then label the
         * fields. the box starts again once the editor lets it go */
        if (!placement_editing(task)) {
            work->mode = WIND_START;
        }
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        njDrawLine3D(&wind_origin, 12, 0);
        TURN(njRotateY, -work->ang.y);
        njScale(NULL, work->scl.x, work->scl.y, work->scl.z);
        njDrawLine3D(wind_box, 12, 0);
        njPopMatrix(1);
        BOX(work)->slice_offset += work->ang.x;
        /* NOLINTBEGIN(bugprone-float-loop-counter) */
        for (y = bottom + (BOX(work)->slice_offset & 127); y < top;
             y += 128.0F) {
            /* NOLINTEND(bugprone-float-loop-counter) */
            njPushMatrix(NULL);
            njTranslate(NULL, work->pos.x, y, work->pos.z);
            TURN(njRotateY, -work->ang.y);
            njScale(NULL, work->scl.x, 0.0F, work->scl.z);
            njDrawLine3D(wind_box, 12, 0);
            njPopMatrix(1);
        }
        njPrintC(EDITOR_LABEL(EDITOR_ROW_ANG_X), "<- POWER{ 1-10}");
        njPrintC(EDITOR_LABEL(EDITOR_ROW_ANG_Y), "<- COLLIRotateY");
        njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_X), "<- COLLISIZE X");
        njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_Y), "<- COLLISIZE Y");
        njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_Z), "<- COLLISIZE Z");
        return;
    /* @hack redundant: a default that only breaks, which calls
     * placement_editing through R3 (0x0c908810); without it, 2 of 780 words
     * differ */
    default:
        break;
    }
    if (placement_editing(task) || edit_mode == 1) {
        work->mode = WIND_EDIT;
    }
}

/* a column's kind, the low bit of ang.x. it is 1 for a column whose model rises
 * and sinks, and 0 for one that rests */
#define RISES(work) ((work)->ang.x & 1)

struct column_wind_vars {
    Uint8 dwell[4];
    Uint8 unk_04[2];
    /* how far above the column's bottom the set editor's slices start, mod 128.
     * the offset rises by the power each step */
    Sint8 slice_offset;
    Uint8 unk_07;
    /* the height of the column's model, dat_0c9bfa2c, above work->pos. the
     * model is scaled by the column's radius in x and z. it isn't drawn at
     * MODEL_LOW or below */
    Float model_y;
};

#define COLUMN(work) ((struct column_wind_vars *)&(work)->vars)

/* a column reaches from COLUMN_FOOT below work->pos to scl.y above */
#define COLUMN_FOOT 10.0F
/* a column whose model rises and sinks starts with its model at MODEL_LOW.
 * while player 0 is within scl.x plus MODEL_REACH of work->pos, the model
 * rises MODEL_RISE a step. the column rests once the model passes MODEL_REST.
 * otherwise the model sinks MODEL_SINK a step, to MODEL_LOW */
#define MODEL_LOW (-50.0F)
#define MODEL_REST (-1.0F)
#define MODEL_RISE 1.0F
#define MODEL_SINK 0.5F
#define MODEL_REACH 30.0F

/* work->tick is 1 on the steps the column lifts. on each of those steps, it
 * rolls for a rising leaf. on a pass, it sends a leaf up from a random point
 * near its axis. the point is within half the radius on x and on z. it lies
 * from work->pos up to COLUMN_FOOT above the top. the column also lifts a
 * player it finds in it. it may send a leaf to swarm about the player.
 * resting, its model sits within 1 below work->pos */
void windy_column_wind_step(struct task *task)
{
    struct task_work *work;
    struct task_work *player;
    struct task_motion *motion;
    struct player *record;
    struct task *child;
    struct task_work *leaf;
    Float top;
    Float bottom;
    Float offset;
    Float density;
    Float lift;
    Float distance;
    Float radius;
    Float slice_spacing;
    Float limit;
    Float nearest;
    Uint8 i;
    Sint8 side;

    if (placement_try_leave(task)) {
        return;
    }
    /* @hack spelling: work read from task after the range check, which loads it
     * at 0x0c908908, past the check's return; read at its declaration, 7 of
     * 898 words differ */
    work = task->work;
    top = work->pos.y + work->scl.y;
    bottom = work->pos.y - COLUMN_FOOT;
    if (work->mode == WIND_START || work->mode == WIND_EDIT) {
        if (work->scl.x < 1.0F) {
            work->scl.x = 1.0F;
        } else if (work->scl.x > 100.0F) {
            work->scl.x = 100.0F;
        }
        if (work->scl.y < 10.0F) {
            work->scl.y = 10.0F;
        } else if (work->scl.y > 100.0F) {
            work->scl.y = 100.0F;
        }
        if (work->scl.z < 1.0F) {
            work->scl.z = 1.0F;
        } else if (work->scl.z > 10.0F) {
            work->scl.z = 10.0F;
        }
        /* @hack redundant: a never-run store of 100, which keeps 100 in FR4,
         * loaded once at 0x0c908930, for the radius's and the height's upper
         * clamps; without it, 10 of 898 words differ */
        if (0) {
            work->scl.x = 100.0F;
        }
    }
    /* @hack redundant: a never-run test of polygon_point_distance2's address,
     * which keeps it in R11 and player_work's in R10 (0x0c90896e,
     * 0x0c9089c2); without it, 21 of 898 words differ */
    if (0) {
        if (&polygon_point_distance2) {
        }
    }
    switch (work->mode) {
    case WIND_START:
        work->tick = 0;
        if (RISES(work)) {
            COLUMN(work)->model_y = MODEL_LOW;
            work->mode = WIND_LIFT;
        } else {
            COLUMN(work)->model_y = 0.0F;
            work->mode = WIND_REST;
        }
        break;
    case WIND_LIFT:
        work->tick = 1;
        radius = work->scl.x + MODEL_REACH;
        nearest = limit = radius * radius;
        /* @bug memory: the loop steps player from player 0's work by its
         * own size, so the second pass reads the 64 bytes after that work, not
         * player 1's: MOV.L @R10,R10 at 0x0c9089e0 loads player_work[0], and
         * ADD #64,R10 at 0x0c908a5e steps it */
        for (i = 0, player = player_work[0]; i < 2; i++, player++) {
            if (!player) {
                continue;
            }
            distance = polygon_point_distance2(&work->pos, &player->pos);
            if (distance < nearest) {
                nearest = distance;
            }
        }
        if (nearest < limit) {
            if ((COLUMN(work)->model_y += MODEL_RISE) > MODEL_REST) {
                work->mode = WIND_REST;
            }
        } else {
            if ((COLUMN(work)->model_y += -MODEL_SINK) < MODEL_LOW) {
                COLUMN(work)->model_y = MODEL_LOW;
            }
        }
        break;
    case WIND_REST:
        work->tick = 0;
        COLUMN(work)->model_y = -(rand() * RAND_SCALE);
        if (RISES(work)) {
            radius = work->scl.x + MODEL_REACH;
            nearest = limit = radius * radius;
            /* @bug memory: the loop steps player from player 0's work
             * by its own size, so the second pass reads the 64 bytes after
             * that work, not player 1's: MOV.L @R10,R10 at 0x0c908aea loads
             * player_work[0], and ADD #64,R10 at 0x0c908b0a steps it */
            for (i = 0, player = player_work[0]; i < 2; i++, player++) {
                if (!player) {
                    continue;
                }
                distance = polygon_point_distance2(&work->pos, &player->pos);
                if (distance < nearest) {
                    nearest = distance;
                }
            }
            if (!(nearest < limit)) {
                work->mode = WIND_LIFT;
            }
        }
        break;
    /* @hack redundant: a case for mode 6, which no step of the column sets,
     * with its CMP/EQ #6 at 0x0c90898c. without it, the function is 32 bytes
     * shorter, box_wind_destroy after it moves, and 629 of 712 words
     * differ */
    case 6:
        break;
    case WIND_EDIT:
        /* draw a unit cube at the position, the column, and for a column that
         * rests, its model. then draw level slices of the column 128 apart,
         * which rise by the power each step. then label the fields. the column
         * starts again once the editor lets it go */
        if (!placement_editing(task)) {
            work->mode = WIND_START;
        }
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        njDrawLine3D(&wind_origin, 12, 0);
        njScale(NULL, work->scl.x, work->scl.y, work->scl.x);
        for (side = 0; side < 8; side++) {
            njRotateY(NULL, 0x2000);
            njDrawLine3D(wind_wedge, 5, 0);
        }
        njPopMatrix(1);
        if (!RISES(work)) {
            texture_set_stage();
            njPushMatrix(NULL);
            njTranslateV(NULL, &work->pos);
            njScale(NULL, work->scl.x, 1.0F, work->scl.x);
            draw_tree(&dat_0c9bfa2c);
            njPopMatrix(1);
        }
        COLUMN(work)->slice_offset += (int)work->scl.z & 127;
        /* NOLINTBEGIN(bugprone-float-loop-counter) */
        for (offset = bottom + (COLUMN(work)->slice_offset & 127),
            slice_spacing = 128.0F;
             offset < top; offset += slice_spacing) {
            /* NOLINTEND(bugprone-float-loop-counter) */
            njPushMatrix(NULL);
            njTranslate(NULL, work->pos.x, offset, work->pos.z);
            njScale(NULL, work->scl.x, 0.0F, work->scl.x);
            for (side = 0; side < 8; side++) {
                njRotateY(NULL, 0x2000);
                njDrawLine3D(wind_wedge, 5, 0);
            }
            njPopMatrix(1);
        }
        njPrintC(EDITOR_LABEL(EDITOR_ROW_ANG_X),
                 "<- MODE 0:CONSTANG 1: UP DOWN");
        njPrintC(EDITOR_LABEL(EDITOR_ROW_ANG_Y), "<- RotateY");
        njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_X), "<- COLLISIZE RAD");
        njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_Y), "<- COLLISIZE HEIGHT");
        njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_Z), "<- POWER");
        return;
    /* @hack redundant: a default that only breaks, which calls
     * placement_editing through R3 and sets WIND_EDIT through R3 (0x0c908d4a,
     * 0x0c908d54); without it, 4 of 898 words differ */
    default:
        break;
    }
    if (placement_editing(task)) {
        work->mode = WIND_EDIT;
    }
    if (work->tick == 1) {
        density =
            LEAF_ODDS + (work->scl.x * work->scl.x * work->scl.y * COLUMN_ODDS);
        if (rand() * RAND_SCALE < density) {
            if ((child = task_create(TASK_WORK, 6, leaf_rise_step)) != NULL) {
                leaf = child->work;
                LEAF(leaf)->around.top = top;
                LEAF_SPEED(leaf).y = work->scl.z;
                leaf->pos.x = (work->scl.x * RANDOM(-0.5F, 0.5F)) + work->pos.x;
                leaf->pos.y =
                    (rand() * (top - bottom) * RAND_SCALE) + work->pos.y;
                leaf->pos.z = (work->scl.x * RANDOM(-0.5F, 0.5F)) + work->pos.z;
            }
        }
        /* @bug memory: the loop steps player and motion from player 0's
         * work and motion by their own sizes, so the second pass reads the 64
         * bytes after each, not player 1's: MOV.L @R11,R11 at 0x0c908e0c and
         * MOV.L @R3,R2 at 0x0c908e0a load player_work[0] and player_motion[0],
         * and ADD #64 at 0x0c908f3c and 0x0c908f40 step them */
        for (i = 0, player = player_work[0], motion = player_motion[0]; i < 2;
             i++, player++, motion++) {
            /* @hack redundant: a never-run test of player, which keeps player
             * in R11 and the index into dwell in R10 (0x0c908e0c,
             * 0x0c908e22); without it, 14 of 898 words differ */
            if (0) {
                if (player) {
                }
            }
            if (!player) {
                continue;
            }
            if (player->pos.y > top || player->pos.y < bottom) {
                COLUMN(work)->dwell[i] = 0;
                continue;
            }
            distance = polygon_point_distance2_across(&work->pos, &player->pos);
            radius = work->scl.x;
            if (distance > radius * radius) {
                COLUMN(work)->dwell[i] = 0;
                continue;
            }
            if (++COLUMN(work)->dwell[i] > DWELL_MAX) {
                COLUMN(work)->dwell[i] = DWELL_MAX;
            }
            record = motion->record;
            if (COLUMN(work)->dwell[i] > DWELL_WAIT) {
                lift = (work->scl.z - record->speed.y) * LIFT_SCALE;
            } else {
                lift = 0.0F;
            }
            player_updraft(i, 0.0F, lift, 0.0F);
            if (rand() * RAND_SCALE < SWARM_ODDS) {
                if ((child = task_create(TASK_WORK, 6, leaf_swarm_step)) !=
                    NULL) {
                    leaf = child->work;
                    LEAF(leaf)->around.player = player;
                    LEAF_SPEED(leaf).y = work->scl.z;
                }
            }
        }
    }
    if (COLUMN(work)->model_y > MODEL_LOW) {
        texture_set_stage();
        njPushMatrix(NULL);
        njTranslate(NULL, work->pos.x, work->pos.y + COLUMN(work)->model_y,
                    work->pos.z);
        TURN(njRotateY, work->ang.y);
        njScale(NULL, work->scl.x, 1.0F, work->scl.x);
        draw_tree(&dat_0c9bfa2c);
        njPopMatrix(1);
    }
}

static void box_wind_destroy(struct task *task)
{
}
