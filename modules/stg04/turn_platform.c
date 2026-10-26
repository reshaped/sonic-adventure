/* O TurnAsi: a deck that hangs from a pivot and swings player 0 up and back */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/model.h"
#include "object/placement.h"
#include "player/player.h"
#include "sound/sound.h"
#include "stg04/turn_platform.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT turn_platform_object;
extern NJS_OBJECT turn_platform_arm_object;
extern NJS_OBJECT turn_platform_deck_object;

#define TURN_PLATFORM_REACH 100.0F

/* the platform's work->mode. it waits at rest, swings, and after a swing
 * over the top turns back to rest. no code sets TURN_PLATFORM_GONE */
enum turn_platform_mode {
    TURN_PLATFORM_INIT,
    TURN_PLATFORM_WAIT,
    TURN_PLATFORM_SWING,
    TURN_PLATFORM_RETURN,
    TURN_PLATFORM_GONE
};

/* the deck's work->mode. it follows the arm, and waits after each swing */
enum deck_mode { DECK_INIT, DECK_FOLLOW, DECK_WAIT, DECK_GONE };

/* a landing is a hit on the deck's record. in the step the deck takes one,
 * it leaves LANDED in the platform's work->tick. each step the deck follows
 * clears the tick first */
#define LANDED 10

/* a bit of the platform's work->flags, set as its turn back to rest ends.
 * nothing reads it */
#define TURN_PLATFORM_RESTED 0x100

/* player 0's speed as it lands. below SPEED_LEAST, the platform doesn't
 * swing. below SPEED_FAST, the swing is a slower one, which player 0 rides
 * held still. from SPEED_FAST, it is a fast one, which player 0 rides curled
 * up */
#define SPEED_LEAST 1.5F
#define SPEED_FAST 4.0F

/* each step, a swing turns the platform SWING_TURN degrees less its brake
 * term. that term is the brake times the steps so far, over the landing
 * speed. a turn below 0 swings the platform back */
#define SWING_TURN 8.0F
/* a slower swing's brake is SWING_BRAKE, or SWING_BRAKE_GENTLE after a
 * landing faster than SPEED_GENTLE. its turn is held to SWING_TURN_MOST
 * either way */
#define SWING_BRAKE 0.63F
#define SWING_BRAKE_GENTLE 0.13F
#define SPEED_GENTLE 3.0F
#define SWING_TURN_MOST 4.3F
/* a fast swing's brake is FAST_SWING_BRAKE. its turn is halved, and not
 * held */
#define FAST_SWING_BRAKE 0.2F

/* a slower swing past HALF_TURN goes over the top and flings player 0 off. a
 * fast one goes over once it reaches HALF_TURN_DEGREES */
#define HALF_TURN 0x8000
#define HALF_TURN_DEGREES 180.0F

/* the platform's turn back to rest each step after a swing over the top */
#define RETURN_TURN NJM_DEG_ANG(1)

/* player 0 rides RIDE_BELOW below the deck */
#define RIDE_BELOW 7.5F

/* how much faster than its landing speed player 0 leaves the deck. a swing
 * that turns back past rest lets it go, and one over the top flings it off.
 * the FAST_ speeds are a fast swing's */
#define LET_GO_SPEED 1.0F
#define FLING_SPEED 3.0F
#define FAST_LET_GO_SPEED 2.0F
#define FAST_FLING_SPEED 1.0F
/* the steps player 0's stick is ignored after a slower swing lets it go,
 * and after a fling */
#define LET_GO_STEPS 20
#define FLING_STEPS 10

#define SE_SWING 100
#define SE_LET_GO 101

struct turn_platform_vars {
    Uint8 unk_00[4];
    struct task *deck;
    Float speed; /* player 0's speed, which the deck records until a landing */
};

#define VARS(work) ((struct turn_platform_vars *)&(work)->vars)

/* marks in the platform's work->scl, each 0 or its MARK_ value. FAST holds
 * MARK_FAST through a fast swing. DONE holds MARK_DONE from a swing's end
 * until the deck clears it to wait. RIDING holds MARK_RIDING while player 0
 * rides */
#define FAST(work) ((work)->scl.x)
#define DONE(work) ((work)->scl.y)
#define RIDING(work) ((work)->scl.z)
#define MARK_FAST 5.0F
#define MARK_DONE 5.0F
#define MARK_RIDING 20.0F

struct deck_vars {
    Uint8 unk_00[8];
    /* player 0's offset in z from the deck at a landing. nothing reads it */
    Float landing_z;
};

#define DECK(work) ((struct deck_vars *)&(work)->vars)

/* the deck's work->scl. LANDING_X holds player 0's offset in x from the deck
 * at a landing. WAIT_LEFT counts the deck's wait after a swing down from
 * DECK_WAIT_STEPS. the wait ends in the step that finds it below 0.
 * HOLDING holds MARK_HOLDING from a landing until that wait ends. after a
 * landing too slow to swing, the platform clears HOLDING at once */
#define LANDING_X(work) ((work)->scl.x)
#define WAIT_LEFT(work) ((work)->scl.y)
#define HOLDING(work) ((work)->scl.z)
#define DECK_WAIT_STEPS 10.0F
#define MARK_HOLDING 10.0F
/* a landing's offsets from the deck are held to these either way */
#define LANDING_X_MOST 15.0F
#define LANDING_Z_MOST 7.0F

static void turn_platform_return(struct task_work *work);
static void turn_platform_init(struct task *task);
static void turn_platform_wait(struct task *task);
static void turn_platform_swing(struct task_work *work);
static void turn_platform_draw(struct task *task);
static void turn_platform_destroy(struct task *task);
static void turn_platform_deck_step(struct task *task);
static void turn_platform_deck_wait(struct task *task);
static void turn_platform_deck_follow(struct task *task);

#define FRAME_SHAPE(kind, x, y, z, size_x, size_y, size_z)                     \
    {(kind) | SHAPE_NO_PUSH,                                                   \
     0,                                                                        \
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),                                           \
     {(x), (y), (z)},                                                          \
     (size_x),                                                                 \
     (size_y),                                                                 \
     (size_z),                                                                 \
     {0},                                                                      \
     0,                                                                        \
     NULL}
static struct collision_shape turn_platform_shapes[7] = {
    FRAME_SHAPE(SHAPE_CYLINDER, 0.0F, -20.0F, 0.0F, 3.0F, 17.0F, 0.0F),
    FRAME_SHAPE(SHAPE_RECTANGLE, 0.0F, -61.0F, 0.0F, 34.5F, 2.5F, 17.0F),
    FRAME_SHAPE(SHAPE_RECTANGLE, 0.0F, -53.0F, 0.0F, 34.5F, 4.0F, 4.0F),
    FRAME_SHAPE(SHAPE_RECTANGLE, 33.0F, -74.0F, 0.0F, 2.5F, 14.0F, 14.0F),
    FRAME_SHAPE(SHAPE_RECTANGLE, -33.0F, -74.0F, 0.0F, 2.5F, 14.0F, 14.0F),
    FRAME_SHAPE(SHAPE_RECTANGLE, 0.0F, -93.0F, 0.0F, 34.5F, 6.0F, 14.0F),
    FRAME_SHAPE(SHAPE_RECTANGLE, 0.0F, -74.0F, -15.0F, 34.5F, 18.0F, 6.0F),
};

/* the deck's shape, which pushes nothing: it only senses a landing */
static struct collision_shape turn_platform_deck_shape = {
    SHAPE_RECTANGLE, 0, 0, {0.0F, 0.0F, 5.0F}, 40.5F, 14.0F, 5.0F, {0}, 0, NULL,
};

void turn_platform_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return;
    }
    switch (work->mode) {
    case TURN_PLATFORM_INIT:
        VARS(work)->deck =
            task_create_child(TASK_WORK, turn_platform_deck_step, task);
        collision_init(task, turn_platform_shapes, COLLISION_OBJECT, 1, 7);
        turn_platform_init(task);
        turn_platform_draw(task);
        task->destructor = turn_platform_destroy;
        task->draw = turn_platform_draw;
        work->collision->radius = TURN_PLATFORM_REACH;
        break;
    case TURN_PLATFORM_WAIT:
        turn_platform_draw(task);
        turn_platform_wait(task);
        task_step_children(task);
        break;
    case TURN_PLATFORM_SWING:
        turn_platform_draw(task);
        task_step_children(task);
        turn_platform_swing(work);
        break;
    case TURN_PLATFORM_RETURN:
        turn_platform_draw(task);
        turn_platform_return(work);
        break;
    case TURN_PLATFORM_GONE:
        placement_kill(task);
        break;
    }
}

static void turn_platform_return(struct task_work *work)
{
    work->ang.x -= RETURN_TURN;
    if (work->ang.x <= 0) {
        work->mode = TURN_PLATFORM_WAIT;
        work->flags |= TURN_PLATFORM_RESTED;
        work->ang.x = 0;
    }
}

static void turn_platform_init(struct task *task)
{
    struct task_work *work = task->work;
    struct task_force *force = task->force;

    force->speed.x = 0.0F;
    force->speed.y = 0.0F;
    force->speed.z = 0.0F;
    force->turn.x = 0;
    force->turn.y = 0;
    force->turn.z = 0;
    work->mode = TURN_PLATFORM_WAIT;
    work->tick = 0;
    FAST(work) = 0.0F;
    DONE(work) = 0.0F;
    RIDING(work) = 0.0F;
    work->ang.x = 0;
}

/* the platform's step at rest, the only one that registers its record. a
 * landing at SPEED_LEAST or more starts a swing and sets how player 0 rides
 * it. a slower landing clears the platform's tick and marks, and the deck's
 * HOLDING. the deck then takes the next landing */
static void turn_platform_wait(struct task *task)
{
    struct task_work *work = task->work;
    struct task *deck;

    collision_register(work);
    if (work->tick != LANDED) {
        return;
    }
    if (VARS(work)->speed < SPEED_LEAST) {
        work->tick = 0;
        FAST(work) = 0.0F;
        DONE(work) = 0.0F;
        RIDING(work) = 0.0F;
        work->ang.x = 0;
        if (VARS(work)->deck != NULL) {
            deck = VARS(work)->deck;
            HOLDING(deck->work) = 0.0F;
        }
        return;
    } else {
        /* @hack redundant: this else, after an arm that returns, which stores
         * TURN_PLATFORM_SWING through R2 at 0x0c9080e2; without it, the store
         * goes through R3, and 2 of 55 words differ */
        work->mode = TURN_PLATFORM_SWING;
    }
    player_set_mode(0, VARS(work)->speed < SPEED_FAST ? PLAYER_MODE_STILL_PUSH
                                                      : PLAYER_MODE_BALL);
}

/* the platform's step in a swing. it holds player 0 RIDE_BELOW under the
 * deck, and gives the deck's work->ang.x each step's turn. as the platform
 * passes back through rest, the swing lets player 0 go, and the platform
 * waits. over the top, the swing flings player 0 off, and the platform
 * turns back to rest */
static void turn_platform_swing(struct task_work *work)
{
    struct task *player_task;
    struct task_work *player;
    struct task *deck;
    Float degrees;
    Angle turn;

    sound_play(SE_SWING, 0, 0, 0);
    /* @hack spelling: player_task assigned inside its test, which tests R0 at
     * 0x0c90812e and copies player_task to R4 in the slot of the BF/S;
     * assigned first, 3 of 415 words differ */
    /* @hack redundant: the else after the return, which loads player_get's
     * address, and later VARS(work)->deck, into R2 at 0x0c908138 and
     * 0x0c908148; without it, both load into R3, and 4 of 415 words differ */
    if ((player_task = player_get_task(0)) == NULL) {
        return;
    } else {
        player = player_task->work;
    }
    if (player_get(0) == NULL) {
        return;
    }
    if (VARS(work)->deck == NULL) {
        return;
    }
    deck = VARS(work)->deck;
    RIDING(work) = MARK_RIDING;
    player->pos.x = deck->work->pos.x + LANDING_X(deck->work);
    player->pos.y = deck->work->pos.y - RIDE_BELOW;
    player->pos.z = deck->work->pos.z;
    if (VARS(work)->speed < SPEED_FAST && FAST(work) != MARK_FAST) {
        work->timer++;
        if (VARS(work)->speed > SPEED_GENTLE) {
            degrees =
                -(TASK_TIMER(work) * SWING_BRAKE_GENTLE / VARS(work)->speed) +
                SWING_TURN;
        } else {
            degrees = -(TASK_TIMER(work) * SWING_BRAKE / VARS(work)->speed) +
                      SWING_TURN;
        }
        if (degrees < 0.0F) {
            degrees = _builtin_fabsf(degrees);
            if (degrees > SWING_TURN_MOST) {
                degrees = SWING_TURN_MOST;
            }
            turn = (Angle)(degrees * ANGLE_PER_DEGREE);
            work->ang.x -= turn;
            deck->work->ang.x = -turn;
        } else {
            if (degrees > SWING_TURN_MOST) {
                degrees = SWING_TURN_MOST;
            }
            turn = (Angle)(degrees * ANGLE_PER_DEGREE);
            work->ang.x += turn;
            deck->work->ang.x = turn;
        }
        if (work->ang.x < 0) {
            work->mode = TURN_PLATFORM_WAIT;
            /* @hack spelling: a bare scope for the locals, which stores the
             * mode at 0x0c908240 before speed's initializer; with the
             * locals at the top of the if, 13 of 415 words differ */
            {
                NJS_VECTOR speed = {LET_GO_SPEED, 0.0F, 0.0F};
                Angle ang[3];

                speed.x += VARS(work)->speed;
                ang[0] = 0;
                ang[1] = work->ang.y;
                ang[2] = 0;
                player_set_mode(0, PLAYER_MODE_RESET);
                player_send_flying(0, &speed, ang, LET_GO_STEPS);
                sound_play(SE_LET_GO, 0, 0, 0);
                VARS(work)->speed = 0.0F;
                work->timer = 0;
                DONE(work) = MARK_DONE;
                RIDING(work) = 0.0F;
            }
            return;
        } else if (work->ang.x > HALF_TURN) {
            work->mode = TURN_PLATFORM_RETURN;
            /* @hack spelling: a bare scope for the locals, which stores the
             * mode at 0x0c9082d6 before speed's initializer; with the
             * locals at the top of the if, 13 of 415 words differ */
            {
                NJS_VECTOR speed = {FLING_SPEED, 0.0F, 0.0F};
                Angle ang[3];

                speed.x += VARS(work)->speed;
                ang[0] = 0;
                ang[1] = work->ang.y;
                ang[2] = 0;
                player_set_mode(0, PLAYER_MODE_RESET);
                player_send_flying(0, &speed, ang, FLING_STEPS);
                sound_play(SE_LET_GO, 0, 0, 0);
                VARS(work)->speed = 0.0F;
                work->timer = 0;
                DONE(work) = MARK_DONE;
                RIDING(work) = 0.0F;
            }
            return;
        }
    } else {
        work->timer++;
        FAST(work) = MARK_FAST;
        degrees = (-(TASK_TIMER(work) * FAST_SWING_BRAKE / VARS(work)->speed) +
                   SWING_TURN) *
                  0.5F;
        if (degrees < 0.0F) {
            degrees = _builtin_fabsf(degrees);
            turn = (Angle)(degrees * ANGLE_PER_DEGREE);
            work->ang.x -= turn;
            deck->work->ang.x = -turn;
        } else {
            turn = (Angle)(degrees * ANGLE_PER_DEGREE);
            work->ang.x += turn;
            deck->work->ang.x = turn;
        }
        if (work->ang.x < 0) {
            work->mode = TURN_PLATFORM_WAIT;
            /* @hack spelling: a bare scope for the locals, which stores the
             * mode at 0x0c90838e before speed's initializer; with the
             * locals at the top of the if, 18 of 415 words differ */
            {
                NJS_VECTOR along;
                NJS_VECTOR speed = {0.0F, 0.0F, FAST_LET_GO_SPEED};

                speed.z += VARS(work)->speed;
                njPushMatrix(&_unit_matrix_);
                TURN(njRotateY, work->ang.y);
                njCalcPoint(NULL, &speed, &along);
                njPopMatrix(1);
                player_set_speed(0, along.x, along.y, along.z);
                player_set_mode(0, PLAYER_MODE_RESET);
                VARS(work)->speed = 0.0F;
                work->timer = 0;
                DONE(work) = MARK_DONE;
                RIDING(work) = 0.0F;
                FAST(work) = 0.0F;
            }
            return;
        } else if (work->ang.x * (360.0F / 65536.0F) >= HALF_TURN_DEGREES) {
            work->mode = TURN_PLATFORM_RETURN;
            /* @hack spelling: a bare scope for the locals, which stores the
             * mode at 0x0c908420 before speed's initializer; with the
             * locals at the top of the if, 11 of 415 words differ */
            {
                NJS_VECTOR speed = {FAST_FLING_SPEED, 0.0F, 0.0F};
                Angle ang[3];

                speed.x += VARS(work)->speed;
                ang[0] = 0;
                ang[1] = work->ang.y;
                ang[2] = 0;
                player_send_flying(0, &speed, ang, FLING_STEPS);
                sound_play(SE_LET_GO, 0, 0, 0);
                player_set_mode(0, PLAYER_MODE_RESET);
                VARS(work)->speed = 0.0F;
                work->timer = 0;
                DONE(work) = MARK_DONE;
                RIDING(work) = 0.0F;
                FAST(work) = 0.0F;
            }
        }
    }
}

/* draw the post and arm turned with the swing, and the deck turned back to stay
 * level */
static void turn_platform_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *post = &turn_platform_object;
    NJS_OBJECT *arm = &turn_platform_arm_object;
    NJS_OBJECT *deck = &turn_platform_deck_object;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
    TURN(njRotateY, work->ang.y);
    TURN(njRotateX, work->ang.x);
    draw_model(post->model);
    njPushMatrix(NULL);
    njTranslate(NULL, arm->pos[0], arm->pos[1], arm->pos[2]);
    draw_model(arm->model);
    njPushMatrix(NULL);
    njTranslate(NULL, deck->pos[0], deck->pos[1], deck->pos[2]);
    TURN(njRotateX, -work->ang.x);
    draw_model(deck->model);
    njPopMatrix(1);
    njPopMatrix(1);
    njPopMatrix(1);
}

static void turn_platform_destroy(struct task *task)
{
    task_destroy_children(task);
}

static void turn_platform_deck_step(struct task *task)
{
    struct task_work *work = task->work;

    /* @hack spelling: a return to end each case, where a break would do,
     * which ends each case with an RTS of its own or a tail call (0x0c9085e4,
     * 0x0c9085ea, 0x0c9085f0, 0x0c9085f6); as breaks, 22 of 37 words
     * differ */
    switch (work->mode) {
    case DECK_INIT:
        collision_init(task, &turn_platform_deck_shape, COLLISION_OBJECT, 1, 1);
        work->mode = DECK_FOLLOW;
        return;
    case DECK_FOLLOW:
        turn_platform_deck_follow(task);
        return;
    case DECK_WAIT:
        turn_platform_deck_wait(task);
        return;
    /* @hack redundant: case DECK_GONE, which no code sets and which shares the
     * default's code, keeps CMP/EQ #3 at 0x0c9085c4; without it, 30 of 37
     * words differ */
    case DECK_GONE:
    /* @hack redundant: the default, which returns as the function's end
     * would, and gives a mode no case names an RTS of its own after the tests
     * (0x0c9085ca). without it, that mode branches to DECK_GONE's return, and
     * 29 of 37 words differ */
    default:
        return;
    }
}

/* the deck's step after a swing. it waits DECK_WAIT_STEPS + 1 steps without
 * registering its record. in the step after those, it clears HOLDING and its
 * record's COLLISION_HIT, and follows the arm again */
static void turn_platform_deck_wait(struct task *task)
{
    struct task_work *work = task->work;

    if (WAIT_LEFT(work)-- < 0.0F) {
        work->mode = DECK_FOLLOW;
        WAIT_LEFT(work) = DECK_WAIT_STEPS;
        work->collision->flags &= ~COLLISION_HIT;
        HOLDING(work) = 0.0F;
    }
}

/* the deck's step while it follows the arm, with player 0 present and the set
 * editor closed. it clears the platform's tick, and waits once the platform
 * marks a swing DONE. it moves to where the deck hangs from the arm, turned
 * further by the deck's own work->ang.x. a swing leaves its last turn there.
 * while player 0 doesn't ride, the deck registers its record. while it also
 * holds no landing, it records player 0's speed for the platform and takes a
 * landing */
static void turn_platform_deck_follow(struct task *task)
{
    struct task_work *work = task->work;
    struct task *player_task;
    struct task_work *player;
    struct player *record;
    struct task_work *platform;
    NJS_OBJECT *arm;
    NJS_OBJECT *deck;
    NJS_POINT3 pos;
    NJS_POINT3 origin;
    NJS_VECTOR speed;
    Float landing;

    /* @hack spelling: player_task assigned inside its test, which tests R0 at
     * 0x0c90868e and copies player_task to R4 in the slot of the BF/S;
     * assigned first, 3 of 248 words differ */
    /* @hack redundant: the else after the return, which loads player_get's
     * address into R2 and edit_mode's into R3 at 0x0c908698 and 0x0c9086aa;
     * without it, the two trade registers, and 5 of 248 words differ */
    if ((player_task = player_get_task(0)) == NULL) {
        return;
    } else {
        player = player_task->work;
    }
    if ((record = player_get(0)) != NULL) {
        if (edit_mode == 0) {
            if (task->parent != NULL) {
                platform = task->parent->work;
                platform->tick = 0;
                if (DONE(platform) == MARK_DONE) {
                    work->mode = DECK_WAIT;
                    WAIT_LEFT(work) = DECK_WAIT_STEPS;
                    DONE(platform) = 0.0F;
                    return;
                }
                deck = &turn_platform_deck_object;
                arm = &turn_platform_arm_object;
                origin.x = 0.0F;
                origin.y = 0.0F;
                origin.z = 0.0F;
                njPushMatrix(&_unit_matrix_);
                njTranslate(NULL, platform->pos.x, platform->pos.y,
                            platform->pos.z);
                TURN(njRotateY, platform->ang.y);
                TURN(njRotateX, platform->ang.x + work->ang.x);
                njTranslate(NULL, arm->pos[0], arm->pos[1], arm->pos[2]);
                njTranslate(NULL, deck->pos[0], deck->pos[1], deck->pos[2]);
                njCalcPoint(NULL, &origin, &pos);
                njPopMatrix(1);
                work->pos.x = pos.x;
                work->pos.y = pos.y;
                work->pos.z = pos.z;
                work->ang.y = platform->ang.y;
                if (RIDING(platform) != MARK_RIDING) {
                    if (HOLDING(work) != MARK_HOLDING) {
                        speed.x = record->speed.x;
                        speed.y = record->speed.y;
                        speed.z = record->speed.z;
                        landing = njScalor(&speed);
                        VARS(platform)->speed = landing;
                        if (work->collision->flags & COLLISION_HIT) {
                            work->collision->flags &= ~COLLISION_HIT;
                            platform->tick = LANDED;
                            HOLDING(work) = MARK_HOLDING;
                            LANDING_X(work) = player->pos.x - work->pos.x;
                            if (_builtin_fabsf(LANDING_X(work)) >=
                                LANDING_X_MOST) {
                                if (LANDING_X(work) < 0.0F) {
                                    LANDING_X(work) = -LANDING_X_MOST;
                                } else {
                                    LANDING_X(work) = LANDING_X_MOST;
                                }
                            }
                            DECK(work)->landing_z = player->pos.z - work->pos.z;
                            if (_builtin_fabsf(DECK(work)->landing_z) >=
                                LANDING_Z_MOST) {
                                if (DECK(work)->landing_z < 0.0F) {
                                    DECK(work)->landing_z = -LANDING_Z_MOST;
                                } else {
                                    DECK(work)->landing_z = LANDING_Z_MOST;
                                }
                            }
                            VARS(platform)->speed = landing;
                        }
                    }
                    collision_register(work);
                }
            }
        }
    }
}
