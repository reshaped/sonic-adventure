/* E-101, the robot E-102 fights in B_E101 */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>
#include <shinobi.h>
#include <stdlib.h>

#include "b_e101/boss.h"
#include "b_e101/camera.h"
#include "b_e101/missile.h"
#include "camera/camera.h"
#include "collision/collision.h"
#include "collision/ground.h"
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
#include "player/e102_effects.h"
#include "player/e102_hit.h"
#include "player/lockon.h"
#include "player/player.h"
#include "player/textures.h"
#include "sound/sound.h"
#include "sound/stream.h"
#include "stage/stage.h"
#include "stg05/hover_dust.h"
#include "stg05/walls.h"
#include "system/frame.h"
#include "system/pad.h"
#include "system/task.h"
#include "system/util.h"

enum boss_mode {
    BOSS_INTRO,
    BOSS_WAIT,
    BOSS_JUMP,
    BOSS_FLY,
    BOSS_LAND,
    BOSS_HURT,
    BOSS_SHOOT,
    BOSS_DEFEAT
};

/* the entries of actions, by the actions they play */
enum boss_action {
    ACTION_STAND,
    ACTION_JUMP,
    ACTION_FALL,
    ACTION_LAND,
    ACTION_FLY,
    ACTION_SHOOT,
    ACTION_6,
    ACTION_HURT,
    ACTION_DEFEAT
};

/* the submode that ends a mode. a mode's step sets it once the mode is
 * done. a new mode, or the boss's destruction, can cut a mode short. the
 * functions step and destroy then set it, and run the mode's step once
 * more. a mode's step that ends with it set runs the mode's ending, if the
 * mode has one */
#define BOSS_DONE (-1)

/* change to mode next after this step, unless it is the mode now.
 * previous_mode keeps the mode it leaves. a later request in the same step
 * replaces an earlier one */
#define SET_MODE(task, next)                                                   \
    do {                                                                       \
        if ((task)->work->mode != (next)) {                                    \
            ((struct boss *)(task)->extra)->next_mode = (next);                \
            ((struct boss *)(task)->extra)->previous_mode =                    \
                (int)(task)->work->mode;                                       \
        }                                                                      \
    } while (0)

/* the bits of its flags */
#define BOSS_FLAME 0x1 /* its jets burn */
#define BOSS_AIMING 0x2 /* its head turns toward player 0 */
#define BOSS_GRAVITY 0x4 /* gravity pulls it */
#define BOSS_GUARDED 0x8 /* a hit does it no harm */
#define BOSS_SMOKE 0x10 /* it trails smoke */
#define BOSS_CRACKLE 0x20 /* lightning crackles about its core */
#define BOSS_GROUNDED 0x40 /* it stands on the floor */

/* the shapes' guard while E-102 can lock on to the boss. E-102's aim passes
 * over a target whose first shape has the full guard, SHAPE_GUARD */
#define LOCKABLE_GUARD GUARD(2)

#define SE_E101_HIT 155
#define SE_E101_LAUNCH 153
#define SE_E101_CRACKLE 156
#define SE_E101_THRUST 152
#define SE_E101_JUMP 783
#define SE_E101_LAND 785

/* hits it takes */
#define BOSS_HEALTH 3
/* knocked back by a hit, it takes this share of the shot's speed */
#define KNOCK_BACK 0.3F
/* hurt, it smokes at SMOKE_HEALTH or less, and crackles at CRACKLE_HEALTH
 * or less */
#define SMOKE_HEALTH 1.0F
#define CRACKLE_HEALTH 1.0F

/* its center's height above the floor it stands on */
#define CENTER_HEIGHT 14.0F
/* move holds the floor it finds below the boss to this range */
#define FLOOR_LOW 0.0F
#define FLOOR_HIGH 40.0F
/* waiting at this height above rest_y or more, it comes down */
#define LAND_HEIGHT 10.0F
/* how far from a wall e100_push_from_walls keeps it */
#define WALL_MARGIN 5.0F
/* player 0 at this height or below falls out of the stage */
#define FALL_Y (-10.0F)

/* player 0 this near in x and z makes it jump as it waits */
#define JUMP_REACH 60.0F
/* player 0 this near in x and z picks the mode after a hurt or a shot */
#define NEAR_REACH 120.0F
/* in flight, it comes down this near its target in x and z */
#define TARGET_REACH 20.0F

/* it fires only while fewer of its missiles than this fly */
#define MISSILE_LIMIT 1
/* the steps BOSS_WAIT waits before it fires or jumps. with no missile
 * flying, it waits at most QUICK_WAIT */
#define STAND_WAIT 120
#define QUICK_WAIT 10
/* the frame of ACTION_SHOOT at which it fires */
#define FIRE_FRAME 35.0F

#define FALL_ACCEL 0.05F /* the speed gravity adds downward each step */
#define DRAG 0.95F /* the share of its speeds in x and z it keeps each step */
/* flying, it keeps LIFT_DRAG of its speed upward each step, and loses any
 * speed downward. as a flight ends, it keeps LANDING_DRAG of its speeds in x
 * and z */
#define LIFT_DRAG 0.95F
#define LANDING_DRAG 0.6F
#define STOP_SPEED 0.05F /* under this either way, a speed in x or z stops */
#define JUMP_SPEED 2.0F /* the speed upward a jump gives it */
/* flying, it tilts forward by TILT_STEP a step up to FLY_TILT. it gains
 * THRUST a step the way it faces */
#define FLY_TILT NJM_DEG_ANG(40)
#define TILT_STEP NJM_DEG_ANG(3)
#define THRUST 0.15F
/* each step, the tilt's spring adds at most TILT_PULL to the tilt's speed
 * either way. it also takes back TILT_DAMPING of the speed */
#define TILT_PULL NJM_DEG_ANG(1)
#define TILT_DAMPING 0.2F
/* under this either way, a tilt and its speed stop */
#define TILT_SETTLED NJM_DEG_ANG(0.5)
/* the most it turns each step. as it flies it turns toward its target, and
 * as it comes down or shoots, toward player 0 */
#define FLY_TURN NJM_DEG_ANG(5)
#define LAND_TURN NJM_DEG_ANG(6.5)
#define SHOOT_TURN NJM_DEG_ANG(4)
/* the most its head turns a step */
#define AIM_TURN NJM_DEG_ANG(2.5)
/* the turn of the bob's phase each step */
#define BOB_STEP NJM_DEG_ANG(18.75)

/* with BOSS_CRACKLE set, it crackles at even odds every CRACKLE_INTERVAL
 * steps. a crackle lasts CRACKLE_STEPS */
#define CRACKLE_INTERVAL 90
#define CRACKLE_STEPS 60
/* the point a crackle's lightning flashes about: the boss's core, which
 * must outlive the crackle */
#define CRACKLE_TARGET(work) ((NJS_POINT3 *)(work)->vars.p[2])

/* the steps of defeat's waits: on the floor before it collapses, then
 * before the stage clears */
#define SLUMP_STEPS 120
#define CLEAR_STEPS 200

/* every action of actions has BOSS_NODES nodes. draw poses only nodes 0 to
 * MUZZLE_0_NODE, and the nodes whose matrices it reads are all among them.
 * its jets draw at CORE_NODE */
#define BOSS_NODES 77
#define MUZZLE_0_NODE 45
#define MUZZLE_1_NODE 36
#define CORE_NODE 30
#define SMOKE_0_NODE 40
#define SMOKE_1_NODE 31
/* the first of the four textures of E-102's flame in e102effect_texlist.
 * the jets' flame shows each in turn for 2 steps */
#define FLAME_TEXTURE 8

/* E-101's record, at task->extra */
struct boss {
    struct motion motion;
    Float health; /* hits left */
    /* the steps since it was made. the defeat mode sets it back to 0 to
     * count its own waits */
    int steps;
    int countdown; /* steps BOSS_WAIT waits before it fires or jumps */
    Uint32 flags;
    int next_mode; /* the mode the boss takes as this step ends */
    int previous_mode; /* the mode it last left, which nothing reads */
    /* the index in flight_targets of the point it flies to */
    int target_index;
    Angle aim; /* its head's turn about y from the body's facing */
    Angle tilt_speed; /* the turn level_tilt adds to ang.x each step */
    /* the height its center rests at: the floor below it, held from
     * FLOOR_LOW to FLOOR_HIGH, plus CENTER_HEIGHT */
    Float rest_y;
    Float height; /* its center's height above rest_y, which nothing reads */
    /* the squared distance in x and z to the point steer last turned it
     * toward */
    Float target_distance_squared;
    /* its position before move last moved it, which nothing reads */
    NJS_POINT3 previous_pos;
    NJS_VECTOR velocity;
    NJS_POINT3 core; /* CORE_NODE's origin, which the crackles read */
    NJS_POINT3 smoke_pos[2]; /* where its smoke starts */
    NJS_POINT3 muzzle_pos[2]; /* where each launcher fires from */
    NJS_VECTOR muzzle_direction[2]; /* the way each launcher aims */
    Angle bob_phase;
    Float bob; /* how far its jets ride up and down */
};

extern NJS_OBJECT jet_object;
extern NJS_OBJECT head_object;

extern NJS_ACTION stand_action;
extern NJS_ACTION jump_action;
extern NJS_ACTION fall_action;
extern NJS_ACTION land_action;
extern NJS_ACTION fly_action;
extern NJS_ACTION shoot_action;
extern NJS_ACTION action_6;
extern NJS_ACTION hurt_action;
extern NJS_ACTION defeat_action;

static NJS_POINT3 start_pos = {0.0F, 10.0F, 0.0F};
/* where the camera holds as it watches player 0 and the boss */
static NJS_POINT3 camera_pos = {20.0F, 70.0F, 133.0F};
/* the points it flies to, which choose_target picks from */
static NJS_POINT3 flight_targets[3] = {
    {-43.0F, 0.5F, 27.0F}, {19.0F, 0.5F, -74.0F}, {57.0F, 0.5F, 27.0F}};
static struct collision_shape shapes[2] = {
    {SHAPE_SPHERE | SHAPE_HURTABLE | LOCKON_NO_NUMBER,
     LOCKABLE_GUARD | SHAPE_HURTS_PLAYERS | SHAPE_HURTS_ENEMIES |
         SHAPE_HURTS_OBJECTS,
     PUSH_ENEMIES | PUSH_FIRM(PUSH_OTHERS),
     {0.0F, 0.0F, 0.0F},
     6.0F,
     4.0F,
     0.65F,
     {0, 0, 0},
     0,
     NULL},
    {SHAPE_SPHERE | SHAPE_HURTABLE,
     LOCKABLE_GUARD | SHAPE_HURTS_PLAYERS | SHAPE_HURTS_ENEMIES |
         SHAPE_HURTS_OBJECTS,
     PUSH_ENEMIES | PUSH_FIRM(PUSH_OTHERS),
     {0.0F, -8.0F, 0.0F},
     4.0F,
     4.0F,
     0.65F,
     {0, 0, 0},
     0,
     NULL}};
static struct motion_entry actions[9] = {
    {&stand_action, BOSS_NODES, MOTION_LOOP, ACTION_STAND, 0.0625F, 0.5F},
    {&jump_action, BOSS_NODES, MOTION_LOOP, ACTION_JUMP, 0.25F, 1.0F},
    {&fall_action, BOSS_NODES, MOTION_LOOP, ACTION_FALL, 0.25F, 1.0F},
    {&land_action, BOSS_NODES, MOTION_ONCE, ACTION_STAND, 0.25F, 1.0F},
    {&fly_action, BOSS_NODES, MOTION_LOOP, ACTION_FLY, 0.25F, 1.0F},
    {&shoot_action, BOSS_NODES, MOTION_ONCE, ACTION_SHOOT, 0.25F, 1.0F},
    {&action_6, BOSS_NODES, MOTION_ONCE, ACTION_6, 0.25F, 1.0F},
    {&hurt_action, BOSS_NODES, MOTION_ONCE, ACTION_HURT, 0.5F, 1.0F},
    {&defeat_action, BOSS_NODES, MOTION_HOLD, ACTION_DEFEAT, 0.5F, 0.5F}};
/* the params of the smoke it trails from each smoke point, which every puff
 * keeps for its life. smoke sets pos before each puff, which copies it */
static struct dust_params smoke_params = {0.5F,
                                          0.03F,
                                          0.19F,
                                          0.05F,
                                          0.025F,
                                          {0.0F, 0.0F, 0.0F},
                                          {0.0F, 0.0F, 0.0F},
                                          {1.0F, 0.28F, 0.28F, 0.35F}};

static const char boss_title[6] = {
#include "b_e101/boss/boss_title.inc"
};
static const NJS_POINT3 attachment_origin = {0.0F, 0.0F, 0.0F};

/* the record draw draws, which its motion callback, turn_head, reads */
static struct boss *drawn_boss;

static void destroy(struct task *task);
static void run(struct task *task);
static void step(struct task *task);
static void draw(struct task *task);
static void turn_head(NJS_OBJECT *object);
static void crackle_create(NJS_POINT3 *target);

void e101_create(void)
{
    struct task *task;
    struct task_work *work;
    struct boss *extra;

    if ((task = task_create(TASK_WORK, 2, step)) != 0) {
        work = task->work;
        collision_init(task, shapes, COLLISION_ENEMY, 1, 2);
        enemy_alloc_record(task, work);
        extra = (struct boss *)motion_create(work, sizeof(struct boss));
        extra->motion.wanted = ACTION_STAND;
        extra->motion.current = ACTION_STAND;
        extra->motion.actions = actions;
        task->extra = extra;
        motion_step(work, &extra->motion);
        work->ang.z = 0;
        work->ang.x = 0;
        extra->health = BOSS_HEALTH;
        life_gauge_health = extra->health;
        extra->previous_mode = BOSS_INTRO;
        extra->next_mode = BOSS_INTRO;
        extra->target_index = 1;
        extra->velocity.x = extra->velocity.y = extra->velocity.z = 0.0F;
        task->draw = draw;
        task->destructor = destroy;
    }
}

/* end the mode it is in, then free the motion's blend */
static void destroy(struct task *task)
{
    struct task_work *work = task->work;
    struct boss *extra = task->extra;

    if (work->submode != BOSS_DONE) {
        work->submode = BOSS_DONE;
        run(task);
    }
    extra->motion.state = MOTION_FREE;
    motion_step(work, &extra->motion);
}

/* while guarded, the boss lets its lock-on go each step, and a hit does
 * nothing. unguarded, a hit costs it one health, unless it is hurt already.
 * at no health it is defeated. with health left, it is hurt, and knocked
 * back by KNOCK_BACK of the shot's speed */
static void take_hit(struct task *task)
{
    struct lockon_target *record;
    struct task_work *work;
    struct boss *extra;
    NJS_POINT3 position;
    NJS_VECTOR velocity;
    struct task *shot;

    work = task->work;
    record = LOCKON_TARGET(task);
    extra = task->extra;
    if (extra->flags & BOSS_GUARDED) {
        lockon_release_hit(task);
        return;
    }
    if (!(work->flags & COLLISION_DAMAGED)) {
        return;
    }
    if (work->mode == BOSS_HURT) {
        return;
    }

    /* @hack spelling: position through its address, which copies R15 to R3 and
     * adds 16 for each store, as at 0x0c9002ae; without it, 147 of 190 words
     * differ */
    /* @hack spelling: velocity through its address, which copies R15 to R3 and
     * adds 4 for each store, as at 0x0c9002d6; without it, 176 of 190 words
     * differ */
    if ((shot = lockon_get_shot(record->lockon)) != NULL) {
        (&position)->x = shot->work->pos.x;
        (&position)->y = shot->work->pos.y;
        (&position)->z = shot->work->pos.z;
        (&velocity)->x = shot->motion->speed.x;
        (&velocity)->y = shot->motion->speed.y;
        (&velocity)->z = shot->motion->speed.z;
    } else {
        (&position)->x = work->pos.x;
        (&position)->y = work->pos.y;
        (&position)->z = work->pos.z;
        (&velocity)->x = (&velocity)->y = (&velocity)->z = 0.0F;
    }
    extra->flags |= BOSS_GUARDED;
    sound_play(SE_E101_HIT, 0, 0, 0);
    e102_hit_create(&position, -15.0F);
    lockon_release(task);
    extra->health -= 1.0F;

    if (!(extra->health > 0.0F)) {
        SET_MODE(task, BOSS_DEFEAT);
    } else {
        SET_MODE(task, BOSS_HURT);
        extra->velocity.x = (&velocity)->x * KNOCK_BACK;
        extra->velocity.y = (&velocity)->y * KNOCK_BACK;
        extra->velocity.z = (&velocity)->z * KNOCK_BACK;
    }
}

/* guard the boss: its first shape takes the full guard, SHAPE_GUARD, which
 * no attack exceeds. unguard lowers it back to LOCKABLE_GUARD */
static void guard(struct task_work *work, struct boss *extra)
{
    extra->flags |= BOSS_GUARDED;
    work->collision->shapes->levels =
        (work->collision->shapes->levels & ~SHAPE_GUARD) | SHAPE_GUARD;
}

static void unguard(struct task_work *work, struct boss *extra)
{
    extra->flags &= ~BOSS_GUARDED;
    work->collision->shapes->levels =
        (work->collision->shapes->levels & ~SHAPE_GUARD) | LOCKABLE_GUARD;
}

static void bob_step(struct task *task)
{
    struct boss *extra = task->extra;
    extra->bob_phase += BOB_STEP;
    /* @hack redundant: the sine times 1, FLDI1 and FMUL at 0x0c90046a and
     * 0x0c90046c; without it, the function is 4 bytes shorter and 7 of 18
     * words differ */
    extra->bob = njSin(extra->bob_phase) * 1.0F;
}

/* TRUE while player 0 lies within the root of range_squared of the boss, in
 * x and z. FALSE otherwise */
static Bool player_nearby(struct task *task, Float range_squared)
{
    struct task_work *work = task->work;
    struct task_work *player = player_work[0];
    Float x = work->pos.x - player->pos.x;
    Float z = work->pos.z - player->pos.z;
    Float distance_squared = (x * x) + (z * z);
    return distance_squared < range_squared;
}

/* turn the boss about y toward target by at most limit.
 * target_distance_squared takes the squared distance in x and z to target */
static void steer(struct task *task, NJS_POINT3 *target, Angle limit)
{
    struct task_work *work = task->work;
    struct boss *extra = task->extra;
    Float x = work->pos.x - target->x;
    Float z = work->pos.z - target->z;
    /* where x and z are both 0, atan2f divides 0 by 0. the turn is then
     * toward whatever angle the NaN converts to */
    Angle turn = -(Angle)(atan2f(z, x) * ANGLE_PER_RADIAN);
    turn = angle_difference(work->ang.y, turn);
    work->ang.y += CLAMP(turn, -limit, limit);
    extra->target_distance_squared = (x * x) + (z * z);
}

/* add speed along -x, turned by angle about y, to velocity's x and z */
static void accelerate(NJS_POINT3 *velocity, Angle angle, Float speed)
{
    NJS_POINT3 direction;
    NJS_POINT3 movement;
    NJS_MATRIX matrix;
    direction.x = -speed;
    direction.y = direction.z = 0.0F;
    njUnitMatrix(&matrix);
    TURN_MATRIX(njRotateY, &matrix, angle);
    njCalcPoint(&matrix, &direction, &movement);
    velocity->x += movement.x;
    velocity->z += movement.z;
}

/* move target_index on to one of the other two points, at random, and
 * return it */
static int choose_target(struct task *task)
{
    struct boss *extra = task->extra;
    int index = extra->target_index + (rand() & 1) + 1;
    if (index >= 3) {
        index -= 3;
    }
    extra->target_index = index;
    return index;
}

static void fire(struct task *task, int index)
{
    struct boss *extra = task->extra;
    e101_missile_create(&extra->muzzle_pos[index],
                        &extra->muzzle_direction[index], task->work->collision);
    sound_play(SE_E101_LAUNCH, 0, 0, 0);
}

static void smoke(struct task *task)
{
    int i = 0;
    struct boss *extra = task->extra;
    do {
        smoke_params.pos.x = extra->smoke_pos[i].x;
        smoke_params.pos.y = extra->smoke_pos[i].y;
        smoke_params.pos.z = extra->smoke_pos[i].z;
        dust_create_from(&smoke_params);
        /* @hack spelling: i = i + 1, which keeps i in R13 and extra in R12,
         * set at 0x0c900648 and 0x0c900658; without it, i and extra trade
         * registers, and 10 of 49 words differ */
        i = i + 1;
    } while (i < 2);
}

static void crackle(struct task *task)
{
    struct boss *extra = task->extra;
    if (extra->steps % CRACKLE_INTERVAL == 0) {
        if (rand() * RAND_SCALE < 0.5F) {
            crackle_create(&extra->core);
        }
    }
}

/* the boss stands at start_pos at full health, guarded, with the time and
 * the pads stopped. the camera swings round it while its name shows. once
 * the swing is done, the time and the pads start again. the gauge shows,
 * and the camera holds at camera_pos. the boss then drops its guard and
 * opens the fight in BOSS_SHOOT */
static void introduce(struct task *task)
{
    struct task_work *work = task->work;
    struct boss *extra = task->extra;

    time_stop();
    pad_disable();
    switch (work->submode) {
    case 0:
        extra->health = BOSS_HEALTH;
        /* @hack spelling: start_pos through its address, which SHC loads into
         * R3 and copies to R2 and R1 for the reads of y and z, at 0x0c900788
         * and 0x0c90078a; without it, 59 of 131 words differ */
        work->pos.x = (&start_pos)->x;
        work->pos.y = (&start_pos)->y;
        work->pos.z = (&start_pos)->z;
        extra->motion.wanted = ACTION_STAND;
        extra->flags &= ~(BOSS_SMOKE | BOSS_CRACKLE);
        extra->flags |= BOSS_GRAVITY;
        guard(work, extra);
        e101_missile_count = 0;
        work->submode = 1;
        break;
    case 1:
        camera_start_swing(work, 0.0F, 30.0F, 240);
        camera_override_mode(CAMERA_MODE_CHAOS_STINIT, CAMERA_ADJUST_NONE);

        boss_name_create(boss_title, -1, 240, 120);
        work->submode = 2;
        stream_request_music(MUSIC_BOSSTRGT);
        break;
    case 2:
        if (camera_swing_done) {
            time_start();
            pad_enable();
            life_gauge_create(0, 0, BOSS_HEALTH);
            unguard(work, extra);
            e101_camera_hold_start(&work->pos, &camera_pos);
            SET_MODE(task, BOSS_SHOOT);
            work->submode = BOSS_DONE;
        }
        break;
    }
    /* @hack redundant: a test of the submode at the end, which loads it at
     * 0x0c90088e and compares it at 0x0c900892 to no effect; without it, 61
     * of 131 words differ, and the object's code is 32 bytes longer */
    if (work->submode == BOSS_DONE) {
        return;
    }
}

/* spring *angle back to 0. each step, *speed gains the short turn from
 * *angle to 0, held to TILT_PULL either way. *speed also loses
 * TILT_DAMPING of itself. *angle then turns by *speed. once each is under
 * TILT_SETTLED either way, both stop at 0 */
static void level_tilt(Angle *angle, Angle *speed)
{
    Angle turn = angle_difference(*angle, 0);
    turn = CLAMP(turn, -TILT_PULL, TILT_PULL);
    turn -= *speed * TILT_DAMPING;
    *speed += turn;
    *angle += *speed;
    if (_builtin_fabsf((Float)*speed) < TILT_SETTLED &&
        _builtin_fabsf((Float)*angle) < TILT_SETTLED) {
        *speed = 0;
        *angle = 0;
    }
}

/* push the boss out of walls, bouncing it off them, and find rest_y. then
 * move it by its velocity, no lower than rest_y, and slow it in x and z.
 * the step it reaches rest_y, it plays SE_E101_LAND. its head turns toward
 * player 0 while BOSS_AIMING, and otherwise back to the body's facing. its
 * tilt springs back to level */
static void move(struct task *task)
{
    struct task_work *work = task->work;
    struct boss *extra = task->extra;
    NJS_VECTOR correction;
    Angle target;
    Angle turn;

    extra->previous_pos = work->pos;
    /* @hack spelling: the wall test's result compared with TRUE, which SHC
     * tests with CMP/EQ #1 (0x0c900968). e100_push_from_walls returns only TRUE
     * or FALSE, so a plain test of it is the same; as one, 2 of 246 words
     * differ */
    if (e100_push_from_walls(&work->pos, WALL_MARGIN, &extra->rest_y,
                             &correction) == TRUE) {
        /* @hack spelling: correction through its address, which copies R15 to
         * R3 for each read, as at 0x0c90096e and 0x0c900974; without it, 39 of
         * 246 words differ */
        work->pos.x = work->pos.x + (&correction)->x;
        work->pos.y = work->pos.y + (&correction)->y;
        work->pos.z = work->pos.z + (&correction)->z;
        extra->velocity.x = -extra->velocity.x;
        extra->velocity.z = -extra->velocity.z;
    }
    extra->rest_y = CLAMP(extra->rest_y, FLOOR_LOW, FLOOR_HIGH);
    extra->rest_y += CENTER_HEIGHT;
    extra->height = work->pos.y - extra->rest_y;
    if (extra->flags & BOSS_GRAVITY) {
        extra->velocity.y -= FALL_ACCEL;
    }
    work->pos.y += extra->velocity.y;
    if (!(work->pos.y > extra->rest_y)) {
        work->pos.y = extra->rest_y;
        extra->velocity.y = 0.0F;
        if (!(extra->flags & BOSS_GROUNDED)) {
            extra->flags |= BOSS_GROUNDED;
            sound_play_at(SE_E101_LAND, 0, 0, 0, work->pos.x, work->pos.y,
                          work->pos.z);
        }
    } else {
        extra->flags &= ~BOSS_GROUNDED;
    }
    work->pos.x += extra->velocity.x;
    work->pos.z += extra->velocity.z;
    extra->velocity.x *= DRAG;
    extra->velocity.z *= DRAG;
    if (extra->velocity.x < STOP_SPEED && extra->velocity.x > -STOP_SPEED) {
        extra->velocity.x = 0.0F;
    }
    if (extra->velocity.z < STOP_SPEED && extra->velocity.z > -STOP_SPEED) {
        extra->velocity.z = 0.0F;
    }
    if (extra->flags & BOSS_AIMING) {
        Float x = work->pos.x - player_work[0]->pos.x;
        Float z = work->pos.z - player_work[0]->pos.z;
        /* here too, x and z both 0 make atan2f's NaN. the head then turns
         * toward whatever angle the NaN converts to */
        target = -work->ang.y - (Angle)(atan2f(z, x) * ANGLE_PER_RADIAN);
    } else {
        target = 0;
    }
    turn = angle_difference(extra->aim, target);
    extra->aim += CLAMP(turn, -AIM_TURN, AIM_TURN);
    level_tilt(&work->ang.x, &extra->tilt_speed);
}

/* the boss stands, aiming at player 0. once its countdown runs out, it
 * fires while fewer than MISSILE_LIMIT missiles fly, and otherwise jumps.
 * player 0 within JUMP_REACH makes it jump. at LAND_HEIGHT or more above
 * rest_y, it comes down. while player 1's input holds A, the fight starts
 * over */
static void wait(struct task *task)
{
    struct task_work *work = task->work;
    struct boss *extra = task->extra;
    switch (work->submode) {
    case 0:
        e101_camera_hold_start(&work->pos, &camera_pos);
        extra->motion.wanted = ACTION_STAND;
        extra->flags |= BOSS_AIMING;
        work->submode = 1;
        break;
    case 1:
        if (pad_records[1].on & PDD_DGT_TA) {
            SET_MODE(task, BOSS_INTRO);
            return;
        }
        if (e101_missile_count == 0 && extra->countdown > QUICK_WAIT) {
            extra->countdown = QUICK_WAIT;
        } else if (--extra->countdown < 0) {
            if (e101_missile_count < MISSILE_LIMIT) {
                SET_MODE(task, BOSS_SHOOT);
            } else {
                SET_MODE(task, BOSS_JUMP);
            }
            extra->countdown = STAND_WAIT;
        }
        if (player_nearby(task, SQUARE(JUMP_REACH))) {
            SET_MODE(task, BOSS_JUMP);
        }
        if (!(work->pos.y - extra->rest_y < LAND_HEIGHT)) {
            SET_MODE(task, BOSS_LAND);
        }
        break;
    }
}

/* once ACTION_JUMP plays, the boss picks another target and jumps. it flies
 * once it stops rising */
static void jump(struct task *task)
{
    struct task_work *work = task->work;
    struct boss *extra = task->extra;
    switch (work->submode) {
    case 0:
        extra->motion.wanted = ACTION_JUMP;
        extra->flags &= ~BOSS_AIMING;
        work->submode = 1;
        break;
    case 1:
        motion_flags &= ~MOTION_HOLDING;
        if (extra->motion.current == ACTION_JUMP) {
            extra->tilt_speed = 0;
            work->ang.x = 0;
            work->submode = 2;
        }
        break;
    case 2:
        choose_target(task);
        extra->velocity.y += JUMP_SPEED;
        sound_play_at(SE_E101_JUMP, 0, 0, 0, work->pos.x, work->pos.y,
                      work->pos.z);
        work->submode = 3;
        break;
    case 3:
        if (!(extra->velocity.y > 0.0F)) {
            SET_MODE(task, BOSS_FLY);
            work->submode = BOSS_DONE;
        }
        break;
    }
    /* @hack redundant: a test of the submode at the end, which loads it at
     * 0x0c900dda and compares it at 0x0c900dde to no effect; without it, 5
     * of 91 words differ */
    if (work->submode == BOSS_DONE) {
        return;
    }
}

/* guarded, on its jets, the boss tilts forward and flies toward its
 * target. it comes down within TARGET_REACH of the target */
static void fly(struct task *task)
{
    struct task_work *work = task->work;
    struct boss *extra = task->extra;
    e100_hover_dust(work, extra->rest_y - CENTER_HEIGHT);
    switch (work->submode) {
    case 0:
        extra->motion.wanted = ACTION_FLY;
        extra->flags |= BOSS_FLAME;
        extra->flags &= ~BOSS_AIMING;
        guard(work, extra);
        work->submode = 1;
        break;
    case 1:
        sound_play_at_for(SE_E101_THRUST, (int)task, 1, 0, work->pos.x,
                          work->pos.y, work->pos.z, 4);
        steer(task, &flight_targets[extra->target_index], FLY_TURN);
        extra->tilt_speed = 0;
        if (work->ang.x < FLY_TILT) {
            work->ang.x += TILT_STEP;
        }
        if (!(extra->velocity.y < 0.0F)) {
            extra->velocity.y *= LIFT_DRAG;
        } else {
            extra->velocity.y = 0.0F;
        }
        accelerate(&extra->velocity, work->ang.y, THRUST);
        if (!(extra->target_distance_squared > SQUARE(TARGET_REACH))) {
            SET_MODE(task, BOSS_LAND);
            work->submode = BOSS_DONE;
        }
        break;
    }
    if (work->submode == BOSS_DONE) {
        extra->velocity.x *= LANDING_DRAG;
        extra->velocity.z *= LANDING_DRAG;
        extra->flags &= ~BOSS_FLAME;
        unguard(work, extra);
    }
}

/* the boss comes down, turning toward player 0, lands, then fires */
static void land(struct task *task)
{
    struct task_work *work = task->work;
    struct boss *extra = task->extra;
    e100_hover_dust(work, extra->rest_y - CENTER_HEIGHT);
    switch (work->submode) {
    case 0:
        extra->motion.wanted = ACTION_FALL;
        work->submode = 1;
        break;
    case 1:
        steer(task, &player_work[0]->pos, LAND_TURN);
        if (extra->rest_y == work->pos.y) {
            extra->motion.wanted = ACTION_LAND;
            work->submode = 2;
        }
        break;
    case 2:
        motion_flags &= ~MOTION_HOLDING;
        if (extra->motion.current == ACTION_LAND) {
            motion_flags &= ~MOTION_ENDED;
            work->submode = 3;
        }
        break;
    case 3:
        if (motion_flags & MOTION_ENDED) {
            SET_MODE(task, BOSS_SHOOT);
            work->submode = BOSS_DONE;
        }
        break;
    }
}

/* guarded, the boss reels. at SMOKE_HEALTH or less it smokes, and at
 * CRACKLE_HEALTH or less it crackles. as the reel ends, it waits while
 * player 0 is within NEAR_REACH, and otherwise fires */
static void hurt(struct task *task)
{
    struct task_work *work = task->work;
    struct boss *extra = task->extra;

    switch (work->submode) {
    case 0:
        extra->motion.wanted = ACTION_HURT;
        extra->flags &= ~BOSS_AIMING;
        guard(work, extra);
        if (!(extra->health > SMOKE_HEALTH)) {
            extra->flags |= BOSS_SMOKE;
        }
        if (!(extra->health > CRACKLE_HEALTH)) {
            extra->flags |= BOSS_CRACKLE;
        }
        work->submode = 1;
        break;
    case 1:
        motion_flags &= ~MOTION_HOLDING;
        if (extra->motion.current == ACTION_HURT) {
            motion_flags &= ~MOTION_ENDED;
            work->submode = 2;
        }
        break;
    case 2:
        if (motion_flags & MOTION_ENDED) {
            if (player_nearby(task, SQUARE(NEAR_REACH))) {
                SET_MODE(task, BOSS_WAIT);
            } else {
                SET_MODE(task, BOSS_SHOOT);
            }
            work->submode = BOSS_DONE;
        }
        break;
    }
    if (work->submode == BOSS_DONE) {
        unguard(work, extra);
    }
}

/* the boss stops and turns toward player 0, and fires from its second
 * launcher at FIRE_FRAME. with MISSILE_LIMIT missiles flying already, it
 * waits instead. it then jumps while player 0 is within NEAR_REACH. farther
 * off, it jumps or waits at even odds */
static void shoot(struct task *task)
{
    struct task_work *work = task->work;
    struct boss *extra = task->extra;
    switch (work->submode) {
    case 0:
        if (e101_missile_count >= MISSILE_LIMIT) {
            SET_MODE(task, BOSS_WAIT);
            return;
        }
        extra->velocity.x = extra->velocity.y = extra->velocity.z = 0.0F;
        extra->motion.wanted = ACTION_SHOOT;
        work->submode = 1;
        break;
    case 1:
        motion_flags &= ~MOTION_HOLDING;
        if (extra->motion.current == ACTION_SHOOT) {
            work->submode = 2;
        }
        break;
    case 2:
        steer(task, &player_work[0]->pos, SHOOT_TURN);
        motion_flags &= ~MOTION_HOLDING;
        if (!(extra->motion.frame < FIRE_FRAME)) {
            fire(task, 1);
            motion_flags &= ~MOTION_ENDED;
            work->submode = 3;
        }
        break;
    case 3:
        if (motion_flags & MOTION_ENDED) {
            if (player_nearby(task, SQUARE(NEAR_REACH))) {
                SET_MODE(task, BOSS_JUMP);
            } else if (rand() < RAND_MAX / 2) {
                SET_MODE(task, BOSS_JUMP);
            } else {
                SET_MODE(task, BOSS_WAIT);
                extra->countdown = STAND_WAIT;
            }
            work->submode = BOSS_DONE;
        }
        break;
    }
    if (work->submode == BOSS_DONE) {
        extra->countdown = STAND_WAIT;
    }
}

/* every missile bursts, and the boss stands, smoking and crackling, with
 * the time and the pads stopped. SLUMP_STEPS after it lands, it collapses,
 * and CLEAR_STEPS after that the stage clears */
static void defeat(struct task *task)
{
    struct task_work *work = task->work;
    struct boss *extra = task->extra;
    switch (work->submode) {
    case 0:
        extra->motion.wanted = ACTION_STAND;
        extra->flags &= ~BOSS_AIMING;
        extra->flags |= (BOSS_SMOKE | BOSS_CRACKLE);
        e101_missile_count = -1;
        time_stop();
        pad_disable();
        work->submode = 1;
        break;
    case 1:
        motion_flags &= ~MOTION_HOLDING;
        if (extra->motion.current == ACTION_STAND) {
            motion_flags &= ~MOTION_ENDED;
            extra->steps = 0;
            work->submode = 2;
        }
        break;
    case 2:
        if (!(extra->flags & BOSS_GROUNDED)) {
            extra->steps = 0;
        }
        if (extra->steps >= SLUMP_STEPS) {
            extra->motion.wanted = ACTION_DEFEAT;
            work->submode = 3;
        }
        break;
    case 3:
        if (extra->motion.current == ACTION_DEFEAT) {
            motion_flags &= ~MOTION_ENDED;
            work->submode = 4;
        }
        break;
    case 4:
        extra->steps = 0;
        work->submode = 5;
        /* fall through */
    case 5:
        if (extra->steps >= CLEAR_STEPS) {
            work->submode = 6;
        }
        break;
    case 6:
        stage_clear();
        work->submode = BOSS_DONE;
        break;
    }
    if (work->submode == BOSS_DONE) {
        extra->flags &= ~(BOSS_SMOKE | BOSS_CRACKLE);
        pad_enable();
    }
}

/* run move, then the mode's step. the intro skips move at submode 0 */
static void run(struct task *task)
{
    struct task_work *work = task->work;
    switch (work->mode) {
    case BOSS_INTRO:
        if (work->submode) {
            move(task);
        }
        introduce(task);
        break;
    case BOSS_WAIT:
        move(task);
        wait(task);
        break;
    case BOSS_JUMP:
        move(task);
        jump(task);
        break;
    case BOSS_FLY:
        move(task);
        fly(task);
        break;
    case BOSS_LAND:
        move(task);
        land(task);
        break;
    case BOSS_HURT:
        move(task);
        hurt(task);
        break;
    case BOSS_SHOOT:
        move(task);
        shoot(task);
        break;
    case BOSS_DEFEAT:
        move(task);
        defeat(task);
        break;
    }
}

/* E-101's step. a mode asked for in this step starts once the mode it
 * leaves has run its ending. player 0 at FALL_Y or below falls out of the
 * stage */
static void step(struct task *task)
{
    struct task_work *work = task->work;
    struct boss *extra = task->extra;
    /* @hack spelling: player 0's pos through its address in a local, which SHC
     * forms by ADD #32 at 0x0c901554, keeping work in R13 and extra in R12
     * (0x0c901556, 0x0c90155e); without it, work and extra trade registers,
     * and 71 of 75 words differ */
    NJS_POINT3 *player_pos = &player_work[0]->pos;

    if (!(player_pos->y > FALL_Y)) {
        player_fall(0);
    }
    ++extra->steps;
    bob_step(task);
    if (extra->flags & BOSS_SMOKE) {
        smoke(task);
    }
    if (extra->flags & BOSS_CRACKLE) {
        crackle(task);
    }
    run(task);
    take_hit(task);
    if (work->mode != extra->next_mode) {
        if (work->submode != BOSS_DONE) {
            work->submode = BOSS_DONE;
            run(task);
        }
        work->mode = extra->next_mode;
        work->submode = 0;
    }
    life_gauge_health = extra->health;
    motion_step(work, &extra->motion);
    collision_register(work);
    draw(task);
}

/* pose the boss to find where its launchers fire from and aim, its smoke
 * points and its core. draw its shadow, and on a frame's first step the
 * boss itself with its jets. their flame draws while BOSS_FLAME */
static void draw(struct task *task)
{
    struct task_work *work = task->work;
    struct boss *extra = task->extra;
    NJS_ACTION *action;
    NJS_MATRIX core;
    NJS_MATRIX muzzle[2];
    NJS_MATRIX smoke[2];
    NJS_POINT3 point;
    Angle angle;

    action = extra->motion.state != MOTION_BLENDING
                 ? extra->motion.actions[extra->motion.current].action
                 : extra->motion.blend;
    drawn_boss = extra;
    pose_action(&_unit_matrix_, action, extra->motion.frame, MUZZLE_0_NODE,
                &muzzle[0]);
    pose_get_matrix(MUZZLE_1_NODE, &muzzle[1]);
    pose_get_matrix(CORE_NODE, &core);
    pose_get_matrix(SMOKE_1_NODE, &smoke[1]);
    pose_get_matrix(SMOKE_0_NODE, &smoke[0]);
    shadow_draw(work, 2.0F);
    njSetTexture(&e101_texlist);
    draw_attr_save();
    draw_attr_clear(0, NJD_FLAG_IGNORE_SPECULAR);
    njPushMatrix(0);
    njTranslateV(0, &work->pos);
    TURN_VIA(njRotateX, work->ang.z, angle);
    TURN_VIA(njRotateY, work->ang.y, angle);
    TURN_VIA(njRotateZ, work->ang.x, angle);
    if (!frame_step) {
        njSetMotionCallback(turn_head);
        draw_action(action, extra->motion.frame);
        njSetMotionCallback(0);
        njPushMatrix(0);
        njMultiMatrix(0, &core);
        njRotateZ(0, 0x8000);
        njTranslate(0, 5.0F, extra->bob + 5.0F, 0.0F);
        draw_tree(&jet_object);
        if (extra->flags & BOSS_FLAME) {
            njSetTexture(&e102effect_texlist);
            e102_flame->model->mats->attr_texId =
                ((extra->steps >> 1) & 3) + FLAME_TEXTURE;
            njSimpleDrawObject(e102_flame);
        }
        njPopMatrix(1);
    }
    njPushMatrix(&_unit_matrix_);
    point = attachment_origin;
    njTranslateV(0, &work->pos);
    TURN_VIA(njRotateX, work->ang.z, angle);
    TURN_VIA(njRotateY, work->ang.y, angle);
    TURN_VIA(njRotateZ, work->ang.x, angle);
    njPushMatrix(0);
    njMultiMatrix(0, &muzzle[0]);
    point.x = 3.0F;
    njCalcPoint(0, &point, &extra->muzzle_pos[0]);
    point.x = -1.0F;
    njCalcVector(0, &point, &extra->muzzle_direction[0]);
    njPopMatrix(1);
    njPushMatrix(0);
    njMultiMatrix(0, &muzzle[1]);
    point.x = 3.0F;
    njCalcPoint(0, &point, &extra->muzzle_pos[1]);
    point.x = -1.0F;
    njCalcVector(0, &point, &extra->muzzle_direction[1]);
    njPopMatrix(1);
    point.x = 0.0F;
    njPushMatrix(0);
    njMultiMatrix(0, &smoke[1]);
    njCalcPoint(0, &point, &extra->smoke_pos[1]);
    njPopMatrix(1);
    njPushMatrix(0);
    njMultiMatrix(0, &smoke[0]);
    njCalcPoint(0, &point, &extra->smoke_pos[0]);
    njPopMatrix(1);
    njMultiMatrix(0, &core);
    njCalcPoint(0, &point, &extra->core);
    njPopMatrix(2);
    draw_attr_restore();
}

/* the motion callback of draw: turn the head by drawn_boss's aim */
static void turn_head(NJS_OBJECT *object)
{
    Angle aim;
    if (object == &head_object) {
        TURN_VIA(njRotateY, drawn_boss->aim, aim);
    }
}

/* until its steps run out, a crackle flashes lightning at even odds on
 * each frame's first step. each flash lies within 5 of the target on each
 * axis */
static void crackle_step(struct task *task)
{
    struct task_work *work;
    NJS_POINT3 pos;

    work = task->work;
    if (--TASK_TIMER(work) <= 0) {
        task_kill(task);
        return;
    }
    if (frame_step == 0 && (rand() * RAND_SCALE) < 0.5F) {
        VECTOR_COPY(&pos, CRACKLE_TARGET(work));
        pos.x += ((rand() * RAND_SCALE) * 10.0F) - 5.0F;
        pos.y += ((rand() * RAND_SCALE) * 10.0F) - 5.0F;
        pos.z += ((rand() * RAND_SCALE) * 10.0F) - 5.0F;
        lightning_create(&pos, 1.0F);
    }
}

/* a crackle about target that lasts CRACKLE_STEPS, when a task is free */
static void crackle_create(NJS_POINT3 *target)
{
    struct task *task;
    struct task_work *work;

    if ((task = task_create(TASK_WORK, 4, crackle_step)) != 0) {
        work = task->work;
        work->timer = CRACKLE_STEPS;
        work->vars.p[2] = target;
        sound_play(SE_E101_CRACKLE, 0, 0, 0);
    }
}
