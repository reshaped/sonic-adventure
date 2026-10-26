/* Path, the second table's four rides. a touch of one of its four switches
 * carries player 0 along that ride's path, curled up. the camera follows
 * behind him */
#include <ninja.h>
#include <sg_xpt.h>

#include "camera/camera.h"
#include "collision/collision.h"
#include "draw/draw.h"
#include "object/common.h"
#include "path/path.h"
#include "player/player.h"
#include "sound/sound.h"
#include "stage/stage.h"
#include "stg09/casino.h"
#include "stg09/paths.h"
#include "stg09/pinball.h"
#include "stg09/rides.h"
#include "system/task.h"
#include "system/util.h"

/* ride_ahead_action runs ahead of player 0 along the third ride's path.
 * ride_start_action plays at one fixed place once the first or the second
 * ride starts */
extern NJS_ACTION ride_ahead_action;
extern NJS_ACTION ride_start_action;

#define RIDES 4
/* Path's work->tick while no ride runs. each waiting step sets it, and a
 * touched switch replaces it with its ride */
#define RIDE_NONE 100

/* Path's work->mode */
enum ride_mode { RIDE_WAITING, RIDE_RIDING };

/* Path's work->submode: which action it draws */
#define RIDE_DRAWS_NONE 0
#define RIDE_DRAWS_AHEAD 1
#define RIDE_DRAWS_START 2

/* in Path's work->flags, set while the second ride has given the camera
 * back */
#define RIDE_CAMERA_FREE 0x8000

/* the camera sits RIDE_CAMERA_BEHIND back along the path from player 0. it
 * looks at a point RIDE_CAMERA_LIFT above him */
#define RIDE_CAMERA_BEHIND 30.0F
#define RIDE_CAMERA_LIFT 5.0F
/* ride_ahead_action keeps this far ahead of player 0 along the path */
#define RIDE_AHEAD 28.0F

/* marks along the second ride's path. past RIDE_FREE_FROM the ride gives
 * the camera back, and past RIDE_FREE_TO it takes it again. in between,
 * past RIDE_IDEA_FROM, it places the idea piece player 0 carries */
#define RIDE_FREE_FROM 2200.0F
#define RIDE_FREE_TO 5200.0F
#define RIDE_IDEA_FROM 3600.0F

#define SE_RIDE_1 250
#define SE_RIDE_2 240
#define SE_IDEA 241

/* the switches' shapes, by ride */
static struct collision_shape ride_shapes[RIDES] = {
    {SHAPE_RECTANGLE,
     ATTACK(3),
     PUSH_FIRM(PUSH_ALL),
     {0.0F, 10.0F, 0.0F},
     30.0F,
     10.0F,
     2.0F,
     {0},
     0,
     NULL},
    {SHAPE_SPHERE,
     ATTACK(3),
     PUSH_FIRM(PUSH_ALL),
     {0.0F, 0.0F, 0.0F},
     65.0F,
     0.0F,
     0.0F,
     {0},
     0,
     NULL},
    {SHAPE_SPHERE,
     ATTACK(3),
     PUSH_FIRM(PUSH_ALL),
     {0.0F, 40.0F, 0.0F},
     60.0F,
     0.0F,
     0.0F,
     {0},
     0,
     NULL},
    {SHAPE_RECTANGLE,
     ATTACK(3),
     PUSH_FIRM(PUSH_ALL),
     {0.0F, 10.0F, 0.0F},
     12.0F,
     10.0F,
     0.5F,
     {0},
     0,
     NULL},
};
/* where each ride leaves player 0 */
static NJS_POINT3 ride_ends[RIDES] = {
    {605.0F, 500.0F, -70.0F},
    {-527.5F, -150.0F, -61.0F},
    {456.0F, 500.0F, -68.0F},
    {-640.0F, -216.4F, 121.5F},
};
static NJS_POINT3 ride_switch_places[RIDES] = {
    {-518.0F, -240.0F, -120.0F},
    {530.0F, 420.0F, -240.0F},
    {530.0F, 520.0F, 70.0F},
    {-568.0F, -240.0F, -3.0F},
};

/* Path's work->vars while a ride runs */
struct ride_vars {
    struct path *path;
    Uint8 unk_04[4];
    Float distance; /* how far player 0 has gone along the path */
};

#define VARS(work) ((struct ride_vars *)&(work)->vars)

static void ride_destroy(struct task *task)
{
}

/* a switch's step. each switch keeps its ride's index in work->timer, and
 * a touch puts it in Path's work->tick */
static void ride_switch_run(struct task *task)
{
    struct task_work *work = task->work;

    if (work->collision->flags & COLLISION_HIT) {
        task->parent->work->tick = work->timer;
        /* the third switch has player 0 carry the next idea piece. he takes
         * it only once pinball_idea_draws reaches PINBALL_IDEA_DRAWS and a
         * piece is left to place */
        if (TASK_TIMER(work) == 2 && pinball_idea_draws >= PINBALL_IDEA_DRAWS &&
            pinball_ideas != PINBALL_IDEAS) {
            casino_flags |= CASINO_IDEA_CARRIED;
        }
        /* while the second table's second countdown runs, the fourth switch
         * pays rings. each touch pays 10 more than the last */
        if (TASK_TIMER(work) == 3 && (casino_flags & CASINO_COUNTDOWN_ACT3_1)) {
            ++pinball_ride_touches;
            pinball_give_rings(pinball_ride_touches * 10);
        }
    }
    collision_register(work);
    object_clear_touch(work, NULL);
}

/* draw the action work->submode names, at the frame scl.y */
static void ride_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (work->submode == RIDE_DRAWS_NONE) {
        return;
    }
    casino_set_texture(CASINO_TEXTURES_ACTS_34);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    TURN_VIA(njRotateY, work->ang.y + NJM_DEG_ANG(180), ang);
    if (work->submode == RIDE_DRAWS_AHEAD) {
        draw_action(&ride_ahead_action, work->scl.y);
    } else {
        njScale(NULL, 5.0F, 5.0F, 5.0F);
        draw_action(&ride_start_action, work->scl.y);
    }
    njPopMatrix(1);
}

/* keep player 0 at his ride's end in x and z while he falls. the task ends
 * once he lands, or once the act is no longer the second table. work->tick
 * is the ride */
static void ride_end_run(struct task *task)
{
    NJS_POINT3 *end;
    Float z;

    if (stage_get_stage_act() != STAGE_ACT(STAGE_CASINO, 3)) {
        task_kill(task);
        return;
    }
    if (!(player_work[0]->flags & PLAYER_ON_LAND)) {
        /* @hack type: end formed from x's address and set inside the read of
         * z, which keeps it in a stack slot (MOV.L R2,@R15 at 0x0c9088e4).
         * set as &ride_ends[task->work->tick], with end->z in the call, the
         * function runs 32 bytes short and 48 of 53 words differ */
        z = (end = (NJS_POINT3 *)&ride_ends[task->work->tick].x)->z;
        player_set_position(0, end->x, player_work[0]->pos.y, z);
    } else {
        task_kill(task);
    }
}

/* Path's step after the first. a switch's touch starts its ride. the ride
 * carries player 0 along its path until he has gone scl.x along it */
static void ride_run(struct task *task)
{
    struct task_work *work = task->work;
    struct path_sample sample;
    NJS_POINT3 *pos;

    switch (work->mode) {
    case RIDE_WAITING:
        work->tick = RIDE_NONE;
        task_step_children(task);
        if (work->tick != RIDE_NONE) {
            player_set_mode(0, PLAYER_MODE_BALL);
            VARS(work)->distance = 0.0F;
            switch (work->tick) {
            case 0:
                VARS(work)->path = &ride_path_0;
                /* the first ride ends once player 0 has gone 1 along its
                 * path, on its first step */
                work->scl.x = 1.0F;
                work->pos.x = -67.0F;
                work->pos.y = 446.0F;
                work->pos.z = -2924.0F;
                work->submode = RIDE_DRAWS_START;
                break;
            case 1:
                camera_override_mode(CAMERA_MODE_RUIN_WAKA1,
                                     CAMERA_ADJUST_NONE);
                sound_play(SE_RIDE_1, 0, 0, 0);
                VARS(work)->path = &ride_path_1;
                work->scl.x = 7708.0F;
                work->pos.x = -67.0F;
                work->pos.y = 446.0F;
                work->pos.z = -2924.0F;
                work->submode = RIDE_DRAWS_START;
                break;
            case 2:
                camera_override_mode(CAMERA_MODE_RUIN_WAKA1,
                                     CAMERA_ADJUST_NONE);
                VARS(work)->path = &ride_path_2;
                work->scl.x = 7652.0F;
                sound_play(SE_RIDE_2, 0, 0, 0);
                /* @hack redundant: a call of sound_play that never runs. SHC
                 * drops it but counts it as a use, so it keeps sound_play's
                 * address in R10 (0x0c90895c), as the image does. without it,
                 * the address loads at each call, and 254 of 453 words
                 * differ */
                if (0) {
                    sound_play(0, 0, 0, 0);
                }
                work->submode = RIDE_DRAWS_AHEAD;
                break;
            case 3:
                camera_override_mode(CAMERA_MODE_RUIN_WAKA1,
                                     CAMERA_ADJUST_RELATIVE1);
                VARS(work)->path = &ride_path_3;
                work->scl.x = 367.0F;
                break;
            }
            work->mode++;
        }
        break;
    case RIDE_RIDING:
        pos = &player_work[0]->pos;
        sample.distance = VARS(work)->distance;
        path_sample(VARS(work)->path, &sample);
        pos->x = sample.pos.x;
        pos->y = sample.pos.y;
        pos->z = sample.pos.z;
        sample.distance = VARS(work)->distance - RIDE_CAMERA_BEHIND;
        if (sample.distance < 0.0F) {
            sample.distance = 0.0F;
        }
        path_sample(VARS(work)->path, &sample);
        camera_work->scl.x = sample.pos.x;
        camera_work->scl.y = sample.pos.y;
        camera_work->scl.z = sample.pos.z;
        CAMERA_VARS(camera_work)->target.x = pos->x;
        CAMERA_VARS(camera_work)->target.y = pos->y + RIDE_CAMERA_LIFT;
        CAMERA_VARS(camera_work)->target.z = pos->z;
        if (work->submode == RIDE_DRAWS_AHEAD) {
            sample.distance = VARS(work)->distance + RIDE_AHEAD;
            if (sample.distance < work->scl.x) {
                path_sample(VARS(work)->path, &sample);
                work->pos.x = sample.pos_copy.x;
                work->pos.y = sample.pos_copy.y;
                work->pos.z = sample.pos_copy.z;
            } else {
                work->submode = RIDE_DRAWS_NONE;
            }
        }
        switch (work->tick) {
        case 0:
            VARS(work)->distance += 13.0F;
            break;
        case 1:
            VARS(work)->distance += 5.0F;
            break;
        case 2:
            VARS(work)->distance += 7.0F;
            break;
        case 3:
            VARS(work)->distance += 3.0F;
            break;
        }
        if (work->tick == 1) {
            if (!(work->flags & RIDE_CAMERA_FREE)) {
                if (VARS(work)->distance > RIDE_FREE_FROM &&
                    VARS(work)->distance < RIDE_FREE_TO) {
                    work->flags |= RIDE_CAMERA_FREE;
                    camera_restore();
                }
            } else {
                if (VARS(work)->distance > RIDE_IDEA_FROM &&
                    (casino_flags & CASINO_IDEA_CARRIED)) {
                    casino_flags &= ~CASINO_IDEA_CARRIED;
                    pinball_idea_draws = 0;
                    pinball_ideas++;
                    sound_play(SE_IDEA, 0, 0, 0);
                    if (pinball_ideas == PINBALL_IDEAS) {
                        pinball_give_rings(50);
                        pinball_give_rings(50);
                        pinball_give_rings(50);
                        pinball_give_rings(50);
                        pinball_give_rings(50);
                        pinball_give_rings(50);
                    }
                }
                if (VARS(work)->distance > RIDE_FREE_TO) {
                    work->flags &= ~RIDE_CAMERA_FREE;
                    camera_override_mode(CAMERA_MODE_RUIN_WAKA1,
                                         CAMERA_ADJUST_RELATIVE1);
                }
            }
        }
        if (VARS(work)->distance >= work->scl.x) {
            pos = &ride_ends[work->tick];
            player_set_position(0, pos->x, pos->y, pos->z);
            player_set_mode(0, PLAYER_MODE_SLOPE_ROLL);
            task_create(TASK_WORK, 3, ride_end_run)->work->tick = work->tick;
            if (work->tick == 1) {
                casino_flags &= ~CASINO_BANJU_DOOR_OPEN;
            }
            if (work->tick != 0) {
                camera_restore();
            }
            if (work->tick == 1 && pinball_ideas == PINBALL_IDEAS) {
                pinball_ideas = 1;
            }
            work->mode = RIDE_WAITING;
            task_step_children(task);
        }
        break;
    }
    if (work->submode) {
        if (work->submode == RIDE_DRAWS_AHEAD) {
            work->scl.y += 0.3F;
            if (work->scl.y >= 120.0F) {
                work->scl.y = 0.0F;
            }
        } else {
            work->scl.y += 0.3F;
            if (work->scl.y >= 100.0F) {
                work->scl.y = 0.0F;
            }
        }
    }
    ride_draw(task);
}

static void ride_switch_init(struct task *task, int ride)
{
    struct task_work *work = task->work;

    work->pos.x = ride_switch_places[ride].x;
    work->pos.y = ride_switch_places[ride].y;
    work->pos.z = ride_switch_places[ride].z;
    collision_init(task, &ride_shapes[ride], COLLISION_OBJECT, 1, 1);
    work->timer = ride;
}

void ride_step(struct task *task)
{
    struct task_work *work = task->work;

    ride_switch_init(task_create_child(TASK_WORK, ride_switch_run, task), 0);
    ride_switch_init(task_create_child(TASK_WORK, ride_switch_run, task), 1);
    ride_switch_init(task_create_child(TASK_WORK, ride_switch_run, task), 2);
    ride_switch_init(task_create_child(TASK_WORK, ride_switch_run, task), 3);
    work->mode = RIDE_WAITING;
    work->submode = RIDE_DRAWS_NONE;
    task->step = ride_run;
    task->draw = ride_draw;
    task->destructor = ride_destroy;
}
