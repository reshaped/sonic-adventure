/* JdaiL and JdaiR roll player 0 up a ramp, and tip it onto a rail round the
 * table. each has a target on the table */
#include <ninja.h>
#include <sg_xpt.h>

#include "camera/camera.h"
#include "collision/collision.h"
#include "collision/world.h"
#include "draw/tree.h"
#include "object/common.h"
#include "path/path.h"
#include "player/player.h"
#include "stg09/casino.h"
#include "stg09/jdai.h"
#include "stg09/paths.h"
#include "stg09/pinball.h"
#include "system/task.h"
#include "system/util.h"

extern NJS_OBJECT jdai_object;

struct jdai_vars {
    /* after the rail, the camera stands camera_distance from player 0, level
     * with it, at camera_angle about y */
    Angle camera_angle;
    Float camera_distance;
    struct path *path; /* the ramp's or the rail's */
};

#define VARS(work) ((struct jdai_vars *)&(work)->vars)
/* player 0's distance along the Jdai's path */
#define ALONG(work) ((work)->scl.y)
/* the distance along the path where the ride on it ends */
#define END(work) ((work)->scl.x)
/* player 0's speed up the ramp */
#define SPEED(work) ((work)->scl.z)
/* 0 for JdaiL, 1 for JdaiR, in a Jdai's work, its target's and its
 * release's */
#define SIDE(work) ((work)->submode)

enum jdai_mode { JDAI_WAITING, JDAI_RAMP, JDAI_TIPPING, JDAI_RAIL, JDAI_DOWN };

/* set in a Jdai's work->flags while its targets' bonus holds the camera */
#define JDAI_HELD 0x100
/* set in a target's work->flags while it is touched, so a touch counts once */
#define JDAI_TOUCHING 0x8000

/* the steps the bonus holds the camera, counted down in the release's tick */
#define JDAI_HOLD_STEPS 60

/* hitting both targets while the first table's first countdown runs gives
 * JDAI_BONUS rings. each touch of a target gives JDAI_RINGS */
#define JDAI_BONUS 150
#define JDAI_RINGS 20

/* where each ride ends: its path's length, rounded down */
#define LEFT_RAMP_END 78.0F
#define RIGHT_RAMP_END 76.0F
#define LEFT_RAIL_END 1243.0F
#define RIGHT_RAIL_END 1285.0F

/* player 0's speed up the ramp falls by RAMP_SLOW a step. its distance
 * along the rail grows by RAIL_SPEED a step */
#define RAMP_SLOW 0.05F
#define RAIL_SPEED 5.0F
/* on the rail, the camera stands CAMERA_BEHIND back from player 0 along it.
 * it looks at the point TARGET_BEHIND back */
#define CAMERA_BEHIND 20.0F
#define TARGET_BEHIND 10.0F
#define RAIL_DROP 4.0F /* player 0 rides this far below the rail */
#define DROP_SPEED 1.0F /* player 0's speed as the rail lets it go */
/* the camera's distance from player 0 after the rail */
#define CAMERA_DISTANCE 64.0F
/* the Jdai turns about x by TIP_SPEED a step until it has tipped a quarter
 * turn. it turns back by UNTIP_SPEED a step while player 0 rides the rail */
#define TIP_SPEED 0x100
#define UNTIP_SPEED 0x400
/* player 0 below this y is back on the table */
#define TABLE_TOP 35.0F

/* the y angles player 0 faces as it rolls back down JdaiL's ramp and JdaiR's */
#define JDAI_LEFT_BACK 0x4b9
#define JDAI_RIGHT_BACK 0xeeef

#define JDAI_REACH 80.0F /* the radius of a Jdai's collision record */
/* JdaiL's target stands here, and JdaiR's at -TARGET_X, wherever their Jdai
 * stands */
#define TARGET_X 164.5F
#define TARGET_Y 92.5F
#define TARGET_Z 15.5F

/* JdaiL's shape, JdaiR's, and the targets' */
static struct collision_shape jdai_shapes[3] = {
    {
        SHAPE_RECTANGLE,
        SHAPE_ATTACK,
        PUSH_FIRM(PUSH_ALL),
        {0.0F, -61.5F, 15.0F},
        10.0F,
        20.0F,
        0.1F,
        {0},
        0,
        NULL,
    },
    {
        SHAPE_RECTANGLE,
        SHAPE_ATTACK,
        PUSH_FIRM(PUSH_ALL),
        {0.0F, -61.5F, 15.0F},
        10.0F,
        20.0F,
        0.1F,
        {0},
        0,
        NULL,
    },
    {
        SHAPE_SPHERE,
        SHAPE_ATTACK,
        PUSH_FIRM(PUSH_ALL),
        {0.0F, 0.0F, 0.0F},
        10.0F,
        0.0F,
        0.0F,
        {0},
        0,
        NULL,
    },
};

static void jdai_destroy(struct task *task)
{
}

static void jdai_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    casino_set_texture(CASINO_TEXTURES_ACTS_34);
    draw_tree(&jdai_object);
    njPopMatrix(1);
}

/* the release, a child task that the targets' bonus gives a Jdai. when its
 * tick runs out, it eases the tables' camera to view 0. it raises StoperL for
 * JdaiR or StoperR for JdaiL. then it clears the Jdai's JDAI_HELD and kills
 * itself */
static void jdai_release_run(struct task *task)
{
    struct task_work *work = task->work;

    /* @hack spelling: the tick reached through task->work, not work, which has
     * SHC keep work in a 4-byte stack frame (MOV.L R3,@R15 at 0x0c9046dc) and
     * read it back from there. through work, 23 of its 35 words differ */
    if (--task->work->tick == 0) {
        pinball_view = PINBALL_VIEW_MOVING | PINBALL_VIEW_TABLE;
        if (SIDE(work) != 0) {
            casino_flags |= CASINO_STOPPER_L_UP;
        } else {
            casino_flags |= CASINO_STOPPER_R_UP;
        }
        task->parent->work->flags &= ~JDAI_HELD;
        task_kill(task);
    }
}

static void jdai_target_run(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *jdai = task->parent->work;
    struct task_work *release;

    if (work->collision->flags & COLLISION_HIT) {
        if (!(work->flags & JDAI_TOUCHING)) {
            if (casino_flags & CASINO_COUNTDOWN_ACT2_0) {
                if (SIDE(work) == 0) {
                    casino_flags |= CASINO_COUNTDOWN0_HIDE_A;
                } else {
                    casino_flags |= CASINO_COUNTDOWN0_HIDE_B;
                }
                if ((casino_flags &
                     (CASINO_COUNTDOWN0_HIDE_A | CASINO_COUNTDOWN0_HIDE_B)) ==
                    (CASINO_COUNTDOWN0_HIDE_A | CASINO_COUNTDOWN0_HIDE_B)) {
                    casino_flags &=
                        ~(CASINO_COUNTDOWN0_HIDE_A | CASINO_COUNTDOWN0_HIDE_B);
                    pinball_give_rings(JDAI_BONUS);
                    if (SIDE(jdai) != 0) {
                        pinball_view =
                            PINBALL_VIEW_MOVING | PINBALL_VIEW_RIGHT_BONUS;
                    } else {
                        pinball_view =
                            PINBALL_VIEW_MOVING | PINBALL_VIEW_LEFT_BONUS;
                    }
                    release = task_create_child(TASK_WORK, jdai_release_run,
                                                task->parent)
                                  ->work;
                    release->tick = JDAI_HOLD_STEPS;
                    release->submode = SIDE(jdai);
                    casino_flags |= CASINO_HOUDAI_L_FIRES;
                    casino_flags |= CASINO_HOUDAI_R_FIRES;
                    jdai->flags |= JDAI_HELD;
                }
            }
            pinball_give_rings(JDAI_RINGS);
            work->flags |= JDAI_TOUCHING;
        }
    } else {
        work->flags &= ~JDAI_TOUCHING;
    }
    collision_register(work);
    object_clear_touch(work, NULL);
}

static void jdai_run(struct task *task)
{
    struct task_work *work = task->work;
    NJS_POINT3 *pos = &player_work[0]->pos;
    struct path_sample sample;
    NJS_VECTOR speed;
    Float along_rail;
    Float along_ramp;

    switch (work->mode) {
    case JDAI_WAITING:
        if (work->collision->flags & COLLISION_HIT) {
            SPEED(work) = pinball_player_speed();
            player_set_mode(0, PLAYER_MODE_BALL);
            if (SIDE(work) != 0) {
                VARS(work)->path = &jdai_right_ramp_path;
                END(work) = RIGHT_RAMP_END;
            } else {
                VARS(work)->path = &jdai_left_ramp_path;
                END(work) = LEFT_RAMP_END;
            }
            path_find_nearest(VARS(work)->path, pos, &sample.ahead,
                              &along_ramp);
            ALONG(work) = along_ramp;
            work->mode++;
        }
        break;
    case JDAI_RAMP: {
        Angle ang[3];

        sample.distance = ALONG(work);
        path_sample(VARS(work)->path, &sample);
        pos->x = sample.pos.x;
        pos->y = sample.pos.y;
        pos->z = sample.pos.z;
        SPEED(work) -= RAMP_SLOW;
        ALONG(work) += SPEED(work);
        if (ALONG(work) >= END(work)) {
            if (SIDE(work) != 0) {
                pinball_view = PINBALL_VIEW_MOVING | PINBALL_VIEW_RIGHT_RAMP;
            } else {
                pinball_view = PINBALL_VIEW_MOVING | PINBALL_VIEW_LEFT_RAMP;
            }
            work->mode++;
        } else if (ALONG(work) <= 0.0F) {
            work->mode = JDAI_WAITING;
            player_set_mode(0, PLAYER_MODE_SLOPE_ROLL);
            ang[0] = 0;
            if (SIDE(work) != 0) {
                ang[1] = JDAI_RIGHT_BACK;
            } else {
                ang[1] = JDAI_LEFT_BACK;
            }
            ang[2] = 0;
            speed.x = -SPEED(work);
            speed.y = 0.0F;
            speed.z = 0.0F;
            player_send_flying(0, &speed, ang, 0);
        }
        break;
    }
    case JDAI_TIPPING:
        work->ang.x -= TIP_SPEED;
        if (work->ang.x < -NJM_DEG_ANG(90)) {
            work->ang.x = -NJM_DEG_ANG(90);
            if (SIDE(work) != 0) {
                VARS(work)->path = &jdai_right_rail_path;
                END(work) = RIGHT_RAIL_END;
                path_find_nearest(&jdai_right_rail_path, pos, &sample.ahead,
                                  &along_rail);
            } else {
                VARS(work)->path = &jdai_left_rail_path;
                END(work) = LEFT_RAIL_END;
                path_find_nearest(&jdai_left_rail_path, pos, &sample.ahead,
                                  &along_rail);
            }
            ALONG(work) = along_rail;
            /* the tables' camera stops easing, and JDAI_RAIL moves camera_work
             * itself */
            pinball_view &= ~PINBALL_VIEW_MOVING;
            work->mode++;
        }
        break;
    case JDAI_RAIL: {
        Angle ang[3];

        sample.distance = ALONG(work);
        path_sample(VARS(work)->path, &sample);
        pos->x = sample.pos.x;
        pos->y = sample.pos.y - RAIL_DROP;
        pos->z = sample.pos.z;
        sample.distance = ALONG(work) - TARGET_BEHIND;
        path_sample(VARS(work)->path, &sample);
        CAMERA_VARS(camera_work)->target.x = sample.pos.x;
        CAMERA_VARS(camera_work)->target.y = sample.pos.y;
        CAMERA_VARS(camera_work)->target.z = sample.pos.z;
        sample.distance = ALONG(work) - CAMERA_BEHIND;
        path_sample(VARS(work)->path, &sample);
        camera_work->scl.x = sample.pos.x;
        camera_work->scl.y = sample.pos.y;
        camera_work->scl.z = sample.pos.z;
        ALONG(work) += RAIL_SPEED;
        if (ALONG(work) >= END(work)) {
            player_set_mode(0, PLAYER_MODE_SLOPE_ROLL);
            if (SIDE(work) == 0) {
                world_tilt_gravity(0, 0);
            }
            speed.x = DROP_SPEED;
            speed.y = 0.0F;
            speed.z = 0.0F;
            ang[0] = 0;
            ang[1] = 0;
            ang[2] = 0;
            player_send_flying(0, &speed, ang, 0);
            /* JDAI_DOWN sets camera_angle and camera_distance again before
             * it reads them */
            VARS(work)->camera_angle = NJM_DEG_ANG(180);
            VARS(work)->camera_distance = 20.0F;
            work->mode++;
        } else if (work->ang.x != 0) {
            work->ang.x += UNTIP_SPEED;
            if (work->ang.x > 0) {
                work->ang.x = 0;
            }
        }
        break;
    }
    case JDAI_DOWN: {
        /* @hack spelling: the distance through a local, which SHC loads into
         * FR4 ahead of the side's test (0x0c904c5e). stored directly, SHC
         * loads it from the pool at the function's end, the function runs 32
         * bytes longer, and 114 of its 464 words differ */
        Float distance = CAMERA_DISTANCE;

        /* @hack redundant: a goto to the next statement, which SHC deletes but
         * which has it load the angle into R2 (0x0c904c66, 0x0c904ca0).
         * without it, the angle takes R3, and 3 of its 464 words differ */
        goto turned;
    turned:
        VARS(work)->camera_angle =
            SIDE(work) ? NJM_DEG_ANG(90) : NJM_DEG_ANG(270);
        VARS(work)->camera_distance = distance;
        if (!(work->flags & JDAI_HELD)) {
            CAMERA_VARS(camera_work)->target.x = pos->x;
            CAMERA_VARS(camera_work)->target.y = camera_work->scl.y = pos->y;
            CAMERA_VARS(camera_work)->target.z = pos->z;
            camera_work->scl.x = (njSin(VARS(work)->camera_angle) *
                                  VARS(work)->camera_distance) +
                                 pos->x;
            camera_work->scl.z = (njCos(VARS(work)->camera_angle) *
                                  VARS(work)->camera_distance) +
                                 pos->z;
            if (pos->y < TABLE_TOP) {
                work->mode = JDAI_WAITING;
                pinball_view = PINBALL_VIEW_MOVING | PINBALL_VIEW_TABLE;
                if (SIDE(work) != 0) {
                    casino_flags |= CASINO_STOPPER_L_UP;
                } else {
                    casino_flags |= CASINO_STOPPER_R_UP;
                }
            }
        }
        task_step_children(task);
        break;
    }
    }
    collision_register(work);
    object_clear_touch(work, NULL);
    jdai_draw(task);
}

static void jdai_init(struct task *task, int side)
{
    struct task_work *work = task->work;
    struct task *target = task_create_child(TASK_WORK, jdai_target_run, task);

    if (side == 0) {
        target->work->pos.x = TARGET_X;
        target->work->pos.y = TARGET_Y;
        target->work->pos.z = TARGET_Z;
    } else {
        target->work->pos.x = -TARGET_X;
        target->work->pos.y = TARGET_Y;
        target->work->pos.z = TARGET_Z;
    }
    target->work->submode = side;
    collision_init(target, &jdai_shapes[2], COLLISION_OBJECT, 1, 1);
    collision_init(task, &jdai_shapes[side], COLLISION_OBJECT, 1, 1);
    work->collision->radius = JDAI_REACH;
    work->ang.x = 0;
    work->submode = side;
    work->mode = JDAI_WAITING;
    task->step = jdai_run;
    task->draw = jdai_draw;
    task->destructor = jdai_destroy;
}

void jdai_left_step(struct task *task)
{
    jdai_init(task, 0);
}

void jdai_right_step(struct task *task)
{
    jdai_init(task, 1);
}
