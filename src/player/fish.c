/* fish: what the fish of Big's stages share, and the frog he fishes for */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "collision/collision.h"
#include "collision/polygon.h"
#include "collision/world.h"
#include "draw/draw.h"
#include "player/big.h"
#include "player/big_kaeru_textures.h"
#include "player/fish.h"
#include "player/fishing.h"
#include "player/player.h"
#include "sound/sound.h"
#include "stage/stage.h"
#include "system/task.h"
#include "system/util.h"
#include "vmu/vibration.h"

#define SE_FROG_KICK 808
#define SE_FROG_HOOKED 1312

enum frog_mode {
    FROG_INIT,
    FROG_WANDER,
    FROG_CHASE,
    FROG_BITE,
    FROG_HOOKED,
    FROG_CARRIED
};

extern NJS_ACTION frog_swim_action;
extern NJS_ACTION frog_kick_action;

/* the frog's shape, which nothing reads. it starts this object's data, at
 * 0x8c5868b8 */
/* clang-format off */
static struct collision_shape frog_shapes[] = {
    {SHAPE_SPHERE, 0, PUSH_ALL | PUSH_FIRM(PUSH_ALL), {0.0F, 0.0F, 0.0F}, 5.0F,
     0.0F, 0.0F, {0}, 0, NULL},
};
/* clang-format on */
static Sint16 frog_weld_pairs[8] = {
#include "player/fish/frog_weld_pairs.inc"
};
static struct model_weld frog_welds[] = {
    {4, 1, 12, frog_weld_pairs},
    {0},
};

static void frog_draw(struct task *task);
static void frog_destroy(struct task *task);
static void frog_set_action(struct task *task, NJS_ACTION *action);
static void frog_animate(struct task *task);
static void frog_kick(struct task *task);
static Bool frog_chase(struct task *task);
static void frog_pull(struct task *task);
static void frog_land(struct task *task);
static void frog_carry(struct task *task);
static void frog_let_go(struct task *task);

static void frog_draw(struct task *task)
{
    struct task_work *work = task->work;

    njSetTexture(&big_kaeru_texlist);
    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
    TURN(njRotateZ, work->ang.z);
    TURN(njRotateX, work->ang.x);
    TURN(njRotateY, work->ang.y);
    njTranslate(NULL, 2.0F, 0.0F, 0.0F);
    model_weld(FISH_VARS(work)->action->object, FISH_VARS(work)->action->motion,
               FISH_VARS(work)->frame, frog_welds);
    draw_action(FISH_VARS(work)->action, FISH_VARS(work)->frame);
    njPopMatrix(1);
}

static void frog_destroy(struct task *task)
{
}

static void frog_set_action(struct task *task, NJS_ACTION *action)
{
    struct task_work *work = task->work;

    if (FISH_VARS(work)->action != action) {
        FISH_VARS(work)->action = action;
        FISH_VARS(work)->frame = 0.0F;
    }
}

static void frog_animate(struct task *task)
{
    struct task_work *work = task->work;

    FISH_VARS(work)->frame += 1.0F;
    if (FISH_VARS(work)->frame >= FISH_VARS(work)->action->motion->nbFrame) {
        FISH_VARS(work)->frame = 0.0F;
    }
}

void fish_wander(struct task *task)
{
    struct task_work *work = task->work;

    if (--FISH_VARS(work)->turn_steps <= 0) {
        FISH_VARS(work)->turn_steps = 3;
        work->ang.y += NJM_DEG_ANG(1);
        work->ang.y &= 0xffff;
    }
}

/* every 120 steps the frog's aim turns half round. with a chance of 0.15
 * it then kicks hard, setting FISHING_KICK until frog_pull has counted its
 * timer out. each step it turns toward its aim */
static void frog_kick(struct task *task)
{
    struct task_work *work = task->work;
    struct fish_motion *motion = FISH_MOTION(task);

    if (--FISH_VARS(work)->turn_steps <= 0) {
        FISH_VARS(work)->turn_steps = 120;
        motion->aim.y ^= 0x8000;
        if (rand() * RAND_SCALE < 0.15F) {
            sound_play_at(SE_FROG_KICK, 0, 0, 0, work->pos.x, work->pos.y,
                          work->pos.z);
            work->timer = 120;
            fishing_flags |= FISHING_KICK;
            frog_set_action(task, &frog_kick_action);
            vibration_start(0, 5, 15, 4);
        }
    }
    if ((Uint16)(work->ang.y - motion->aim.y) > 0x8000) {
        work->ang.y += 0x100;
    } else {
        work->ang.y -= 0x100;
    }
    work->ang.y &= 0xffff;
}

Bool fish_keep_ahead_of_player(struct task *task, NJS_POINT3 *point)
{
    struct task_work *player = player_work[0];
    struct fish_motion *motion = FISH_MOTION(task);
    Angle off;
    Angle bearing;

    bearing = 0x4000 - (Angle)(atan2f(point->x - player->pos.x,
                                      point->z - player->pos.z) *
                               ANGLE_PER_RADIAN);
    /* @hack spelling: off formed, then cut to 16 bits, in two statements, which
     * load NJM_DEG_ANG(30) into R2 and the player's heading into R3
     * (0x8c0f75e4, 0x8c0f75f0); as one statement, the two trade and 12 of
     * 73 words differ */
    off = bearing - player->ang.y;
    off = (Uint16)off;
    if (off > NJM_DEG_ANG(30) && off < NJM_DEG_ANG(180)) {
        motion->aim.y = (Uint16)(bearing + NJM_DEG_ANG(100));
        return FALSE;
    }
    if (off < NJM_DEG_ANG(330) && off > NJM_DEG_ANG(180)) {
        motion->aim.y = (Uint16)(bearing - NJM_DEG_ANG(100));
        return FALSE;
    }
    if (njDistanceP2P(&player->pos, point) > 300.0F) {
        motion->aim.y = (Uint16)(player->ang.y + 0x8000);
        return FALSE;
    }
    return TRUE;
}

void fish_turn_to_lure(struct task *task)
{
    struct task_work *work = task->work;
    struct fish_motion *motion = FISH_MOTION(task);
    struct task_work *lure;

    if (big_lure != NULL) {
        lure = big_lure->work;
        /* @hack spelling: the aim stored through FISH_MOTION(task), which the
         * image reloads from task at the store (MOV.L @(36,R13),R8 at
         * 0x8c0f76b2); through motion, the frame is 16 bytes, not 24, and
         * 55 of 64 words differ */
        FISH_MOTION(task)->aim.y = ((Angle)(atan2f(work->pos.x - lure->pos.x,
                                                   work->pos.z - lure->pos.z) *
                                            ANGLE_PER_RADIAN) +
                                    0xc000) &
                                   0xffff;
        if ((Uint16)(work->ang.y - motion->aim.y) > 0x8000) {
            work->ang.y += 0x80;
        } else {
            work->ang.y -= 0x80;
        }
        work->ang.y &= 0xffff;
    }
}

void fish_swim(struct task *task)
{
    struct fish_motion *motion = FISH_MOTION(task);
    struct task_work *work = task->work;
    Float speed;
    Bool hit;
    NJS_MATRIX matrix;
    NJS_VECTOR move;
    NJS_POINT3 pos;
    struct angles slope;

    speed = njScalor(&motion->speed);
    move.y = 0.0F;
    move.z = 0.0F;
    if (FISH_VARS(work)->frame > 25.0F && FISH_VARS(work)->frame < 45.0F) {
        move.x = -0.2F;
    } else if (-speed < -0.05F) {
        move.x = 0.004F - speed;
    } else {
        move.x = -0.1F;
    }
    move.x *= 2.0F;
    njUnitMatrix(&matrix);
    TURN_MATRIX(njRotateY, &matrix, work->ang.y);
    njCalcVector(&matrix, &move, &move);
    slope.x = slope.y = slope.z = 0;
    pos = work->pos;
    hit = world_sweep_all_hit(&pos, &move, &slope, FISH_RADIUS);
    motion->speed.x = move.x * 0.5F;
    motion->speed.y = move.y * 0.5F;
    motion->speed.z = move.z * 0.5F;
    if (!hit) {
        VECTOR_ADD(&work->pos, &motion->speed);
    } else {
        motion->speed.z = 0.0F;
        motion->speed.y = 0.0F;
        motion->speed.x = 0.0F;
    }
}

/* swim the frog at the lure, at least 0.1 under the water's surface. TRUE
 * while the chase goes on. the frog holds still while its aim is more than
 * 22.5 degrees off its heading. FALSE ends the chase: with no lure, with
 * FISHING_CAMERA clear or FISHING_BOTTOM set, or with the frog within 50
 * of player 0 */
static Bool frog_chase(struct task *task)
{
    struct fish_motion *motion = FISH_MOTION(task);
    struct task_work *work = task->work;
    struct task_work *player = player_work[0];
    struct task *lure;
    struct task_work *at;
    Float speed;
    Float step;
    Angle off;
    NJS_POINT3 pos;
    NJS_VECTOR away;
    struct angles slope;

    speed = njScalor(&motion->speed);
    lure = big_lure;
    if (lure == NULL) {
        return FALSE;
    }
    if (!(fishing_flags & FISHING_CAMERA) || (fishing_flags & FISHING_BOTTOM) ||
        njDistanceP2P(&work->pos, &player->pos) < 50.0F) {
        return FALSE;
    }
    at = big_lure->work;
    away.x = at->pos.x - work->pos.x;
    away.y = at->pos.y - work->pos.y;
    away.z = at->pos.z - work->pos.z;
    off = work->ang.y - motion->aim.y;
    if (off < 0) {
        off += 0x10000;
    }
    if (off > NJM_DEG_ANG(22.5) && off < NJM_DEG_ANG(337.5)) {
        return TRUE;
    }
    njUnitVector(&away);
    if (FISH_VARS(work)->frame > 25.0F && FISH_VARS(work)->frame < 45.0F) {
        step = -0.2F;
    } else if (-speed < -0.05F) {
        step = 0.004F - speed;
    } else {
        step = -0.1F;
    }
    step *= 2.0F;
    motion->speed.x = -(away.x * step);
    motion->speed.y = -(away.y * step);
    motion->speed.z = -(away.z * step);
    slope.x = slope.y = slope.z = 0;
    pos = work->pos;
    if (world_sweep_all_hit(&pos, &motion->speed, &slope, FISH_RADIUS) ==
        FALSE) {
        motion->speed.x *= 0.5F;
        motion->speed.y *= 0.5F;
        motion->speed.z *= 0.5F;
        work->pos.x += motion->speed.x;
        work->pos.y += motion->speed.y;
        if (work->pos.y > motion->surface - 0.1F) {
            work->pos.y = motion->surface - 0.1F;
        }
        work->pos.z += motion->speed.z;
        return TRUE;
    }
    VECTOR_CLEAR(&motion->speed);
    /* @bug uninitialized: after a blocked sweep, the function returns no value.
     * R0 still holds 12, speed.z's offset, from MOV #12,R0 at 0x8c0f7ade.
     * frog_step's TST at 0x8c0f82d8 takes it as TRUE, so the chase goes on */
    /* NOLINTNEXTLINE(clang-diagnostic-return-type) */
}

/* the hooked frog pulls the lure along its heading. its speed is the one
 * fish_swim gives for its action's frame, or 0 where its sweep hits. the
 * lure moves by that speed while the frog pulls it. the frog itself moves
 * to the hook. once its timer runs out, FISHING_KICK clears */
static void frog_pull(struct task *task)
{
    struct fish_motion *motion = FISH_MOTION(task);
    struct task_work *work = task->work;
    struct task *lure;
    Float speed;
    NJS_MATRIX matrix;
    NJS_VECTOR move;
    NJS_POINT3 pos;
    struct angles slope;

    speed = njScalor(&motion->speed);
    lure = big_lure;
    if (lure == NULL) {
        return;
    }
    move.y = 0.0F;
    move.z = 0.0F;
    if (FISH_VARS(work)->frame > 25.0F && FISH_VARS(work)->frame < 45.0F) {
        move.x = -0.2F;
    } else if (-speed < -0.05F) {
        move.x = 0.004F - speed;
    } else {
        move.x = -0.1F;
    }
    move.x *= 2.0F;
    njUnitMatrix(&matrix);
    TURN_MATRIX(njRotateY, &matrix, work->ang.y);
    njCalcVector(&matrix, &move, &motion->speed);
    slope.x = slope.y = slope.z = 0;
    pos = work->pos;
    if (world_sweep_all_hit(&pos, &motion->speed, &slope, FISH_RADIUS)) {
        motion->speed.z = 0.0F;
        motion->speed.y = 0.0F;
        motion->speed.x = 0.0F;
    } else {
        motion->speed.x *= 0.5F;
        motion->speed.y *= 0.5F;
        motion->speed.z *= 0.5F;
    }
    fishing_hook_point(&work->pos);
    if (work->timer) {
        work->timer--;
    } else {
        fishing_flags &= ~FISHING_KICK;
        frog_set_action(task, &frog_swim_action);
    }
}

/* lay the landed frog on its side, kicking, and set FISHING_GOAL */
static void frog_land(struct task *task)
{
    struct task_work *work = task->work;

    work->ang.x = work->ang.y = 0;
    work->ang.z = 0x4000;
    fishing_flags |= FISHING_GOAL;
    frog_set_action(task, &frog_kick_action);
}

/* hold the frog in player 0's hand: at his weld_point0, 2 further along
 * his z */
static void frog_carry(struct task *task)
{
    struct player *record = player_motion[0]->record;
    struct task_work *work = task->work;
    struct task_work *player = player_work[0];
    NJS_VECTOR hand;
    NJS_VECTOR offset;

    njPushMatrix(&_unit_matrix_);
    TURN(njRotateZ, player->ang.z);
    TURN(njRotateX, player->ang.x);
    TURN(njRotateY, 0x8000 - player->ang.y);
    hand.x = record->weld_point0.x;
    hand.y = record->weld_point0.y + record->consts.center_y;
    /* @hack spelling: z copied, then 2 added, in two statements, which store
     * the weld's z into hand and add 2 to it there (0x8c0f7d38 to
     * 0x8c0f7d46); as one statement, 51 of 105 words differ */
    hand.z = record->weld_point0.z;
    hand.z += 2.0F;
    njCalcVector(NULL, &hand, &offset);
    njPopMatrix(1);
    work->pos.x = player->pos.x + offset.x;
    work->pos.y = player->pos.y + offset.y;
    work->pos.z = player->pos.z + offset.z;
    work->ang.y = 0x4000 - player->ang.y;
    work->ang.x = work->ang.z = 0;
}

Bool fish_watch_lure(struct task *task)
{
    struct task_work *work = task->work;
    struct task *lure;
    struct task_work *at;
    Float dot;
    Float distance;
    Float cosine;
    Float range;
    NJS_VECTOR away;
    NJS_VECTOR front;
    NJS_MATRIX matrix;

    /* @hack spelling: the return in an else, which reads the lure's work after
     * the test (MOV.L @(32,R5),R4 at 0x8c0f7dfa); with an early return,
     * lure shares R4 with at, that read moves into the flag test's slot,
     * and 7 of 153 words differ */
    lure = big_lure;
    if (lure != NULL) {
        at = lure->work;
    } else {
        return FALSE;
    }
    if (!(fishing_flags & FISHING_CAMERA)) {
        return FALSE;
    }
    away.x = at->pos.x - work->pos.x;
    away.y = at->pos.y - work->pos.y;
    away.z = at->pos.z - work->pos.z;
    njUnitMatrix(&matrix);
    front.x = -1.0F;
    front.y = front.z = 0.0F;
    TURN_MATRIX(njRotateY, &matrix, work->ang.y);
    njCalcVector(&matrix, &front, &front);
    dot = njInnerProduct(&away, &front);
    distance = njScalor(&away);
    /* @bug uninitialized: at a distance of 0, no path sets cosine. the BT
     * at 0x8c0f7e86 skips the FDIV into FR6, and the tests at 0x8c0f7f0a
     * and 0x8c0f7f1c read FR6 as njScalor left it, holding away.z */
    if (distance != 0.0F) {
        cosine = dot / distance;
    }
    /* @hack spelling: scl.y read through task->work again (MOV.L @(32,R14),R3
     * at 0x8c0f7e8e), which keeps task in R14 and work in R13 (0x8c0f7dea,
     * 0x8c0f7df2); through work, 42 of 153 words differ */
    range = task->work->scl.y;
    if (range == 0.0F) {
        range = 100.0F;
    } else if (range == 1.0F) {
        range = 60.0F;
    } else {
        range = 30.0F;
    }
    if (range > distance) {
        if (cosine <= 1.0F && cosine > 0.0F) {
            if (fishing_flags & FISHING_TWITCH) {
                FISH_MOTION(task)->interest += 2.0F;
                if (FISH_MOTION(task)->interest >= 3.0F) {
                    return TRUE;
                }
            }
        } else {
            if (fishing_flags & FISHING_TWITCH) {
                FISH_MOTION(task)->interest += 1.0F;
                if (FISH_MOTION(task)->interest >= 3.0F) {
                    return TRUE;
                }
            }
        }
    }
    return FALSE;
}

Bool fish_try_bite(struct task *task)
{
    struct fish_motion *motion = FISH_MOTION(task);
    struct task *lure;
    Float distance;

    lure = big_lure;
    if (lure == NULL) {
        return FALSE;
    }
    distance = njDistanceP2P(&task->work->pos, &lure->work->pos);
    if (distance < 1.0F) {
        motion->surface = fishing_get_water();
        return TRUE;
    }
    return FALSE;
}

Bool fish_claim_lure(struct task *task)
{
    struct task *lure;
    struct task *holder;
    Float distance;
    Float limit;
    int busy;

    lure = big_lure;
    if (lure == NULL) {
        return FALSE;
    }
    if (!(fishing_flags & FISHING_CAMERA) &&
        !(fishing_flags & FISHING_HOOKED)) {
        big_catch = NULL;
        return FALSE;
    }
    distance = njDistanceP2P(&task->work->pos, &lure->work->pos);
    holder = big_catch;
    limit = 100.0F;
    if (holder == NULL) {
        if (limit > distance) {
            big_catch = task;
            return TRUE;
        }
    } else {
        busy = fishing_flags & (FISHING_HOOKED | FISHING_LANDED | FISHING_LOST);
        if (holder != task) {
            if (busy) {
                return FALSE;
            }
            limit = njDistanceP2P(&holder->work->pos, &big_lure->work->pos);
            if (limit > distance) {
                big_catch = task;
                return TRUE;
            }
        } else if (busy || limit > distance) {
            return TRUE;
        } else {
            big_catch = NULL;
        }
    }
    return FALSE;
}

/* the frog gives up the lure: it wanders again, a little less drawn to it,
 * and big_catch is cleared */
static void frog_let_go(struct task *task)
{
    struct task_work *work = task->work;

    work->mode = FROG_WANDER;
    if (FISH_MOTION(task)->interest > 1.0F) {
        FISH_MOTION(task)->interest -= 1.0F;
    }
    big_catch = NULL;
    frog_set_action(task, &frog_swim_action);
}

void frog_step(struct task *task)
{
    struct task_work *work = task->work;
    struct fish_motion *motion = FISH_MOTION(task);
    struct task *lure;
    struct task_work *at;

    lure = big_lure;
    if (lure == NULL) {
        return;
    }
    at = lure->work;
    /* @hack spelling: the second test's hook flag and mode test joined by &,
     * which ANDs them as bits (MOVT R1 at 0x8c0f8134, AND R1,R0 at 0x8c0f813e).
     * FISHING_HOOKED is bit 0, and the mode test gives 0 or 1 and has no
     * effect, so & gives what && does. with &&, fish.c's code runs 96 bytes
     * longer and 292 of 365 words differ */
    /* @hack redundant: the three tests as one else-if chain, though each arm
     * returns, which loads stage_get_stage_act's address into R2 and
     * STAGE_MASK into R3 (0x8c0f8150, 0x8c0f8156); as three ifs, the two
     * trade and 6 of 365 words differ */
    if (njDistanceP2P(&at->pos, &work->pos) > 400.0F &&
        work->mode < FROG_CHASE) {
        task_kill(task);
        return;
    } else if ((fishing_flags & FISHING_HOOKED) & (work->mode < FROG_CHASE) &&
               big_catch != task) {
        task_kill(task);
        return;
    } else if (IN_STAGE(STAGE_SHELTER) && !fishing_shelter_ready) {
        return;
    }
    switch (work->mode) {
    case FROG_INIT:
        work->mode = FROG_WANDER;
        task->draw = frog_draw;
        task->destructor = frog_destroy;
        frog_set_action(task, &frog_swim_action);
        /* @hack spelling: the weight, scale and kind stored through task, which
         * keeps motion in the frame (MOV.L R3,@R15 at 0x8c0f80fa); through
         * motion and work, motion takes R10, the frame shrinks by 4 bytes,
         * fish.c's code runs 32 bytes longer and 195 of 365 words differ */
        FISH_MOTION(task)->weight = 500.0F;
        task->work->scl.y = 2.0F;
        FISH_VARS(task->work)->kind = FISH_FROG;
        frog_draw(task);
        break;
    case FROG_WANDER:
        frog_draw(task);
        if (fish_claim_lure(task) && fish_watch_lure(task)) {
            work->mode = FROG_CHASE;
            motion->surface = fishing_get_water();
            break;
        }
        fish_wander(task);
        fish_swim(task);
        frog_animate(task);
        collision_register(work);
        break;
    case FROG_CHASE:
        frog_draw(task);
        if (fish_try_bite(task)) {
            fishing_flags |= FISHING_HOOKED;
            work->mode = FROG_BITE;
            vibration_start(0, 3, 10, 2);
            motion->aim.y = big_lure->work->ang.y + 0x2000;
            frog_set_action(task, &frog_kick_action);
            work->timer = 45;
            break;
        }
        fish_turn_to_lure(task);
        /* @hack spelling: the two tests, each with an arm of its own that
         * lets go, which keep motion in the frame (MOV.L R3,@R15 at
         * 0x8c0f80fa); as one test of ors, motion takes R10, the frame
         * shrinks by 4 bytes, fish.c's code runs 32 bytes longer and 166 of
         * 365 words differ */
        if (!fish_claim_lure(task)) {
            fishing_flags &= ~FISHING_KICK;
            frog_let_go(task);
            break;
        } else if (!frog_chase(task)) {
            fishing_flags &= ~FISHING_KICK;
            frog_let_go(task);
            break;
        }
        frog_animate(task);
        collision_register(work);
        break;
    case FROG_BITE:
        frog_draw(task);
        if (fishing_flags & FISHING_LANDED) {
            work->mode = FROG_CARRIED;
            frog_land(task);
            break;
        } else if (fishing_flags & (FISHING_LOST | FISHING_SNAPPED)) {
            fishing_flags &= ~(FISHING_HOOKED | FISHING_KICK);
            frog_let_go(task);
            break;
        }
        if (work->timer) {
            if (fishing_flags & FISHING_PULL) {
                work->mode = FROG_HOOKED;
                sound_play(SE_FROG_HOOKED, 0, 0, 0);
                vibration_start(0, 3, 10, 2);
                fishing_start_fight();
                break;
            }
        } else {
            fishing_flags &= ~(FISHING_HOOKED | FISHING_KICK);
            frog_let_go(task);
            break;
        }
        if (!fish_claim_lure(task)) {
            fishing_flags &= ~(FISHING_HOOKED | FISHING_KICK);
            frog_let_go(task);
            break;
        }
        frog_kick(task);
        frog_pull(task);
        frog_animate(task);
        collision_register(work);
        break;
    case FROG_HOOKED:
        frog_draw(task);
        if (fishing_flags & FISHING_LANDED) {
            work->mode = FROG_CARRIED;
            frog_land(task);
            break;
        } else if (fishing_flags & (FISHING_LOST | FISHING_SNAPPED)) {
            fishing_flags &= ~(FISHING_HOOKED | FISHING_KICK);
            frog_let_go(task);
            break;
        }
        frog_kick(task);
        frog_pull(task);
        frog_animate(task);
        collision_register(work);
        break;
    case FROG_CARRIED:
        frog_carry(task);
        frog_animate(task);
        frog_draw(task);
        if (big_not_fishing(task)) {
            task_kill(task);
            return;
        }
        break;
    }
}

Bool fish_sweep(NJS_POINT3 *pos, NJS_VECTOR *speed, struct angles *ang,
                Float radius)
{
    struct polygon_ball probe;
    /* @hack redundant: zero, which is cleared and never read, and takes 12
     * bytes of the image's 216-byte frame (MOV.L R11,@R4 at 0x8c0f84e2 to
     * 0x8c0f84e6); without it, the frame is 204 bytes and 61 of 143 words
     * differ */
    /* NOLINTNEXTLINE(clang-diagnostic-unused-but-set-variable) */
    struct angles zero;
    NJS_POINT3 normal;
    NJS_VECTOR up;
    /* @hack spelling: objects declared ahead of ball, which has SHC form
     * zero's and ball's frame addresses where the image does (MOV R15,R14,
     * ADD #40,R4 and ADD #52,R14 at 0x8c0f84ce, 0x8c0f84d0, 0x8c0f84e0);
     * the other way round, 8 of 143 words differ */
    /* @hack spelling: world_objects read into objects, which the image keeps
     * in the frame (MOV.L R3,@(12,R15) at 0x8c0f84de, MOV.L @(12,R15),R12
     * at 0x8c0f85b6); read in the loop's header, 37 of 143 words differ */
    struct world_object *objects = world_objects;
    /* @hack spelling: probe reached through ball, a pointer to it that SHC
     * keeps in R14 (0x8c0f84ce, 0x8c0f84e0); through probe itself, fish.c's
     * code runs 32 bytes longer and 103 of 143 words differ */
    struct polygon_ball *ball = &probe;
    struct world_object *object;
    Uint16 i;

    zero.x = 0;
    zero.y = 0;
    zero.z = 0;
    if (ang == NULL) {
        ball->normal.x = 0.0F;
        ball->normal.y = 1.0F;
        ball->normal.z = 0.0F;
    } else {
        up.x = 0.0F;
        up.y = 1.0F;
        up.z = 0.0F;
        njPushMatrix(&_unit_matrix_);
        TURN(njRotateZ, ang->z);
        TURN(njRotateX, ang->x);
        TURN(njRotateY, ang->y);
        njCalcVector(NULL, &up, &normal);
        njPopMatrix(1);
        ball->normal = normal;
    }
    ball->pos = *pos;
    ball->radius = radius;
    ball->speed = *speed;
    polygon_collect_near(ball);
    /* @hack spelling: i zeroed ahead of the loop, which has it share R11 with
     * zero's stores (MOV #0,R11 at 0x8c0f84d2, ADD #1,R11 at 0x8c0f85d4);
     * zeroed in the loop's header, the zeros take R10, i takes a copy, and
     * 41 of 143 words differ */
    i = 0;
    for (object = objects; i < world_count; i++, object++) {
        if (polygon_sweep(ball, object->object, object->flags)) {
            speed->x = speed->y = speed->z = 0.0F;
            return TRUE;
        }
    }
    return FALSE;
}
