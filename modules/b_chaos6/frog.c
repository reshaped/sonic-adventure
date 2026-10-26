/* Chaos 6's frog: Big's frog rides a path inside the boss. the first two
 * times Big's lure comes within reach, the frog slows and its mark grows.
 * once Big hooks it, it kicks and pulls against the line as he reels it in.
 * the stage is clear when it lands */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "b_chaos6/boss.h"
#include "b_chaos6/frog.h"
#include "camera/camera.h"
#include "chaos/chaos.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "draw/motion.h"
#include "draw/tree.h"
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

extern NJS_ACTION frog_swim_action;
extern NJS_ACTION frog_kick_action;

#define SE_FROG_KICK 808
#define SE_FROG_HOOKED 1312

/* big.c's BIG_REEL: Big's mode once he has cast, while he doesn't turn the
 * reel */
#define BIG_REEL 35

/* a bit of the frog's work->flags: its mark draws while it is set */
#define FROG_MARK 0x100

/* the hooked frog lands once it is this near player 0 */
#define FROG_LAND_DISTANCE 20.0F
/* chaos6_frog_camera's distance from the frog */
#define FROG_CAMERA_DISTANCE 20.0F

/* a lure hooks the frog within FROG_HOOK_HEIGHT above or below it and
 * FROG_HOOK_ACROSS across, in x and z alone. a lure within FROG_LURE_REACH
 * times the mark's size draws it, and chaos6_frog_watch_lure then sets its
 * cooldown to FROG_LURE_WAIT steps */
#define FROG_HOOK_HEIGHT 10.0F
#define FROG_HOOK_ACROSS 5.0F
#define FROG_LURE_REACH 10.0F
#define FROG_LURE_WAIT 120

/* the mark's scale for each unit of the frog's work->scl.x */
#define FROG_MARK_SCALE 2.5F

/* the frog's work->vars, which VARS casts in place */
struct frog_vars {
    NJS_ACTION *action;
    /* the frame of the frog's action. chaos6_frog_watch_lure keeps its
     * cooldown in the frame's low 16 bits */
    union {
        Float frame;
        Sint16 cooldown;
    } time;
    Sint16 turn_steps; /* the steps left to its next turn */
};

#define VARS(work) ((struct frog_vars *)&(work)->vars)

enum frog_mode { FROG_START, FROG_RIDE, FROG_HOOKED, FROG_LANDED };

/* chaos6_frog_watch_lure's results: the lure did nothing, drew the frog a
 * step further, or hooked it */
enum frog_lure { LURE_NONE, LURE_NEAR, LURE_HOOKED };

/* the frog's path in the boss's frame. x is frog_ride_across times the
 * sine of frog_phase_across, and y frog_ride_up times the sine of
 * frog_phase_up, above frog_ride_height. z is frog_ride_depth.
 * chaos6_frog_ride steps each phase by a multiple of its pace. the mark
 * draws frog_mark_near from the frog, toward the camera */
#pragma section HEAD
static Float frog_ride_across = 14.0F;
static Float frog_ride_up = 8.0F;
static Float frog_ride_height = 18.0F;
static Float frog_ride_depth = 15.0F;
static Angle frog_ride_pace_across = 8;
static Angle frog_ride_pace_up = 18;
static Float frog_mark_near = 30.0F;
#pragma section

/* the frog's mark, a sight. frog_mark_object is a ring with a corner at
 * each diagonal, and its child frog_cross_object a cross at its center */
static NJS_MATERIAL frog_cross_materials[1] = {
#include "b_chaos6/frog/frog_cross_materials.inc"
};

static Sint16 frog_cross_meshes[10] = {
#include "b_chaos6/frog/frog_cross_meshes.inc"
};

static NJS_COLOR frog_cross_uvs[8] = {
#include "b_chaos6/frog/frog_cross_uvs.inc"
};

static NJS_MESHSET frog_cross_meshsets[1] = {
#include "b_chaos6/frog/frog_cross_meshsets.inc"
};

static NJS_POINT3 frog_cross_points[8] = {
#include "b_chaos6/frog/frog_cross_points.inc"
};

static NJS_VECTOR frog_cross_normals[8] = {
#include "b_chaos6/frog/frog_cross_normals.inc"
};

static NJS_MODEL frog_cross_model = {
#include "b_chaos6/frog/frog_cross_model.inc"
};

static NJS_OBJECT frog_cross_object = {
#include "b_chaos6/frog/frog_cross_object.inc"
};

static NJS_MATERIAL frog_mark_materials[2] = {
#include "b_chaos6/frog/frog_mark_materials.inc"
};

static Sint16 frog_mark_meshes0[15] = {
#include "b_chaos6/frog/frog_mark_meshes0.inc"
};

static Sint16 frog_mark_meshes1[17] = {
#include "b_chaos6/frog/frog_mark_meshes1.inc"
};

static NJS_COLOR frog_mark_uvs0[14] = {
#include "b_chaos6/frog/frog_mark_uvs0.inc"
};

static NJS_COLOR frog_mark_uvs1[12] = {
#include "b_chaos6/frog/frog_mark_uvs1.inc"
};

static NJS_MESHSET frog_mark_meshsets[2] = {
#include "b_chaos6/frog/frog_mark_meshsets.inc"
};

static NJS_POINT3 frog_mark_points[24] = {
#include "b_chaos6/frog/frog_mark_points.inc"
};

static NJS_VECTOR frog_mark_normals[24] = {
#include "b_chaos6/frog/frog_mark_normals.inc"
};

static NJS_MODEL frog_mark_model = {
#include "b_chaos6/frog/frog_mark_model.inc"
};

static NJS_OBJECT frog_mark_object = {
#include "b_chaos6/frog/frog_mark_object.inc"
};

/* model_weld's table for the frog: four points of node 1's model onto
 * four of node 12's, the pairs fish.c's frog_welds uses */
static Sint16 chaos6_frog_weld_pairs[8] = {
#include "b_chaos6/frog/chaos6_frog_weld_pairs.inc"
};
static struct model_weld chaos6_frog_welds[] = {
    {4, 1, 12, chaos6_frog_weld_pairs},
    {0},
};

/* the frog's work->scl.x, the mark's size, by frog_drawn */
static Float frog_mark_sizes[3] = {1.0F, 1.5F, 2.0F};

/* the times the lure has drawn the frog, at most 2 */
static Sint8 frog_drawn;
static struct task *frog_task; /* for chaos6_frog_camera */
static Angle frog_phase_across;
static Angle frog_phase_up;

void chaos6_frog_camera(void)
{
    struct task_work *work = frog_task->work;
    NJS_VECTOR toward;
    NJS_POINT3 pos;
    Float scale;

    toward = player_work[0]->pos;
    njSubVector(&toward, &work->pos);
    scale = FROG_CAMERA_DISTANCE / njScalor(&toward);
    toward.x *= scale;
    toward.y *= scale;
    toward.z *= scale;
    pos = work->pos;
    njAddVector(&pos, &toward);
    camera_state->target.x = work->pos.x;
    camera_state->target.y = work->pos.y;
    camera_state->target.z = work->pos.z;
    camera_state->pos.x = pos.x;
    camera_state->pos.y = pos.y;
    camera_state->pos.z = pos.z;
}

/* draw the frog at its action's frame, welded, and while FROG_MARK is set
 * its mark, turned to the camera's angles */
static void chaos6_frog_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;
    Float size;

    njSetTexture(&big_kaeru_texlist);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    TURN_VIA(njRotateY, work->ang.y, ang);
    njTranslate(NULL, 2.0F, 0.0F, 0.0F);
    model_weld(VARS(work)->action->object, VARS(work)->action->motion,
               VARS(work)->time.frame, chaos6_frog_welds);
    draw_action(VARS(work)->action, VARS(work)->time.frame);
    njPopMatrix(1);
    if (work->flags & FROG_MARK) {
        size = work->scl.x * FROG_MARK_SCALE;
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, camera_work->ang.y, ang);
        TURN_VIA(njRotateX, camera_work->ang.x, ang);
        njTranslate(NULL, 0.0F, 0.0F, frog_mark_near);
        njScale(NULL, size, size, size);
        draw_tree(&frog_mark_object);
        njPopMatrix(1);
    }
}

static void chaos6_frog_set_action(struct task *task, NJS_ACTION *action)
{
    struct task_work *work = task->work;

    if (VARS(work)->action != action) {
        VARS(work)->action = action;
        VARS(work)->time.frame = 0.0F;
    }
}

static void chaos6_frog_animate(struct task *task)
{
    struct task_work *work = task->work;

    VARS(work)->time.frame += 1.0F;
    if (VARS(work)->time.frame >= VARS(work)->action->motion->nbFrame) {
        VARS(work)->time.frame = 0.0F;
    }
}

/* turn the frog a degree every third step, as turn_steps counts down */
static void chaos6_frog_wander(struct task *task)
{
    struct task_work *work = task->work;

    if (--VARS(work)->turn_steps <= 0) {
        VARS(work)->turn_steps = 3;
        work->ang.y += NJM_DEG_ANG(1);
    }
}

/* every 120 steps the frog's aim turns half round. with a chance of 0.3 it
 * then kicks hard, setting FISHING_KICK until chaos6_frog_swim has counted
 * its timer out. each step it turns toward its aim */
static void chaos6_frog_kick(struct task *task)
{
    struct task_work *work = task->work;
    struct fish_motion *motion = FISH_MOTION(task);

    if (--VARS(work)->turn_steps <= 0) {
        VARS(work)->turn_steps = 120;
        motion->aim.y ^= 0x8000;
        if (rand() * RAND_SCALE < 0.3F) {
            sound_play(SE_FROG_KICK, 0, 0, 0);
            work->timer = 120;
            fishing_flags |= FISHING_KICK;
            chaos6_frog_set_action(task, &frog_kick_action);
        }
    }
    if ((Uint16)(work->ang.y - motion->aim.y) > 0x8000) {
        work->ang.y += 0x100;
    } else {
        work->ang.y -= 0x100;
    }
    work->ang.y &= 0xffff;
}

/* set the frog's pos on its path in the boss's frame. each phase steps by
 * its pace times 3, and times 1 once the lure has drawn the frog. once the
 * lure has drawn it twice, only the up phase steps, by its pace, until the
 * phase passes 91 turns */
static void chaos6_frog_ride(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *boss;
    NJS_POINT3 point;
    Float pace;
    Angle ang;

    switch (frog_drawn) {
    case 0:
        pace = 3.0F;
        break;
    case 1:
        pace = 1.0F;
        break;
    default:
        pace = 0.0F;
        break;
    }
    frog_phase_across += frog_ride_pace_across * pace;
    frog_phase_up += frog_ride_pace_up * pace;
    /* the test compares the phase in degrees with 32768, and 32768 degrees
     * is 91 turns */
    if (frog_drawn == 2 && frog_phase_up * (360.0F / 65536.0F) < 32768.0F) {
        frog_phase_up += frog_ride_pace_up;
    }
    njPushMatrix(&_unit_matrix_);
    boss = boss_work;
    njTranslateV(NULL, &boss->pos);
    TURN_VIA(njRotateZ, boss->ang.z, ang);
    TURN_VIA(njRotateX, boss->ang.x, ang);
    TURN_VIA(njRotateY, 0x4000 - boss->ang.y, ang);
    njTranslate(NULL, 0.0F, 0.0F, 0.0F);
    point.x = frog_ride_across * njSin(frog_phase_across);
    point.y = (frog_ride_up * njSin(frog_phase_up)) + frog_ride_height;
    point.z = frog_ride_depth;
    njCalcPoint(NULL, &point, &work->pos);
    njPopMatrix(1);
}

/* set the hooked frog's speed along its heading: 0.2 a step while its
 * action's frame is between 25 and 45, its kick. otherwise it slows by 0.004
 * a step while faster than 0.05, and swims at 0.1 once it isn't. the frog
 * then sits at the hook. once its timer runs out, FISHING_KICK clears and
 * the swim action plays. nothing while big_lure or the lure's work is
 * null */
static void chaos6_frog_swim(struct task *task)
{
    struct task_work *work = task->work;
    struct fish_motion *motion = FISH_MOTION(task);
    NJS_MATRIX matrix;
    NJS_VECTOR move;
    struct angles slope;
    Angle ang;
    Float speed = njScalor(&motion->speed);
    struct task_work *lure;

    if (big_lure == NULL) {
        return;
    }
    lure = big_lure->work;
    if (lure == NULL) {
        return;
    }
    move.y = 0.0F;
    move.z = 0.0F;
    if (VARS(work)->time.frame > 25.0F && VARS(work)->time.frame < 45.0F) {
        move.x = -0.2F;
    } else if (-speed < -0.05F) {
        move.x = 0.004F - speed;
    } else {
        move.x = -0.1F;
    }
    njUnitMatrix(&matrix);
    TURN_MATRIX_VIA(njRotateY, &matrix, work->ang.y, ang);
    njCalcVector(&matrix, &move, &motion->speed);
    /* @bug uninitialized: slope goes to world_sweep_all_hit unset, and the
     * sweep turns its ball's up by it. nothing stores to R15+4 to R15+15
     * before the JSR at 0x0c906c24. fish.c's frog_pull zeroes its slope
     * first */
    world_sweep_all_hit(&work->pos, &motion->speed, &slope, 1.0F);
    fishing_hook_point(&work->pos);
    if (work->timer) {
        work->timer--;
    } else {
        fishing_flags &= ~FISHING_KICK;
        chaos6_frog_set_action(task, &frog_swim_action);
    }
}

/* lay the landed frog on its side, kicking, set FISHING_GOAL and
 * chaos6_frog_caught, and clear the stage */
static void chaos6_frog_land(struct task *task)
{
    struct task_work *work = task->work;

    work->ang.x = work->ang.y = 0;
    work->ang.z = 0x4000;
    fishing_flags |= FISHING_GOAL;
    chaos6_frog_set_action(task, &frog_kick_action);
    stage_clear();
    chaos6_frog_caught = TRUE;
}

/* hook the frog on the lure, or draw it a step further, while Big is in
 * BIG_REEL. a lure within FROG_HOOK_HEIGHT and FROG_HOOK_ACROSS hooks it,
 * and the frog asks the boss for BOSS_FROG_HOOKED. otherwise, once the
 * cooldown has run out, a lure within FROG_LURE_REACH times the mark's size
 * draws the frog and grows its mark. returns LURE_HOOKED for a hook,
 * LURE_NEAR for a draw, and LURE_NONE otherwise */
static enum frog_lure chaos6_frog_watch_lure(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *player = player_work[0];
    Float distance;
    Float height;
    Float across;
    Float range;

    if (big_lure != NULL && big_lure->work != NULL &&
        player->mode == BIG_REEL) {
        distance = njDistanceP2P(&work->pos, &big_lure->work->pos);
        across = chaos_distance_across(&work->pos, &big_lure->work->pos);
        height = njAbs(work->pos.y - big_lure->work->pos.y);
        if (height <= FROG_HOOK_HEIGHT && across <= FROG_HOOK_ACROSS) {
            chaos6_set_mode(BOSS_FROG_HOOKED);
            vibration_start(0, 5, 15, 4);
            return LURE_HOOKED;
        }
        range = work->scl.x * FROG_LURE_REACH;
        if (VARS(work)->time.cooldown > 0) {
            VARS(work)->time.cooldown--;
        } else if (distance <= range) {
            frog_drawn = frog_drawn + 1 < 2 ? frog_drawn + 1 : 2;
            work->scl.x = frog_mark_sizes[frog_drawn];
            /* @bug logic: the lure can draw the frog again within 60 steps, not
             * FROG_LURE_WAIT. the cooldown lies in the low 16 bits of the
             * action's frame, a float, which chaos6_frog_animate adds 1 to each
             * step. each time the frame passes a power of 2 from 2 to 32, that
             * add halves those bits, and the frame's return to 0 after frame 59
             * clears them. the image stores the cooldown by the MOV.W at
             * 0x0c906de4, and adds to the frame by the FADD at 0x0c906868 */
            VARS(work)->time.cooldown = FROG_LURE_WAIT;
            vibration_start(0, 1, 48, 0);
            return LURE_NEAR;
        }
    }
    return LURE_NONE;
}

void chaos6_frog_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *player = player_work[0];
    struct task_work *lure;

    if (big_lure == NULL) {
        return;
    }
    lure = big_lure->work;
    if (lure == NULL) {
        return;
    }
    switch (work->mode) {
    case FROG_START:
        if (!placement_editing(task)) {
            work->mode = FROG_RIDE;
        }
        task->draw = chaos6_frog_draw;
        chaos6_frog_set_action(task, &frog_swim_action);
        FISH_MOTION(task)->weight = 1000.0F;
        task->work->scl.x = 1.0F;
        task->work->flags |= FROG_MARK;
        chaos6_frog_draw(task);
        frog_drawn = 0;
        frog_task = task;
        break;
    case FROG_RIDE:
        chaos6_frog_draw(task);
        if (chaos6_frog_watch_lure(task) == LURE_HOOKED) {
            work->mode = FROG_HOOKED;
            sound_play(SE_FROG_HOOKED, 0, 0, 0);
            fishing_start_fight();
            work->flags &= ~FROG_MARK;
        }
        chaos6_frog_wander(task);
        chaos6_frog_ride(task);
        chaos6_frog_animate(task);
        break;
    case FROG_HOOKED:
        chaos6_frog_draw(task);
        if (njDistanceP2P(&work->pos, &player->pos) < FROG_LAND_DISTANCE) {
            work->mode = FROG_LANDED;
            chaos6_frog_land(task);
            break;
        }
        chaos6_frog_kick(task);
        chaos6_frog_swim(task);
        chaos6_frog_animate(task);
        break;
    case FROG_LANDED:
        task_kill(task);
        break;
    }
}
