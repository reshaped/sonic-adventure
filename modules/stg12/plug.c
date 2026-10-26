/* the plug at the bottom of act 0's upper pool, which Big fishes out */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>

#include "draw/draw.h"
#include "draw/tree.h"
#include "player/big.h"
#include "player/fishing.h"
#include "player/player.h"
#include "sound/sound.h"
#include "stg12/plug.h"
#include "stg12/upper_pool.h"
#include "stg12/water.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT shelter_plug_object;

#define SE_HOOK 318
#define SE_PULL 314

/* the plug's place, which its draw turns it about */
#define PLUG_X 750.0F
#define PLUG_Y 75.5F
#define PLUG_Z (-650.0F)
/* how far below that place the draw puts the model */
#define HANG 10.5F

/* the height of the hook point, which is at the plug's x and z */
#define HOOK_Y 82.5F
/* a waiting plug takes big_catch, setting it to itself, if no task holds
 * it. it does so while the lure is within CATCH_RADIUS of the hook point,
 * and gives big_catch up beyond */
#define CATCH_RADIUS 40.0F
/* a lure within HOOK_RADIUS of the hook point hooks the plug once its
 * squared speed is under SETTLED */
#define HOOK_RADIUS 8.0F
#define SETTLED 0.035F
/* the height the hook moves the lure to, at the plug's x and z */
#define HOLD_Y 70.0F

/* after the pull, each step raises pos.y by RISE, which the draw doesn't
 * show. each step also turns the plug SPIN_X about x and SPIN_Y about y.
 * its alpha falls FADE each step */
#define RISE 0.4F
#define SPIN_X 1000
#define SPIN_Y 2000
#define FADE 0.005F

enum shelter_plug_mode {
    PLUG_STILL,
    PLUG_WAIT,
    PLUG_MODE_2, /* no code sets it */
    PLUG_HOOKED,
    PLUG_PULLED,
    PLUG_GONE
};

struct shelter_plug_vars {
    Float fade; /* the alpha draw_set_material gets, from 0 down to -1 */
};

#define VARS(work) ((struct shelter_plug_vars *)&(work)->vars)

/* task->motion as the plug lays it out. spin_x takes the word struct
 * task_motion leaves as unk_28 */
struct shelter_plug_motion {
    Uint8 unk_00[4];
    NJS_VECTOR speed;
    NJS_VECTOR accel;
    Uint8 unk_1c[12];
    Angle spin_x; /* added to ang.x each step after the pull */
};

#define MOTION(task) ((struct shelter_plug_motion *)(task)->motion)

static void shelter_plug_still(struct task *task);
static void shelter_plug_step(struct task *task);
static void shelter_plug_draw(struct task *task);

void shelter_plug_start(struct task *task)
{
    struct task_work *work = task->work;
    struct shelter_plug_motion *motion = MOTION(task);

    work->pos.x = PLUG_X;
    work->pos.y = PLUG_Y;
    work->pos.z = PLUG_Z;
    work->ang.x = 0;
    work->ang.y = 0;
    work->ang.z = 0;
    VARS(work)->fade = 0.0F;
    motion->speed.x = 0.0F;
    motion->speed.y = 0.0F;
    motion->speed.z = 0.0F;
    if (player_get_character(0) == CHARACTER_BIG) {
        work->mode = PLUG_WAIT;
        task->step = shelter_plug_step;
    } else {
        work->mode = PLUG_STILL;
        task->step = shelter_plug_still;
    }
    task->draw = shelter_plug_draw;
}

static void shelter_plug_still(struct task *task)
{
    shelter_plug_draw(task);
}

/* the plug's step for Big, which does nothing but draw while big_lure is
 * null. a waiting plug kills itself once BIG_PULLED is set and the water is
 * SHELTER_WATER_RISEN. Big's pull frees a hooked plug. the freed plug sets
 * BIG_PULLED, turns its heading toward the lure, then tumbles as it fades */
static void shelter_plug_step(struct task *task)
{
    struct task_work *work = task->work;
    struct shelter_plug_motion *motion = MOTION(task);
    struct task *lure = big_lure;
    struct task_work *lure_work;
    struct task_motion *lure_motion;
    NJS_POINT3 *lure_pos;
    Float dx;
    Float dy;
    Float dz;
    Float distance2;

    if (lure != NULL) {
        lure_work = lure->work;
        lure_motion = lure->motion;
        /* @hack spelling: lure_pos, a second pointer to the lure's pos, in R13
         * (0x0c910b3e). PLUG_WAIT measures and moves the lure through it, and
         * PLUG_MODE_2 measures it. PLUG_MODE_2's move and PLUG_HOOKED's turn
         * reach the pos through lure_work instead, in R4 (0x0c910cdc,
         * 0x0c910d2e). through lure_work throughout, 180 of 278 words and 2
         * pool words differ, and through lure_pos throughout, 153 of 278 */
        lure_pos = &lure_work->pos;
        switch (work->mode) {
        case PLUG_WAIT:
            dx = lure_pos->x - work->pos.x;
            dy = lure_pos->y - HOOK_Y;
            dz = lure_pos->z - work->pos.z;
            distance2 = (dx * dx) + (dy * dy) + (dz * dz);
            if ((shelter_big_flags & BIG_PULLED) &&
                shelter_water_state == SHELTER_WATER_RISEN) {
                task_kill(task);
                return;
            }
            if (distance2 < CATCH_RADIUS * CATCH_RADIUS) {
                if (big_catch == NULL) {
                    big_catch = task;
                }
                if (distance2 < HOOK_RADIUS * HOOK_RADIUS) {
                    if (njScalor2(&lure_motion->speed) < SETTLED) {
                        work->timer = 0;
                        work->mode = PLUG_HOOKED;
                        fishing_flags |= FISHING_HOOKED | FISHING_KICK;
                        lure_pos->x = work->pos.x;
                        lure_pos->y = HOLD_Y;
                        lure_pos->z = work->pos.z;
                        sound_play(SE_HOOK, 0, 0, 0);
                    }
                }
            } else if (big_catch == task) {
                big_catch = NULL;
            }
            break;
        case PLUG_MODE_2:
            dx = lure_pos->x - work->pos.x;
            dy = lure_pos->y - HOLD_Y;
            dz = lure_pos->z - work->pos.z;
            distance2 = (dx * dx) + (dy * dy) + (dz * dz);
            if (distance2 >= 1.0F) {
                lure_work->pos.x -= motion->accel.x;
                lure_work->pos.y -= motion->accel.y;
                lure_work->pos.z -= motion->accel.z;
            }
            break;
        case PLUG_HOOKED:
            if (fishing_flags & FISHING_PULL) {
                fishing_flags &= ~(FISHING_HOOKED | FISHING_KICK);
                shelter_big_flags |= BIG_PULLED;
                work->mode = PLUG_PULLED;
                if (big_catch == task) {
                    big_catch = NULL;
                }
                motion->speed.y = 0.05F;
                dx = lure_work->pos.x - work->pos.x;
                dz = lure_work->pos.z - work->pos.z;
                work->ang.y = (Angle)(atan2f(dx, dz) * ANGLE_PER_RADIAN);
                motion->spin_x = SPIN_X;
                sound_play(SE_PULL, 0, 0, 0);
            }
            break;
        case PLUG_PULLED:
            work->pos.y += RISE;
            work->ang.x += motion->spin_x;
            work->ang.y += SPIN_Y;
            VARS(work)->fade -= FADE;
            if (VARS(work)->fade < -1.0F) {
                VARS(work)->fade = -1.0F;
                work->mode = PLUG_GONE;
            }
            break;
        case PLUG_GONE:
            /* @hack redundant: three stores that never run. SHC still counts
             * their uses when it gives out registers: lure_motion takes R10
             * (0x0c910b34), and PLUG_WAIT's dx, dy and dz take FR4, FR5 and
             * FR6 (0x0c910b6c to 0x0c910b84); without them, 88 of 278 words
             * and a pool word differ */
            if (0) {
                lure_motion->record = (struct player *)lure_motion;
                lure_motion->record = (struct player *)lure_motion;
                motion->spin_x = shelter_big_flags;
            }
            break;
        }
    }
    shelter_plug_draw(task);
}

static void shelter_plug_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (frame_step == 0) {
        texture_set_stage();
        njPushMatrix(NULL);
        /* at the plug's place, not at pos, so the draw never shows the rise
         * after the pull */
        njTranslate(NULL, PLUG_X, PLUG_Y, PLUG_Z);
        TURN_VIA(njRotateY, work->ang.y, ang);
        TURN_VIA(njRotateX, work->ang.x, ang);
        draw_attr_save();
        draw_attr_set(0, NJD_FLAG_USE_ALPHA);
        draw_set_material(VARS(work)->fade, 0.0F, 0.0F, 0.0F);
        njTranslate(NULL, 0.0F, -HANG, 0.0F);
        draw_tree(&shelter_plug_object);
        draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
        draw_attr_restore();
        njPopMatrix(1);
    }
}
