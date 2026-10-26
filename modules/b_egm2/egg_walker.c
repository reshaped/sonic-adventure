/* the Egg Walker. it waits while the camera swings round it, then walks
 * the street. it picks each attack by player 0's distance */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>
#include <shinobi.h>
#include <stdlib.h>

#include "b_egm2/bombs.h"
#include "b_egm2/camera.h"
#include "b_egm2/cannons.h"
#include "b_egm2/cars.h"
#include "b_egm2/defeat.h"
#include "b_egm2/egg_walker.h"
#include "b_egm2/eggman.h"
#include "b_egm2/flames.h"
#include "b_egm2/hints.h"
#include "b_egm2/jump.h"
#include "b_egm2/leg_pose.h"
#include "b_egm2/legs.h"
#include "b_egm2/parts.h"
#include "b_egm2/texlists.h"
#include "camera/camera.h"
#include "collision/collision.h"
#include "collision/ground.h"
#include "draw/draw.h"
#include "draw/motion.h"
#include "draw/tree.h"
#include "hud/hud.h"
#include "hud/life_gauge.h"
#include "player/player.h"
#include "sound/sound.h"
#include "system/frame.h"
#include "system/pad.h"
#include "system/task.h"
#include "system/util.h"
#include "vmu/vibration.h"

extern struct motion_entry egg_walker_actions[];
extern struct weld egg_walker_welds[];
/* the turret, and the cannon and the nozzle it carries */
extern NJS_OBJECT egm2_turret_object;
extern NJS_OBJECT egm2_cannon_object;
extern NJS_OBJECT egm2_nozzle_object;

#define HEALTH 5
#define SE_FOOTFALL 0x1d7

#define QUARTER_TURN 0x4000
#define HALF_TURN 0x8000

/* camera_start_swing's height, distance and steps for the swing round the
 * boss as the fight opens */
#define SWING_HEIGHT 20.0F
#define SWING_DISTANCE 90.0F
#define SWING_STEPS 180

/* the walk counts down from PICK_TIME before each pick, and picks as the
 * count passes below 0 */
#define PICK_TIME 60

/* player 0 within CLOSE_NOW_REACH across the ground brings the close
 * attack at once. a pick takes the close attack within PICK_CLOSE_REACH,
 * and the flames within PICK_FLAMES_REACH */
#define CLOSE_NOW_REACH 80.0F
#define PICK_CLOSE_REACH 100.0F
#define PICK_FLAMES_REACH 200.0F

/* with MOVE_HEALTH hits left or fewer, the walk moves the boss between
 * attacks. each pick counts moves_since_attack up, and makes a move while
 * the count is below MOVE_LIMIT. a move goes to player 0 when player 0 is
 * MOVE_Z or more away along z. a nearer player 0 brings a move of MOVE_Z
 * along z */
#define MOVE_HEALTH 3.0F
#define MOVE_LIMIT 2
#define MOVE_Z 100.0F

/* a move's target x lies within TARGET_SWAY of TARGET_X, across the
 * street. alternate_target puts it TARGET_SWAY to one side or the other */
#define TARGET_X (-440.0F)
#define TARGET_SWAY 40.0F
#define ALTERNATE_STRIDE 50.0F

/* the walk's pace by the hits left. it is SLOW_PACE with 4 or more,
 * MIDDLE_PACE with 2 or 3, and FAST_PACE with fewer */
#define SLOW_PACE 0.7F
#define MIDDLE_PACE 1.1F
#define FAST_PACE 1.5F

/* a walk stops within STOP_REACH of its target across the ground */
#define STOP_REACH 60.0F

/* the frames of the walk at which a foot comes down. each footfall shakes
 * pad 0 by FOOTFALL_POWER and FOOTFALL_FREQ, and the camera by
 * FOOTFALL_SHAKE */
#define FIRST_FOOTFALL 12.0F
#define SECOND_FOOTFALL 37.0F
#define FOOTFALL_POWER 3
#define FOOTFALL_FREQ 30
#define FOOTFALL_SHAKE 1

/* the shares of the short way the boss's body and its turret turn each
 * step */
#define FACE_PACE 0.01F
#define AIM_PACE 0.06F

#define SHADOW_SCALE 1.7F

/* the boss's submodes in EGG_WALKER_WALK */
enum pick_mode {
    PICK_START,
    PICK_WAIT, /* the countdown */
    PICK_ATTACK,
    PICK_MOVE, /* for the walk to stop, then a new target */
    PICK_MOVING /* for the walk to the target to stop */
};

static const NJS_POINT3 start_pos = {-430.0F, STAND_Y, 1380.0F};
/* the points on the turret about which the cannon and the nozzle pitch */
static const NJS_POINT3 cannon_mount = {-21.9F, 24.67F, -24.4F};
static const NJS_POINT3 nozzle_mount = {21.9F, 24.67F, -24.4F};

/* the moves the boss has made since its last attack */
static Uint8 moves_since_attack = 0;

/* each walk's start sets it to 0, and nothing reads it */
static Float dat_0c992708;

static void egg_walker_step(struct task *task);
static void egg_walker_draw(struct task *task);
static void egg_walker_destroy(struct task *task);
static void choose_attack(struct task *task);
static Uint8 face_target(struct task *task);
static void draw_foot_shadows(struct task *task);

void egg_walker_create(void)
{
    struct task *task;

    if ((task = task_create(TASK_WORK | TASK_MOTION, 1, egg_walker_step)) !=
        NULL) {
        task->step = egg_walker_step;
        task->draw = egg_walker_draw;
    }
}

/* allocate the boss's record and set the fight up. the boss starts at
 * start_pos with HEALTH hits, the camera swings round it, and the pads go
 * off. it returns the record, or null when the record can't be allocated,
 * and then sets up nothing */
static struct motion *egg_walker_init(struct task *task)
{
    struct task_work *work = task->work;
    struct motion *motion = motion_create(work, sizeof(struct egg_walker));
    struct egg_walker *boss;

    if (motion != NULL) {
        task->destructor = egg_walker_destroy;
        egg_walker_shapes_init(task);
        egg_walker_leg_copies_init();
        motion->wanted = EGG_WALKER_ACTION_STAND;
        motion->current = EGG_WALKER_ACTION_STAND;
        motion->actions = egg_walker_actions;
        motion->welds = egg_walker_welds;
        work->mode = boss_next_mode = EGG_WALKER_INTRO;
        task->extra = motion;
        /* @hack spelling: boss read back from task->extra rather than taken
         * from motion. SHC then copies the record's pointer into R4 for
         * boss's stores (MOV R14,R4 at 0x0c9002b6); from motion, they go
         * through R14, the function runs 2 bytes short and 62 of 86 words
         * differ */
        boss = task->extra;
        boss_work = work;
        boss->health = HEALTH;
        work->pos = start_pos;
        boss->pace = SLOW_PACE;
        boss->cannon_pitch = NJM_DEG_ANG(-25);
        boss->nozzle_pitch = 0;
        camera_start_swing(egg_walker_camera_target_create(&work->pos),
                           SWING_HEIGHT, SWING_DISTANCE, SWING_STEPS);
        camera_override_mode(CAMERA_MODE_CHAOS_STINIT, CAMERA_ADJUST_NONE);
        motion_step(work, motion);
        pad_disable();
    }
    return motion;
}

/* start the fight once the camera's swing is done. it makes the life
 * gauge, takes the fight's camera and asks for EGG_WALKER_WALK. the pads
 * and the time start too */
static void egg_walker_intro(struct task_work *work)
{
    if (camera_swing_done) {
        life_gauge_create(0, 0, HEALTH);
        egg_walker_camera_start(&work->pos, &work->ang);
        if (work->mode != EGG_WALKER_WALK) {
            boss_next_mode = EGG_WALKER_WALK;
            boss_last_mode = work->mode;
        }
        pad_enable();
        time_start();
    }
}

/* the boss's step in EGG_WALKER_WALK: a countdown, then a pick of an
 * attack or a move. player 0 within CLOSE_NOW_REACH brings the close attack
 * at once. that needs the walk stopped and the legs not blending */
static void egg_walker_walk(struct task *task, struct task_work *work)
{
    struct egg_walker *boss = task->extra;

    egg_walker_aim_turret(task);
    egg_walker_hint_legs();
    egg_walker_hint_cockpit();
    switch (work->submode) {
    case PICK_START:
        EGG_WALKER_VARS(work)->timer = PICK_TIME;
        work->submode = PICK_WAIT;
    case PICK_WAIT:
        if (njSqrt(SQUARE(player_work[0]->pos.x - work->pos.x) +
                   SQUARE(player_work[0]->pos.z - work->pos.z)) <
                CLOSE_NOW_REACH &&
            boss->walk == WALK_STOPPED && !egg_walker_all_legs_blending()) {
            if (work->mode != EGG_WALKER_CLOSE) {
                boss_next_mode = EGG_WALKER_CLOSE;
                boss_last_mode = work->mode;
            }
            work->submode = MODE_DONE;
            break;
        }
        if (--EGG_WALKER_VARS(work)->timer < 0) {
            if (boss->health <= MOVE_HEALTH &&
                ++moves_since_attack < MOVE_LIMIT) {
                work->submode = PICK_MOVE;
            } else {
                moves_since_attack = 0;
                work->submode = PICK_ATTACK;
            }
        }
        break;
    case PICK_ATTACK:
        choose_attack(task);
        work->submode = MODE_DONE;
        break;
    case PICK_MOVE:
        /* the new target is player 0, or MOVE_Z along z toward player 0. a
         * move that would end at or past WALK_Z_MIN or WALK_Z_MAX goes the
         * other way */
        if (boss->walk == WALK_STOPPED) {
            Float distance =
                _builtin_fabsf(work->pos.z - player_work[0]->pos.z);
            if (distance >= MOVE_Z) {
                boss->target = player_work[0]->pos;
            } else {
                boss->target = work->pos;
                if (work->pos.z > player_work[0]->pos.z) {
                    if (work->pos.z - MOVE_Z > WALK_Z_MIN) {
                        boss->target.z -= MOVE_Z;
                    } else {
                        boss->target.z += MOVE_Z;
                    }
                } else if (work->pos.z + MOVE_Z < WALK_Z_MAX) {
                    boss->target.z += MOVE_Z;
                } else {
                    boss->target.z -= MOVE_Z;
                }
            }
            boss->target.x =
                RANDOM(TARGET_X - TARGET_SWAY, TARGET_X + TARGET_SWAY);
            boss->walk = WALK_START;
            work->submode = PICK_MOVING;
        }
        break;
    case PICK_MOVING:
        if (boss->walk == WALK_STOPPED) {
            work->submode = PICK_START;
        }
        break;
    }
}

/* ask for an attack by player 0's distance across the ground. it is the
 * close attack within PICK_CLOSE_REACH, the flames within PICK_FLAMES_REACH,
 * and the cannons beyond */
static void choose_attack(struct task *task)
{
    struct task_work *work = task->work;
    Float distance = njSqrt(SQUARE(player_work[0]->pos.x - work->pos.x) +
                            SQUARE(player_work[0]->pos.z - work->pos.z));

    /* @hack spelling: each new mode set inside a comma expression, which keeps
     * boss_next_mode's address in R5 and boss_last_mode's in R4 (0x0c9006a0,
     * 0x0c9006a4), with their pool words in that order at 0x0c900708; as
     * two statements, both pairs trade and 6 of 68 words differ */
    if (distance < PICK_CLOSE_REACH) {
        if (work->mode != EGG_WALKER_CLOSE) {
            boss_last_mode = (boss_next_mode = EGG_WALKER_CLOSE, work->mode);
        }
    } else if (distance < PICK_FLAMES_REACH) {
        if (work->mode != EGG_WALKER_FLAMES) {
            boss_last_mode = (boss_next_mode = EGG_WALKER_FLAMES, work->mode);
        }
    } else {
        if (work->mode != EGG_WALKER_CANNONS) {
            boss_last_mode = (boss_next_mode = EGG_WALKER_CANNONS, work->mode);
        }
    }
}

/* while alternate_target_x is set, move the target's x to the other side
 * of TARGET_X. it moves each time the boss's z gets more than
 * ALTERNATE_STRIDE from last_alternate_z. a stopped walk clears the flag */
static void alternate_target(struct task *task)
{
    struct task_work *work = task->work;
    /* @hack spelling: the record through a struct motion and a struct
     * egg_walker, which keeps its pointer in R14 (0x0c900724); through one,
     * it takes R4, the function runs 2 bytes short and 41 of 43 words
     * differ */
    struct motion *motion = task->extra;
    struct egg_walker *boss = (struct egg_walker *)motion;
    Float offset;

    if (boss->alternate_target_x) {
        /* @hack redundant: a never-run store of WALK_START, with a jump to
         * walk_test, which puts walk's load in R1 and the zero that clears the
         * flag in R2 (0x0c900730, 0x0c900738); without it, they take R3 and R1
         * and 4 of 43 words differ */
        if (0) {
            boss->walk = WALK_START;
            goto walk_test;
        }
    walk_test:
        if (boss->walk == WALK_STOPPED) {
            boss->alternate_target_x = 0;
        }
        if (_builtin_fabsf(work->pos.z - boss->last_alternate_z) >
            ALTERNATE_STRIDE) {
            boss->last_alternate_z = work->pos.z;
            offset = TARGET_SWAY;
            if (boss->target.x > TARGET_X) {
                offset = -TARGET_SWAY;
            }
            boss->target.x = TARGET_X + offset;
        }
    }
}

/* stop the walk at part 0, with the legs and the body blending to rest */
static void stop_walking(struct task *task)
{
    struct task_work *work = task->work;
    /* @hack spelling: the record through a struct motion and a struct
     * egg_walker, which keeps its pointer at @R15 across the calls (MOV.L
     * R3,@R15 at 0x0c900784); through one, it takes R13 and 31 of 33 words
     * differ */
    struct motion *motion = task->extra;
    struct egg_walker *boss = task->extra;

    boss->walk = WALK_STOPPED;
    work->pos.x = egg_walker_part_pos(0)->x;
    work->pos.z = egg_walker_part_pos(0)->z;
    egg_walker_legs_rest();
    motion->wanted = EGG_WALKER_ACTION_STAND;
}

/* run the boss's walk, each step in every mode. pad 0's y button sends a
 * stopped boss to player 0. setting out picks the way, and the pace by the
 * hits left. under way, the walk waits for the legs to blend. it stops
 * within STOP_REACH of the target, or once part 0 passes the bound ahead */
static void walk_update(struct task *task)
{
    struct task_work *work = task->work;
    struct egg_walker *boss = task->extra;
    NJS_ACTION *action;
    Float frame;

    alternate_target(task);
    switch (boss->walk) {
    case WALK_STOPPED:
        if (per_info[0]->press & PDD_DGT_TY) {
            VECTOR_COPY(&boss->target, &player_work[0]->pos);
            boss->walk = WALK_START;
        }
        break;
    case WALK_START:
        switch ((int)boss->health) {
        /* @hack redundant: case 5 and case 4 below the default, whose arm they
         * share. the CMP/EQ #5 at 0x0c90089c then falls into the arm with no
         * branch after it; without them, 49 of 259 words differ */
        default:
        case 5:
        case 4:
            boss->pace = SLOW_PACE;
            break;
        case 2:
        case 3:
            boss->pace = MIDDLE_PACE;
            break;
        case 0:
        case 1:
            boss->pace = FAST_PACE;
            break;
        }
        if (face_target(task) == WALK_LOWER_Z) {
            boss->motion.wanted = EGG_WALKER_ACTION_WALK_LOWER_Z;
            boss->walk = WALK_LOWER_Z;
        } else {
            boss->motion.wanted = EGG_WALKER_ACTION_WALK_HIGHER_Z;
            boss->walk = WALK_HIGHER_Z;
        }
        egg_walker_legs_walk();
        dat_0c992708 = 0.0F;
        break;
    case WALK_LOWER_Z:
    case WALK_HIGHER_Z:
        if (egg_walker_all_legs_blending()) {
            break;
        }
        /* @hack spelling: the three tests as three arms that each stop the
         * walk, which keeps egg_walker_part_pos's address in R11 (0x0c900942);
         * joined by ||, it loads for each call, the object runs 32 bytes long
         * and 102 of 259 words differ */
        if (njSqrt(SQUARE(boss->target.x - work->pos.x) +
                   SQUARE(boss->target.z - work->pos.z)) < STOP_REACH) {
            stop_walking(task);
            return;
        } else if (egg_walker_part_pos(0)->z < WALK_Z_MIN &&
                   boss->walk == WALK_LOWER_Z) {
            stop_walking(task);
            return;
        } else if (egg_walker_part_pos(0)->z > WALK_Z_MAX &&
                   boss->walk == WALK_HIGHER_Z) {
            stop_walking(task);
            return;
        }
        face_target(task);
        action = boss->motion.actions[boss->motion.current].action;
        frame = boss->motion.frame;
        boss->motion.frame += boss->pace;
        if ((frame < FIRST_FOOTFALL && boss->motion.frame >= FIRST_FOOTFALL) ||
            (frame < SECOND_FOOTFALL &&
             boss->motion.frame >= SECOND_FOOTFALL)) {
            sound_play(SE_FOOTFALL, 0, 0, 0);
            vibration_start(0, FOOTFALL_POWER, FOOTFALL_FREQ, 1);
            egg_walker_camera_shake(FOOTFALL_SHAKE);
        }
        if (boss->motion.frame >= action->motion->nbFrame - 1.0F) {
            boss->motion.frame = 0.0F;
            work->pos.x = egg_walker_part_pos(0)->x;
            work->pos.z = egg_walker_part_pos(0)->z;
        }
        break;
    }
}

/* turn ang.y FACE_PACE of the short way to the target's bearing. with the
 * target at no lower z, it turns toward the opposite bearing. it returns
 * the walk that goes there, WALK_LOWER_Z or WALK_HIGHER_Z */
static Uint8 face_target(struct task *task)
{
    struct task_work *work = task->work;
    /* @hack spelling: the record through a struct motion and a struct
     * egg_walker, which loads its pointer into R1 (0x0c900aaa); through one,
     * it takes R4 and 4 of 78 words differ */
    const struct motion *motion = task->extra;
    const struct egg_walker *boss = (const struct egg_walker *)motion;
    NJS_VECTOR delta;
    Angle yaw;
    int direction;
    int turn;

    VECTOR_DIFF(&delta, &boss->target, &work->pos);
    yaw =
        ((Angle)(atan2f(delta.z, delta.x) * ANGLE_PER_RADIAN) + QUARTER_TURN) &
        0xffff;
    if (delta.z >= 0.0F) {
        direction = WALK_HIGHER_Z;
        yaw += HALF_TURN;
    } else {
        direction = WALK_LOWER_Z;
    }
    /* @hack spelling: turn assigned inside the sum, which loads ang.y in the
     * BT/S's delay slot at 0x0c900b00. assigned in a comma expression before
     * SHORT_TURN, as the cannons and the flames assign theirs, 11 of 78 words
     * differ; as a statement of its own, the load follows the branch and 16
     * of 78 words differ */
    work->ang.y +=
        ((turn = (Uint16)(yaw - work->ang.y)) <= 0x8000 ? turn
                                                        : turn - 0x10000) *
        FACE_PACE;
    return direction;
}

int egg_walker_legs_spent(struct task *task)
{
    struct task_work *work = task->work;
    const struct egg_walker *boss = task->extra;
    const Uint8 *leg;
    int i;
    int count;

    /* @hack spelling: count zeroed before i, which copies the zero to i's R5
     * and then count's R6 (0x0c900b68, 0x0c900b6a); the other way round,
     * the two copies trade and 2 of 32 words differ */
    count = 0;
    i = 0;
    leg = boss->legs;
    /* @hack redundant: a never-run store to work->mode. it keeps a frame,
     * the load of task->work (0x0c900b64) and the record's pointer stored
     * at @R15 (0x0c900b6e); without it, all three go, the function runs 4
     * bytes short and 32 of 32 words differ */
    if (1) {
    } else {
        work->mode = 1;
    }
    for (; i < LEGS; i++, leg++) {
        switch (*leg) {
        case LEG_NONE:
            break;
        case LEG_GLOW:
        case LEG_SELECTED:
            count++;
            break;
        case LEG_FADE:
            break;
        }
    }
    return count == 0;
}

void egg_walker_aim_turret(struct task *task)
{
    struct task_work *work = task->work;
    struct egg_walker *boss = task->extra;
    NJS_VECTOR delta;
    /* @hack redundant: yaw's initializer, which the next store overwrites.
     * it puts player_work's address in R13 and egg_walker_part_pos's in R12
     * (0x0c900bce, 0x0c900bd4); without it, the two trade and 8 of 99 words
     * differ */
    Angle yaw = boss->turret_yaw;
    int turn;

    VECTOR_DIFF(&delta, &player_work[0]->pos, egg_walker_part_pos(0));
    yaw = QUARTER_TURN - (Angle)(atan2f(delta.z, delta.x) * ANGLE_PER_RADIAN);
    boss->turret_aim = -yaw - work->ang.y + HALF_TURN;
    turn = (Uint16)(boss->turret_aim - boss->turret_yaw);
    boss->turret_yaw += SHORT_TURN(turn) * AIM_PACE;
}

static void egg_walker_step(struct task *task)
{
    struct task_work *work = task->work;
    struct egg_walker *boss;

    switch (work->mode) {
    case EGG_WALKER_INIT:
        if (!egg_walker_init(task)) {
            return;
        }
        egg_walker_cannons_init();
        egg_walker_flames_init();
        egg_walker_glows_init();
        egg_walker_smoke_reset();
        egm2_cars_create();
        time_stop();
        break;
    case EGG_WALKER_INTRO:
        time_stop();
        egg_walker_intro(work);
        break;
    case EGG_WALKER_WALK:
        egg_walker_walk(task, work);
        break;
    case EGG_WALKER_IDLE:
        break;
    case EGG_WALKER_JUMP:
        egg_walker_jump(task);
        break;
    case EGG_WALKER_CANNONS:
        egg_walker_cannons(task);
        break;
    case EGG_WALKER_FLAMES:
        egg_walker_flames(task);
        break;
    case EGG_WALKER_BOMBS:
        egg_walker_bombs(task);
        break;
    case EGG_WALKER_CLOSE:
        egg_walker_close(task);
        break;
    case EGG_WALKER_DEFEAT:
        egg_walker_defeat(task);
        break;
    }
    /* read after the switch, since EGG_WALKER_INIT's step allocates the
     * record */
    boss = task->extra;
    if (work->mode != boss_next_mode) {
        /* @hack redundant: a test of submode against MODE_DONE whose result
         * goes unread (CMP/EQ #-1 at 0x0c900dd6); without it, the function
         * runs 4 bytes short and 53 of 143 words differ */
        if (work->submode == MODE_DONE) {
        }
        boss_last_mode = work->mode;
        work->mode = boss_next_mode;
        work->submode = 0;
    }
    walk_update(task);
    motion_step(work, &boss->motion);
    motion_weld(work, &boss->motion);
    egg_walker_parts_update(task);
    egg_walker_leg_pose_step();
    egg_walker_leg_shapes_lift(work);
    egg_walker_feet_attack(work);
    collision_register(work);
    egg_walker_smoke_step();
    egg_walker_draw(task);
    egg_walker_glows_step(task);
    life_gauge_health = boss->health;
}

/* draw the sky and the boss, on a frame's first step. the body takes its
 * motion's pose. the turret sits on part 0, and carries the cannon, the
 * nozzle and Eggman. the leg copies and the feet's shadows draw too */
static void egg_walker_draw(struct task *task)
{
    struct task_work *work = task->work;
    struct egg_walker *boss = task->extra;
    NJS_ACTION *action;
    /* @hack redundant: a point never read, which gives the frame 16 bytes
     * (ADD #-16,R15 at 0x0c900e8e); without it, the frame takes 4 and 3 of
     * 164 words differ */
    NJS_POINT3 unused;

    if (frame_step) {
        return;
    }
    egg_walker_sky_draw();
    njSetTexture(&egm2_common_texlist);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateY, HALF_TURN - work->ang.y);
    /* @bug memory: a draw while the game is paused, before a step has
     * allocated the record, reads the motion through a null extra (MOV.L
     * @(44,R13),R14 at 0x0c900e9c, MOV.W @(4,R14),R0 at 0x0c900ed6) */
    if (boss->motion.state != MOTION_BLENDING) {
        action = boss->motion.actions[boss->motion.current].action;
    } else {
        action = boss->motion.blend;
    }
    draw_action(action, boss->motion.frame);
    njPopMatrix(1);
    egg_walker_leg_copies_draw(task);
    njPushMatrix(NULL);
    njTranslateV(NULL, egg_walker_part_pos(0));
    TURN(njRotateY, HALF_TURN - work->ang.y);
    njPushMatrix(NULL);
    TURN(njRotateY, -boss->turret_yaw);
    draw_tree(&egm2_turret_object);
    njPushMatrix(NULL);
    njTranslateV(NULL, (NJS_VECTOR *)&cannon_mount);
    TURN(njRotateX, boss->cannon_pitch);
    draw_tree(&egm2_cannon_object);
    njPopMatrix(1);
    njPushMatrix(NULL);
    njTranslateV(NULL, (NJS_VECTOR *)&nozzle_mount);
    TURN(njRotateX, boss->nozzle_pitch);
    draw_tree(&egm2_nozzle_object);
    njPopMatrix(1);
    egg_walker_eggman_draw(task);
    njPopMatrix(1);
    njPopMatrix(1);
    draw_foot_shadows(task);
}

/* free the motion's blend before task_destroy frees the record, and end
 * Tikal's hint */
static void egg_walker_destroy(struct task *task)
{
    struct task_work *work = task->work;
    struct motion *motion = task->extra;

    /* @hack redundant: a test of submode against MODE_DONE whose result goes
     * unread (CMP/EQ #-1 at 0x0c900fd6); without it, the function runs 4
     * bytes short and 16 of 19 words differ */
    if (work->submode == MODE_DONE) {
    }
    motion->state = MOTION_FREE;
    motion_step(work, motion);
    egg_walker_hint_end();
    /* task_destroy nulls the step task_kill sets here, once the destructor
     * returns */
    task_kill(task);
}

/* draw a shadow under each foot, at STAND_Y, on a frame's first step */
static void draw_foot_shadows(struct task *task)
{
    struct task_work *work = task->work;
    NJS_POINT3 point;
    int i;
    /* @hack redundant: unused, set from ang.y each pass and never read. it
     * keeps work, egg_walker_part_pos's address and shadow_draw_at's in R13,
     * R12 and R11 (0x0c90107a, 0x0c901088, 0x0c90107e); without it, they
     * take R11, R13 and R12 and 9 of 70 words differ */
    /* NOLINTNEXTLINE(clang-diagnostic-unused-but-set-variable) */
    Angle unused;

    if (frame_step) {
        return;
    }
    /* two shadows a pass: in the image, the loop's body calls each function
     * twice. the two rounds of calls start at 0x0c90108c and 0x0c9010b0 */
    for (i = 0; i < LEGS;) {
        point = *egg_walker_part_pos(i + PART_FEET);
        point.y = STAND_Y;
        shadow_draw_at(&work->ang, &point, SHADOW_SCALE);
        i++;
        point = *egg_walker_part_pos(i + PART_FEET);
        point.y = STAND_Y;
        shadow_draw_at(&work->ang, &point, SHADOW_SCALE);
        i++;
        unused = work->ang.y;
    }
    /* @hack redundant: a never-run call of egg_walker_part_pos, which puts
     * shadow_draw_at's address in R11 and its pool word first, and
     * egg_walker_part_pos's in R12 (0x0c9010aa, 0x0c90108e); without it, the
     * two trade and 4 of 70 words differ */
    if (0) {
        egg_walker_part_pos(PART_FEET);
    }
}
