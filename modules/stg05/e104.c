/* E-104, the object list's E E_104, which E-102 fights on Red Mountain */
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
#include "stg05/camera_sweep.h"
#include "stg05/e104.h"
#include "stg05/e104_actions.h"
#include "stg05/e104_textures.h"
#include "stg05/explosion.h"
#include "stg05/hover_dust.h"
#include "stg05/missile.h"
#include "stg05/walls.h"
#include "system/frame.h"
#include "system/pad.h"
#include "system/task.h"
#include "system/util.h"

#define TEXT_1_LENGTH 5
#include "stg05/e104/text.inc"

extern NJS_OBJECT e104_head_object;
extern NJS_OBJECT e104_jet_object;

/* E-104's record, at task->extra */
struct e104 {
    struct motion motion;
    Float health; /* the hits it can still take */
    /* the steps e104_run has run. e104_defeat sets it back to 0 to time its
     * phases */
    Sint32 ticks;
    Sint32 wait; /* the steps E104_STAND waits before it fires or jumps */
    Uint32 flags;
    Sint32 next_mode; /* the mode E104_SET_MODE asked for */
    Sint32 last_mode; /* the mode it was in at that request; nothing reads it */
    Sint32 target; /* which of e104_targets it flies to */
    Angle aim; /* its head's turn about y from its body */
    Angle tilt_speed; /* the turn its tilt, work->ang.x, takes each step */
    /* the height its center rests at: E104_CENTER_HEIGHT above the floor
     * below it */
    Float ground;
    Float height; /* how far its center is above ground */
    /* the square of its distance in x and z from the point e104_turn_to last
     * turned it toward */
    Float distance_squared;
    NJS_POINT3 last_pos; /* its pos before this step's move; nothing reads it */
    NJS_VECTOR speed;
    /* the point its explosion and its sparks follow, on E104_CORE_NODE */
    NJS_POINT3 core;
    NJS_POINT3 smoke[2]; /* where it smokes from */
    NJS_POINT3 launchers[2]; /* where each side's missiles start */
    NJS_VECTOR launch_dirs[2]; /* the way each side's missiles start out */
    Angle bob_phase; /* bob's phase, which gains E104_BOB_TURN a step */
    Float bob; /* how far its jets ride up from where they rest, -1 to 1 */
};

/* in its flags */
#define E104_FLYING 0x1 /* its jets burn */
#define E104_AIMING 0x2 /* its head turns toward player 0 */
#define E104_FALLS 0x4 /* gravity pulls it */
#define E104_GUARDED 0x8 /* hits do it no harm */
#define E104_SMOKES 0x10 /* it smokes, as it does at 2 health or less */
#define E104_SPARKS 0x20 /* it sparks, as it does at 1 health or less */
#define E104_GONE 0x40 /* it's no longer drawn */
#define E104_LANDED 0x80 /* it's on the floor */

enum e104_mode {
    E104_INTRO,
    E104_STAND,
    E104_JUMP,
    E104_RISE,
    E104_FLY,
    E104_LAND,
    E104_HURT,
    E104_FIRE_RIGHT,
    E104_FIRE_LEFT,
    E104_DEFEAT
};

/* its actions, as indexes into e104_actions */
enum e104_action {
    ACTION_STAND,
    ACTION_JUMP, /* E-102's jump */
    ACTION_FALL, /* E-102's fall */
    ACTION_LANDING, /* E-102's hard landing */
    ACTION_FLY,
    ACTION_FIRE_RIGHT,
    ACTION_FIRE_LEFT,
    ACTION_HURT,
    ACTION_DEFEAT
};

/* ask for next as the mode after this step. a later request replaces this
 * one, but a request for the mode it's in changes nothing */
#define E104_SET_MODE(task, next)                                              \
    do {                                                                       \
        if ((task)->work->mode != (next)) {                                    \
            ((struct e104 *)(task)->extra)->next_mode = (next);                \
            ((struct e104 *)(task)->extra)->last_mode =                        \
                (Sint32)(task)->work->mode;                                    \
        }                                                                      \
    } while (0)

/* the submode in which a mode's step ends the mode, as by dropping its
 * guard. a step can set it as it asks for the next mode. it then ends the
 * mode in that call. if the step hasn't, e104_run sets it before the change
 * and calls e104_run_mode once more. e104_destroy does the same */
#define E104_DONE (-1)

/* its center's height above the floor it stands on */
#define E104_CENTER_HEIGHT 14.0F

/* its full health: the hits that defeat it */
#define E104_HEALTH 5.0F

/* a floor at this height or lower is low, as e104_over_low_floor tests it.
 * e104_take_off compares ground with it instead, and ground is
 * E104_CENTER_HEIGHT above the floor. so to e104_take_off, a floor is low
 * only at 36 or lower */
#define E104_LOW_FLOOR 50.0F

/* the pos.y it climbs to on its jets before it flies on, whatever the
 * floor */
#define E104_CRUISE_HEIGHT 120.0F

/* a standing E-104 this high above ground or higher lands. so does a
 * flying one this high or lower */
#define E104_LANDING_HEIGHT 10.0F

/* within E104_SPOT_RANGE, player 0 on the ground ends E104_INTRO's wait.
 * within E104_CLOSE_RANGE, player 0 makes E-104 jump as a shot ends, and
 * stand as a hurt ends. within E104_JUMP_RANGE, player 0 makes a standing
 * E-104 jump. it has reached its target within E104_ARRIVAL_RANGE. each is a
 * distance in x and z */
#define E104_SPOT_RANGE 150.0F
#define E104_CLOSE_RANGE 120.0F
#define E104_JUMP_RANGE 60.0F
#define E104_ARRIVAL_RANGE 30.0F

/* the most it turns in a step */
#define E104_FLY_TURN NJM_DEG_ANG(5) /* toward its target, rising or flying */
#define E104_LAND_TURN NJM_DEG_ANG(6.5) /* toward player 0 as it lands */
#define E104_FIRE_TURN NJM_DEG_ANG(4) /* toward player 0 as it fires */
#define E104_AIM_TURN NJM_DEG_ANG(2.5) /* its head, toward player 0 or back */

/* in each step it flies, e104_fly adds E104_TILT_STEP to its tilt while the
 * tilt is under E104_FLY_TILT. e104_move has taken a degree back first. so
 * the tilt climbs about 2 degrees a step, then stays near E104_FLY_TILT */
#define E104_TILT_STEP NJM_DEG_ANG(3)
#define E104_FLY_TILT NJM_DEG_ANG(40)

/* what bob_phase gains a step: a bob takes about 38 steps */
#define E104_BOB_TURN 0x6aa

/* it fires no missile while this many fly */
#define E104_MOST_MISSILES 3

/* the steps E104_STAND waits before it fires or jumps, or at most
 * E104_QUICK_WAIT while no missile flies */
#define E104_WAIT 60
#define E104_QUICK_WAIT 10

/* from this frame of its hurt, it takes off over low ground */
#define E104_TAKE_OFF_FRAME 25.0F
/* a shot's missile leaves at this frame of the shot */
#define E104_FIRE_FRAME 35.0F

/* defeated, it stands this many steps on the floor before its defeat
 * plays */
#define E104_DEFEAT_WAIT 120
/* the camera's sweep round it centers on this angle about y */
#define E104_SWEEP_ANG NJM_DEG_ANG(45)
/* the steps its explosion casts rays and the camera sweeps. once they pass,
 * it frees an animal and is gone, and the explosion's sphere fades */
#define E104_BLAST_STEPS 480
/* the steps from when it's gone until the stage clears */
#define E104_CLEAR_WAIT 200

/* the nodes whose matrices e104_draw takes from the pose. the pose runs as
 * far as E104_LAUNCHER_0_NODE, the last of them */
#define E104_LAUNCHER_0_NODE 45
#define E104_LAUNCHER_1_NODE 36
#define E104_CORE_NODE 30
#define E104_SMOKE_1_NODE 31
#define E104_SMOKE_0_NODE 40

/* the hits, the missiles' launch, the sparks, the thrust, the jump and the
 * landing */
#define SE_E104_HIT 155
#define SE_E104_LAUNCH 153
#define SE_E104_SPARKS 156
#define SE_E104_THRUST 152
#define SE_E104_JUMP 783
#define SE_E104_LAND 785
/* played as the stage clears */
#define SE_E104_DOWN 0x530

/* the line by which the camera frames it and player 0 */
static NJS_POINT3 e104_camera_points[2] = {
    {-2300.0F, 100.0F, 2400.0F},
    {-2100.0F, 120.0F, 2500.0F},
};

/* the points it flies to. e104_next_target picks the next at random */
static NJS_POINT3 e104_targets[3] = {
    {-2150.0F, 67.0F, 2650.0F},
    {-2300.0F, 67.0F, 2600.0F},
    {-2400.0F, 67.0F, 2500.0F},
};

static struct collision_shape e104_shapes[2] = {
    {LOCKON_NO_NUMBER | SHAPE_SPHERE | SHAPE_HURTABLE,
     SHAPE_HURTS_OBJECTS | SHAPE_HURTS_ENEMIES | SHAPE_HURTS_PLAYERS | GUARD(2),
     PUSH_ENEMIES | PUSH_FIRM(PUSH_OTHERS),
     {0.0F, 0.0F, 0.0F},
     6.0F,
     4.0F,
     0.65F,
     {0, 0, 0},
     0,
     NULL},
    {SHAPE_SPHERE | SHAPE_HURTABLE,
     SHAPE_HURTS_OBJECTS | SHAPE_HURTS_ENEMIES | SHAPE_HURTS_PLAYERS | GUARD(2),
     PUSH_ENEMIES | PUSH_FIRM(PUSH_OTHERS),
     {0.0F, -8.0F, 0.0F},
     4.0F,
     4.0F,
     0.65F,
     {0, 0, 0},
     0,
     NULL},
};

static struct motion_entry e104_actions[9] = {
    {&e104_stand_action, 77, MOTION_LOOP, ACTION_STAND, 0.0625F, 0.5F},
    {&e104_jump_action, 77, MOTION_LOOP, ACTION_JUMP, 0.25F, 1.0F},
    {&e104_fall_action, 77, MOTION_LOOP, ACTION_FALL, 0.25F, 1.0F},
    {&e104_landing_action, 77, MOTION_ONCE, ACTION_STAND, 0.25F, 1.0F},
    {&e104_fly_action, 77, MOTION_LOOP, ACTION_FLY, 0.25F, 1.0F},
    {&e104_fire_right_action, 77, MOTION_ONCE, ACTION_FIRE_RIGHT, 0.25F, 1.0F},
    {&e104_fire_left_action, 77, MOTION_ONCE, ACTION_FIRE_LEFT, 0.25F, 1.0F},
    {&e104_hurt_action, 77, MOTION_ONCE, ACTION_HURT, 0.5F, 1.0F},
    {&e104_defeat_action, 77, MOTION_HOLD, ACTION_DEFEAT, 0.5F, 0.5F},
};

/* its smoke, which e104_smoke puts at each of its smoke points */
static struct dust_params e104_smoke_params = {
    0.5F,
    0.03F,
    0.19F,
    0.05F,
    0.025F,
    {0.0F, 0.0F, 0.0F},
    {0.0F, 0.0F, 0.0F},
    {1.0F, 0.28F, 0.28F, 0.35F},
};

static const NJS_POINT3 e104_no_offset;

/* the record e104_draw is drawing, which e104_turn_head reads */
static struct e104 *e104_drawn;

/* the work of a task that makes sparks */
struct e104_spark_work {
    Uint8 unk_00[6];
    Uint16 life; /* the steps it has left */
    Uint8 unk_08[8];
    NJS_POINT3 *pos; /* where the sparks fly from, which must outlive it */
};

static void e104_destroy(struct task *task);
static void e104_run_mode(struct task *task);
static void e104_run(struct task *task);
static void e104_draw(struct task *task);
static void e104_turn_head(NJS_OBJECT *object);
static void e104_spark_create(NJS_POINT3 *pos);

/* the object list's step. in the set editor, it makes the record if there
 * is none, then plays the motion and draws. outside it, it makes the record
 * at full health. it then gives the camera's E103 mode, by camera_set_e103,
 * its work and e104_camera_points. e104_run steps E-104 from then on,
 * starting in E104_INTRO */
void e104_step(struct task *task)
{
    struct task_work *work = task->work;
    struct e104 *e104 = task->extra;

    if (placement_editing(task)) {
        if (!task->extra) {
            collision_init(task, e104_shapes, COLLISION_ENEMY, 1, 2);
            enemy_alloc_record(task, work);
            e104 = (struct e104 *)motion_create(work, sizeof(struct e104));
            e104->motion.current = e104->motion.wanted = ACTION_STAND;
            e104->motion.actions = e104_actions;
            task->extra = e104;
        }
        motion_step(work, &e104->motion);
        e104_draw(task);
    } else {
        collision_init(task, e104_shapes, COLLISION_ENEMY, 1, 2);
        enemy_alloc_record(task, work);
        e104 = (struct e104 *)motion_create(work, sizeof(struct e104));
        e104->motion.current = e104->motion.wanted = ACTION_STAND;
        e104->motion.actions = e104_actions;
        task->extra = e104;
        motion_step(work, &e104->motion);
        work->ang.x = work->ang.z = 0;
        life_gauge_health = e104->health = E104_HEALTH;
        e104->next_mode = e104->last_mode = E104_INTRO;
        e104->target = 1;
        e104->speed.x = e104->speed.y = e104->speed.z = 0.0F;
        task->step = e104_run;
        task->draw = e104_draw;
        task->destructor = e104_destroy;
        camera_set_e103(work, e104_camera_points);
    }
}

/* end the mode it's in, and free the motion's blend */
static void e104_destroy(struct task *task)
{
    struct task_work *work = task->work;
    struct e104 *e104 = task->extra;

    if (work->submode != E104_DONE) {
        work->submode = E104_DONE;
        e104_run_mode(task);
    }
    e104->motion.state = MOTION_FREE;
    motion_step(work, &e104->motion);
}

/* take this step's hit. a guarded E-104 lets any lock-on go, hit or not.
 * outside E104_HURT, a hit on an unguarded one costs it one health and
 * guards it. with health left, it changes to E104_HURT, knocked back at 0.3
 * of the shot's speed. without, it changes to E104_DEFEAT */
static void e104_take_hit(struct task *task)
{
    struct task_work *work = task->work;
    struct lockon_target *record = (struct lockon_target *)task->motion;
    struct e104 *e104 = task->extra;
    struct task *shot;
    NJS_POINT3 pos;
    NJS_VECTOR speed;

    if (e104->flags & E104_GUARDED) {
        lockon_release_hit(task);
        return;
    }
    if (!(work->flags & COLLISION_DAMAGED)) {
        return;
    }
    if (work->mode == E104_HURT) {
        return;
    }
    /* @hack spelling: pos and speed through their addresses, which SHC forms
     * again for each store (MOV R15,R3 and ADD #16,R3 at 0x0c900dee); by
     * name, R11 is saved too and 184 of 190 words differ */
    if ((shot = lockon_get_shot(record->lockon))) {
        (&pos)->x = shot->work->pos.x;
        (&pos)->y = shot->work->pos.y;
        (&pos)->z = shot->work->pos.z;
        (&speed)->x = shot->motion->speed.x;
        (&speed)->y = shot->motion->speed.y;
        (&speed)->z = shot->motion->speed.z;
    } else {
        (&pos)->x = work->pos.x;
        (&pos)->y = work->pos.y;
        (&pos)->z = work->pos.z;
        (&speed)->x = (&speed)->y = (&speed)->z = 0.0F;
    }
    e104->flags |= E104_GUARDED;
    sound_play(SE_E104_HIT, 0, 0, 0);
    e102_hit_create(&pos, -15.0F);
    lockon_release(task);
    e104->health -= 1.0F;
    if (!(e104->health > 0.0F)) {
        E104_SET_MODE(task, E104_DEFEAT);
    } else {
        E104_SET_MODE(task, E104_HURT);
        e104->speed.x = (&speed)->x * 0.3F;
        e104->speed.y = (&speed)->y * 0.3F;
        e104->speed.z = (&speed)->z * 0.3F;
    }
}

/* set E104_GUARDED, and its body's guard to 3, which no attack exceeds.
 * e104_unguard puts both back, to guard 2 */
static void e104_guard(struct task_work *work, struct e104 *e104)
{
    e104->flags |= E104_GUARDED;
    work->collision->shapes->levels =
        (work->collision->shapes->levels & ~SHAPE_GUARD) | GUARD(3);
}

static void e104_unguard(struct task_work *work, struct e104 *e104)
{
    e104->flags &= ~E104_GUARDED;
    work->collision->shapes->levels =
        (work->collision->shapes->levels & ~SHAPE_GUARD) | GUARD(2);
}

static void e104_bob(struct task *task)
{
    struct e104 *e104 = task->extra;

    e104->bob_phase += E104_BOB_TURN;
    /* @hack redundant: the sine times 1, FLDI1 and FMUL at 0x0c900faa and
     * 0x0c900fac; without it, the function is 4 bytes shorter and 7 of 18
     * words differ */
    e104->bob = njSin(e104->bob_phase) * 1.0F;
}

/* 1 if player 0's distance from it in x and z, squared, is below
 * range_squared, else 0 */
static int e104_player_within(struct task *task, Float range_squared)
{
    struct task_work *work = task->work;
    struct task_work *player = player_work[0];
    Float x = work->pos.x - player->pos.x;
    Float z = work->pos.z - player->pos.z;

    x = (x * x) + (z * z);
    return x < range_squared;
}

/* turn toward target, by at most turn. distance_squared gets the square of
 * target's distance from it in x and z */
static void e104_turn_to(struct task *task, NJS_POINT3 *target, Angle turn)
{
    struct task_work *work = task->work;
    struct e104 *e104 = task->extra;
    Float z;
    Float x;
    Angle diff;

    x = work->pos.x - target->x;
    z = work->pos.z - target->z;
    diff = angle_difference(work->ang.y,
                            -(Angle)(atan2f(z, x) * ANGLE_PER_RADIAN));
    work->ang.y += CLAMP(diff, -turn, turn);
    e104->distance_squared = (x * x) + (z * z);
}

/* add distance to vector's x and z, the way E-104 faces when its y angle is
 * heading */
static void e104_add_ahead(NJS_POINT3 *vector, Angle heading, Float distance)
{
    NJS_POINT3 ahead;
    NJS_POINT3 turned;
    NJS_MATRIX matrix;

    ahead.x = -distance;
    ahead.y = ahead.z = 0.0F;
    njUnitMatrix(&matrix);
    TURN_MATRIX(njRotateY, &matrix, heading);
    njCalcPoint(&matrix, &ahead, &turned);
    vector->x += turned.x;
    vector->z += turned.z;
}

/* pick one of the two targets it isn't at, at random, and return it */
static int e104_next_target(struct task *task)
{
    struct e104 *e104 = task->extra;
    int target = (rand() & 1) + e104->target + 1;

    if (target >= 3) {
        target -= 3;
    }
    e104->target = target;
    return target;
}

/* if its ground is E104_LOW_FLOOR or lower, pick another target and ask for
 * E104_RISE. returns 1 then, else 0 */
static int e104_take_off(struct task *task)
{
    struct e104 *e104 = task->extra;

    if (e104->ground <= E104_LOW_FLOOR) {
        e104_next_target(task);
        E104_SET_MODE(task, E104_RISE);
        return 1;
    }
    return 0;
}

/* 1 if the floor below it is E104_LOW_FLOOR or lower, else 0 */
static int e104_over_low_floor(struct task *task)
{
    struct e104 *e104 = task->extra;

    if (e104->ground - E104_CENTER_HEIGHT <= E104_LOW_FLOOR) {
        return 1;
    }
    return 0;
}

static void e104_fire(struct task *task, int side)
{
    struct e104 *e104 = task->extra;

    e100_missile_create(&e104->launchers[side], &e104->launch_dirs[side],
                        task->work->collision);
    sound_play(SE_E104_LAUNCH, 0, 0, 0);
}

static void e104_smoke(struct task *task)
{
    struct e104 *e104 = task->extra;
    int i;

    for (i = 0; i < 2; ++i) {
        /* @hack redundant: a loop of one pass around the copy, which keeps SHC
         * from unrolling the two passes of the loop around it; without it,
         * SHC unrolls them, the function is 58 bytes longer and 27 of 49
         * words differ */
        do {
            e104_smoke_params.pos.x = e104->smoke[i].x;
            e104_smoke_params.pos.y = e104->smoke[i].y;
            e104_smoke_params.pos.z = e104->smoke[i].z;
        } while (0);
        dust_create_from(&e104_smoke_params);
    }
}

/* every 90 steps, at a chance of 1 in 2, start sparks at its core */
static void e104_sparks(struct task *task)
{
    struct e104 *e104 = task->extra;

    if (e104->ticks % 90 == 0) {
        if ((Float)rand() * RAND_SCALE < 0.5F) {
            e104_spark_create(&e104->core);
        }
    }
}

static void e104_explode(struct task *task)
{
    struct e104 *e104 = task->extra;
    struct explosion_params params;

    params.pos = &e104->core;
    params.flags = 0;
    params.ray_start = 0;
    params.ray_end = E104_BLAST_STEPS;
    params.ray_length = 100.0F;
    params.ray_turn = 91;
    params.grow_start = 100;
    params.grow_end = 200;
    params.fade_start = E104_BLAST_STEPS;
    params.fade_end = 510;
    params.radius = 60.0F;
    e100_explosion_create(&params);
}

/* 1 if player 0 is on the ground within E104_SPOT_RANGE, else 0 */
static int e104_player_near(struct task *task)
{
    struct task_work *player = player_get_task(0)->work;

    if (e104_player_within(task, E104_SPOT_RANGE * E104_SPOT_RANGE) &&
        (player->flags & PLAYER_ON_GROUND)) {
        return 1;
    }
    return 0;
}

/* E-104 waits, guarded, for player 0 to stand within E104_SPOT_RANGE. then
 * the time and the pads stop, and the camera swings round it as its name is
 * typed. once the swing is round, both start again, and the fight starts
 * with E104_FIRE_RIGHT. until then, it hangs still over a low floor */
static void e104_intro(struct task *task)
{
    struct task_work *work = task->work;
    struct e104 *e104 = task->extra;

    /* @hack redundant: a never-run store of the submode, which gives work R13
     * and the record R12 (0x0c90138c, 0x0c901390), which otherwise trade;
     * without it, 23 of 151 words differ */
    if (0) {
        work->submode = 1;
    }
    if (placement_try_leave(task)) {
        task_kill(task);
        return;
    }
    switch (work->submode) {
    case 0:
        e104->health = E104_HEALTH;
        e104->motion.wanted = ACTION_STAND;
        e104->flags &= ~(E104_SMOKES | E104_SPARKS);
        e104_guard(work, e104);
        e100_missile_count = 0;
        work->submode = 1;
        break;
    case 1:
        if (e104_player_near(task)) {
            time_stop();
            pad_disable();
            camera_start_swing(work, 0.0F, 30.0F, 240);
            camera_override_mode(CAMERA_MODE_CHAOS_STINIT, CAMERA_ADJUST_NONE);
            boss_name_create(TEXT_1, -1, 240, 120);
            work->submode = 2;
            stream_request_music(MUSIC_BOSSTRGT);
        }
        break;
    case 2:
        if (camera_swing_done) {
            time_start();
            pad_enable();
            life_gauge_create(0, 0, 5);
            e104_unguard(work, e104);
            camera_override_mode(CAMERA_MODE_E103, CAMERA_ADJUST_NONE);
            E104_SET_MODE(task, E104_FIRE_RIGHT);
            work->submode = E104_DONE;
        }
        break;
    }
    if (work->submode == E104_DONE) {
        e104->flags |= E104_FALLS;
    } else {
        if (e104_over_low_floor(task)) {
            e104->flags &= ~E104_FALLS;
            e104->speed.y = 0.0F;
        } else {
            e104->flags |= E104_FALLS;
        }
    }
}

/* spring *ang back toward 0. each call, *speed gains a pull toward 0 of up
 * to a degree, less a fifth of itself. *ang then turns by *speed. once both
 * are under half a degree, both settle at 0 */
static void e104_settle(Angle *ang, Angle *speed)
{
    Angle diff = angle_difference(*ang, 0);

    diff = CLAMP(diff, -NJM_DEG_ANG(1), NJM_DEG_ANG(1));
    diff -= (Float)*speed * 0.2F;
    *speed += diff;
    *ang += *speed;
    if (_builtin_fabsf((Float)*speed) < (Float)NJM_DEG_ANG(0.5)) {
        if (_builtin_fabsf((Float)*ang) < (Float)NJM_DEG_ANG(0.5)) {
            *speed = 0;
            *ang = 0;
        }
    }
}

/* move it by its speed, out of walls, and no lower than ground. a wall
 * reverses its speed in x and z. that speed loses 5% each step. it sets
 * E104_LANDED as it reaches ground. its head turns toward player 0 while it
 * aims, and back otherwise. its tilt springs back to 0 */
static void e104_move(struct task *task)
{
    struct task_work *work = task->work;
    struct e104 *e104 = task->extra;
    NJS_VECTOR push;
    Angle heading;
    Angle diff;

    e104->last_pos = work->pos;
    if (e100_push_from_walls(&work->pos, 5.0F, &e104->ground, &push) == 1) {
        /* @hack spelling: push through its address, which reads each member
         * through a copy of R15 (MOV R15,R3 at 0x0c90163a); by name, it reads
         * them from the frame, the function is 6 bytes shorter and 90 of 232
         * words differ */
        work->pos.x = work->pos.x + (&push)->x;
        work->pos.y = work->pos.y + (&push)->y;
        work->pos.z = work->pos.z + (&push)->z;
        e104->speed.x = -e104->speed.x;
        e104->speed.z = -e104->speed.z;
    }
    e104->ground += E104_CENTER_HEIGHT;
    e104->height = work->pos.y - e104->ground;
    if (e104->flags & E104_FALLS) {
        e104->speed.y -= 0.05F;
    }
    work->pos.y += e104->speed.y;
    if (!(work->pos.y > e104->ground)) {
        work->pos.y = e104->ground;
        e104->speed.y = 0.0F;
        if (!(e104->flags & E104_LANDED)) {
            e104->flags |= E104_LANDED;
            sound_play_at(SE_E104_LAND, 0, 0, 0, work->pos.x, work->pos.y,
                          work->pos.z);
        }
    } else {
        e104->flags &= ~E104_LANDED;
    }
    work->pos.x += e104->speed.x;
    work->pos.z += e104->speed.z;
    e104->speed.x *= 0.95F;
    e104->speed.z *= 0.95F;
    if (e104->speed.x < 0.05F && e104->speed.x > -0.05F) {
        e104->speed.x = 0.0F;
    }
    if (e104->speed.z < 0.05F && e104->speed.z > -0.05F) {
        e104->speed.z = 0.0F;
    }
    if (e104->flags & E104_AIMING) {
        Float dx = work->pos.x - player_work[0]->pos.x;
        Float dz = work->pos.z - player_work[0]->pos.z;

        heading = -work->ang.y - (Angle)(atan2f(dz, dx) * ANGLE_PER_RADIAN);
    } else {
        heading = 0;
    }
    diff = angle_difference(e104->aim, heading);
    e104->aim += CLAMP(diff, -E104_AIM_TURN, E104_AIM_TURN);
    e104_settle(&work->ang.x, &e104->tilt_speed);
}

/* it stands, aiming, and takes off at once over low ground. once wait runs
 * out, it fires, or jumps if E104_MOST_MISSILES fly. it jumps at once if
 * player 0 comes within E104_JUMP_RANGE. but at E104_LANDING_HEIGHT or more
 * above ground, it lands instead */
static void e104_stand(struct task *task)
{
    struct task_work *work = task->work;
    struct e104 *e104 = task->extra;

    switch (work->submode) {
    case 0:
        camera_override_mode(CAMERA_MODE_E103, CAMERA_ADJUST_NONE);
        e104->motion.wanted = ACTION_STAND;
        e104->flags |= E104_AIMING;
        work->submode = 1;
        break;
    case 1:
        if (e104_take_off(task) == 1) {
            break;
        }
        if (e100_missile_count == 0 && e104->wait > E104_QUICK_WAIT) {
            e104->wait = E104_QUICK_WAIT;
        } else if (--e104->wait < 0) {
            if (e100_missile_count < E104_MOST_MISSILES) {
                E104_SET_MODE(task, E104_FIRE_RIGHT);
            } else {
                E104_SET_MODE(task, E104_JUMP);
            }
            e104->wait = E104_WAIT;
        }
        if (e104_player_within(task, E104_JUMP_RANGE * E104_JUMP_RANGE)) {
            E104_SET_MODE(task, E104_JUMP);
        }
        if (!(work->pos.y - e104->ground < E104_LANDING_HEIGHT)) {
            E104_SET_MODE(task, E104_LAND);
        }
        break;
    }
}

/* it plays its jump, picks another target, and leaps up. at the top of the
 * leap, it flies */
static void e104_jump(struct task *task)
{
    struct task_work *work = task->work;
    struct e104 *e104 = task->extra;

    switch (work->submode) {
    case 0:
        e104->motion.wanted = ACTION_JUMP;
        e104->flags &= ~E104_AIMING;
        work->submode = 1;
        break;
    case 1:
        motion_flags &= ~MOTION_HOLDING;
        if (e104->motion.current == ACTION_JUMP) {
            e104->tilt_speed = 0;
            work->ang.x = 0;
            work->submode = 2;
        }
        break;
    case 2:
        e104_next_target(task);
        e104->speed.y += 2.0F;
        sound_play_at(SE_E104_JUMP, 0, 0, 0, work->pos.x, work->pos.y,
                      work->pos.z);
        work->submode = 3;
        break;
    case 3:
        if (!(e104->speed.y > 0.0F)) {
            E104_SET_MODE(task, E104_FLY);
            work->submode = E104_DONE;
        }
        break;
    }
    /* @hack redundant: the submode tested for nothing, MOV.B and CMP/EQ at
     * 0x0c901a3a and 0x0c901a3e; without it, the function is 4 bytes
     * shorter and 5 of 91 words differ */
    if (work->submode == E104_DONE) {
    }
}

/* guarded, it climbs on its jets to E104_CRUISE_HEIGHT, turning toward its
 * target, and flies on from there */
static void e104_rise(struct task *task)
{
    struct task_work *work = task->work;
    struct e104 *e104 = task->extra;

    e100_hover_dust(work, e104->ground + -E104_CENTER_HEIGHT);
    switch (work->submode) {
    case 0:
        e104->motion.wanted = ACTION_FLY;
        e104->flags |= E104_FLYING;
        e104->flags &= ~E104_AIMING;
        e104_guard(work, e104);
        work->submode = 1;
        break;
    case 1:
        sound_play_at_for(SE_E104_THRUST, (int)task, 1, 0, work->pos.x,
                          work->pos.y, work->pos.z, 4);
        e104_turn_to(task, &e104_targets[e104->target], E104_FLY_TURN);
        if (e104->speed.y < 0.75F) {
            e104->speed.y += 0.1F;
        }
        if (!(work->pos.y < E104_CRUISE_HEIGHT)) {
            E104_SET_MODE(task, E104_FLY);
            work->submode = E104_DONE;
        }
        break;
    }
    if (work->submode == E104_DONE) {
        e104->flags &= ~E104_FLYING;
        e104_unguard(work, e104);
    }
}

/* guarded, it flies on its jets toward its target. it lands within
 * E104_ARRIVAL_RANGE of the target, or E104_LANDING_HEIGHT or less above
 * ground. below E104_CRUISE_HEIGHT, it takes off again over low ground */
static void e104_fly(struct task *task)
{
    struct task_work *work = task->work;
    struct e104 *e104 = task->extra;

    e100_hover_dust(work, e104->ground + -E104_CENTER_HEIGHT);
    switch (work->submode) {
    case 0:
        e104->motion.wanted = ACTION_FLY;
        e104->flags |= E104_FLYING;
        e104->flags &= ~E104_AIMING;
        e104_guard(work, e104);
        work->submode = 1;
        break;
    case 1:
        sound_play_at_for(SE_E104_THRUST, (int)task, 1, 0, work->pos.x,
                          work->pos.y, work->pos.z, 4);
        e104_turn_to(task, &e104_targets[e104->target], E104_FLY_TURN);
        e104->tilt_speed = 0;
        if (work->ang.x < E104_FLY_TILT) {
            work->ang.x += E104_TILT_STEP;
        }
        if (work->pos.y < E104_CRUISE_HEIGHT && e104_take_off(task) == 1) {
            break;
        }
        if (!(e104->speed.y < 0.0F)) {
            e104->speed.y *= 0.95F;
        } else {
            e104->speed.y = 0.0F;
        }
        e104_add_ahead(&e104->speed, work->ang.y, 0.15F);
        if (!(e104->distance_squared >
              E104_ARRIVAL_RANGE * E104_ARRIVAL_RANGE)) {
            E104_SET_MODE(task, E104_LAND);
            work->submode = E104_DONE;
        }
        if (!(e104->height > E104_LANDING_HEIGHT)) {
            E104_SET_MODE(task, E104_LAND);
            work->submode = E104_DONE;
        }
        break;
    }
    if (work->submode == E104_DONE) {
        e104->speed.x *= 0.6F;
        e104->speed.z *= 0.6F;
        e104->flags &= ~E104_FLYING;
        e104_unguard(work, e104);
    }
}

/* it falls, turning toward player 0. on the floor it plays its landing, and
 * as that ends, it fires */
static void e104_land(struct task *task)
{
    struct task_work *work = task->work;
    struct e104 *e104 = task->extra;

    e100_hover_dust(work, e104->ground + -E104_CENTER_HEIGHT);
    switch (work->submode) {
    case 0:
        e104->motion.wanted = ACTION_FALL;
        work->submode = 1;
        break;
    case 1:
        e104_turn_to(task, &player_work[0]->pos, E104_LAND_TURN);
        if (e104->ground == work->pos.y) {
            e104->motion.wanted = ACTION_LANDING;
            work->submode = 2;
        }
        break;
    case 2:
        motion_flags &= ~MOTION_HOLDING;
        if (e104->motion.current == ACTION_LANDING) {
            motion_flags &= ~MOTION_ENDED;
            work->submode = 3;
        }
        break;
    case 3:
        if (motion_flags & MOTION_ENDED) {
            E104_SET_MODE(task, E104_FIRE_RIGHT);
            work->submode = E104_DONE;
        }
        break;
    }
}

/* guarded, it plays its hurt. at 2 health or less it starts to smoke, and
 * at 1 or less to spark. from the hurt's E104_TAKE_OFF_FRAME, it takes off
 * over low ground. as the hurt ends, it stands if player 0 is within
 * E104_CLOSE_RANGE, or fires */
static void e104_hurt(struct task *task)
{
    struct task_work *work = task->work;
    struct e104 *e104 = task->extra;

    switch (work->submode) {
    case 0:
        e104->motion.wanted = ACTION_HURT;
        e104->flags &= ~E104_AIMING;
        e104_guard(work, e104);
        if (!(e104->health > 2.0F)) {
            e104->flags |= E104_SMOKES;
        }
        if (!(e104->health > 1.0F)) {
            e104->flags |= E104_SPARKS;
        }
        work->submode = 1;
        break;
    case 1:
        motion_flags &= ~MOTION_HOLDING;
        if (e104->motion.current == ACTION_HURT) {
            motion_flags &= ~MOTION_ENDED;
            work->submode = 2;
        }
        break;
    case 2:
        if (!(e104->motion.frame < E104_TAKE_OFF_FRAME) &&
            e104_take_off(task) == 1) {
            break;
        }
        if (motion_flags & MOTION_ENDED) {
            if (e104_player_within(task, E104_CLOSE_RANGE * E104_CLOSE_RANGE)) {
                E104_SET_MODE(task, E104_STAND);
            } else {
                E104_SET_MODE(task, E104_FIRE_RIGHT);
            }
            work->submode = E104_DONE;
        }
        break;
    }
    if (work->submode == E104_DONE) {
        e104_unguard(work, e104);
    }
}

/* the step of both fire modes. it stops, turns toward player 0 and fires a
 * missile from the mode's side. it then jumps if player 0 is within
 * E104_CLOSE_RANGE, or at a chance of 1 in 3. else a right shot leads to a
 * left one, and a left one to E104_STAND. while E104_MOST_MISSILES fly, it
 * stands instead of firing */
static void e104_fire_mode(struct task *task)
{
    struct task_work *work = task->work;
    struct e104 *e104 = task->extra;

    switch (work->submode) {
    case 0:
        if (e100_missile_count >= E104_MOST_MISSILES) {
            E104_SET_MODE(task, E104_STAND);
            return;
        }
        e104->speed.x = e104->speed.y = e104->speed.z = 0.0F;
        e104->motion.wanted = work->mode == E104_FIRE_RIGHT ? ACTION_FIRE_RIGHT
                                                            : ACTION_FIRE_LEFT;
        work->submode = 1;
        break;
    case 1:
        motion_flags &= ~MOTION_HOLDING;
        if (e104->motion.current == ACTION_FIRE_RIGHT ||
            e104->motion.current == ACTION_FIRE_LEFT) {
            work->submode = 2;
        }
        break;
    case 2:
        e104_turn_to(task, &player_work[0]->pos, E104_FIRE_TURN);
        motion_flags &= ~MOTION_HOLDING;
        if (!(e104->motion.frame < E104_FIRE_FRAME)) {
            e104_fire(task, work->mode == E104_FIRE_RIGHT);
            motion_flags &= ~MOTION_ENDED;
            work->submode = 3;
        }
        break;
    case 3:
        if (motion_flags & MOTION_ENDED) {
            if (e104_player_within(task, E104_CLOSE_RANGE * E104_CLOSE_RANGE)) {
                E104_SET_MODE(task, E104_JUMP);
            } else if (rand() < RAND_MAX / 3) {
                E104_SET_MODE(task, E104_JUMP);
            } else if (work->mode == E104_FIRE_RIGHT) {
                E104_SET_MODE(task, E104_FIRE_LEFT);
            } else {
                E104_SET_MODE(task, E104_STAND);
                e104->wait = E104_WAIT;
            }
            work->submode = E104_DONE;
        }
        break;
    }
    if (work->submode == E104_DONE) {
        e104->wait = E104_WAIT;
    }
}

/* the mode of an E-104 with no health left. it bursts every missile, stops
 * the time and the pads, and smokes and sparks. once it has stood
 * E104_DEFEAT_WAIT steps on the floor, it stops smoking and sparking, and
 * starts its defeat. once the motion has changed to the defeat, it
 * explodes, and the camera sweeps round it for E104_BLAST_STEPS. then it
 * frees an animal and is no longer drawn. E104_CLEAR_WAIT steps later, the
 * stage clears and the pads start again */
static void e104_defeat(struct task *task)
{
    struct task_work *work = task->work;
    struct e104 *e104 = task->extra;

    switch (work->submode) {
    case 0:
        e104->motion.wanted = ACTION_STAND;
        e104->flags &= ~E104_AIMING;
        e104->flags |= E104_SMOKES | E104_SPARKS;
        e100_missile_count = -1;
        time_stop();
        pad_disable();
        work->submode = 1;
        break;
    case 1:
        motion_flags &= ~MOTION_HOLDING;
        if (e104->motion.current == ACTION_STAND) {
            motion_flags &= ~MOTION_ENDED;
            e104->ticks = 0;
            work->submode = 2;
        }
        break;
    case 2:
        if (!(e104->flags & E104_LANDED)) {
            e104->ticks = 0;
        }
        if (e104->ticks >= E104_DEFEAT_WAIT) {
            e104->flags &= ~(E104_SMOKES | E104_SPARKS);
            e104->motion.wanted = ACTION_DEFEAT;
            work->submode = 3;
        }
        break;
    case 3:
        if (e104->motion.current == ACTION_DEFEAT) {
            motion_flags &= ~MOTION_ENDED;
            work->submode = 4;
        }
        break;
    case 4:
        e104->ticks = 0;
        camera_sweep_start(&work->pos, E104_SWEEP_ANG, E104_BLAST_STEPS);
        e104_explode(task);
        work->submode = 5;
        break;
    case 5:
        if (e104->ticks >= E104_BLAST_STEPS) {
            e104->ticks = 0;
            ++work->submode;
            animal_create_staying(e104->core.x, e104->core.y, e104->core.z,
                                  animal_start_tuba);
            e104->flags |= E104_GONE;
        }
        break;
    case 6:
        if (e104->ticks >= E104_CLEAR_WAIT) {
            ++work->submode;
        }
        break;
    case 7:
        sound_play(SE_E104_DOWN, 0, 0, 0);
        stage_clear();
        work->submode = E104_DONE;
        break;
    }
    if (work->submode == E104_DONE) {
        e104->flags &= ~(E104_SMOKES | E104_SPARKS);
        pad_enable();
    }
}

/* run the mode's step after e104_move. E104_INTRO's first step runs
 * without the move */
static void e104_run_mode(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case E104_INTRO:
        if (work->submode != 0) {
            e104_move(task);
        }
        e104_intro(task);
        break;
    case E104_STAND:
        e104_move(task);
        e104_stand(task);
        break;
    case E104_JUMP:
        e104_move(task);
        e104_jump(task);
        break;
    case E104_RISE:
        e104_move(task);
        e104_rise(task);
        break;
    case E104_FLY:
        e104_move(task);
        e104_fly(task);
        break;
    case E104_LAND:
        e104_move(task);
        e104_land(task);
        break;
    case E104_HURT:
        e104_move(task);
        e104_hurt(task);
        break;
    case E104_FIRE_RIGHT:
        e104_move(task);
        e104_fire_mode(task);
        break;
    case E104_FIRE_LEFT:
        e104_move(task);
        e104_fire_mode(task);
        break;
    case E104_DEFEAT:
        e104_move(task);
        e104_defeat(task);
        break;
    }
}

/* the task's step after e104_step's first. it bobs its jets, and smokes and
 * sparks as its flags say. it then runs the mode and takes this step's hit.
 * if next_mode differs from the mode, it ends the mode and starts next_mode
 * at submode 0. last, it plays the motion and draws */
static void e104_run(struct task *task)
{
    struct task_work *work = task->work;
    struct e104 *e104 = task->extra;
    /* @hack spelling: the motion through a local, which keeps the record in
     * R12 and work in R13 (0x0c90244a, 0x0c902450); passed as &e104->motion,
     * the two trade and 17 of 65 words differ */
    struct motion *motion;

    e104->ticks++;
    e104_bob(task);
    if (e104->flags & E104_SMOKES) {
        e104_smoke(task);
    }
    if (e104->flags & E104_SPARKS) {
        e104_sparks(task);
    }
    e104_run_mode(task);
    e104_take_hit(task);
    if (work->mode != e104->next_mode) {
        if (work->submode != E104_DONE) {
            work->submode = E104_DONE;
            e104_run_mode(task);
        }
        work->mode = e104->next_mode;
        work->submode = 0;
    }
    life_gauge_health = e104->health;
    motion = &e104->motion;
    motion_step(work, motion);
    collision_register(work);
    e104_draw(task);
}

/* pose it, and on a frame's first step draw it, its shadow and its jets,
 * until it's gone. the jets' flame shows while it flies. on every step, it
 * then finds from the pose its launchers, its smoke points and its core */
static void e104_draw(struct task *task)
{
    struct task_work *work = task->work;
    struct e104 *e104 = task->extra;
    NJS_ACTION *action;
    NJS_MATRIX core_matrix;
    NJS_MATRIX launcher_1_matrix;
    NJS_MATRIX launcher_0_matrix;
    NJS_MATRIX smoke_1_matrix;
    NJS_MATRIX smoke_0_matrix;
    NJS_MATRIX *launcher_0;
    NJS_MATRIX *launcher_1;
    NJS_MATRIX *smoke_0;
    NJS_MATRIX *smoke_1;
    NJS_POINT3 offset;
    Angle ang;

    action = e104->motion.state != MOTION_BLENDING
                 ? e104->motion.actions[e104->motion.current].action
                 : e104->motion.blend;
    e104_drawn = e104;
    /* @hack spelling: launcher_0, launcher_1, smoke_1 and smoke_0 are each set
     * inside the call that fills its matrix. SHC then keeps all four on the
     * stack, in a frame of 352 bytes (H'FEA0 at 0x0c9024d0). set ahead of the
     * calls, one takes R8, the frame is 348 bytes, and 325 of 336 words
     * differ */
    pose_action(&_unit_matrix_, action, e104->motion.frame,
                E104_LAUNCHER_0_NODE, (launcher_0 = &launcher_0_matrix));
    pose_get_matrix(E104_LAUNCHER_1_NODE, (launcher_1 = &launcher_1_matrix));
    pose_get_matrix(E104_CORE_NODE, &core_matrix);
    pose_get_matrix(E104_SMOKE_1_NODE, (smoke_1 = &smoke_1_matrix));
    pose_get_matrix(E104_SMOKE_0_NODE, (smoke_0 = &smoke_0_matrix));
    if (!(e104->flags & E104_GONE)) {
        shadow_draw(work, 2.0F);
    }
    njSetTexture(&mountain_e104_texlist);
    draw_attr_save();
    draw_attr_clear(0, NJD_FLAG_IGNORE_SPECULAR);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    /* its tilt, work->ang.x, turns it about z, and ang.z turns it about x */
    TURN_VIA(njRotateX, work->ang.z, ang);
    TURN_VIA(njRotateY, work->ang.y, ang);
    TURN_VIA(njRotateZ, work->ang.x, ang);
    if (!frame_step && !(e104->flags & E104_GONE)) {
        njSetMotionCallback(e104_turn_head);
        draw_action(action, e104->motion.frame);
        njSetMotionCallback(NULL);
        njPushMatrix(NULL);
        njMultiMatrix(NULL, &core_matrix);
        njRotateZ(NULL, 0x8000);
        njTranslate(NULL, 5.0F, e104->bob + 5.0F, 0.0F);
        draw_tree(&e104_jet_object);
        if (e104->flags & E104_FLYING) {
            njSetTexture(&e102effect_texlist);
            e102_flame->model->mats[0].attr_texId =
                ((e104->ticks >> 1) & 3) + 8;
            njSimpleDrawObject(e102_flame);
        }
        njPopMatrix(1);
    }
    njPushMatrix(&_unit_matrix_);
    offset = e104_no_offset;
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateX, work->ang.z, ang);
    TURN_VIA(njRotateY, work->ang.y, ang);
    TURN_VIA(njRotateZ, work->ang.x, ang);
    njPushMatrix(NULL);
    njMultiMatrix(NULL, launcher_0);
    offset.x = 3.0F;
    njCalcPoint(NULL, &offset, &e104->launchers[0]);
    offset.x = -1.0F;
    njCalcVector(NULL, &offset, &e104->launch_dirs[0]);
    njPopMatrix(1);
    njPushMatrix(NULL);
    njMultiMatrix(NULL, launcher_1);
    offset.x = 3.0F;
    njCalcPoint(NULL, &offset, &e104->launchers[1]);
    offset.x = -1.0F;
    njCalcVector(NULL, &offset, &e104->launch_dirs[1]);
    njPopMatrix(1);
    offset.x = 0.0F;
    njPushMatrix(NULL);
    njMultiMatrix(NULL, smoke_1);
    njCalcPoint(NULL, &offset, &e104->smoke[1]);
    njPopMatrix(1);
    njPushMatrix(NULL);
    njMultiMatrix(NULL, smoke_0);
    njCalcPoint(NULL, &offset, &e104->smoke[0]);
    njPopMatrix(1);
    njMultiMatrix(NULL, &core_matrix);
    njCalcPoint(NULL, &offset, &e104->core);
    njPopMatrix(2);
    draw_attr_restore();
}

/* draw_action's motion callback. at the head, it turns by e104_drawn's aim
 * about y */
static void e104_turn_head(NJS_OBJECT *object)
{
    Angle ang;

    if (object == &e104_head_object) {
        TURN_VIA(njRotateY, e104_drawn->aim, ang);
    }
}

/* on a frame's first step, flash lightning at a chance of 1 in 2. it
 * flashes at a random point within 5 of *pos along each axis. the task ends
 * once its life runs out */
static void e104_spark_step(struct task *task)
{
    struct e104_spark_work *work = (struct e104_spark_work *)task->work;
    NJS_POINT3 at;
    /* @hack spelling: rand through a local, which ends pos's lifetime before
     * the jitter; called by name, R13 is saved too (0x0c9028aa), the function
     * is 2 bytes longer and 62 of 92 words differ */
    int (*random)(void);

    if (--work->life <= 0) {
        task_kill(task);
        return;
    }
    if (frame_step) {
        return;
    }
    if ((Float)rand() * RAND_SCALE < 0.5F) {
        /* @hack spelling: the scale and the bias through locals, which put
         * the bias in FR14 and the scale in FR15 (0x0c902914, 0x0c902916); as
         * literals, the two trade and 7 of 92 words differ */
        Float scale;
        Float bias;

        /* @hack spelling: at through its address, which copies the work's pos
         * into it; by name, the function is 2 bytes shorter and 62 of 92 words
         * differ */
        (&at)->x = work->pos->x;
        (&at)->y = work->pos->y;
        (&at)->z = work->pos->z;
        scale = 10.0F * RAND_SCALE;
        bias = -5.0F;
        random = rand;
        at.x += ((Float)random() * scale) + bias;
        at.y += ((Float)random() * scale) + bias;
        at.z += ((Float)random() * scale) + bias;
        lightning_create(&at, 1.0F);
    }
}

static void e104_spark_create(NJS_POINT3 *pos)
{
    struct task *task;

    if ((task = task_create(TASK_WORK, 4, e104_spark_step))) {
        struct e104_spark_work *work = (struct e104_spark_work *)task->work;

        work->life = 60;
        work->pos = pos;
        sound_play(SE_E104_SPARKS, 0, 0, 0);
    }
}

/* @hack spelling: defined after the code, which places it after the name's
 * string among the constants, at 0x0cadc5a0; defined above, it comes before
 * the string, and 7 of the constants' 10 words differ, none of the code's */
static const NJS_POINT3 e104_no_offset = {0.0F, 0.0F, 0.0F};
