/* Zero, the boss of the Egg Carrier deck fight. a hit from anything but a
 * player, such as the fence, shocks Zero and knocks it down open. a
 * player's hit hurts Zero only while it is open */
#include <ninja.h>
#include <sg_xpt.h>

#include "b_robo/arms.h"
#include "b_robo/banks.h"
#include "b_robo/beam.h"
#include "b_robo/camera.h"
#include "b_robo/debris.h"
#include "b_robo/effects.h"
#include "b_robo/erobo.h"
#include "b_robo/fence.h"
#include "b_robo/marker.h"
#include "b_robo/shock_wave.h"
#include "camera/camera.h"
#include "collision/collision.h"
#include "collision/ground.h"
#include "draw/draw.h"
#include "draw/motion.h"
#include "draw/pose.h"
#include "effect/bomb.h"
#include "event/script.h"
#include "hud/boss_name.h"
#include "hud/hud.h"
#include "hud/life_gauge.h"
#include "object/tikal.h"
#include "player/player.h"
#include "sound/sound.h"
#include "stage/stage.h"
#include "system/frame.h"
#include "system/math.h"
#include "system/pad.h"
#include "system/task.h"
#include "system/util.h"
#include "vmu/vibration.h"

#define TEXT_1_LENGTH 30
#define TEXT_2_LENGTH 30
#include "b_robo/erobo/text.inc"

/* Zero's modes, which index erobo_modes. a mode asks for the next with
 * SET_MODE */
enum erobo_mode {
    EROBO_INTRO, /* it stands while the camera swings round it */
    EROBO_DECIDE, /* it chooses its next attack, and the target to turn to */
    EROBO_WALK, /* it walks toward its target until it can leap or spin */
    EROBO_TURN, /* it turns its head, then its body, to its target */
    EROBO_KNOCKED, /* it hops away from a player's hit */
    /* it fires the laser from its eye. a laser that reaches player 0 marks
     * a lock */
    EROBO_BEAM,
    EROBO_THROW, /* it throws hand 0 at the lock */
    EROBO_LEAP, /* it leaps along its heading and lands with a shock wave */
    EROBO_SPIN, /* it spins with both arms out */
    EROBO_SHOCKED, /* it shakes in lightning, then is knocked down open */
    EROBO_HURT, /* it loses a unit of health to a player's hit */
    EROBO_DEFEAT /* it crackles, explodes and clears the stage */
};

/* the submode that ends a mode. each mode cleans up in a run that ends at
 * MODE_DONE. when erobo_step leaves a mode that hasn't set MODE_DONE, it
 * sets it and runs the mode once more */
#define MODE_DONE (-1)

/* the attacks choose_attack picks from, and ATTACK_NONE before its first
 * pick */
enum erobo_attack {
    ATTACK_NONE,
    ATTACK_BEAM,
    ATTACK_LEAP,
    ATTACK_SPIN /* Zero walks or leaps to the deck's middle first */
};

/* struct erobo's flags */
#define EROBO_RECOILS 0x1 /* the next player's hit knocks it away */
#define EROBO_OPEN 0x2 /* the next player's hit hurts it */
/* erobo_step leaves the motion still. nothing sets it */
#define EROBO_FROZEN 0x4
/* fall adds gravity to its vertical speed. the intro sets it for good */
#define EROBO_FALLS 0x8
#define EROBO_BOUNCED 0x10 /* fall bounced it off the floor this step */
#define EROBO_LANDED 0x20 /* fall stopped it on the floor this step */
#define EROBO_NO_BOUNCE 0x40 /* fall lands it without a bounce */
/* erobo_draw neither draws Zero nor looks for its floor. the defeat sets
 * it as Zero breaks apart */
#define EROBO_HIDDEN 0x80

/* the units of health Zero starts with. each hit while it is open costs
 * one */
#define EROBO_HEALTH 3

/* Zero's record, at its task's extra. it begins with its motion, since
 * motion_create allocates the whole record */
struct erobo {
    struct motion motion;
    Sint32 steps; /* the steps since the intro began. nothing reads it */
    Sint32 timer;
    Sint32 flags;
    Sint32 unk_44; /* erobo_create zeroes this word, and no code reads it */
    Sint32 next_mode; /* an enum erobo_mode */
    Sint32 last_mode; /* the mode SET_MODE found running. nothing reads it */
    Sint32 attack; /* an enum erobo_attack */
    Float health;
    /* the leaps choose_attack has counted since its last beam or spin */
    Sint32 leaps;
    Bool choose; /* TRUE asks choose_attack for a new attack */
    /* the height at which fall stops Zero's position: FLOOR_OFFSET above
     * the floor erobo_draw finds */
    Float floor;
    /* floor as it was a step before. only erobo_draw reads it */
    Float last_floor;
    Float height; /* its position's height above floor. nothing reads it */
    NJS_VECTOR velocity;
    /* its speed along the way it faces, which set_velocity turns into
     * velocity. a negative speed moves it backward */
    Float speed;
    Float gravity; /* added to velocity.y each step under EROBO_FALLS */
    Angle spin; /* added to its heading each step of EROBO_SPIN */
    Float lead; /* how many steps aim leads player 0 by */
    Float target_distance2; /* squared distance to *target in x and z */
    /* where player 0 will be in lead steps, at its speed now */
    NJS_POINT3 aim;
    /* EROBO_TURN's head. turn_size is how far it turned before the body
     * followed. turn is its angle from the body's heading */
    Angle turn_size;
    Angle turn;
    Angle target_angle; /* the heading that faces *target */
    NJS_POINT3 *target; /* aim, lock or deck_middle */
    NJS_POINT3 lock; /* the marker's lock, which EROBO_THROW aims at */
    /* beam_origin is where the laser starts, at the eye. beam_velocity, half
     * the eye's move over the last step, moves the laser's aim each step */
    NJS_POINT3 beam_origin;
    NJS_VECTOR beam_velocity;
    NJS_POINT3 arm_ends[2]; /* where each hand sits at home */
};

/* ask erobo_step for mode m at the end of the step, unless m is running */
#define SET_MODE(work, boss, m)                                                \
    do {                                                                       \
        if ((work)->mode != (m)) {                                             \
            (boss)->next_mode = (m);                                           \
            (boss)->last_mode = (Sint32)(work)->mode;                          \
        }                                                                      \
    } while (0)

/* play action a, unless it is playing */
#define PLAY(boss, a)                                                          \
    do {                                                                       \
        if ((boss)->motion.current != (a)) {                                   \
            (boss)->motion.wanted = (a);                                       \
        }                                                                      \
    } while (0)

/* Zero's actions, which index erobo_actions. a move that src/enemy/zero.c
 * plays too has the name it has there */
enum erobo_action {
    ACTION_STAND,
    ACTION_SETTLE, /* the stand once through, as it blends in */
    ACTION_WALK,
    ACTION_BEAM, /* posed: EROBO_BEAM moves its frame */
    ACTION_WIND_UP,
    ACTION_THROW, /* hand 0 leaves at THROW_FRAME */
    ACTION_THROW_END,
    ACTION_HAND_OUT, /* while hand 0 is away */
    ACTION_HAND_HOME,
    ACTION_HIT_FRONT, /* knocked backward */
    ACTION_HIT_FRONT_LAND,
    ACTION_HIT_BEHIND, /* knocked forward */
    ACTION_HIT_BEHIND_LAND,
    ACTION_TURN_AROUND,
    ACTION_LEAP,
    ACTION_LEAP_FALL,
    ACTION_SPIN,
    ACTION_SHOCKED, /* posed: EROBO_SHOCKED moves its frame */
    ACTION_KNOCKED_DOWN,
    ACTION_HURT,
    ACTION_GET_UP
};

/* the nodes of Zero's figure. the last three actions play on its open
 * figure, which has OPEN_NODES */
#define FIGURE_NODES 48
#define OPEN_NODES 55
/* the nodes at the ends of the arms, and the eye's node */
#define NODE_ARM0_END 30
#define NODE_ARM1_END 18
#define NODE_EYE 10
/* the neck's object, as an offset into the model bank. its y angle turns
 * the head */
#define NECK_OBJECT 0x8e40

/* Zero's collision shapes. the open body and the weak point take the
 * place of the body while Zero is open */
#define SHAPES 5
#define SHAPE_BODY 0
#define SHAPE_HAND0 1
#define SHAPE_HAND1 2
#define SHAPE_OPEN_BODY 3
#define SHAPE_WEAK_POINT 4

/* the body pushes every group. no group but pickups and rings can push
 * it */
#define PUSH_BODY                                                              \
    (PUSH_ALL | PUSH_FIRM(PUSH_PLAYERS | PUSH_ENEMIES | PUSH_OBJECTS))

/* take shape n's attack away, or give it the strongest, SHAPE_ATTACK */
/* @hack spelling: the &= written out, which reads the byte through
 * @(R0,R6) with 4 in R0 (0x0c900532); as &=, 74 of the 248 words of
 * shapes_close and erobo_throw differ */
#define CLEAR_ATTACK(work, n)                                                  \
    ((work)->collision->shapes[n].levels =                                     \
         (work)->collision->shapes[n].levels & ~SHAPE_ATTACK)
/* @hack redundant: the attack cleared before it is set, as the image clears
 * it with AND #252 and sets it with OR #3 (0x0c9005c4, 0x0c9005c6); as |=,
 * 100 of the 242 words of shapes_spin and erobo_throw differ */
#define FULL_ATTACK(work, n)                                                   \
    ((work)->collision->shapes[n].levels =                                     \
         ((work)->collision->shapes[n].levels & ~SHAPE_ATTACK) | SHAPE_ATTACK)

/* where floor and last_floor start, 10 below the deck. erobo_draw takes a
 * floor at or below it for none */
#define FLOOR_START 742.09F
/* how far above the floor shadow_draw finds Zero's position stops */
#define FLOOR_OFFSET 12.0F
/* player 0 at this height or lower has fallen off the deck */
#define FALL_Y 393.47635F

/* without EROBO_FALLS, the part of its vertical speed Zero keeps a step */
#define RISE_DRAG 0.9F
/* a landing at BOUNCE_LEAST or faster down bounces Zero back up, at BOUNCE
 * times the landing's speed */
#define BOUNCE_LEAST (-0.2F)
#define BOUNCE (-0.5F)
/* the part of its speed Zero keeps on a bounce, and on a step on the
 * floor */
#define BOUNCE_DRAG 0.8F
#define LANDED_DRAG 0.93F

/* the field of view set each step of the intro */
#define BOSS_PERSPECTIVE NJM_DEG_ANG(55)

/* the most Zero turns toward its target in a step: TURN_THROW as it winds
 * up, and TURN_WALK otherwise */
#define TURN_WALK NJM_DEG_ANG(4)
#define TURN_THROW NJM_DEG_ANG(3)

/* the most walking speed, and how much a step adds and takes away */
#define SPEED_MOST 1.25F
#define SPEED_GAIN 0.04F
#define SPEED_LOSS (-0.1F)

/* the most spin, and how much a step adds and takes away */
#define SPIN_MOST 0x1fd
#define SPIN_GAIN 3
#define SPIN_LOSS 7

/* gravity, and gravity while it leaps and while it is shocked */
#define GRAVITY (-0.1F)
#define GRAVITY_LEAP (-0.025F)
#define GRAVITY_SHOCKED (-0.004F)

/* choose_attack picks LEAPS_UNHURT leaps before each beam at full health,
 * and LEAPS_HURT below it. below 2 units of health, more leaps follow until
 * Zero is within MIDDLE_NEAR of the deck's middle. a spin then comes before
 * the beam */
#define LEAPS_UNHURT 1
#define LEAPS_HURT 5
#define MIDDLE_NEAR 90.0F

/* Zero spins at once when its target is within SPIN_NEAR. a walk ends in a
 * leap from further than LEAP_LEAST up to LEAP_MOST away */
#define SPIN_NEAR 5.0F
#define LEAP_LEAST 15.0F
#define LEAP_MOST 70.0F

/* a leap's length is the distance to the target, held from LEAP_LEAST to
 * LEAP_MOST. it stops LEAP_SHORT short unless the attack is the spin. it
 * lasts LEAP_STEPS, and aim leads player 0 by LEAP_LEAD steps meanwhile */
#define LEAP_SHORT 15.0F
#define LEAP_STEPS 55
#define LEAP_LEAD 20.0F

/* the most the head turns in a step. a turn of TURN_AROUND_LEAST or more
 * toward player 0 plays ACTION_TURN_AROUND. the laser fires without a turn
 * at a target within BEAM_AIMED of the heading */
#define HEAD_TURN NJM_DEG_ANG(11.25)
#define TURN_AROUND_LEAST NJM_DEG_ANG(90)
#define BEAM_AIMED NJM_DEG_ANG(20)

/* a player's hit knocks Zero away at this speed */
#define KNOCK_SPEED 2.0F

/* the laser fires for at most BEAM_STEPS, and at most LOCKED_STEPS more
 * once it reaches player 0. Zero walks toward its target while further
 * than BEAM_WALK_NEAR, and slows nearer */
#define BEAM_STEPS 240
#define LOCKED_STEPS 60
#define BEAM_WALK_NEAR 50.0F
/* a lock within LOCK_NEAR gets a leap rather than a throw */
#define LOCK_NEAR 20.0F
/* the frame of ACTION_THROW at which hand 0 leaves */
#define THROW_FRAME 12.0F

/* Zero spins with its arms spread for SPIN_STEPS before it slows. its zaps
 * strike ZAP_RADIUS from it */
#define SPIN_STEPS 500
#define ZAP_RADIUS 7.0F

/* the shock lasts SHOCK_STEPS, then knocks Zero toward the deck's middle
 * at SHOCK_SPEED. dust rises for DUST_STEPS as Zero falls. Zero lies for
 * LIE_STEPS before it gets up */
#define SHOCK_STEPS 90
#define SHOCK_SPEED 0.6F
#define DUST_STEPS 8
#define LIE_STEPS 45
/* the shock's lightning strikes within SHOCK_RANGE of Zero in x and z. each
 * step of the dust raises DUST_PUFFS puffs, DUST_HEIGHT above Zero's
 * position */
#define SHOCK_RANGE 8.0F
#define DUST_PUFFS 8
#define DUST_HEIGHT 10.0F

/* the defeat crackles for CRACKLE_STEPS. Zero breaks apart with
 * BREAK_LEFT steps of the explosion left. the stage is clear CLEAR_WAIT
 * steps after the explosion ends */
#define CRACKLE_STEPS 200
#define BREAK_LEFT 100
#define CLEAR_WAIT 200
/* the crackle's beams are CRACKLE_LENGTH long, and turn by up to
 * CRACKLE_TURN a step about each axis. the defeat's lightning strikes
 * within CRACKLE_RANGE of Zero in x and z */
#define CRACKLE_LENGTH 60.0F
#define CRACKLE_TURN NJM_DEG_ANG(1)
#define CRACKLE_RANGE 5.0F

/* the clock, as time_get gives it, packed into one number that grows with
 * it. it counts a minute as 6000 and a second as 100 */
#define CLOCK_TIME(minutes, seconds, frames)                                   \
    (((minutes) * 6000) + ((seconds) * 100) + (frames))

/* the voice Tikal speaks the hint in. the hint waits until the fight has
 * run longer than HINT_TIME by the clock */
#define HINT_VOICE 0x23d
#define HINT_TIME CLOCK_TIME(1, 0, 0)

/* sound effects. a sound src/enemy/zero.c plays too has the name it has
 * there */
#define SE_STOMP 710
#define SE_THUD 762
#define SE_SHOCK_END 765
#define SE_SHOCK 766
#define SE_HIT 767
#define SE_HUM 769
#define SE_WHIRL 770
#define SE_LEAP 771
#define SE_TURN 772
#define SE_BEAM 773
#define SE_LOCK 774
#define SE_KNOCK_LAND 775

/* the table Zero's motion plays, in enum erobo_action's order */
static struct motion_entry erobo_actions[] = {
    {&erobo_stand_action, FIGURE_NODES, MOTION_LOOP, ACTION_STAND, 1.0F, 1.0F},
    {&erobo_stand_action, FIGURE_NODES, MOTION_ONCE, ACTION_STAND, 0.0625F,
     1.0F},
    {&erobo_walk_action, FIGURE_NODES, MOTION_LOOP, ACTION_STAND, 0.0625F,
     1.0F},
    {&erobo_beam_action, FIGURE_NODES, MOTION_POSED, ACTION_STAND, 0.0625F,
     1.0F},
    {&erobo_wind_up_action, FIGURE_NODES, MOTION_ONCE, ACTION_THROW, 1.0F,
     1.0F},
    {&erobo_throw_action, FIGURE_NODES, MOTION_ONCE, ACTION_THROW_END, 1.0F,
     1.0F},
    {&erobo_throw_end_action, FIGURE_NODES, MOTION_HOLD, ACTION_STAND, 1.0F,
     1.0F},
    {&erobo_hand_out_action, FIGURE_NODES, MOTION_LOOP, ACTION_STAND, 1.0F,
     1.0F},
    {&erobo_hand_home_action, FIGURE_NODES, MOTION_HOLD, ACTION_STAND, 1.0F,
     1.0F},
    {&erobo_hit_front_action, FIGURE_NODES, MOTION_ONCE, ACTION_HIT_FRONT_LAND,
     1.0F, 1.0F},
    {&erobo_hit_front_land_action, FIGURE_NODES, MOTION_ONCE, ACTION_STAND,
     1.0F, 1.0F},
    {&erobo_hit_behind_action, FIGURE_NODES, MOTION_ONCE,
     ACTION_HIT_BEHIND_LAND, 1.0F, 1.0F},
    {&erobo_hit_behind_land_action, FIGURE_NODES, MOTION_ONCE, ACTION_STAND,
     1.0F, 1.0F},
    {&erobo_turn_around_action, FIGURE_NODES, MOTION_HOLD, ACTION_STAND, 0.125F,
     0.8F},
    {&erobo_leap_action, FIGURE_NODES, MOTION_ONCE, ACTION_LEAP_FALL, 1.0F,
     0.7F},
    {&erobo_leap_fall_action, FIGURE_NODES, MOTION_HOLD, ACTION_STAND, 1.0F,
     1.0F},
    {&erobo_spin_action, FIGURE_NODES, MOTION_HOLD, ACTION_STAND, 0.25F, 0.5F},
    {&erobo_shocked_action, FIGURE_NODES, MOTION_POSED, ACTION_STAND, 1.0F,
     0.0F},
    {&erobo_knocked_down_action, OPEN_NODES, MOTION_HOLD, ACTION_STAND, 1.0F,
     1.0F},
    {&erobo_hurt_action, OPEN_NODES, MOTION_HOLD, ACTION_GET_UP, 1.0F, 1.0F},
    {&erobo_get_up_action, OPEN_NODES, MOTION_HOLD, ACTION_STAND, 1.0F, 1.0F},
};

/* TRUE once erobo_hint has started Tikal's hint, or once Zero is hurt */
static Bool hint_given = FALSE;

/* the body hurts players only while shapes_spin gives it an attack. a
 * hand's sphere is on only in a throw or a leap. it hurts players only in a
 * throw. while Zero is open, the open body and the weak point take the
 * body's place. the open body's guard is full. the weak point, 10 above
 * Zero's position, has no guard */
static struct collision_shape erobo_shapes[SHAPES] = {
    {SHAPE_SPHERE | SHAPE_HURTABLE,
     SHAPE_HURTS_PLAYERS,
     PUSH_BODY,
     {0.0F, 0.0F, 0.0F},
     10.0F,
     0.0F,
     0.0F,
     {0, 0, 0},
     0,
     NULL},
    {SHAPE_SPHERE | SHAPE_OFF | SHAPE_IN_WORLD | SHAPE_HURTABLE,
     SHAPE_HURTS_PLAYERS | SHAPE_GUARD,
     PUSH_FIRM(PUSH_ALL),
     {0.0F, 0.0F, 0.0F},
     4.0F,
     0.0F,
     0.0F,
     {0, 0, 0},
     0,
     NULL},
    {SHAPE_SPHERE | SHAPE_OFF | SHAPE_IN_WORLD | SHAPE_HURTABLE,
     SHAPE_HURTS_PLAYERS | SHAPE_GUARD,
     PUSH_FIRM(PUSH_ALL),
     {0.0F, 0.0F, 0.0F},
     4.0F,
     0.0F,
     0.0F,
     {0, 0, 0},
     0,
     NULL},
    {SHAPE_SPHERE | SHAPE_OFF | SHAPE_HURTABLE,
     SHAPE_HURTS_PLAYERS | SHAPE_GUARD,
     PUSH_BODY,
     {0.0F, 0.0F, 0.0F},
     10.0F,
     0.0F,
     0.0F,
     {0, 0, 0},
     0,
     NULL},
    {SHAPE_SPHERE | SHAPE_OFF | SHAPE_HURTABLE,
     SHAPE_HURTS_PLAYERS,
     PUSH_BODY,
     {0.0F, 10.0F, 0.0F},
     2.0F,
     0.0F,
     0.0F,
     {0, 0, 0},
     0,
     NULL},
};

static void erobo_intro(struct task *task);
static void erobo_decide(struct task *task);
static void erobo_walk(struct task *task);
static void erobo_turn(struct task *task);
static void erobo_knocked(struct task *task);
static void erobo_beam(struct task *task);
static void erobo_throw(struct task *task);
static void erobo_leap(struct task *task);
static void erobo_spin(struct task *task);
static void erobo_shocked(struct task *task);
static void erobo_hurt(struct task *task);
static void erobo_defeat(struct task *task);

static void (*erobo_modes[])(struct task *task) = {
    erobo_intro,   erobo_decide,  erobo_walk,  erobo_turn,
    erobo_knocked, erobo_beam,    erobo_throw, erobo_leap,
    erobo_spin,    erobo_shocked, erobo_hurt,  erobo_defeat,
};

static const NJS_POINT3 deck_middle = {DECK_MIDDLE_X, DECK_FLOOR_Y,
                                       DECK_MIDDLE_Z};
static const NJS_POINT3 start_pos = {DECK_MIDDLE_X, DECK_FLOOR_Y,
                                     DECK_MIDDLE_Z};

static const char boss_name[5] = {
#include "b_robo/erobo/boss_name.inc"
};

/* where the laser starts, in NODE_EYE's space, and the origin of a
 * node's space */
static const NJS_POINT3 eye_offset = {4.5F, -5.0F, 0.0F};
static const NJS_POINT3 node_origin = {0.0F, 0.0F, 0.0F};

/* Tikal's hint: two pages, and the steps each shows */
static const struct script_page hint_pages[3] = {
    {TEXT_1, 120},
    {TEXT_2, 160},
    {NULL, 0},
};

/* Zero's task, for erobo_get_arm_end, erobo_set_hand_shape and
 * erobo_get_heading. erobo_create sets it, and nothing clears it */
static struct task *erobo_task;
/* the clock as the intro began, as CLOCK_TIME packs it */
static int start_time;
/* the matrix of the node erobo_pose last asked the pose for */
static NJS_MATRIX pose_matrix;

static void erobo_step(struct task *task);
static void erobo_draw(struct task *task);
static void erobo_destroy(struct task *task);

void erobo_create(void)
{
    struct task *task;
    struct task_work *work;
    struct erobo *boss;

    if ((task = task_create(TASK_WORK, 2, erobo_step)) != NULL) {
        work = task->work;
        collision_init(task, erobo_shapes, COLLISION_ENEMY, 1, SHAPES);
        /* @hack spelling: the work through task->work, which the image loads
         * again after collision_init (MOV.L @(32,R13),R3 at 0x0c90040c);
         * through work, 61 of 85 words differ */
        task->work->collision->radius = 1000.0F;
        boss = (struct erobo *)motion_create(work, sizeof(struct erobo));
        task->extra = boss;
        boss->motion.wanted = ACTION_STAND;
        boss->motion.current = ACTION_STAND;
        boss->motion.actions = erobo_actions;
        boss->last_mode = EROBO_INTRO;
        boss->next_mode = EROBO_INTRO;
        boss->unk_44 = 0;
        boss->target = &boss->aim;
        boss_name_create(boss_name, -1, 240, 120);
        task->draw = erobo_draw;
        task->destructor = erobo_destroy;
        erobo_beam_init(&boss->beam_origin, &boss->beam_velocity, &work->ang.y);
        erobo_arms_start();
        erobo_effects_start(task);
        erobo_fence_create();
        erobo_task = task;
    }
}

static void erobo_destroy(struct task *task)
{
    struct task_work *work = task->work;
    struct erobo *boss = task->extra;

    erobo_beam_detach();
    erobo_arms_end();
    erobo_effects_end();
    /* the motion's last step frees its blend */
    boss->motion.state = MOTION_FREE;
    motion_step(work, &boss->motion);
    tikal_hint_end();
}

/* close Zero: the body on with no attack, and the open shapes off */
static void shapes_close(struct task_work *work, struct erobo *boss)
{
    work->collision->shapes[SHAPE_BODY].flags &= ~SHAPE_OFF;
    CLEAR_ATTACK(work, SHAPE_BODY);
    work->collision->shapes[SHAPE_OPEN_BODY].flags |= SHAPE_OFF;
    work->collision->shapes[SHAPE_WEAK_POINT].flags |= SHAPE_OFF;
    boss->flags &= ~EROBO_OPEN;
}

/* open Zero: the open shapes on, and the body and the hands off */
static void shapes_open(struct task_work *work, struct erobo *boss)
{
    work->collision->shapes[SHAPE_OPEN_BODY].flags &= ~SHAPE_OFF;
    work->collision->shapes[SHAPE_WEAK_POINT].flags &= ~SHAPE_OFF;
    work->collision->shapes[SHAPE_BODY].flags |= SHAPE_OFF;
    work->collision->shapes[SHAPE_HAND0].flags |= SHAPE_OFF;
    work->collision->shapes[SHAPE_HAND1].flags |= SHAPE_OFF;
    boss->flags |= EROBO_OPEN;
}

/* arm Zero to spin: the body on at the strongest attack, and the hands
 * off */
static void shapes_spin(struct task_work *work, struct erobo *boss)
{
    work->collision->shapes[SHAPE_BODY].flags &= ~SHAPE_OFF;
    FULL_ATTACK(work, SHAPE_BODY);
    work->collision->shapes[SHAPE_HAND0].flags |= SHAPE_OFF;
    work->collision->shapes[SHAPE_HAND1].flags |= SHAPE_OFF;
}

/* turn the hands' spheres on, and stop the body pushing players.
 * hands_off undoes both */
static void hands_on(struct task_work *work)
{
    work->collision->shapes[SHAPE_HAND0].flags &= ~SHAPE_OFF;
    work->collision->shapes[SHAPE_HAND1].flags &= ~SHAPE_OFF;
    work->collision->shapes[SHAPE_BODY].push &= ~PUSH_PLAYERS;
}

static void hands_off(struct task_work *work)
{
    work->collision->shapes[SHAPE_HAND0].flags |= SHAPE_OFF;
    work->collision->shapes[SHAPE_HAND1].flags |= SHAPE_OFF;
    work->collision->shapes[SHAPE_BODY].push |= PUSH_PLAYERS;
}

void erobo_get_arm_end(int arm, NJS_POINT3 *pos)
{
    struct erobo *boss = erobo_task->extra;

    pos->x = boss->arm_ends[arm].x;
    pos->y = boss->arm_ends[arm].y;
    pos->z = boss->arm_ends[arm].z;
}

void erobo_set_hand_shape(int hand, NJS_POINT3 *pos)
{
    struct task_work *work = erobo_task->work;

    work->collision->shapes[hand + SHAPE_HAND0].center.x = pos->x;
    work->collision->shapes[hand + SHAPE_HAND0].center.y = pos->y;
    work->collision->shapes[hand + SHAPE_HAND0].center.z = pos->z;
}

Angle erobo_get_heading(void)
{
    struct task_work *work = erobo_task->work;

    return work->ang.y;
}

/* answer the step's hit, if any. a player's hit hurts Zero while it is
 * open, or else knocks it away while it recoils. a hit from any other kind
 * of record shocks it */
static void erobo_take_hit(struct task *task)
{
    struct task_work *work = task->work;
    struct erobo *boss = task->extra;
    struct collision_work *collision;

    if (work->flags & COLLISION_DAMAGED) {
        collision = work->collision;
        if (collision->hit->kind == COLLISION_PLAYER) {
            if (boss->flags & EROBO_OPEN) {
                boss->flags &= ~EROBO_OPEN;
                SET_MODE(work, boss, EROBO_HURT);
                return;
            } else if (boss->flags & EROBO_RECOILS) {
                boss->flags &= ~EROBO_RECOILS;
                SET_MODE(work, boss, EROBO_KNOCKED);
                return;
            }
            /* @hack redundant: the mode tested here and again in SET_MODE,
             * as the image tests it twice (CMP/EQ #9 at 0x0c900768 and
             * 0x0c90076e); tested in SET_MODE alone, 16 of 60 words
             * differ */
        } else if (work->mode != EROBO_SHOCKED) {
            SET_MODE(work, boss, EROBO_SHOCKED);
        }
    }
}

/* swing the camera round Zero as the fight starts */
static void intro_camera(struct task *task)
{
    struct task_work *work = task->work;

    camera_start_swing(work, 12.0F, 40.0F, 240);
    camera_override_mode(CAMERA_MODE_CHAOS_STINIT, CAMERA_ADJUST_NONE);
}

/* start the fight's camera, which follows the player and turns toward
 * Zero */
static void fight_camera(struct task *task)
{
    struct task_work *work = task->work;

    erobo_camera_set_center(&work->pos);
    erobo_camera_start();
    erobo_camera_set_mode(EROBO_CAMERA_TURN);
}

/* move Zero up or down a step. below the floor, it rises at least to it.
 * a landing bounces when it is fast enough and EROBO_NO_BOUNCE is clear.
 * any other landing stops on the floor */
static void fall(struct task_work *work, struct erobo *boss)
{
    Float rise;
    Float speed;

    boss->flags &= ~(EROBO_BOUNCED | EROBO_LANDED);
    rise = boss->floor - work->pos.y;
    speed = boss->velocity.y;
    if (rise > 0.0F && rise > speed) {
        speed = rise;
    }
    if (boss->flags & EROBO_FALLS) {
        speed += boss->gravity;
    } else {
        speed *= RISE_DRAG;
    }
    boss->velocity.y = speed;
    work->pos.y += speed;
    if (work->pos.y <= boss->floor) {
        work->pos.y = boss->floor;
        if (!(boss->flags & EROBO_NO_BOUNCE) &&
            boss->velocity.y <= BOUNCE_LEAST) {
            boss->velocity.y *= BOUNCE;
            boss->flags |= EROBO_BOUNCED;
        } else {
            boss->velocity.y = 0.0F;
            boss->flags |= EROBO_LANDED;
        }
    }
}

/* move Zero by its velocity, then fall. fall moves y a second time, by the
 * vertical speed it leaves */
static void move(struct task *task)
{
    struct task_work *work = task->work;
    struct erobo *boss = task->extra;

    VECTOR_SUM(&work->pos, &work->pos, &boss->velocity);
    fall(work, boss);
}

/* the heading Zero needs, standing at from, to face the point to. Zero
 * faces along the negated cosine and sine of its heading */
static Angle heading_to(NJS_POINT3 *from, NJS_POINT3 *to)
{
    Float dx = from->x - to->x;
    Float dz = from->z - to->z;

    return angle_arc_tan2(dz, dx);
}

/* turn the heading toward target_angle by no more than limit */
static void turn_toward(struct task *task, Angle limit)
{
    struct task_work *work = task->work;
    struct erobo *boss = task->extra;
    Angle diff;

    diff = angle_difference(work->ang.y, boss->target_angle);
    work->ang.y += CLAMP(diff, -limit, limit);
}

/* set velocity's x and z to speed along the way Zero faces */
static void set_velocity(struct task *task)
{
    struct task_work *work = task->work;
    struct erobo *boss = task->extra;
    Float sine;
    Float cosine;

    _builtin_fsca(work->ang.y, &sine, &cosine);
    boss->velocity.x = -(cosine * boss->speed);
    boss->velocity.z = -(sine * boss->speed);
}

/* set the velocity of a leap to target over steps steps, under Zero's
 * gravity. with steps 0, the velocity is the whole gap, without gravity */
static void leap_to(struct task *task, NJS_POINT3 *target, int steps)
{
    NJS_POINT3 *pos = &task->work->pos;
    struct erobo *boss = task->extra;
    NJS_VECTOR gap;
    NJS_VECTOR *velocity;
    Float scale;

    velocity = &boss->velocity;
    VECTOR_DIFF(&gap, target, pos);
    if (!steps) {
        VECTOR_COPY(velocity, &gap);
        return;
    }
    scale = 1.0F / steps;
    /* @hack spelling: gap's x and z through its address, which reads each
     * through a copy of R15 (MOV R15,R3 at 0x0c900a2a and 0x0c900a36),
     * where y's read takes R15 itself (0x0c900a44); by name, 9 of 63 words
     * differ */
    velocity->x = (&gap)->x * scale;
    velocity->z = (&gap)->z * scale;
    velocity->y = (gap.y * scale) - (boss->gravity * steps * 0.5F);
}

/* pose the action playing, or the blend, to place the arm ends and the
 * laser's start. then set aim lead steps ahead of player 0, and the
 * heading and the squared distance to *target */
static void erobo_pose(struct task *task)
{
    struct task_work *work = task->work;
    struct erobo *boss = task->extra;
    NJS_ACTION *action;
    NJS_POINT3 point;
    NJS_VECTOR shift;
    NJS_VECTOR *speed;
    NJS_POINT3 *pos;

    njPushMatrix(&_unit_matrix_);
    if (boss->motion.state != MOTION_BLENDING) {
        action = boss->motion.actions[boss->motion.current].action;
    } else {
        action = boss->motion.blend;
    }
    njTranslateV(NULL, &work->pos);
    TURN(njRotateZ, work->ang.z);
    TURN(njRotateY, -work->ang.y);
    TURN(njRotateX, work->ang.x);
    pose_action(NULL, action, boss->motion.frame, NODE_ARM0_END, &pose_matrix);
    njCalcPoint(&pose_matrix, (NJS_POINT3 *)&node_origin, &boss->arm_ends[0]);
    pose_get_matrix(NODE_ARM1_END, &pose_matrix);
    njCalcPoint(&pose_matrix, (NJS_POINT3 *)&node_origin, &boss->arm_ends[1]);
    pose_get_matrix(NODE_EYE, &pose_matrix);
    njCalcPoint(&pose_matrix, (NJS_POINT3 *)&eye_offset, &point);
    VECTOR_DIFF(&shift, &point, &boss->beam_origin);
    VECTOR_SCALE(&boss->beam_velocity, &shift, 0.5F);
    VECTOR_COPY(&boss->beam_origin, &point);
    njPopMatrix(1);
    pos = &player_work[0]->pos;
    speed = &player_motion[0]->speed;
    VECTOR_COPY(&point, speed);
    VECTOR_SCALE(&point, &point, boss->lead);
    VECTOR_SUM(&boss->aim, pos, &point);
    boss->target_angle = heading_to(&work->pos, boss->target);
    boss->target_distance2 =
        point_distance_squared_across(&work->pos, boss->target);
}

/* break Zero's figure into debris, posed as it stands */
static void break_apart(struct task *task)
{
    struct task_work *work = task->work;
    struct erobo *boss = task->extra;
    NJS_ACTION *action;

    if (boss->motion.state != MOTION_BLENDING) {
        action = boss->motion.actions[boss->motion.current].action;
    } else {
        action = boss->motion.blend;
    }
    njPushMatrix(&_unit_matrix_);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateZ, work->ang.z);
    TURN(njRotateY, -work->ang.y);
    TURN(njRotateX, work->ang.x);
    erobo_debris_create(&work->pos, action, boss->motion.frame);
    njPopMatrix(1);
}

/* show Tikal's hint once the fight has run longer than HINT_TIME by the
 * clock. the hint waits for a retry, as stage_get_retries counts them. a
 * hurt Zero rules it out */
static void erobo_hint(struct erobo *boss)
{
    Uint8 minutes;
    Uint8 seconds;
    Uint8 frames;
    int time;

    if (!hint_given) {
        if (boss->health != EROBO_HEALTH) {
            hint_given = TRUE;
        } else if (stage_get_retries() > 0) {
            time_get(&minutes, &seconds, &frames);
            time = CLOCK_TIME(minutes, seconds, frames);
            if ((time - start_time < 0 ? start_time - time
                                       : time - start_time) > HINT_TIME) {
                tikal_hint_create((struct script_page *)hint_pages, HINT_VOICE);
                hint_given = TRUE;
            }
        }
    }
}

/* pick the next attack if choose is TRUE, and set choose FALSE. the pick
 * goes by Zero's health and its leaps since the last beam or spin */
static void choose_attack(struct task *task)
{
    struct task_work *work = task->work;
    struct erobo *boss = task->extra;
    Float distance2;

    if (boss->choose == TRUE) {
        switch ((int)boss->health) {
        case 0:
        case 1:
            if (boss->leaps < LEAPS_HURT) {
                boss->attack = ATTACK_LEAP;
                boss->leaps++;
            } else {
                distance2 = point_distance_squared_across(
                    &work->pos, (NJS_POINT3 *)&deck_middle);
                if (distance2 < MIDDLE_NEAR * MIDDLE_NEAR) {
                    boss->attack = ATTACK_SPIN;
                    boss->leaps = 0;
                } else {
                    boss->attack = ATTACK_LEAP;
                }
            }
            break;
        case 2:
            if (boss->leaps < LEAPS_HURT) {
                boss->attack = ATTACK_LEAP;
                boss->leaps++;
            } else {
                boss->attack = ATTACK_BEAM;
                boss->leaps = 0;
            }
            break;
        /* @hack redundant: case 3, which shares the default's code, keeps the
         * image's CMP/EQ #3 at 0x0c900f14; without it, 85 of 85 words
         * differ */
        case 3:
        default:
            if (boss->leaps < LEAPS_UNHURT) {
                boss->attack = ATTACK_LEAP;
                boss->leaps++;
            } else {
                boss->attack = ATTACK_BEAM;
                boss->leaps = 0;
            }
            break;
        }
        boss->choose = FALSE;
    }
}

/* set the fight up, and stand while the camera swings round Zero. the
 * clock and both pads stay stopped until the swing is done */
static void erobo_intro(struct task *task)
{
    struct task_work *work = task->work;
    struct erobo *boss = task->extra;
    Uint8 minutes;
    Uint8 seconds;
    Uint8 frames;

    camera_set_perspective(BOSS_PERSPECTIVE);
    time_stop();
    pad_disable_player(0);
    pad_disable_player(1);
    switch (work->submode) {
    case 0:
        boss->attack = ATTACK_NONE;
        boss->choose = TRUE;
        VECTOR_COPY(&work->pos, &start_pos);
        work->ang.x = work->ang.y = work->ang.z = 0;
        boss->velocity.x = boss->velocity.y = boss->velocity.z = 0.0F;
        boss->flags = 0;
        boss->steps = 0;
        boss->health = EROBO_HEALTH;
        boss->floor = boss->last_floor = FLOOR_START;
        boss->height = 0.0F;
        boss->gravity = GRAVITY;
        /* past every count of leaps, so the first attack is the beam */
        boss->leaps = 100;
        shapes_close(work, boss);
        boss->target = &boss->aim;
        boss->lead = 1.0F;
        time_get(&minutes, &seconds, &frames);
        start_time = CLOCK_TIME(minutes, seconds, frames);
        boss->flags |= EROBO_FALLS;
        erobo_fence_start();
        intro_camera(task);
        PLAY(boss, ACTION_STAND);
        work->submode++;
        break;
    case 1:
        if (camera_swing_done) {
            work->submode = MODE_DONE;
        }
        break;
    }
    if (work->submode == MODE_DONE) {
        time_start();
        pad_enable_player(0);
        pad_enable_player(1);
        life_gauge_create(0, 0, (int)boss->health);
        fight_camera(task);
        SET_MODE(work, boss, EROBO_DECIDE);
    }
    move(task);
}

/* choose an attack and its target, and go on to EROBO_TURN. the spin's
 * target is the deck's middle, and within SPIN_NEAR of it Zero spins at
 * once */
static void erobo_decide(struct task *task)
{
    struct task_work *work = task->work;
    struct erobo *boss = task->extra;

    move(task);
    switch (work->submode) {
    case 0:
        boss->flags |= EROBO_RECOILS;
        PLAY(boss, ACTION_STAND);
        boss->flags &= ~EROBO_HIDDEN;
        boss->lead = 1.0F;
        choose_attack(task);
        work->submode++;
        break;
    case 1:
        VECTOR_SCALE(&boss->velocity, &boss->velocity, 0.95F);
        switch (boss->attack) {
        case ATTACK_BEAM:
            boss->target = &boss->aim;
            SET_MODE(work, boss, EROBO_TURN);
            break;
        case ATTACK_LEAP:
            boss->target = &boss->aim;
            SET_MODE(work, boss, EROBO_TURN);
            break;
        case ATTACK_SPIN:
            boss->target = (NJS_POINT3 *)&deck_middle;
            if (boss->target_distance2 < SPIN_NEAR * SPIN_NEAR) {
                SET_MODE(work, boss, EROBO_SPIN);
            } else {
                SET_MODE(work, boss, EROBO_TURN);
            }
            break;
        }
        break;
    }
    /* @hack redundant: an empty test of the submode, which keeps the image's
     * CMP/EQ #-1 (0x0c9012d6); without it, 6 of 124 words differ */
    if (work->submode == MODE_DONE) {
    }
}

/* speed up along the way Zero faces, and turn toward its target */
static void walk_on(struct task *task)
{
    struct erobo *boss = task->extra;

    boss->speed += SPEED_GAIN;
    boss->speed = CLAMP(boss->speed, 0.0F, SPEED_MOST);
    set_velocity(task);
    turn_toward(task, TURN_WALK);
}

/* walk toward the target, and leap or spin once it is near enough */
static void erobo_walk(struct task *task)
{
    struct task_work *work = task->work;
    struct erobo *boss = task->extra;

    move(task);
    switch (work->submode) {
    case 0:
        PLAY(boss, ACTION_WALK);
        sound_play(SE_TURN, 0, 0, 0);
        work->submode++;
        break;
    case 1:
        if (boss->target_distance2 <= LEAP_LEAST * LEAP_LEAST) {
            if (boss->target_distance2 < SPIN_NEAR * SPIN_NEAR) {
                SET_MODE(work, boss, EROBO_SPIN);
                work->submode = MODE_DONE;
                break;
            }
        } else if (boss->target_distance2 <= LEAP_MOST * LEAP_MOST) {
            SET_MODE(work, boss, EROBO_LEAP);
            work->submode = MODE_DONE;
            break;
        }
        walk_on(task);
        break;
    }
    /* @hack redundant: an empty test of the submode, which keeps the image's
     * CMP/EQ #-1 (0x0c90142a); without it, 10 of 81 words differ */
    if (work->submode == MODE_DONE) {
    }
}

/* turn the head to the target, then the body after it. go on to the
 * attack, or for the spin to EROBO_WALK. the head's turn goes into the
 * neck object in the model bank */
static void erobo_turn(struct task *task)
{
    struct task_work *work = task->work;
    struct erobo *boss = task->extra;
    Angle ang;
    NJS_OBJECT *object;

    move(task);
    switch (work->submode) {
    case 0:
        boss->flags |= EROBO_RECOILS;
        boss->turn_size = 0;
        boss->turn = 0;
        if (boss->attack == ATTACK_BEAM) {
            ang = angle_distance(work->ang.y, boss->target_angle);
            if (ang <= BEAM_AIMED) {
                SET_MODE(work, boss, EROBO_BEAM);
                work->submode = MODE_DONE;
                break;
            }
        }
        work->submode++;
        break;
    case 1:
        ang = angle_difference(work->ang.y, boss->target_angle);
        boss->turn = angle_approach(boss->turn, ang, HEAD_TURN);
        if (angle_distance(ang, boss->turn) < HEAD_TURN) {
            boss->turn_size = angle_distance(boss->turn, 0);
            work->submode++;
        }
        break;
    case 2:
        ang = angle_approach(boss->turn, 0, HEAD_TURN);
        work->ang.y += angle_difference(ang, boss->turn);
        boss->turn = ang;
        if (angle_distance(boss->turn, 0) < HEAD_TURN) {
            if (boss->target == &boss->aim &&
                boss->turn_size >= TURN_AROUND_LEAST) {
                PLAY(boss, ACTION_TURN_AROUND);
                motion_flags &= ~MOTION_HOLDING;
                work->submode = 3;
            } else {
                work->submode = 4;
            }
        }
        break;
    case 3:
        turn_toward(task, TURN_WALK);
        if (motion_flags & MOTION_HOLDING) {
            PLAY(boss, ACTION_SETTLE);
            work->submode++;
        }
        break;
    case 4:
        switch (boss->attack) {
        case ATTACK_BEAM:
            SET_MODE(work, boss, EROBO_BEAM);
            break;
        case ATTACK_LEAP:
            SET_MODE(work, boss, EROBO_LEAP);
            break;
        case ATTACK_SPIN:
            SET_MODE(work, boss, EROBO_WALK);
            break;
        }
        work->submode = MODE_DONE;
        break;
    }
    if (work->submode == MODE_DONE) {
        boss->turn = 0;
    }
    object = (NJS_OBJECT *)(erobo_models + NECK_OBJECT);
    object->ang[1] = -boss->turn;
}

/* hop away from player 0. Zero hops backward when it faces within a
 * quarter turn of player 0, and forward otherwise. each bounce and each
 * step on the floor slows it */
static void erobo_knocked(struct task *task)
{
    struct task_work *work = task->work;
    struct erobo *boss = task->extra;
    NJS_POINT3 *pos;
    NJS_VECTOR away;
    Float sine;
    Float cosine;
    Angle ang;

    move(task);
    switch (work->submode) {
    case 0:
        pos = &player_work[0]->pos;
        /* @hack spelling: away's x and z through its address, which stores each
         * through a copy of R15 plus 8 (MOV R15,R3 at 0x0c90170a and
         * 0x0c901716); by name, 192 of 219 words differ */
        (&away)->x = work->pos.x - pos->x;
        (&away)->z = work->pos.z - pos->z;
        ang = angle_arc_tan2(away.z, away.x);
        /* nothing reads the sine and cosine */
        _builtin_fsca(ang, &sine, &cosine);
        boss->velocity.y = 1.0F;
        if (angle_distance(work->ang.y, ang) < NJM_DEG_ANG(90)) {
            work->ang.y = ang;
            boss->speed = -KNOCK_SPEED;
            PLAY(boss, ACTION_HIT_FRONT);
        } else {
            work->ang.y = ang + NJM_DEG_ANG(180);
            boss->speed = KNOCK_SPEED;
            PLAY(boss, ACTION_HIT_BEHIND);
        }
        erobo_marker_lose();
        sound_play_at(SE_THUD, 0, 0, 0, work->pos.x, work->pos.y, work->pos.z);
        sound_play_at(SE_KNOCK_LAND, 0, 0, 0, work->pos.x, work->pos.y,
                      work->pos.z);
        work->submode++;
        break;
    case 1:
        set_velocity(task);
        if (boss->flags & EROBO_BOUNCED) {
            boss->speed *= BOUNCE_DRAG;
        }
        if (boss->flags & EROBO_LANDED) {
            boss->speed *= LANDED_DRAG;
        }
        if (boss->motion.current == ACTION_HIT_FRONT_LAND ||
            boss->motion.current == ACTION_HIT_BEHIND_LAND) {
            work->submode++;
        }
        break;
    case 2:
        set_velocity(task);
        if (boss->flags & EROBO_BOUNCED) {
            boss->speed *= BOUNCE_DRAG;
        }
        if (boss->flags & EROBO_LANDED) {
            boss->speed *= LANDED_DRAG;
        }
        if (boss->motion.current == ACTION_STAND) {
            boss->velocity.x = boss->velocity.y = boss->velocity.z = 0.0F;
            SET_MODE(work, boss, EROBO_DECIDE);
            work->submode = MODE_DONE;
        }
        break;
    }
    if (work->submode == MODE_DONE) {
        boss->velocity.x = boss->velocity.y = boss->velocity.z = 0.0F;
    }
}

/* walk toward the target while it is further than BEAM_WALK_NEAR, and
 * slow down nearer. Zero turns toward it either way */
static void beam_walk(struct task *task)
{
    struct erobo *boss = task->extra;

    if (boss->target_distance2 > BEAM_WALK_NEAR * BEAM_WALK_NEAR) {
        boss->speed += SPEED_GAIN;
    } else {
        boss->speed += SPEED_LOSS;
    }
    boss->speed = CLAMP(boss->speed, 0.0F, SPEED_MOST);
    set_velocity(task);
    turn_toward(task, TURN_WALK);
}

/* play ACTION_BEAM forward, fire the laser, and play the action back. a
 * laser that reached player 0 leads to a throw at the lock. a lock within
 * LOCK_NEAR gets a leap instead */
static void erobo_beam(struct task *task)
{
    struct task_work *work = task->work;
    struct erobo *boss = task->extra;
    NJS_ACTION *action;
    NJS_POINT3 lock;
    Float distance2;

    move(task);
    action = &erobo_beam_action;
    switch (work->submode) {
    case 0:
        boss->target = &boss->aim;
        boss->flags |= EROBO_RECOILS;
        PLAY(boss, ACTION_BEAM);
        work->submode++;
        break;
    case 1:
        boss->motion.frame += 1.0F;
        if (boss->motion.frame >= (Float)action->motion->nbFrame - 1.0F) {
            boss->motion.frame = (Float)action->motion->nbFrame - 1.0F;
            erobo_beam_set_mode(EROBO_BEAM_FIRE);
            boss->timer = BEAM_STEPS;
            work->submode++;
        }
        break;
    case 2:
        beam_walk(task);
        if (erobo_beam_reached() == TRUE) {
            erobo_marker_create();
            sound_play(SE_LOCK, 0, 0, 0);
            if (boss->timer >= LOCKED_STEPS) {
                boss->timer = LOCKED_STEPS;
            }
            work->submode++;
        }
        if (--boss->timer <= 0) {
            work->submode++;
        }
        break;
    case 3:
        beam_walk(task);
        if (--boss->timer <= 0) {
            boss->timer = 0;
            erobo_beam_set_mode(EROBO_BEAM_STOP);
            work->submode++;
        }
        break;
    case 4:
        boss->motion.frame -= 1.0F;
        if (boss->motion.frame <= 0.0F) {
            boss->motion.frame = 0.0F;
            if (erobo_beam_reached() == TRUE) {
                erobo_marker_get_lock(&lock);
                distance2 = point_distance_squared_across(&lock, &work->pos);
                if (distance2 < LOCK_NEAR * LOCK_NEAR) {
                    erobo_marker_lose();
                    SET_MODE(work, boss, EROBO_LEAP);
                } else {
                    erobo_marker_wind_up();
                    SET_MODE(work, boss, EROBO_THROW);
                }
            } else {
                SET_MODE(work, boss, EROBO_DECIDE);
            }
            work->submode = MODE_DONE;
        }
        break;
    }
    switch (work->submode) {
    case 2:
    case 3:
        sound_play_for(SE_BEAM, (int)task, 1, 0, 4);
        break;
    }
    if (work->submode == MODE_DONE) {
        if (boss->attack == ATTACK_BEAM) {
            boss->choose = TRUE;
        }
        erobo_beam_set_mode(EROBO_BEAM_STOP);
    }
}

/* wind up toward the marker's lock, and throw hand 0 at it. hand 0's
 * sphere hurts players until the arms are idle again */
static void erobo_throw(struct task *task)
{
    struct task_work *work = task->work;
    struct erobo *boss = task->extra;
    int holding;

    move(task);
    erobo_marker_get_lock(&boss->lock);
    holding = motion_flags & MOTION_HOLDING;
    switch (work->submode) {
    case 0:
        PLAY(boss, ACTION_WIND_UP);
        boss->velocity.x = boss->velocity.y = boss->velocity.z = 0.0F;
        boss->target = &boss->lock;
        work->submode++;
        break;
    case 1:
        turn_toward(task, TURN_THROW);
        if (boss->motion.current == ACTION_THROW) {
            work->submode++;
        }
        break;
    case 2:
        turn_toward(task, TURN_THROW);
        if (boss->motion.frame >= THROW_FRAME) {
            erobo_marker_lose();
            erobo_arms_set_mode(EROBO_ARMS_THROW);
            work->submode++;
        }
        break;
    case 3:
        work->collision->shapes[SHAPE_HAND0].flags &= ~SHAPE_OFF;
        FULL_ATTACK(work, SHAPE_HAND0);
        if (boss->motion.current == ACTION_THROW_END) {
            motion_flags &= ~MOTION_HOLDING;
            work->submode++;
        }
        break;
    case 4:
        if (holding) {
            PLAY(boss, ACTION_HAND_OUT);
            work->submode++;
        }
        break;
    case 5:
        if (erobo_arms_get_mode() == EROBO_ARMS_IDLE) {
            work->collision->shapes[SHAPE_HAND0].flags |= SHAPE_OFF;
            CLEAR_ATTACK(work, SHAPE_HAND0);
            PLAY(boss, ACTION_HAND_HOME);
            motion_flags &= ~MOTION_HOLDING;
            work->submode++;
        }
        break;
    case 6:
        if (holding) {
            SET_MODE(work, boss, EROBO_DECIDE);
            work->submode = MODE_DONE;
        }
        break;
    }
    if (work->submode == MODE_DONE) {
        if (boss->attack == ATTACK_BEAM) {
            boss->choose = TRUE;
        }
        work->collision->shapes[SHAPE_HAND0].flags |= SHAPE_OFF;
        CLEAR_ATTACK(work, SHAPE_HAND0);
        boss->target = &boss->aim;
    }
}

/* leap along the heading and land with a shock wave. the hands' spheres
 * are on as Zero comes down */
static void erobo_leap(struct task *task)
{
    struct task_work *work = task->work;
    struct erobo *boss = task->extra;
    /* @hack spelling: distance declared ahead of sine and cosine, which has
     * SHC copy it from FR4 to FR2 for x's product (FMOV FR4,FR2 at
     * 0x0c901f78); declared last, 64 of 255 words differ */
    Float distance;
    Float sine;
    Float cosine;
    NJS_POINT3 target;

    move(task);
    erobo_set_hand_shape(0, &boss->arm_ends[0]);
    erobo_set_hand_shape(1, &boss->arm_ends[1]);
    switch (work->submode) {
    case 0:
        boss->gravity = GRAVITY_LEAP;
        boss->flags |= EROBO_NO_BOUNCE;
        boss->flags &= ~EROBO_RECOILS;
        PLAY(boss, ACTION_LEAP);
        boss->lead = LEAP_LEAD;
        work->submode++;
        break;
    case 1:
        distance = point_distance_across(&work->pos, boss->target);
        distance = CLAMP(distance, LEAP_LEAST, LEAP_MOST);
        if (boss->attack != ATTACK_SPIN) {
            distance -= LEAP_SHORT;
        }
        _builtin_fsca(work->ang.y, &sine, &cosine);
        target.x = work->pos.x - (distance * cosine);
        target.z = work->pos.z - (distance * sine);
        target.y = work->pos.y;
        sound_play_at(SE_LEAP, 0, 0, 0, work->pos.x, work->pos.y, work->pos.z);
        leap_to(task, &target, LEAP_STEPS);
        work->submode++;
        break;
    case 2:
        if (boss->motion.current == ACTION_LEAP_FALL) {
            hands_on(work);
            motion_flags &= ~MOTION_HOLDING;
            work->submode++;
        }
        break;
    case 3:
        if ((boss->flags & EROBO_LANDED) || (boss->flags & EROBO_BOUNCED)) {
            boss->velocity.x = boss->velocity.y = boss->velocity.z = 0.0F;
            erobo_shock_wave_create(work);
            vibration_start(0, 3, 50, 2);
            sound_play_at(SE_THUD, 0, 0, 0, work->pos.x, work->pos.y,
                          work->pos.z);
            sound_play_at(SE_STOMP, 0, 0, 0, work->pos.x, work->pos.y,
                          work->pos.z);
            hands_off(work);
            work->submode++;
        }
        break;
    case 4:
        if (motion_flags & MOTION_HOLDING) {
            SET_MODE(work, boss, EROBO_DECIDE);
            work->submode = MODE_DONE;
        }
        break;
    }
    if (work->submode == MODE_DONE) {
        if (boss->attack == ATTACK_LEAP) {
            boss->choose = TRUE;
        }
        erobo_beam_set_mode(EROBO_BEAM_STOP);
        boss->flags &= ~EROBO_NO_BOUNCE;
        boss->gravity = GRAVITY;
        hands_off(work);
    }
}

/* stop, spin up with both arms out, and zap round Zero. then slow the
 * spin to a stop as the arms come back. the beam is the next attack */
static void erobo_spin(struct task *task)
{
    struct task_work *work = task->work;
    struct erobo *boss = task->extra;

    move(task);
    switch (work->submode) {
    case 0:
        boss->speed += SPEED_LOSS;
        boss->speed = CLAMP(boss->speed, 0.0F, SPEED_MOST);
        set_velocity(task);
        boss->spin = 0;
        erobo_camera_set_mode(EROBO_CAMERA_FAR);
        if (boss->speed == 0.0F) {
            shapes_spin(work, boss);
            PLAY(boss, ACTION_SPIN);
            boss->flags &= ~EROBO_RECOILS;
            erobo_sparkles_set_mode(EROBO_SPARKLES_IN);
            erobo_sparkles_set_points(&boss->arm_ends[0]);
            work->submode++;
        }
        break;
    case 1:
        if (boss->motion.current == ACTION_SPIN) {
            motion_flags &= ~MOTION_HOLDING;
            sound_play_held(SE_WHIRL, (int)task, 1, 0);
            sound_play_held(SE_HUM, (int)task, 1, 0);
            work->submode++;
        }
        break;
    case 2:
        boss->spin += SPIN_GAIN;
        boss->spin = CLAMP(boss->spin, 0, SPIN_MOST);
        if (motion_flags & MOTION_HOLDING) {
            erobo_glow_set_mode(EROBO_GLOW_IN);
            erobo_arms_set_mode(EROBO_ARMS_SPREAD);
            boss->timer = SPIN_STEPS;
            work->submode++;
        }
        break;
    case 3:
        boss->spin += SPIN_GAIN;
        boss->spin = CLAMP(boss->spin, 0, SPIN_MOST);
        if (!frame_step) {
            erobo_zap_create(&work->pos, ZAP_RADIUS);
        }
        if (--boss->timer <= 0) {
            work->submode++;
        }
        break;
    case 4:
        boss->spin -= SPIN_LOSS;
        boss->spin = CLAMP(boss->spin, 0, SPIN_MOST);
        erobo_zap_create(&work->pos, ZAP_RADIUS);
        /* the spin is an integer, never negative, so this holds only at 0 */
        if ((Float)boss->spin <= 180.0F / 65536.0F) {
            sound_stop(SE_WHIRL);
            erobo_sparkles_set_mode(EROBO_SPARKLES_OUT);
            erobo_arms_set_mode(EROBO_ARMS_RETURN);
            work->submode++;
        }
        break;
    case 5:
        boss->spin -= SPIN_LOSS;
        boss->spin = CLAMP(boss->spin, 0, SPIN_MOST);
        erobo_zap_create(&work->pos, ZAP_RADIUS);
        if (erobo_arms_get_mode() == EROBO_ARMS_IDLE) {
            erobo_glow_set_mode(EROBO_GLOW_OUT);
            shapes_close(work, boss);
            sound_stop(SE_HUM);
            PLAY(boss, ACTION_SETTLE);
            work->submode++;
        }
        break;
    case 6:
        if (boss->motion.current == ACTION_STAND) {
            SET_MODE(work, boss, EROBO_DECIDE);
            work->submode = MODE_DONE;
        }
        break;
    }
    work->ang.y += boss->spin;
    if (work->submode == MODE_DONE) {
        boss->attack = ATTACK_BEAM;
        erobo_glow_set_mode(EROBO_GLOW_OUT);
        if (erobo_arms_get_mode() != EROBO_ARMS_IDLE) {
            erobo_arms_set_mode(EROBO_ARMS_RETURN);
        }
        erobo_sparkles_set_mode(EROBO_SPARKLES_OUT);
        sound_stop(SE_WHIRL);
        erobo_camera_set_mode(EROBO_CAMERA_TURN);
    }
}

static void raise_dust(struct task *task)
{
    struct task_work *work = task->work;
    NJS_POINT3 pos;
    int i;

    VECTOR_COPY(&pos, &work->pos);
    pos.y += DUST_HEIGHT;
    for (i = 0; i < DUST_PUFFS; i++) {
        erobo_dust_create(&pos, 1.0F);
    }
}

/* shake in lightning, facing the deck's middle. a Zero in the air sinks
 * slowly meanwhile. then fall open toward the middle, lie, and get up */
static void erobo_shocked(struct task *task)
{
    struct task_work *work = task->work;
    struct erobo *boss = task->extra;
    NJS_ACTION *action;

    move(task);
    switch (work->submode) {
    case 0:
        boss->velocity.x = boss->velocity.y = boss->velocity.z = 0.0F;
        PLAY(boss, ACTION_SHOCKED);
        boss->timer = SHOCK_STEPS;
        work->ang.y = heading_to(&work->pos, (NJS_POINT3 *)&deck_middle);
        boss->motion.frame = 0.0F;
        boss->gravity = GRAVITY_SHOCKED;
        erobo_glow_set_mode(EROBO_GLOW_BRIGHT);
        work->submode++;
        break;
    case 1:
        if (!frame_step) {
            action = &erobo_shocked_action;
            boss->motion.frame += 1.0F;
            if (boss->motion.frame >= (Float)action->motion->nbFrame - 1.0F) {
                boss->motion.frame = 0.0F;
            }
            erobo_lightning_create(&work->pos, SHOCK_RANGE);
        }
        sound_play_for(SE_SHOCK, (int)task, 1, 0, 4);
        if (--boss->timer <= 0) {
            boss->gravity = GRAVITY;
            boss->speed = SHOCK_SPEED;
            set_velocity(task);
            boss->velocity.y = 1.0F;
            erobo_glow_set_mode(EROBO_GLOW_OUT);
            shapes_open(work, boss);
            boss->timer = DUST_STEPS;
            PLAY(boss, ACTION_KNOCKED_DOWN);
            motion_flags &= ~MOTION_HOLDING;
            sound_play_at(SE_SHOCK_END, 0, 0, 0, work->pos.x, work->pos.y,
                          work->pos.z);
            work->submode++;
        }
        break;
    case 2:
        if (boss->timer > 0) {
            boss->timer--;
            raise_dust(task);
        }
        if (!frame_step) {
            erobo_lightning_create(&work->pos, SHOCK_RANGE);
        }
        set_velocity(task);
        if (boss->flags & EROBO_LANDED) {
            /* the drag twice over. SHC folds the product to the image's
             * float, 0x3f5d6a14, where 0.8649F would be 0x3f5d6a16 */
            boss->speed *= LANDED_DRAG * LANDED_DRAG;
        }
        if (motion_flags & MOTION_HOLDING) {
            boss->timer = LIE_STEPS;
            work->submode++;
        }
        break;
    case 3:
        if (!frame_step) {
            erobo_lightning_create(&work->pos, SHOCK_RANGE);
        }
        if (--boss->timer <= 0) {
            PLAY(boss, ACTION_GET_UP);
            motion_flags &= ~MOTION_HOLDING;
            work->submode++;
        }
        break;
    case 4:
        if (motion_flags & MOTION_HOLDING) {
            boss->timer = 20;
            shapes_close(work, boss);
            PLAY(boss, ACTION_STAND);
            SET_MODE(work, boss, EROBO_DECIDE);
            work->submode = MODE_DONE;
        }
        break;
    }
    if (work->submode == MODE_DONE) {
        boss->choose = TRUE;
        shapes_close(work, boss);
        boss->gravity = GRAVITY;
    }
}

/* lose a unit of health, get up and leap, or at 0 be defeated */
static void erobo_hurt(struct task *task)
{
    struct task_work *work = task->work;
    struct erobo *boss = task->extra;
    int holding;
    NJS_POINT3 pos;

    move(task);
    holding = motion_flags & MOTION_HOLDING;
    switch (work->submode) {
    case 0:
        boss->velocity.x = boss->velocity.y = boss->velocity.z = 0.0F;
        PLAY(boss, ACTION_HURT);
        motion_flags &= ~MOTION_HOLDING;
        VECTOR_COPY(&pos, &work->pos);
        pos.y += 10.0F;
        bomb_create(&pos, 0.3F);
        sound_play_at(SE_HIT, 0, 0, 0, work->pos.x, work->pos.y, work->pos.z);
        boss->health -= 1.0F;
        if (boss->health <= 0.0F) {
            boss->health = 0.0F;
            SET_MODE(work, boss, EROBO_DEFEAT);
            work->submode = MODE_DONE;
        } else {
            work->submode++;
        }
        break;
    case 1:
        if (holding) {
            PLAY(boss, ACTION_GET_UP);
            motion_flags &= ~MOTION_HOLDING;
            work->submode++;
        }
        break;
    case 2:
        if (holding) {
            SET_MODE(work, boss, EROBO_LEAP);
            work->submode = MODE_DONE;
        }
        break;
    }
    if (work->submode == MODE_DONE) {
        /* past every count of leaps. below 2 units of health, choose_attack
         * then picks the spin once Zero is near the deck's middle */
        if ((int)boss->health <= 1) {
            boss->leaps = 50;
        }
        boss->choose = TRUE;
    }
}

/* stop the clock and the fence, and crackle with beams. then explode,
 * break apart and clear the stage */
static void erobo_defeat(struct task *task)
{
    struct task_work *work = task->work;
    struct erobo *boss = task->extra;

    move(task);
    switch (work->submode) {
    case 0:
        work->submode++;
        time_stop();
        erobo_fence_stop();
        boss->timer = CRACKLE_STEPS;
        break;
    case 1:
        /* a beam comes when timer is a multiple of timer / 20. the beams
         * come about every tenth step at first, and every step once timer
         * is below 40 */
        /* @bug arithmetic: below 20, timer / 20 is 0, and SHC's __modls
         * answers timer % 0 with 0 (JSR at 0x0c902a2c), so a beam comes every
         * step to the end */
#ifdef GCC
        /* keeps the value the tag names, 0: below 20 the test holds. GCC's
         * own timer % 0 gives timer, which fails it */
        if (boss->timer / 20 == 0 || boss->timer % (boss->timer / 20) == 0) {
#else
        if (boss->timer % (boss->timer / 20) == 0) {
#endif
            erobo_beams_create(&work->pos, CRACKLE_LENGTH, CRACKLE_TURN);
            erobo_lightning_create(&work->pos, CRACKLE_RANGE);
        }
        if (--boss->timer <= 0) {
            pad_disable();
            boss->timer = erobo_explosion_start(&work->pos);
            erobo_camera_set_mode(EROBO_CAMERA_CIRCLE);
            work->submode++;
        }
        break;
    case 2:
        if (!frame_step) {
            erobo_lightning_create(&work->pos, CRACKLE_RANGE);
        }
        if (--boss->timer <= BREAK_LEFT) {
            boss->flags |= EROBO_HIDDEN;
            break_apart(task);
            work->submode++;
        }
        break;
    case 3:
        if (--boss->timer <= 0) {
            work->submode++;
        }
        break;
    case 4:
        if (++boss->timer >= CLEAR_WAIT) {
            work->submode = MODE_DONE;
            stage_clear();
        }
        break;
    }
    if (work->submode == MODE_DONE) {
        pad_enable();
    }
}

/* run Zero's mode a step. at the end of the step, change to the mode
 * SET_MODE asked for, if any */
static void erobo_step(struct task *task)
{
    struct task_work *work = task->work;
    struct erobo *boss = task->extra;
    NJS_POINT3 *pos = &player_work[0]->pos;

    if (pos->y <= FALL_Y) {
        player_fall(0);
    }
    boss->steps++;
    player_set_look_point(0, &work->pos);
    erobo_take_hit(task);
    erobo_pose(task);
    erobo_modes[work->mode](task);
    erobo_beam_step();
    erobo_arms_step();
    erobo_hint(boss);
    if (work->mode != boss->next_mode) {
        if (work->submode != MODE_DONE) {
            work->submode = MODE_DONE;
            erobo_modes[work->mode](task);
        }
        work->mode = boss->next_mode;
        work->submode = 0;
    }
    life_gauge_health = boss->health;
    if (!(boss->flags & EROBO_FROZEN)) {
        motion_step(work, &boss->motion);
    }
    collision_register(work);
    erobo_draw(task);
}

/* draw the laser, the arms and the shadow, and find the floor under Zero.
 * on a frame's first step, draw Zero too */
static void erobo_draw(struct task *task)
{
    struct task_work *work = task->work;
    struct erobo *boss = task->extra;
    NJS_ACTION *action;
    Float floor;

    if (boss->motion.state != MOTION_BLENDING) {
        action = boss->motion.actions[boss->motion.current].action;
    } else {
        action = boss->motion.blend;
    }
    erobo_beam_draw();
    erobo_arms_draw();
    if (boss->flags & EROBO_HIDDEN) {
        floor = boss->floor - FLOOR_OFFSET;
    } else {
        floor = shadow_draw(work, 3.0F);
    }
    if (floor <= FLOOR_START) {
        if (boss->floor == FLOOR_START) {
            floor = FLOOR_START;
        } else {
            floor = boss->floor - FLOOR_OFFSET;
        }
    }
    floor += FLOOR_OFFSET;
    if (boss->last_floor == FLOOR_START) {
        boss->last_floor = floor;
    } else {
        boss->last_floor = boss->floor;
    }
    boss->floor = floor;
    boss->height = work->pos.y - floor;
    if (!frame_step) {
        if (!(boss->flags & EROBO_HIDDEN)) {
            njPushMatrix(NULL);
            njTranslateV(NULL, &work->pos);
            TURN(njRotateZ, work->ang.z);
            TURN(njRotateY, -work->ang.y);
            TURN(njRotateX, work->ang.x);
            njSetTexture(erobo_textures);
            draw_action(action, boss->motion.frame);
            njPopMatrix(1);
        }
    }
}
