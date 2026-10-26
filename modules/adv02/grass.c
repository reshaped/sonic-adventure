/* the clumps of grass the object list names O GRASS and O BIG GRASS. O GRASS
 * rustles as player 0 comes near, and O BIG GRASS sways as player 0 runs
 * through it */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv02/grass.h"
#include "draw/draw.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "player/player.h"
#include "sound/sound.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT grass_object;
extern NJS_ACTION grass_action;
extern NJS_OBJECT big_grass_object;
extern NJS_ACTION big_grass_action;

#define SE_RUSTLE 0x253
#define SE_SWISH 0x255

/* in work->flags: the next draw plays the action at VARS(work)->frame, not the
 * still tree, and clears the flag */
#define GRASS_MOVING 0x100

/* O GRASS rustles RUSTLES times once player 0 comes within GRASS_NEAR. each
 * rustle plays its action from frame 0 to RUSTLE_END, RUSTLE_SPEED a step */
#define GRASS_NEAR 50.0F
#define RUSTLES 3
#define RUSTLE_END 16.0F
#define RUSTLE_SPEED 0.5F

/* O BIG GRASS sways once player 0 comes within BIG_GRASS_NEAR, on the ground
 * and faster than RUN_SPEED. a sway plays its action from frame 0 to SWAY_END,
 * a frame each step */
#define BIG_GRASS_NEAR 15.0F
#define RUN_SPEED 0.2F
#define SWAY_END 120.0F

enum grass_mode { GRASS_INIT, GRASS_STILL, GRASS_RUSTLING };

/* for O GRASS, work->tick counts the rustles left */
struct grass_vars {
    Uint8 unk_00[8];
    Float frame;
};

#define VARS(work) ((struct grass_vars *)&(work)->vars)

static void grass_draw(struct task *task);
static void grass_destroy(struct task *task);
static void big_grass_draw(struct task *task);
static void big_grass_destroy(struct task *task);

void grass_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return;
    }
    switch (work->mode) {
    case GRASS_INIT:
        task->draw = grass_draw;
        task->destructor = grass_destroy;
        work->mode = GRASS_STILL;
        break;
    case GRASS_STILL:
        if (player_in_range(&work->pos, GRASS_NEAR) == 1) {
            work->mode = GRASS_RUSTLING;
            work->tick = RUSTLES;
            VARS(work)->frame = 0.0F;
            sound_play_at(SE_RUSTLE, 0, 0, 0, work->pos.x, work->pos.y,
                          work->pos.z);
        }
        break;
    case GRASS_RUSTLING:
        work->flags |= GRASS_MOVING;
        VARS(work)->frame += RUSTLE_SPEED;
        if (VARS(work)->frame >= RUSTLE_END) {
            work->tick--;
            if (!work->tick) {
                work->mode = GRASS_STILL;
            }
            VARS(work)->frame = 0.0F;
        }
        break;
    }
    grass_draw(task);
}

static void grass_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    if (work->flags & GRASS_MOVING) {
        work->flags &= ~GRASS_MOVING;
        draw_action(&grass_action, VARS(work)->frame);
    } else {
        draw_tree(&grass_object);
    }
    njPopMatrix(1);
}

static void grass_destroy(struct task *task)
{
}

void big_grass_step(struct task *task)
{
    struct task_work *work = task->work;
    Float speed;

    if (placement_try_leave(task)) {
        return;
    }
    switch (work->mode) {
    case GRASS_INIT:
        task->draw = big_grass_draw;
        task->destructor = big_grass_destroy;
        work->mode = GRASS_STILL;
        break;
    case GRASS_STILL:
        if (player_in_range(&work->pos, BIG_GRASS_NEAR) == 1) {
            if (player_ground_speed(0, &speed)) {
                if (speed > RUN_SPEED) {
                    VARS(work)->frame = 0.0F;
                    sound_play(SE_SWISH, 0, 0, 0);
                    work->mode = GRASS_RUSTLING;
                }
            }
        }
        break;
    case GRASS_RUSTLING:
        work->flags |= GRASS_MOVING;
        VARS(work)->frame += 1.0F;
        if (VARS(work)->frame >= SWAY_END) {
            work->mode = GRASS_STILL;
            VARS(work)->frame = 0.0F;
        }
        break;
    }
    big_grass_draw(task);
}

static void big_grass_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    if (work->flags & GRASS_MOVING) {
        work->flags &= ~GRASS_MOVING;
        draw_action(&big_grass_action, VARS(work)->frame);
    } else {
        draw_tree(&big_grass_object);
    }
    njPopMatrix(1);
}

static void big_grass_destroy(struct task *task)
{
}
