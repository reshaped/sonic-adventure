/* the avalanche on the board run: two waves of pieces that chase player 0
 * down their paths. the second wave drops rocks */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "collision/polygon.h"
#include "draw/model.h"
#include "path/path.h"
#include "player/player.h"
#include "sound/sound.h"
#include "stg08/avalanche.h"
#include "stg08/avalanche_paths.h"
#include "stg08/jet_smoke.h"
#include "stg08/snow_burst.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"
#include "vmu/vibration.h"

extern NJS_MODEL avalanche_model;
extern NJS_MODEL avalanche_rock_model;

/* each wave starts once player 0 is at or below its height */
#define FIRST_WAVE_HEIGHT (-11675.515F)
#define SECOND_WAVE_HEIGHT (-13561.109F)
/* each step, a wave's pieces move along their paths by player 0's speed
 * ahead plus their wave's lead */
#define FIRST_WAVE_LEAD 8.0F
#define SECOND_WAVE_LEAD 5.0F
/* a piece of the second wave ends once player 0 is this far past it along z */
#define SECOND_WAVE_PASSED 200.0F

#define SE_AVALANCHE 234
#define SE_ROCK_FALL 232
#define SE_ROCK_LAND 233

/* the cloud a first-wave piece raises each step. jet_smoke_create takes
 * these as its height, its first frame, and how much wider it is than tall */
#define CLOUD_HEIGHT 3.0F
#define CLOUD_FRAME 5.0F
#define CLOUD_SPREAD 2.0F

/* a rock drops only where its piece finds ground within ROCK_REACH below
 * it. the rock starts ROCK_DROP below the piece and falls ROCK_FALL a step.
 * the step it lands on is the last it is drawn. it then hurts players,
 * unseen, for ROCK_HURT_STEPS steps */
#define ROCK_DROP 20.0F
#define ROCK_REACH 200.0F
#define ROCK_FALL 10.0F
#define ROCK_HURT_STEPS 40

/* the second wave's pieces take turns to drop a rock, from 1 to DROP_TURNS.
 * a piece tries to drop one on each step its turn comes round */
#define DROP_TURNS 15

struct avalanche_vars {
    Float distance;
};

struct avalanche_rock_vars {
    Float ground; /* the height of the ground it lands on */
    Sint32 left; /* the steps it hurts for yet */
};

#define VARS(work) ((struct avalanche_vars *)&(work)->vars)
#define ROCK_VARS(work) ((struct avalanche_rock_vars *)&(work)->vars)

/* a landed rock. it hurts players, and they can't push it */
static struct collision_shape avalanche_rock_shape = {
    SHAPE_SPHERE | SHAPE_HURTABLE,
    ATTACK(2) | GUARD(1) | SHAPE_HURTS_PLAYERS,
    PUSH_FIRM(PUSH_PLAYERS),
    {0.0F, 0.0F, 0.0F},
    20.0F,
    0.0F,
    0.0F,
    {0},
    0,
    NULL,
};

/* four points no code reads */
static NJS_POINT3 dat_0cab15c4[4] = {
    {-42.5F, -13.5F, -38.0F},
    {-30.0F, -4.0F, -32.0F},
    {30.0F, -4.0F, -32.0F},
    {42.5F, -13.5F, -38.0F},
};

/* whose turn it is to drop a rock, 1 to DROP_TURNS. only the second wave's
 * first piece moves the turn on, so it stops once that piece ends */
static Sint32 avalanche_drop_turn;

static void avalanche_destroy(struct task *task)
{
}

static void avalanche_rock_hurt(struct task *task)
{
    struct task_work *work = task->work;

    if (ROCK_VARS(work)->left-- == 0) {
        task_destroy(task);
    } else {
        collision_register(work);
    }
}

static void avalanche_rock_draw(struct task *task)
{
    struct task_work *work = task->work;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateY, work->ang.y);
    draw_model(&avalanche_rock_model);
    njPopMatrix(1);
}

static void avalanche_rock_fall(struct task *task)
{
    struct task_work *work = task->work;

    if (work->pos.y <= ROCK_VARS(work)->ground) {
        sound_play_at(SE_ROCK_LAND, 0, 0, 96, work->pos.x, work->pos.y,
                      work->pos.z);
        snow_burst_create(&work->pos);
        vibration_start(0, 5, 15, 0);
        ROCK_VARS(work)->left = ROCK_HURT_STEPS;
        collision_init(task, &avalanche_rock_shape, COLLISION_OBJECT, 1, 1);
        task->step = avalanche_rock_hurt;
        task->draw = NULL;
    }
    avalanche_rock_draw(task);
    work->pos.y -= ROCK_FALL;
}

static void avalanche_draw(struct task *task)
{
    struct task_work *work = task->work;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateZ, work->ang.z);
    TURN(njRotateX, work->ang.x);
    TURN(njRotateY, work->ang.y);
    draw_model(&avalanche_model);
    njPopMatrix(1);
}

/* place the piece at its distance along path, facing the way the path runs.
 * a turn of 0 raises a cloud. any other turn drops a rock when it comes
 * round */
static void avalanche_follow(struct task_work *work, struct path *path,
                             int turn)
{
    NJS_POINT3 *pos = &work->pos;
    struct angles *ang = &work->ang;

    /* @hack redundant: a bare scope for sample and ahead, which lets query
     * take their bytes and keeps the frame at 100 bytes (0x0c907ac8); with
     * the function's other locals, 174 of 178 words differ */
    {
        struct path_sample sample;
        NJS_VECTOR ahead;

        sample.distance = VARS(work)->distance;
        path_sample(path, &sample);
        pos->x = sample.pos_copy.x;
        pos->y = sample.pos_copy.y;
        pos->z = sample.pos_copy.z;
        ang->x = sample.smooth_x;
        ang->z = sample.smooth_z;
        njPushMatrix(&_unit_matrix_);
        TURN(njRotateX, -ang->x);
        TURN(njRotateZ, -ang->z);
        njCalcVector(NULL, &sample.ahead, &ahead);
        njPopMatrix(1);
        ang->y = (Angle)(atan2f(ahead.x, ahead.z) * ANGLE_PER_RADIAN);
    }
    if (turn == 0) {
        jet_smoke_create(&work->pos, CLOUD_HEIGHT, CLOUD_FRAME, CLOUD_SPREAD);
    } else if (turn == avalanche_drop_turn) {
        struct ray_query query;

        query.length = ROCK_REACH;
        query.origin.x = work->pos.x;
        query.origin.y = work->pos.y;
        query.origin.z = work->pos.z;
        query.dir.x = 0.0F;
        query.dir.y = -1.0F;
        query.dir.z = 0.0F;
        if (polygon_cast_ray(&query)) {
            struct task *rock = task_create(
                TASK_WORK | TASK_MOTION | TASK_FORCE, 3, avalanche_rock_fall);
            struct task_work *rock_work = rock->work;

            ROCK_VARS(rock_work)->ground = query.hit.y;
            rock_work->pos.x = pos->x;
            rock_work->pos.y = pos->y - ROCK_DROP;
            rock_work->pos.z = pos->z;
            rock_work->ang.y = work->ang.y;
            sound_play_at(SE_ROCK_FALL, 0, 0, 64, work->pos.x, work->pos.y,
                          work->pos.z);
            rock->draw = avalanche_rock_draw;
            rock->destructor = avalanche_destroy;
        }
    }
}

/* the second wave's pieces. each ends once its distance reaches a bound
 * just under its path's length. it is never placed past the path's end. it
 * ends sooner once player 0 is SECOND_WAVE_PASSED beyond it along z. the
 * first piece also moves the drop turn on, each step it runs */
static void avalanche_second_1(struct task *task)
{
    struct task_work *work = task->work;

    if (player_work[0]->pos.z > work->pos.z + SECOND_WAVE_PASSED ||
        VARS(work)->distance >= 14147.0F) {
        task_destroy(task);
    } else {
        avalanche_follow(work, &avalanche_path_2, 2);
        sound_play_at_for(SE_AVALANCHE, (int)work, 1, 64, work->pos.x,
                          work->pos.y, work->pos.z, 3);
        avalanche_draw(task);
        VARS(work)->distance += player_get(0)->speed.x + SECOND_WAVE_LEAD;
    }
    if (avalanche_drop_turn++ == DROP_TURNS) {
        avalanche_drop_turn = 1;
    }
}

static void avalanche_second_2(struct task *task)
{
    struct task_work *work = task->work;

    if (player_work[0]->pos.z > work->pos.z + SECOND_WAVE_PASSED ||
        VARS(work)->distance >= 13790.0F) {
        task_destroy(task);
    } else {
        avalanche_follow(work, &avalanche_path_4, 6);
        sound_play_at_for(SE_AVALANCHE, (int)work, 1, 64, work->pos.x,
                          work->pos.y, work->pos.z, 2);
        avalanche_draw(task);
        VARS(work)->distance += player_get(0)->speed.x + SECOND_WAVE_LEAD;
    }
}

static void avalanche_second_3(struct task *task)
{
    struct task_work *work = task->work;

    if (player_work[0]->pos.z > work->pos.z + SECOND_WAVE_PASSED ||
        VARS(work)->distance >= 13420.0F) {
        task_destroy(task);
    } else {
        avalanche_follow(work, &avalanche_path_6, 9);
        sound_play_at_for(SE_AVALANCHE, (int)work, 1, 64, work->pos.x,
                          work->pos.y, work->pos.z, 2);
        avalanche_draw(task);
        VARS(work)->distance += player_get(0)->speed.x + SECOND_WAVE_LEAD;
    }
}

static void avalanche_second_4(struct task *task)
{
    struct task_work *work = task->work;

    if (player_work[0]->pos.z > work->pos.z + SECOND_WAVE_PASSED ||
        VARS(work)->distance >= 13151.0F) {
        task_destroy(task);
    } else {
        avalanche_follow(work, &avalanche_path_8, 12);
        sound_play_at_for(SE_AVALANCHE, (int)work, 1, 64, work->pos.x,
                          work->pos.y, work->pos.z, 2);
        avalanche_draw(task);
        VARS(work)->distance += player_get(0)->speed.x + SECOND_WAVE_LEAD;
    }
}

static void avalanche_second_5(struct task *task)
{
    struct task_work *work = task->work;

    if (player_work[0]->pos.z > work->pos.z + SECOND_WAVE_PASSED ||
        VARS(work)->distance >= 14575.0F) {
        task_destroy(task);
    } else {
        avalanche_follow(work, &avalanche_path_10, 15);
        sound_play_at_for(SE_AVALANCHE, (int)work, 1, 64, work->pos.x,
                          work->pos.y, work->pos.z, 2);
        avalanche_draw(task);
        VARS(work)->distance += player_get(0)->speed.x + SECOND_WAVE_LEAD;
    }
}

/* create a wave's piece, whose task runs step, at distance start along its
 * path. piece and work get its task and its work */
#define AVALANCHE_PIECE(piece, work, step, start)                              \
    do {                                                                       \
        (piece) =                                                              \
            task_create(TASK_WORK | TASK_MOTION | TASK_FORCE, 3, (step));      \
        (work) = (piece)->work;                                                \
        VARS(work)->distance = (start);                                        \
        (piece)->draw = avalanche_draw;                                        \
        (piece)->destructor = avalanche_destroy;                               \
    } while (0)

static void avalanche_wait_second(struct task *task)
{
    struct task *piece;
    struct task_work *work;

    if (player_work[0]->pos.y <= SECOND_WAVE_HEIGHT) {
        AVALANCHE_PIECE(piece, work, avalanche_second_1, 850.0F);
        AVALANCHE_PIECE(piece, work, avalanche_second_2, 550.0F);
        AVALANCHE_PIECE(piece, work, avalanche_second_3, 0.0F);
        AVALANCHE_PIECE(piece, work, avalanche_second_4, 0.0F);
        AVALANCHE_PIECE(piece, work, avalanche_second_5, 1100.0F);
        task_destroy(task);
    }
}

/* the first wave's pieces. each ends once its distance reaches a bound just
 * under its path's length. it is never placed past the path's end */
static void avalanche_first_1(struct task *task)
{
    struct task_work *work = task->work;

    if (VARS(work)->distance >= 12896.0F) {
        task_destroy(task);
    } else {
        avalanche_follow(work, &avalanche_path_1, 0);
        sound_play_at_for(SE_AVALANCHE, (int)work, 1, 64, work->pos.x,
                          work->pos.y, work->pos.z, 2);
        avalanche_draw(task);
        VARS(work)->distance += player_get(0)->speed.x + FIRST_WAVE_LEAD;
    }
}

static void avalanche_first_2(struct task *task)
{
    struct task_work *work = task->work;

    if (VARS(work)->distance >= 13602.0F) {
        task_destroy(task);
    } else {
        avalanche_follow(work, &avalanche_path_3, 0);
        sound_play_at_for(SE_AVALANCHE, (int)work, 1, 64, work->pos.x,
                          work->pos.y, work->pos.z, 2);
        avalanche_draw(task);
        VARS(work)->distance += player_get(0)->speed.x + FIRST_WAVE_LEAD;
    }
}

static void avalanche_first_3(struct task *task)
{
    struct task_work *work = task->work;

    if (VARS(work)->distance >= 12926.0F) {
        task_destroy(task);
    } else {
        avalanche_follow(work, &avalanche_path_5, 0);
        sound_play_at_for(SE_AVALANCHE, (int)work, 1, 64, work->pos.x,
                          work->pos.y, work->pos.z, 2);
        avalanche_draw(task);
        VARS(work)->distance += player_get(0)->speed.x + FIRST_WAVE_LEAD;
    }
}

static void avalanche_first_4(struct task *task)
{
    struct task_work *work = task->work;

    if (VARS(work)->distance >= 13281.0F) {
        task_destroy(task);
    } else {
        avalanche_follow(work, &avalanche_path_7, 0);
        sound_play_at_for(SE_AVALANCHE, (int)work, 1, 64, work->pos.x,
                          work->pos.y, work->pos.z, 2);
        avalanche_draw(task);
        VARS(work)->distance += player_get(0)->speed.x + FIRST_WAVE_LEAD;
    }
}

static void avalanche_first_5(struct task *task)
{
    struct task_work *work = task->work;

    if (VARS(work)->distance >= 13059.0F) {
        task_destroy(task);
    } else {
        avalanche_follow(work, &avalanche_path_9, 0);
        sound_play_at_for(SE_AVALANCHE, (int)work, 1, 64, work->pos.x,
                          work->pos.y, work->pos.z, 2);
        avalanche_draw(task);
        VARS(work)->distance += player_get(0)->speed.x + FIRST_WAVE_LEAD;
    }
}

static void avalanche_wait_first(struct task *task)
{
    struct task *piece;
    struct task_work *work;

    if (player_work[0]->pos.y <= FIRST_WAVE_HEIGHT) {
        AVALANCHE_PIECE(piece, work, avalanche_first_1, 0.0F);
        AVALANCHE_PIECE(piece, work, avalanche_first_2, 0.0F);
        AVALANCHE_PIECE(piece, work, avalanche_first_3, 400.0F);
        AVALANCHE_PIECE(piece, work, avalanche_first_4, 400.0F);
        AVALANCHE_PIECE(piece, work, avalanche_first_5, 300.0F);
        task->step = avalanche_wait_second;
    }
}

void avalanche_step(struct task *task)
{
    avalanche_drop_turn = 1;
    task->step = avalanche_wait_first;
    task->destructor = avalanche_destroy;
}
