/* E-103, the robot E-102 fights in Windy Valley */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "animal/animal.h"
#include "camera/camera.h"
#include "collision/collision.h"
#include "collision/ground.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "draw/motion.h"
#include "draw/pose.h"
#include "draw/tree.h"
#include "effect/dust.h"
#include "effect/hit.h"
#include "enemy/enemy.h"
#include "hud/boss_name.h"
#include "hud/hud.h"
#include "hud/life_gauge.h"
#include "object/placement.h"
#include "player/e102_effects.h"
#include "player/e102_hit.h"
#include "player/lockon.h"
#include "player/player.h"
#include "player/textures.h"
#include "sound/sound.h"
#include "sound/stream.h"
#include "stage/stage.h"
#include "stg02/e103.h"
#include "stg02/e103_actions.h"
#include "stg02/missile.h"
#include "stg05/camera_sweep.h"
#include "stg05/explosion.h"
#include "stg05/hover_dust.h"
#include "stg05/walls.h"
#include "system/frame.h"
#include "system/pad.h"
#include "system/task.h"
#include "system/util.h"

/* the steps a spark lasts */
#define SPARK_STEPS 60
/* the point a spark's lightning flashes about: E-103's core, which must
 * outlive the spark */
#define SPARK_TARGET(work) ((NJS_POINT3 *)(work)->vars.p[2])

extern NJS_OBJECT e103_head_object;
extern NJS_OBJECT e103_jet_object;
static const char e103_boss_title[6];
struct e103;
/* the record e103_draw draws, which its motion callback, e103_turn_head,
 * reads */
static struct e103 *e103_drawing;

/* E-103's record, at task->extra */
struct e103 {
    struct motion motion;
    Float health; /* hits left */
    /* steps since E-103 started, or in E103_FALL since its phase began */
    Sint32 ticks;
    Sint32 countdown; /* steps E103_STAND waits before it fires or jumps */
    Uint32 flags;
    Sint32 next_mode; /* the mode e103_run changes to after this step */
    Sint32 previous_mode;
    Sint32 target; /* the index in e103_targets of the point it flies to */
    Angle aim; /* its head's turn about y from the body's facing */
    /* the turn that e103_settle_pitch adds to ang.x each step */
    Angle pitch_speed;
    /* the height its center rests at: the floor below it, plus
     * CENTER_HEIGHT */
    Float ground;
    Float height; /* its center's height above ground */
    /* the squared distance in x and z to the point e103_turn_to last turned
     * it toward */
    Float distance_squared;
    NJS_POINT3 previous_pos;
    NJS_VECTOR velocity;
    /* the origin of CORE_NODE. the sparks and the explosion read it through
     * pointers, and the animal E-103 frees starts there */
    NJS_POINT3 core;
    NJS_POINT3 smoke_pos[2]; /* where its smoke starts */
    NJS_POINT3 shot_pos[2]; /* where each launcher fires from */
    NJS_VECTOR shot_dir[2]; /* the way each launcher aims */
    Angle bob_phase;
    Float bob; /* how far its jets ride up and down */
};

enum e103_mode {
    E103_INTRO,
    E103_STAND,
    E103_JUMP,
    E103_RISE,
    E103_FLY,
    E103_LAND,
    E103_HURT,
    E103_SHOOT,
    E103_FALL
};

/* the entries of e103_actions, by the actions they play */
enum e103_action {
    ACTION_IDLE,
    ACTION_JUMP,
    ACTION_DESCEND,
    ACTION_LAND,
    ACTION_FLY,
    ACTION_SHOOT,
    ACTION_6,
    ACTION_HURT,
    ACTION_FALL
};

/* the bits of its flags */
#define E103_FLYING 0x1 /* its jets burn */
#define E103_AIMING 0x2 /* its head turns toward player 0 */
#define E103_GRAVITY 0x4 /* gravity pulls it */
#define E103_GUARDED 0x8 /* a hit does it no harm */
#define E103_SMOKE 0x10 /* it trails smoke */
#define E103_SPARKS 0x20 /* it throws sparks */
#define E103_HIDDEN 0x40 /* neither it nor its shadow is drawn */
#define E103_GROUNDED 0x80 /* it stands on the floor */

/* the submode that ends a mode. a mode's step sets it as the mode finishes.
 * e103_run and e103_destroy set it to end the mode they leave. a step that
 * finds it set runs the mode's ending, if the mode has one */
#define E103_DONE (-1)

#define SE_E103_HIT 155
#define SE_E103_LAUNCH 153
#define SE_E103_SPARKS 156
#define SE_E103_THRUST 152
#define SE_E103_JUMP 783
#define SE_E103_LAND 785
/* played as E-103's fall clears the stage */
#define SE_E103_DOWN 1329

/* hits it takes */
#define E103_HEALTH 5
/* knocked back by a hit, it takes this share of the shot's speed */
#define KNOCK_BACK 0.3F

/* its center's height above the floor it stands on */
#define CENTER_HEIGHT 14.0F
/* at this height above ground or more, it takes off for a target. in its
 * intro, gravity doesn't pull it there */
#define TAKE_OFF_HEIGHT 150.0F
/* standing, it lands from this height above ground or more. flying, it
 * lands at this height or less */
#define LAND_HEIGHT 10.0F
/* the height in the world it rises to before it flies */
#define FLIGHT_Y (-470.0F)
/* how far from a wall e100_push_from_walls keeps it */
#define WALL_MARGIN 5.0F

/* player 0 on the ground this near in x and z starts the fight */
#define INTRO_REACH 150.0F
/* player 0 this near in x and z makes it jump as it stands */
#define JUMP_REACH 60.0F
/* as a hurt or shot ends, player 0 this near in x and z picks its next
 * mode */
#define NEAR_REACH 120.0F
/* in flight, it lands this near its target in x and z */
#define TARGET_REACH 30.0F

/* it fires only while fewer of its missiles than this fly */
#define MISSILE_LIMIT 2
/* the steps E103_STAND waits before it fires or jumps, or with no missile
 * flying, at most QUICK_WAIT */
#define STAND_WAIT 60
#define QUICK_WAIT 10
/* the frame of ACTION_SHOOT at which it fires */
#define FIRE_FRAME 35.0F
/* from this frame of ACTION_HURT, it can take off */
#define RECOVER_FRAME 25.0F

#define FALL_ACCEL 0.05F /* the speed gravity adds downward each step */
#define DRAG 0.95F /* the share of its speeds in x and z it keeps each step */
/* flying, it keeps LIFT_DRAG of its speed upward each step. as a flight
 * ends, it keeps LANDING_DRAG of its speeds in x and z */
#define LIFT_DRAG 0.95F
#define LANDING_DRAG 0.6F
#define STOP_SPEED 0.05F /* under this either way, a speed in x or z stops */
#define JUMP_SPEED 2.0F /* the speed upward a jump gives it */
/* rising, it gains RISE_ACCEL a step up to RISE_SPEED */
#define RISE_SPEED 0.75F
#define RISE_ACCEL 0.1F
/* flying, it pitches by PITCH_STEP a step up to FLY_PITCH. it gains THRUST a
 * step the way it faces */
#define FLY_PITCH NJM_DEG_ANG(40)
#define PITCH_STEP NJM_DEG_ANG(3)
#define THRUST 0.15F
/* under this either way, a pitch and its speed stop: half a degree */
#define PITCH_SETTLED 91.0F
/* each step, the pitch's spring adds at most PITCH_PULL to its speed either
 * way. it also takes back PITCH_DAMPING of the speed */
#define PITCH_PULL NJM_DEG_ANG(1)
#define PITCH_DAMPING 0.2F
/* the turn of the bob's phase each step */
#define BOB_STEP 1706U

/* with E103_SPARKS set, it sparks at even odds every SPARK_INTERVAL ticks */
#define SPARK_INTERVAL 90

/* the steps of e103_fall's waits. the camera's sweep round E-103 as it explodes
 * lasts SWEEP_STEPS, centered on SWEEP_ANGLE */
#define SLUMP_STEPS 120
#define SWEEP_STEPS 480
#define CLEAR_STEPS 200
#define SWEEP_ANGLE 0xe000

/* every action of e103_actions has E103_NODES nodes. e103_draw poses only
 * nodes 0 to LAUNCHER_0_NODE, and the nodes whose matrices it reads are all
 * among them. its jets draw at CORE_NODE */
#define E103_NODES 77
#define LAUNCHER_0_NODE 45
#define LAUNCHER_1_NODE 36
#define CORE_NODE 30
#define SMOKE_0_NODE 40
#define SMOKE_1_NODE 31

/* change to mode next after this step, unless it is the mode now. a later
 * request in the same step replaces an earlier one */
#define SET_MODE(task, next)                                                   \
    do {                                                                       \
        if ((task)->work->mode != (next)) {                                    \
            ((struct e103 *)(task)->extra)->next_mode = (next);                \
            ((struct e103 *)(task)->extra)->previous_mode =                    \
                (Sint32)(task)->work->mode;                                    \
        }                                                                      \
    } while (0)

/* the line by which CAMERA_MODE_E103 frames E-103 and player 0 */
static NJS_POINT3 e103_camera_points[2] = {{3010.0F, -480.0F, -1300.0F},
                                           {3280.0F, -420.0F, -1360.0F}};
/* the points it flies to, which e103_next_target picks from */
static NJS_POINT3 e103_targets[3] = {{3170.0F, -482.0F, -1444.0F},
                                     {3043.0F, -521.0F, -1460.0F},
                                     {3020.0F, -525.0F, -1433.0F}};

static struct collision_shape e103_shapes[2] = {
    {LOCKON_NO_NUMBER | SHAPE_HURTABLE,
     SHAPE_HURTS_OBJECTS | SHAPE_HURTS_ENEMIES | SHAPE_HURTS_PLAYERS | GUARD(2),
     PUSH_ENEMIES | PUSH_FIRM(PUSH_OTHERS),
     {0.0F, 0.0F, 0.0F},
     6.0F,
     4.0F,
     0.65F,
     {0, 0, 0},
     0,
     NULL},
    {SHAPE_HURTABLE,
     SHAPE_HURTS_OBJECTS | SHAPE_HURTS_ENEMIES | SHAPE_HURTS_PLAYERS | GUARD(2),
     PUSH_ENEMIES | PUSH_FIRM(PUSH_OTHERS),
     {0.0F, -8.0F, 0.0F},
     4.0F,
     4.0F,
     0.65F,
     {0, 0, 0},
     0,
     NULL}};

static struct motion_entry e103_actions[9] = {
    {&e103_idle_action, E103_NODES, MOTION_LOOP, ACTION_IDLE, 0.0625F, 0.5F},
    {&e103_jump_action, E103_NODES, MOTION_LOOP, ACTION_JUMP, 0.25F, 1.0F},
    {&e103_descending_action, E103_NODES, MOTION_LOOP, ACTION_DESCEND, 0.25F,
     1.0F},
    {&e103_land_action, E103_NODES, MOTION_ONCE, ACTION_IDLE, 0.25F, 1.0F},
    {&e103_fly_action, E103_NODES, MOTION_LOOP, ACTION_FLY, 0.25F, 1.0F},
    {&e103_shoot_action, E103_NODES, MOTION_ONCE, ACTION_SHOOT, 0.25F, 1.0F},
    {&e103_action_6, E103_NODES, MOTION_ONCE, ACTION_6, 0.25F, 1.0F},
    {&e103_hurt_action, E103_NODES, MOTION_ONCE, ACTION_HURT, 0.5F, 1.0F},
    {&e103_fall_action, E103_NODES, MOTION_HOLD, ACTION_FALL, 0.5F, 0.5F}};

/* the smoke it trails from each smoke point */
static struct dust_params e103_smoke_params = {0.5F,
                                               0.03F,
                                               0.19F,
                                               0.05F,
                                               0.025F,
                                               {0.0F, 0.0F, 0.0F},
                                               {0.0F, 0.0F, 0.0F},
                                               {1.0F, 0.28F, 0.28F, 0.35F}};

static void e103_destroy(struct task *task);
static void e103_run_mode(struct task *task);
static void e103_run(struct task *task);
static void e103_draw(struct task *task);
static void e103_turn_head(NJS_OBJECT *object);
static void e103_spark_create(NJS_POINT3 *target);

/* in the set editor, E-103 stands and draws. otherwise it starts at full
 * health, and e103_run steps it from then on */
void e103_start(struct task *task)
{
    struct task_work *work = task->work;
    struct e103 *e103 = task->extra;
    if (placement_editing(task)) {
        if (!task->extra) {
            collision_init(task, e103_shapes, COLLISION_ENEMY, 1, 2);
            enemy_alloc_record(task, work);
            e103 = (struct e103 *)motion_create(work, sizeof(struct e103));
            e103->motion.wanted = ACTION_IDLE;
            e103->motion.current = ACTION_IDLE;
            e103->motion.actions = e103_actions;
            task->extra = e103;
        }
        motion_step(work, &e103->motion);
        e103_draw(task);
    } else {
        collision_init(task, e103_shapes, COLLISION_ENEMY, 1, 2);
        enemy_alloc_record(task, work);
        e103 = (struct e103 *)motion_create(work, sizeof(struct e103));
        e103->motion.wanted = ACTION_IDLE;
        e103->motion.current = ACTION_IDLE;
        e103->motion.actions = e103_actions;
        task->extra = e103;
        motion_step(work, &e103->motion);
        work->ang.x = work->ang.z = 0;
        life_gauge_health = e103->health = E103_HEALTH;
        e103->previous_mode = E103_INTRO;
        e103->next_mode = E103_INTRO;
        e103->target = 1;
        e103->velocity.x = e103->velocity.y = e103->velocity.z = 0.0F;
        task->step = e103_run;
        task->draw = e103_draw;
        task->destructor = e103_destroy;
        camera_set_e103(work, e103_camera_points);
    }
}

/* end the mode it is in, then free the motion's blend */
static void e103_destroy(struct task *task)
{
    struct task_work *work = task->work;
    struct e103 *e103 = task->extra;
    if (work->submode != E103_DONE) {
        work->submode = E103_DONE;
        e103_run_mode(task);
    }
    e103->motion.state = MOTION_FREE;
    motion_step(work, &e103->motion);
}

/* while guarded, E-103 lets its lock-on go each step, and a hit does
 * nothing. unguarded, a hit costs it one health, unless it is hurt already.
 * at no health it falls. with health left, it is hurt, and knocked back by
 * KNOCK_BACK of the shot's speed */
static void e103_take_hit(struct task *task)
{
    struct lockon_target *record;
    struct task_work *work;
    struct e103 *e103;
    NJS_POINT3 pos;
    NJS_VECTOR velocity;
    struct task *shot;
    work = task->work;
    record = (struct lockon_target *)task->motion;
    e103 = task->extra;
    if (e103->flags & E103_GUARDED) {
        lockon_release_hit(task);
        return;
    }
    if (!(work->flags & COLLISION_DAMAGED)) {
        return;
    }
    if (work->mode == E103_HURT) {
        return;
    }

    /* @hack spelling: pos through its address, which copies R15 to R3 for its
     * stores, as at 0x0c901712; without it, 155 of 192 words differ */
    /* @hack spelling: velocity through its address, which copies R15 to R3 for
     * its stores, as at 0x0c90171c; without it, SHC keeps its base in a
     * register of its own, saving R10 too, and 188 of 192 words differ */
    if ((shot = lockon_get_shot(record->lockon)) != 0) {
        (&pos)->x = shot->work->pos.x;
        (&pos)->y = shot->work->pos.y;
        (&pos)->z = shot->work->pos.z;
        (&velocity)->x = shot->motion->speed.x;
        (&velocity)->y = shot->motion->speed.y;
        (&velocity)->z = shot->motion->speed.z;
    } else {
        (&pos)->x = work->pos.x;
        (&pos)->y = work->pos.y;
        (&pos)->z = work->pos.z;
        (&velocity)->x = (&velocity)->y = (&velocity)->z = 0.0F;
    }
    e103->flags |= E103_GUARDED;
    sound_play(SE_E103_HIT, 0, 0, 0);
    e102_hit_create(&pos, -15.0F);
    lockon_release(task);
    e103->health -= 1.0F;
    if (e103->health <= 0.0F) {
        SET_MODE(task, E103_FALL);
    } else {
        SET_MODE(task, E103_HURT);
        e103->velocity.x = (&velocity)->x * KNOCK_BACK;
        e103->velocity.y = (&velocity)->y * KNOCK_BACK;
        e103->velocity.z = (&velocity)->z * KNOCK_BACK;
    }
}

/* guard E-103: its first shape's guard rises to 3, which no attack exceeds.
 * e103_unguard lowers it back to 2 */
static void e103_guard(struct task_work *work, struct e103 *e103)
{
    e103->flags |= E103_GUARDED;
    work->collision->shapes->levels =
        (work->collision->shapes->levels & ~SHAPE_GUARD) | GUARD(3);
}

static void e103_unguard(struct task_work *work, struct e103 *e103)
{
    e103->flags &= ~E103_GUARDED;
    work->collision->shapes->levels =
        (work->collision->shapes->levels & ~SHAPE_GUARD) | GUARD(2);
}

static void e103_bob(struct task *task)
{
    struct e103 *e103 = task->extra;
    e103->bob_phase += BOB_STEP;
    /* @hack redundant: the sine times 1, FLDI1 and FMUL at 0x0c9018ea and
     * 0x0c9018ec; without it, the function is 4 bytes shorter and 7 of 18
     * words differ */
    e103->bob = njSin(e103->bob_phase) * 1.0F;
}

/* 1 while player 0 is within the root of radius_squared in x and z, else
 * 0 */
static int e103_player_within(struct task *task, Float radius_squared)
{
    struct task_work *work = task->work;
    Float z;
    /* @hack redundant: register on x, which keeps x in FR5 and z in FR6,
     * loaded at 0x0c901900 and 0x0c901906; without it, x and z trade
     * registers, and 8 of 19 words differ */
    register Float x;
    Float distance;
    x = work->pos.x - player_work[0]->pos.x;
    z = work->pos.z - player_work[0]->pos.z;
    distance = (x * x) + (z * z);
    return distance < radius_squared;
}

/* turn E-103 about y toward target by at most limit. distance_squared takes the
 * squared distance in x and z to target */
static void e103_turn_to(struct task *task, NJS_POINT3 *target, Angle limit)
{
    struct task_work *work = task->work;
    struct e103 *e103 = task->extra;
    Float z;
    Float x;
    Angle difference;
    x = work->pos.x - target->x;
    z = work->pos.z - target->z;
    /* where x and z are both 0, atan2f divides 0 by 0. the turn is then toward
     * whatever angle the NaN converts to */
    difference = angle_difference(
        work->ang.y, -(Uint32)(Angle)(atan2f(z, x) * ANGLE_PER_RADIAN));
    work->ang.y += (Uint32)CLAMP(difference, -limit, limit);
    e103->distance_squared = (x * x) + (z * z);
}

/* add speed along -x, turned by angle about y, to vector's x and z */
static void e103_push(NJS_POINT3 *vector, Angle angle, Float speed)
{
    NJS_POINT3 direction;
    NJS_POINT3 movement;
    NJS_MATRIX matrix;
    direction.x = -speed;
    direction.y = direction.z = 0.0F;
    njUnitMatrix(&matrix);
    TURN_MATRIX(njRotateY, &matrix, angle);
    njCalcPoint(&matrix, &direction, &movement);
    vector->x += movement.x;
    vector->z += movement.z;
}

/* move target on to one of the other two points, at random, and return
 * it */
static int e103_next_target(struct task *task)
{
    struct e103 *e103 = task->extra;
    int target = e103->target + (rand() & 1) + 1;
    if (target >= 3) {
        target -= 3;
    }
    e103->target = target;
    return target;
}

/* at TAKE_OFF_HEIGHT or more above ground, rise toward another target and
 * return 1. otherwise return 0 */
static int e103_take_off(struct task *task)
{
    struct e103 *e103 = task->extra;
    if (e103->height >= TAKE_OFF_HEIGHT) {
        e103_next_target(task);
        SET_MODE(task, E103_RISE);
        return 1;
    }
    return 0;
}

/* 1 at TAKE_OFF_HEIGHT or more above ground, else 0 */
static int e103_high_above_ground(struct task *task)
{
    struct e103 *e103 = task->extra;
    /* @hack spelling: the test's result returned as 1 or 0 through if and
     * else, not returned itself, which keeps the function a call, the BSR at
     * 0x0c901e40. as return e103->height >= TAKE_OFF_HEIGHT, SHC expands it
     * in e103_intro, where 56 of 147 words differ, and the functions after
     * it move; in this function, 12 of 12 words differ */
    if (e103->height >= TAKE_OFF_HEIGHT) {
        return 1;
    } else {
        return 0;
    }
}

static void e103_fire(struct task *task, int index)
{
    struct e103 *e103 = task->extra;
    e103_missile_create(&e103->shot_pos[index], &e103->shot_dir[index],
                        task->work->collision);
    sound_play(SE_E103_LAUNCH, 0, 0, 0);
}

static void e103_smoke(struct task *task)
{
    int i = 0;
    struct e103 *e103 = task->extra;
    do {
        e103_smoke_params.pos.x = e103->smoke_pos[i].x;
        e103_smoke_params.pos.y = e103->smoke_pos[i].y;
        e103_smoke_params.pos.z = e103->smoke_pos[i].z;
        dust_create_from(&e103_smoke_params);
        /* @hack spelling: i = i + 1, which keeps i in R13 and e103 in R12, at
         * 0x0c901b0e and 0x0c901b1e; without it, i and e103 trade registers,
         * and 10 of 49 words differ */
        i = i + 1;
    } while (i < 2);
}

static void e103_sparks(struct task *task)
{
    struct e103 *e103 = task->extra;
    if (e103->ticks % SPARK_INTERVAL == 0) {
        if (rand() * RAND_SCALE < 0.5F) {
            e103_spark_create(&e103->core);
        }
    }
}

static void e103_explode(struct task *task)
{
    struct e103 *e103 = task->extra;
    struct explosion_params params;
    params.pos = &e103->core;
    params.flags = 0;
    params.ray_start = 0;
    params.ray_end = 480;
    params.ray_length = 100.0F;
    params.ray_turn = 91;
    params.grow_start = 100;
    params.grow_end = 200;
    params.fade_start = 480;
    params.fade_end = 510;
    params.radius = 60.0F;
    e100_explosion_create(&params);
}

/* 1 while player 0 stands on the ground within INTRO_REACH in x and z, else
 * 0 */
static int e103_player_ready(struct task *task)
{
    struct task_work *player = player_get_task(0)->work;
    if (e103_player_within(task, SQUARE(INTRO_REACH))) {
        if (player->flags & PLAYER_ON_GROUND) {
            return 1;
        }
    }
    return 0;
}

/* out of range, E-103 destroys itself. it waits, guarded, for player 0 to
 * be ready, and gravity pulls it only below TAKE_OFF_HEIGHT above ground.
 * the camera then swings round it as its name shows. once the swing is
 * done, the fight starts with a shot */
static void e103_intro(struct task *task)
{
    struct task_work *work = task->work;
    struct e103 *e103 = task->extra;
    if (placement_try_leave(task)) {
        task_kill(task);
        return;
    }
    switch (work->submode) {
    case 0:
        e103->health = E103_HEALTH;
        e103->motion.wanted = ACTION_IDLE;
        e103->flags &= ~(E103_SMOKE | E103_SPARKS);
        e103_guard(work, e103);
        e103_missile_count = 0;
        work->submode = 1;
        break;
    case 1:
        if (e103_player_ready(task)) {
            time_stop();
            pad_disable();
            camera_start_swing(work, 0.0F, 30.0F, 240);
            camera_override_mode(CAMERA_MODE_CHAOS_STINIT, CAMERA_ADJUST_NONE);
            boss_name_create(e103_boss_title, -1, 240, 120);
            work->submode = 2;
            stream_request_music(MUSIC_BOSSTRGT);
        }
        break;
    case 2:
        if (camera_swing_done) {
            time_start();
            pad_enable();
            life_gauge_create(0, 0, E103_HEALTH);
            e103_unguard(work, e103);
            camera_override_mode(CAMERA_MODE_E103, CAMERA_ADJUST_NONE);
            SET_MODE(task, E103_SHOOT);
            work->submode = E103_DONE;
        }
        break;
    }
    if (work->submode == E103_DONE) {
        e103->flags |= E103_GRAVITY;
    } else {
        if (e103_high_above_ground(task)) {
            e103->flags &= ~E103_GRAVITY;
        } else {
            e103->flags |= E103_GRAVITY;
        }
    }
}

/* spring *angle back to 0. each step, *velocity gains the short turn from
 * *angle to 0, held to PITCH_PULL either way. *velocity also loses
 * PITCH_DAMPING of itself. *angle then turns by *velocity. once each is under
 * PITCH_SETTLED either way, both stop at 0 */
static void e103_settle_pitch(Angle *angle, Angle *velocity)
{
    Angle difference = angle_difference(*angle, 0);

    difference = CLAMP(difference, -PITCH_PULL, PITCH_PULL);
    difference -= (Float)*velocity * PITCH_DAMPING;

    *velocity += difference;
    *angle += *velocity;
    if (njAbs((Float)*velocity) < PITCH_SETTLED) {
        if (njAbs((Float)*angle) < PITCH_SETTLED) {
            *velocity = 0;
            *angle = 0;
        }
    }
}

/* push E-103 out of walls, bouncing it off them, and find the floor below.
 * then move it by its velocity, no lower than ground, and slow it in x and z.
 * the step it reaches the floor, it plays SE_E103_LAND. its head turns
 * toward player 0 while E103_AIMING, and otherwise back to the body's
 * facing. its pitch springs back to level */
static void e103_move(struct task *task)
{
    struct task_work *work = task->work;
    struct e103 *e103 = task->extra;
    NJS_VECTOR correction;
    Angle target_angle;
    Angle difference;

    e103->previous_pos = work->pos;
    if (e100_push_from_walls(&work->pos, WALL_MARGIN, &e103->ground,
                             &correction) == 1) {
        /* @hack spelling: correction through its address, which copies R15 to
         * R3 for its reads, at 0x0c901f7a and 0x0c901f80; without it, 90 of 232
         * words differ */
        work->pos.x = work->pos.x + (&correction)->x;
        work->pos.y = work->pos.y + (&correction)->y;
        work->pos.z = work->pos.z + (&correction)->z;
        e103->velocity.x = -e103->velocity.x;
        e103->velocity.z = -e103->velocity.z;
    }
    e103->ground += CENTER_HEIGHT;
    e103->height = work->pos.y - e103->ground;
    if (e103->flags & E103_GRAVITY) {
        e103->velocity.y -= FALL_ACCEL;
    }
    work->pos.y += e103->velocity.y;
    if (!(work->pos.y > e103->ground)) {
        work->pos.y = e103->ground;
        e103->velocity.y = 0.0F;
        if (!(e103->flags & E103_GROUNDED)) {
            e103->flags |= E103_GROUNDED;
            sound_play_at(SE_E103_LAND, 0, 0, 0, work->pos.x, work->pos.y,
                          work->pos.z);
        }
    } else {
        e103->flags &= ~E103_GROUNDED;
    }
    work->pos.x += e103->velocity.x;
    work->pos.z += e103->velocity.z;
    e103->velocity.x *= DRAG;
    e103->velocity.z *= DRAG;
    if (e103->velocity.x < STOP_SPEED && e103->velocity.x > -STOP_SPEED) {
        e103->velocity.x = 0.0F;
    }
    if (e103->velocity.z < STOP_SPEED && e103->velocity.z > -STOP_SPEED) {
        e103->velocity.z = 0.0F;
    }
    if (e103->flags & E103_AIMING) {
        Float x = work->pos.x - player_work[0]->pos.x;
        Float z = work->pos.z - player_work[0]->pos.z;
        /* here too, x and z both 0 make atan2f's NaN. the head then turns
         * toward whatever angle the NaN converts to */
        target_angle = -(Uint32)work->ang.y -
                       (Uint32)(Angle)(atan2f(z, x) * ANGLE_PER_RADIAN);
    } else {
        target_angle = 0;
    }
    difference = angle_difference(e103->aim, target_angle);
    e103->aim += (Uint32)CLAMP(difference, -NJM_DEG_ANG(2.5), NJM_DEG_ANG(2.5));
    e103_settle_pitch(&work->ang.x, &e103->pitch_speed);
}

/* E-103 stands, aiming at player 0. once its countdown runs out, it fires
 * while fewer than MISSILE_LIMIT missiles fly, and otherwise jumps. player 0
 * within JUMP_REACH makes it jump. it lands from LAND_HEIGHT above ground,
 * and takes off from TAKE_OFF_HEIGHT */
static void e103_stand(struct task *task)
{
    struct task_work *work = task->work;
    struct e103 *e103 = task->extra;
    switch (work->submode) {
    case 0:
        camera_override_mode(CAMERA_MODE_E103, CAMERA_ADJUST_NONE);
        e103->motion.wanted = ACTION_IDLE;
        e103->flags |= E103_AIMING;
        work->submode = 1;
        break;
    case 1:
        if (e103_take_off(task) == 1) {
            break;
        }
        if (e103_missile_count == 0 && e103->countdown > QUICK_WAIT) {
            e103->countdown = QUICK_WAIT;
        } else {
            if (--e103->countdown < 0) {
                if (e103_missile_count < MISSILE_LIMIT) {
                    SET_MODE(task, E103_SHOOT);
                } else {
                    SET_MODE(task, E103_JUMP);
                }
                e103->countdown = STAND_WAIT;
            }
        }
        if (e103_player_within(task, SQUARE(JUMP_REACH))) {
            SET_MODE(task, E103_JUMP);
        }
        if (work->pos.y - e103->ground >= LAND_HEIGHT) {
            SET_MODE(task, E103_LAND);
        }
        break;
    }
}

/* once ACTION_JUMP plays, E-103 picks another target and jumps. it flies once
 * it stops rising */
static void e103_jump(struct task *task)
{
    struct task_work *work = task->work;
    struct e103 *e103 = task->extra;
    switch (work->submode) {
    case 0:
        e103->motion.wanted = ACTION_JUMP;
        e103->flags &= ~E103_AIMING;
        work->submode = 1;
        break;
    case 1:
        motion_flags &= ~MOTION_HOLDING;
        if (e103->motion.current == ACTION_JUMP) {
            e103->pitch_speed = 0;
            work->ang.x = 0;
            work->submode = 2;
        }
        break;
    case 2:
        e103_next_target(task);
        e103->velocity.y += JUMP_SPEED;
        sound_play_at(SE_E103_JUMP, 0, 0, 0, work->pos.x, work->pos.y,
                      work->pos.z);
        work->submode = 3;
        break;
    case 3:
        if (e103->velocity.y <= 0.0F) {
            SET_MODE(task, E103_FLY);
            work->submode = E103_DONE;
        }
        break;
    }

    /* @hack redundant: a test of the submode at the end, which loads it at
     * 0x0c90237a and compares it at 0x0c90237e to no effect; without it, 5 of
     * 91 words differ */
    if (work->submode == E103_DONE) {
        return;
    }
}

/* guarded, E-103 rises on its jets to FLIGHT_Y as it turns toward its target.
 * it then flies */
static void e103_rise(struct task *task)
{
    struct task_work *work = task->work;
    struct e103 *e103 = task->extra;
    e100_hover_dust(work, e103->ground - CENTER_HEIGHT);
    switch (work->submode) {
    case 0:
        e103->motion.wanted = ACTION_FLY;
        e103->flags |= E103_FLYING;
        e103->flags &= ~E103_AIMING;
        e103_guard(work, e103);
        work->submode = 1;
        break;
    case 1:
        sound_play_at_for(SE_E103_THRUST, (int)task, 1, 0, work->pos.x,
                          work->pos.y, work->pos.z, 4);

        e103_turn_to(task, &e103_targets[e103->target], NJM_DEG_ANG(5));
        if (e103->velocity.y < RISE_SPEED) {
            e103->velocity.y += RISE_ACCEL;
        }
        if (work->pos.y >= FLIGHT_Y) {
            SET_MODE(task, E103_FLY);
            work->submode = E103_DONE;
        }
        break;
    }
    if (work->submode == E103_DONE) {
        e103->flags &= ~E103_FLYING;
        e103_unguard(work, e103);
    }
}

/* on its jets, guarded, E-103 pitches forward and flies toward its target.
 * below FLIGHT_Y, it takes off again from TAKE_OFF_HEIGHT above ground. it
 * lands within TARGET_REACH of the target, or at LAND_HEIGHT or less above
 * ground */
static void e103_fly(struct task *task)
{
    struct task_work *work = task->work;
    struct e103 *e103 = task->extra;
    e100_hover_dust(work, e103->ground - CENTER_HEIGHT);
    switch (work->submode) {
    case 0:
        e103->motion.wanted = ACTION_FLY;
        e103->flags |= E103_FLYING;
        e103->flags &= ~E103_AIMING;
        e103_guard(work, e103);
        work->submode = 1;
        break;
    case 1:
        sound_play_at_for(SE_E103_THRUST, (int)task, 1, 0, work->pos.x,
                          work->pos.y, work->pos.z, 4);
        e103_turn_to(task, &e103_targets[e103->target], NJM_DEG_ANG(5));
        e103->pitch_speed = 0;

        if (work->ang.x < FLY_PITCH) {
            work->ang.x += PITCH_STEP;
        }
        if (work->pos.y < FLIGHT_Y) {
            if (e103_take_off(task) == 1) {
                break;
            }
        }

        if (!(e103->velocity.y < 0.0F)) {
            e103->velocity.y *= LIFT_DRAG;
        } else {
            e103->velocity.y = 0.0F;
        }
        e103_push(&e103->velocity, work->ang.y, THRUST);
        if (!(e103->distance_squared > SQUARE(TARGET_REACH))) {
            SET_MODE(task, E103_LAND);
            work->submode = E103_DONE;
        }
        if (!(e103->height > LAND_HEIGHT)) {
            SET_MODE(task, E103_LAND);
            work->submode = E103_DONE;
        }
        break;
    }
    if (work->submode == E103_DONE) {
        e103->velocity.x *= LANDING_DRAG;
        e103->velocity.z *= LANDING_DRAG;
        e103->flags &= ~E103_FLYING;
        e103_unguard(work, e103);
    }
}

/* E-103 comes down, turning toward player 0, lands, then fires */
static void e103_land(struct task *task)
{
    struct e103 *e103 = task->extra;
    struct task_work *work = task->work;
    e100_hover_dust(work, e103->ground - CENTER_HEIGHT);
    switch (work->submode) {
    case 0:
        e103->motion.wanted = ACTION_DESCEND;
        work->submode = 1;
        break;
    case 1:
        e103_turn_to(task, &player_work[0]->pos, NJM_DEG_ANG(6.5));
        if (e103->ground == work->pos.y) {
            e103->motion.wanted = ACTION_LAND;
            work->submode = 2;
        }
        break;
    case 2:
        motion_flags &= ~MOTION_HOLDING;
        if (e103->motion.current == ACTION_LAND) {
            motion_flags &= ~MOTION_ENDED;
            work->submode = 3;
        }
        break;
    case 3:
        if (motion_flags & MOTION_ENDED) {
            SET_MODE(task, E103_SHOOT);
            work->submode = E103_DONE;
        }
        break;
    }
}

/* guarded, E-103 reels. at 2 health or less it smokes, and at 1 or less it
 * sparks. from RECOVER_FRAME, it takes off from TAKE_OFF_HEIGHT above
 * ground. as the reel ends, it stands while player 0 is within NEAR_REACH,
 * and otherwise fires */
static void e103_hurt(struct task *task)
{
    struct task_work *work = task->work;
    struct e103 *e103 = task->extra;
    switch (work->submode) {
    case 0:
        e103->motion.wanted = ACTION_HURT;
        e103->flags &= ~E103_AIMING;
        e103_guard(work, e103);
        if (!(e103->health > 2.0F)) {
            e103->flags |= E103_SMOKE;
        }
        if (!(e103->health > 1.0F)) {
            e103->flags |= E103_SPARKS;
        }
        work->submode = 1;
        break;
    case 1:
        motion_flags &= ~MOTION_HOLDING;
        if (e103->motion.current == ACTION_HURT) {
            motion_flags &= ~MOTION_ENDED;
            work->submode = 2;
        }
        break;
    case 2:
        if (!(e103->motion.frame < RECOVER_FRAME)) {
            if (e103_take_off(task) == 1) {
                break;
            }
        }
        if (motion_flags & MOTION_ENDED) {
            if (e103_player_within(task, SQUARE(NEAR_REACH))) {
                SET_MODE(task, E103_STAND);
            } else {
                SET_MODE(task, E103_SHOOT);
            }
            work->submode = E103_DONE;
        }
        break;
    }
    if (work->submode == E103_DONE) {
        e103_unguard(work, e103);
    }
}

/* E-103 stops and turns toward player 0, and fires from its second launcher at
 * FIRE_FRAME. with MISSILE_LIMIT missiles flying already, it stands instead. it
 * then jumps while player 0 is within NEAR_REACH. farther off, it jumps or
 * stands at even odds */
static void e103_shoot(struct task *task)
{
    struct task_work *work = task->work;
    struct e103 *e103 = task->extra;
    switch (work->submode) {
    case 0:
        if (e103_missile_count >= MISSILE_LIMIT) {
            SET_MODE(task, E103_STAND);
            return;
        }
        e103->velocity.x = e103->velocity.y = e103->velocity.z = 0.0F;
        e103->motion.wanted = ACTION_SHOOT;
        work->submode = 1;
        break;
    case 1:
        motion_flags &= ~MOTION_HOLDING;
        if (e103->motion.current == ACTION_SHOOT) {
            work->submode = 2;
        }
        break;
    case 2:
        e103_turn_to(task, &player_work[0]->pos, NJM_DEG_ANG(4));
        motion_flags &= ~MOTION_HOLDING;
        if (!(e103->motion.frame < FIRE_FRAME)) {
            e103_fire(task, 1);
            motion_flags &= ~MOTION_ENDED;
            work->submode = 3;
        }
        break;
    case 3:
        if (motion_flags & MOTION_ENDED) {
            if (e103_player_within(task, SQUARE(NEAR_REACH))) {
                SET_MODE(task, E103_JUMP);
            } else if (rand() < 16383) {
                SET_MODE(task, E103_JUMP);
            } else {
                SET_MODE(task, E103_STAND);
                e103->countdown = STAND_WAIT;
            }
            work->submode = E103_DONE;
        }
        break;
    }
    if (work->submode == E103_DONE) {
        e103->countdown = STAND_WAIT;
    }
}

/* every missile bursts, and E-103 slumps, smoking and sparking, with the
 * time and the pads stopped. it collapses SLUMP_STEPS after it lands, and
 * then explodes. SWEEP_STEPS later it vanishes and frees an animal, and
 * CLEAR_STEPS after that the stage clears */
static void e103_fall(struct task *task)
{
    struct task_work *work = task->work;
    struct e103 *e103 = task->extra;
    switch (work->submode) {
    case 0:
        e103->motion.wanted = ACTION_IDLE;
        e103->flags &= ~E103_AIMING;
        e103->flags |= (E103_SMOKE | E103_SPARKS);
        e103_missile_count = -1;
        time_stop();
        pad_disable();
        work->submode = 1;
        break;
    case 1:
        motion_flags &= ~MOTION_HOLDING;
        if (e103->motion.current == ACTION_IDLE) {
            motion_flags &= ~MOTION_ENDED;
            e103->ticks = 0;
            work->submode = 2;
        }
        break;
    case 2:
        if (!(e103->flags & E103_GROUNDED)) {
            e103->ticks = 0;
        }
        if (e103->ticks >= SLUMP_STEPS) {
            e103->flags &= ~(E103_SMOKE | E103_SPARKS);
            e103->motion.wanted = ACTION_FALL;
            work->submode = 3;
        }
        break;
    case 3:
        if (e103->motion.current == ACTION_FALL) {
            motion_flags &= ~MOTION_ENDED;
            work->submode = 4;
        }
        break;
    case 4:
        e103->ticks = 0;
        camera_sweep_start(&work->pos, SWEEP_ANGLE, SWEEP_STEPS);
        e103_explode(task);
        work->submode = 5;
        break;
    case 5:
        if (e103->ticks >= SWEEP_STEPS) {
            e103->ticks = 0;
            work->submode++;
            animal_create_staying(e103->core.x, e103->core.y, e103->core.z,
                                  animal_start_oum);
            e103->flags |= E103_HIDDEN;
        }
        break;
    case 6:
        if (e103->ticks >= CLEAR_STEPS) {
            work->submode++;
        }
        break;
    case 7:
        sound_play(SE_E103_DOWN, 0, 0, 0);
        stage_clear();
        work->submode = E103_DONE;
        break;
    }
    if (work->submode == E103_DONE) {
        e103->flags &= ~(E103_SMOKE | E103_SPARKS);
        pad_enable();
    }
}

/* the mode's step, after e103_move, which the intro skips at submode 0 */
static void e103_run_mode(struct task *task)
{
    struct task_work *work = task->work;
    switch (work->mode) {
    case E103_INTRO:
        if (work->submode) {
            e103_move(task);
        }
        e103_intro(task);
        break;
    case E103_STAND:
        e103_move(task);
        e103_stand(task);
        break;
    case E103_JUMP:
        e103_move(task);
        e103_jump(task);
        break;
    case E103_RISE:
        e103_move(task);
        e103_rise(task);
        break;
    case E103_FLY:
        e103_move(task);
        e103_fly(task);
        break;
    case E103_LAND:
        e103_move(task);
        e103_land(task);
        break;
    case E103_HURT:
        e103_move(task);
        e103_hurt(task);
        break;
    case E103_SHOOT:
        e103_move(task);
        e103_shoot(task);
        break;
    case E103_FALL:
        e103_move(task);
        e103_fall(task);
        break;
    }
}

/* E-103's step from its second step on. a mode asked for in this step starts
 * once the mode it leaves has run its ending */
static void e103_run(struct task *task)
{
    struct task_work *work = task->work;
    struct e103 *e103 = task->extra;
    /* @hack spelling: motion_step's motion through a local, which keeps e103
     * in R12 and work in R13, loaded at 0x0c902d2a and 0x0c902d30; without
     * it, e103 and work trade registers, and 17 of 65 words differ */
    struct motion *motion;

    e103->ticks++;
    e103_bob(task);
    if (e103->flags & E103_SMOKE) {
        e103_smoke(task);
    }
    if (e103->flags & E103_SPARKS) {
        e103_sparks(task);
    }
    e103_run_mode(task);
    e103_take_hit(task);
    if (work->mode != e103->next_mode) {
        if (work->submode != E103_DONE) {
            work->submode = E103_DONE;
            e103_run_mode(task);
        }
        work->mode = e103->next_mode;
        work->submode = 0;
    }
    life_gauge_health = e103->health;
    motion = &e103->motion;
    motion_step(work, motion);
    collision_register(work);
    e103_draw(task);
}

static const char e103_boss_title[6] = {
#include "stg02/e103/e103_boss_title.inc"
};

static const NJS_POINT3 e103_no_offset = {0.0F, 0.0F, 0.0F};
/* pose E-103 to find where its launchers fire from and aim, and its smoke
 * points and core. unless E103_HIDDEN, draw its shadow, and on a frame's first
 * step E-103 itself with its jets. their flame draws while E103_FLYING */
static void e103_draw(struct task *task)
{
    struct task_work *work = task->work;
    struct e103 *e103 = task->extra;
    NJS_ACTION *action;
    NJS_MATRIX core_matrix;
    NJS_MATRIX launch1_matrix;
    NJS_MATRIX launch0_matrix;
    NJS_MATRIX smoke1_matrix;
    NJS_MATRIX smoke0_matrix;
    NJS_MATRIX *launch0;
    NJS_MATRIX *launch1;
    NJS_MATRIX *smoke0;
    NJS_MATRIX *smoke1;
    NJS_POINT3 offset;
    Angle angle;

    action = e103->motion.state != MOTION_BLENDING
                 ? e103->motion.actions[e103->motion.current].action
                 : e103->motion.blend;
    e103_drawing = e103;
    /* @hack spelling: each matrix pointer assigned inside the call that fills
     * its matrix, which leaves R8 unsaved: the prologue pushes R14 down to R9,
     * then makes the 352-byte frame by the MOV.W of H'FEA0 at 0x0c902db0.
     * assigned in a statement of its own before each call, SHC saves R8 too,
     * and 325 of 336 words differ */
    pose_action(&_unit_matrix_, action, e103->motion.frame, LAUNCHER_0_NODE,
                (launch0 = &launch0_matrix));

    pose_get_matrix(LAUNCHER_1_NODE, (launch1 = &launch1_matrix));
    pose_get_matrix(CORE_NODE, &core_matrix);
    pose_get_matrix(SMOKE_1_NODE, (smoke1 = &smoke1_matrix));
    pose_get_matrix(SMOKE_0_NODE, (smoke0 = &smoke0_matrix));

    if (!(e103->flags & E103_HIDDEN)) {
        shadow_draw(work, 2.0F);
    }
    njSetTexture(&windy_e103_texlist);
    draw_attr_save();
    draw_attr_clear(0, NJD_FLAG_IGNORE_SPECULAR);
    njPushMatrix(0);
    njTranslateV(0, &work->pos);
    TURN_VIA(njRotateX, work->ang.z, angle);
    TURN_VIA(njRotateY, work->ang.y, angle);
    TURN_VIA(njRotateZ, work->ang.x, angle);

    if (!frame_step && !(e103->flags & E103_HIDDEN)) {
        njSetMotionCallback(e103_turn_head);
        draw_action(action, e103->motion.frame);
        njSetMotionCallback(0);
        njPushMatrix(0);

        njMultiMatrix(0, &core_matrix);
        njRotateZ(0, 0x8000);
        njTranslate(0, 5.0F, e103->bob + 5.0F, 0.0F);
        draw_tree(&e103_jet_object);
        if (e103->flags & E103_FLYING) {
            njSetTexture(&e102effect_texlist);
            e102_flame->model->mats[0].attr_texId =
                ((e103->ticks >> 1) & 3) + 8;
            njSimpleDrawObject(e102_flame);
        }
        njPopMatrix(1);
    }

    njPushMatrix(&_unit_matrix_);
    offset = e103_no_offset;
    njTranslateV(0, &work->pos);
    TURN_VIA(njRotateX, work->ang.z, angle);
    TURN_VIA(njRotateY, work->ang.y, angle);
    TURN_VIA(njRotateZ, work->ang.x, angle);

    njPushMatrix(0);
    njMultiMatrix(0, launch0);
    offset.x = 3.0F;
    njCalcPoint(0, &offset, &e103->shot_pos[0]);
    offset.x = -1.0F;
    njCalcVector(0, &offset, &e103->shot_dir[0]);
    njPopMatrix(1);

    njPushMatrix(0);

    njMultiMatrix(0, launch1);
    offset.x = 3.0F;
    njCalcPoint(0, &offset, &e103->shot_pos[1]);
    offset.x = -1.0F;
    njCalcVector(0, &offset, &e103->shot_dir[1]);
    njPopMatrix(1);

    offset.x = 0.0F;
    njPushMatrix(0);
    njMultiMatrix(0, smoke1);
    njCalcPoint(0, &offset, &e103->smoke_pos[1]);
    njPopMatrix(1);
    njPushMatrix(0);
    njMultiMatrix(0, smoke0);
    njCalcPoint(0, &offset, &e103->smoke_pos[0]);
    njPopMatrix(1);
    njMultiMatrix(0, &core_matrix);
    njCalcPoint(0, &offset, &e103->core);
    njPopMatrix(2);
    draw_attr_restore();
}

/* the motion callback of e103_draw: turn the head by e103_drawing's aim */
static void e103_turn_head(NJS_OBJECT *object)
{
    Angle angle;
    if (object == &e103_head_object) {
        TURN_VIA(njRotateY, e103_drawing->aim, angle);
    }
}

/* until its steps run out, a spark flashes lightning at even odds on each
 * frame's first step. each flash lies within 5 of the target on each axis */
static void e103_spark_step(struct task *task)
{
    struct task_work *work = task->work;
    NJS_POINT3 pos;
    if (--TASK_TIMER(work) <= 0) {
        task_kill(task);
        return;
    }
    if (!frame_step && rand() * RAND_SCALE < 0.5F) {
        /* @hack spelling: pos through its address, which copies R15 to R2 for
         * its stores, as at 0x0c9031cc; without it, 62 of 92 words differ */
        (&pos)->x = SPARK_TARGET(work)->x;
        (&pos)->y = SPARK_TARGET(work)->y;
        (&pos)->z = SPARK_TARGET(work)->z;
        pos.x += (rand() * (10.0F * RAND_SCALE)) - 5.0F;
        pos.y += (rand() * (10.0F * RAND_SCALE)) - 5.0F;
        pos.z += (rand() * (10.0F * RAND_SCALE)) - 5.0F;
        lightning_create(&pos, 1.0F);
    }
}

/* a spark at target that lasts SPARK_STEPS steps, when a task is free */
static void e103_spark_create(NJS_POINT3 *target)
{
    struct task *task;
    if ((task = task_create(TASK_WORK, 4, e103_spark_step)) != 0) {
        struct task_work *work = task->work;
        TASK_TIMER(work) = SPARK_STEPS;
        work->vars.p[2] = target;
        sound_play(SE_E103_SPARKS, 0, 0, 0);
    }
}
