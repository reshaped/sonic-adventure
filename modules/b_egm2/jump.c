/* the Egg Walker's jump. the boss stands, crouches and leaps for its
 * target, then lands with a quake. low on hits, it scatters bombs in flight,
 * and may jump again */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "b_egm2/bombs.h"
#include "b_egm2/camera.h"
#include "b_egm2/egg_walker.h"
#include "b_egm2/jump.h"
#include "draw/motion.h"
#include "player/player.h"
#include "sound/sound.h"
#include "system/task.h"
#include "system/util.h"
#include "vmu/vibration.h"

/* the boss's submodes in EGG_WALKER_JUMP */
enum jump_mode {
    JUMP_AIM, /* pick the target, and the velocity that reaches it */
    JUMP_STAND, /* until the motion stands */
    JUMP_CROUCH,
    JUMP_FLY,
    JUMP_LAND
};

/* the boss flies for JUMP_TIME steps. each step, JUMP_GRAVITY comes off its
 * upward speed */
#define JUMP_TIME (60.0F * 1.2F)
#define JUMP_GRAVITY (9.8F * 4.0F / 60.0F)

/* each step plays CROUCH_PACE frames of the jump's motion as the boss
 * crouches and as it lands. as the boss flies, each step plays FLY_PACE
 * frames. the boss leaves the street at frame RISE_FRAME */
#define CROUCH_PACE 0.3F
#define RISE_FRAME 27.0F
#define FLY_PACE 0.6666667F

/* with BOMB_HEALTH hits left or fewer, the boss scatters BOMBS bombs once
 * in its flight. it scatters them as BOMB_TIME steps of the flight are
 * left */
#define BOMB_HEALTH 4.0F
#define BOMB_TIME 51
#define BOMBS 10

/* the landing lasts LAND_TIME steps. the boss then jumps again if player 0
 * is within AGAIN_REACH across the ground. it does so only with AGAIN_HEALTH
 * hits left or fewer */
#define LAND_TIME 60
#define AGAIN_HEALTH 3.0F
#define AGAIN_REACH 80.0F

#define SE_JUMP_RISE 0x1e2
#define SE_JUMP_LAND 0x1d5

/* as the boss lands, pad 0 shakes at LAND_POWER and LAND_FREQ, and the
 * camera by LAND_SHAKE */
#define LAND_POWER 11
#define LAND_FREQ 30
#define LAND_SHAKE 3

/* the spots of JUMP_TO_RANDOM */
static const NJS_POINT3 jump_spots[3] = {
    {-440.0F, 236.0F, 1561.0F},
    {-689.0F, 192.0F, 836.0F},
    {-302.0F, 245.0F, 1091.0F},
};
/* the walk's ends, for JUMP_TO_END */
static const NJS_POINT3 jump_ends[2] = {
    {-443.0F, STAND_Y, WALK_Z_MIN},
    {-443.0F, STAND_Y, WALK_Z_MAX},
};

static Bool bombs_scattered;

static void jump_launch(struct task *task);

void egg_walker_jump(struct task *task)
{
    struct task_work *work = task->work;
    struct egg_walker *boss = task->extra;
    NJS_VECTOR speed;
    NJS_ACTION *action;
    int i;
    int spot;

    switch (work->submode) {
    case JUMP_AIM:
        switch (boss->jump) {
        case JUMP_TO_END:
            if (work->pos.z > WALK_Z_MIDDLE) {
                boss->target = jump_ends[0];
            } else {
                boss->target = jump_ends[1];
            }
            break;
        case JUMP_TO_RANDOM:
            spot = (int)(rand() * (3 * RAND_SCALE));
            VECTOR_COPY(&boss->target, &jump_spots[spot]);
            break;
        case JUMP_TO_PLAYER:
            boss->target.x = player_work[0]->pos.x;
            boss->target.z = player_work[0]->pos.z;
            boss->target.y = STAND_Y;
            break;
        }
        bombs_scattered = FALSE;
        jump_launch(task);
        boss->walk = WALK_STOPPED;
        boss->motion.wanted = EGG_WALKER_ACTION_STAND;
        work->submode = JUMP_STAND;
        break;
    case JUMP_STAND:
        if (boss->motion.current == EGG_WALKER_ACTION_STAND) {
            boss->motion.wanted = EGG_WALKER_ACTION_JUMP;
            EGG_WALKER_VARS(work)->timer = (int)JUMP_TIME;
            work->submode = JUMP_CROUCH;
        }
        break;
    case JUMP_CROUCH:
        boss->motion.frame += CROUCH_PACE;
        if (boss->motion.frame >= RISE_FRAME) {
            sound_play(SE_JUMP_RISE, 0, 0, 0);
            work->submode = JUMP_FLY;
        }
        break;
    case JUMP_FLY:
        VECTOR_SUM(&work->pos, &work->pos, &boss->velocity);
        /* @hack spelling: -JUMP_GRAVITY added, an FADD of -0.6533 from the
         * pool (0x0c901b04); with -=, an FSUB of 0.6533, and 1 of 447 words
         * differ */
        boss->velocity.y += -JUMP_GRAVITY;
        boss->motion.frame += FLY_PACE;
        if (boss->health <= BOMB_HEALTH && !bombs_scattered &&
            EGG_WALKER_VARS(work)->timer <= BOMB_TIME) {
            for (i = 0; i < BOMBS; i++) {
                speed.x = (rand() * RAND_SCALE) - 0.5F;
                speed.y = (rand() * RAND_SCALE) - 0.5F;
                speed.z = (rand() * RAND_SCALE) - 0.5F;
                egg_walker_bomb_create(&work->pos, &speed, BOMB_SCATTER);
            }
            bombs_scattered = TRUE;
        }
        if (--EGG_WALKER_VARS(work)->timer <= 0) {
            sound_play(SE_JUMP_LAND, 0, 0, 0);
            vibration_start(0, LAND_POWER, LAND_FREQ, 1);
            egg_walker_camera_shake(LAND_SHAKE);
            VECTOR_COPY(&work->pos, &boss->target);
            boss->velocity.x = boss->velocity.y = boss->velocity.z = 0.0F;
            EGG_WALKER_VARS(work)->timer = LAND_TIME;
            work->submode = JUMP_LAND;
        }
        break;
    case JUMP_LAND:
        action = boss->motion.actions[EGG_WALKER_ACTION_JUMP].action;
        boss->motion.frame += CROUCH_PACE;
        if (boss->motion.frame >= action->motion->nbFrame - 1.0F) {
            boss->motion.frame = action->motion->nbFrame - 1.0F;
        }
        if (--EGG_WALKER_VARS(work)->timer <= 0) {
            boss->velocity.x = boss->velocity.y = boss->velocity.z = 0.0F;
            boss->motion.frame = 0.0F;
            if (boss->jump == JUMP_TO_RANDOM) {
                work->submode = JUMP_AIM;
                boss->jump = JUMP_TO_PLAYER;
            } else if (boss->health <= AGAIN_HEALTH &&
                       njSqrt(SQUARE(work->pos.x - player_work[0]->pos.x) +
                              SQUARE(work->pos.z - player_work[0]->pos.z)) <
                           AGAIN_REACH) {
                work->submode = JUMP_AIM;
            } else {
                work->submode = MODE_DONE;
            }
        }
        break;
    }
    if (work->submode == MODE_DONE) {
        if (work->mode != EGG_WALKER_WALK) {
            boss_next_mode = EGG_WALKER_WALK;
            boss_last_mode = work->mode;
        }
        work->submode = JUMP_AIM;
    }
}

/* @hack spelling: the record read through a function SHC inlines, which
 * holds it in R1 and work in R5 (MOV.L @(44,R4),R1 at 0x0c901c60); read in
 * place, the record takes R5 and work R6, and 15 of jump_launch's 47 words
 * differ */
static struct egg_walker *jump_boss(struct task *task)
{
    return task->extra;
}

/* set the boss's velocity for a flight of JUMP_TIME steps to its target,
 * rising and falling under JUMP_GRAVITY */
static void jump_launch(struct task *task)
{
    struct task_work *work = task->work;
    struct egg_walker *boss = jump_boss(task);
    NJS_VECTOR to;

    /* @hack redundant: a goto to the next statement. SHC deletes the jump, but
     * with it, SHC addresses the local to through R3 for the first store
     * (MOV R15,R3 at 0x0c901c62); without it, that store goes through R2
     * and 4 of 47 words differ */
    goto aim;
aim:
    VECTOR_DIFF(&to, &boss->target, &work->pos);
    boss->velocity.x = to.x * (1.0F / JUMP_TIME);
    boss->velocity.z = to.z * (1.0F / JUMP_TIME);
    boss->velocity.y =
        (to.y * (1.0F / JUMP_TIME)) + (JUMP_GRAVITY * JUMP_TIME * 0.5F);
}
