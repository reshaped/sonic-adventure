/* O CARGOSTART, a row of five cargo cars that takes player 0 to the cargo
 * ride. once player 0 stands on them, they pull away and the screen fades to
 * black. player 0 is then set down in the tunnel */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "collision/world.h"
#include "debug/editor.h"
#include "draw/tree.h"
#include "player/player.h"
#include "sound/sound.h"
#include "stg12/cargo_fade.h"
#include "stg12/cargo_start.h"
#include "stg12/texlists.h"
#include "system/frame.h"
#include "system/pad.h"
#include "system/task.h"
#include "system/util.h"

extern NJS_OBJECT cargo_car_object;
extern NJS_OBJECT cargo_container_b_object;
extern NJS_OBJECT cargo_car_world_object;

/* once player 0 or 1 comes within NEAR, the collision world tests the cars */
#define NEAR 95.0F
/* the set editor draws marker_object, a sphere twenty across, at this scale to
 * show NEAR */
#define MARKER_SCALE (NEAR * 0.1F)

/* the cars pull away along -x. each step they rise by RISE_SPEED, up to RISE
 * above where they stood. they also gain ACCEL of speed a step, up to
 * TOP_SPEED */
#define RISE 10.0F
#define RISE_SPEED 0.05F
#define TOP_SPEED 20.0F
#define ACCEL 0.1F

/* each car stands this far from the one before, along z before the cars' turn
 * about y */
#define CAR_SPACING (-172.0F)

/* once player 0 stands on the cars, they count steps. they sound from step
 * SOUND_STEPS on, and pull away once the count passes WAIT_STEPS */
#define SOUND_STEPS 50
#define WAIT_STEPS 210
/* the cars pull away until their count of steps passes FADE_STEPS. then the
 * fade starts */
#define FADE_STEPS 60
/* once the count from the fade's start passes RIDE_STEPS, player 0 is set down
 * in the tunnel. the screen is black by then */
#define RIDE_STEPS 170

/* where player 0 is set down in the tunnel */
#define TUNNEL_X (-48.0F)
#define TUNNEL_Y 10006.0F
#define TUNNEL_Z (-188.0F)

#define SE_TRAIN 325

/* the cars wait for a player to come near, then for player 0 to stand on
 * them. then they sound, pull away, and run on while the screen fades */
enum cargo_start_mode {
    START_WAIT = 0,
    START_BOARD = 1,
    START_READY = 2,
    START_PULL = 3,
    START_RIDE = 4,
    START_DONE = 6
};

struct cargo_start_vars {
    /* the cars' object in the collision world, or NULL when the first step
     * ran in the set editor */
    NJS_OBJECT *world;
    Float speed;
    Float top; /* the height the cars rise to */
};

#define VARS(work) ((struct cargo_start_vars *)&(work)->vars)

static void cargo_start_run(struct task *task);
static void cargo_start_draw(struct task *task);
static void cargo_start_destroy(struct task *task);

void cargo_start_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_force *force = task->force;
    NJS_OBJECT *world;

    work->ang.x = 0;
    work->ang.z = 0;
    work->mode = START_WAIT;
    VARS(work)->speed = 0.0F;
    VARS(work)->top = work->pos.y + RISE;
    force->turn.x = 0;
    force->turn.y = 0;
    force->turn.z = 0;
    force->speed.x = 0.0F;
    force->speed.y = 0.0F;
    force->speed.z = 0.0F;
    if (!placement_editing(task)) {
        VARS(work)->world = world = world_object_alloc();
        world->evalflags = NJD_EVAL_BREAK;
        world->scl[0] = 1.0F;
        world->scl[1] = 1.0F;
        world->scl[2] = 1.0F;
        world->ang[0] = 0;
        world->ang[1] = work->ang.y;
        world->ang[2] = 0;
        world->pos[0] = work->pos.x;
        world->pos[1] = work->pos.y;
        world->pos[2] = work->pos.z;
        world->model = cargo_car_world_object.model;
        world->child = NULL;
        world->sibling = NULL;
        world_add(WORLD_LARGE | WORLD_MOVES, task, world);
        work->flags |= WORLD_TESTED;
    }
    task->step = cargo_start_run;
    task->draw = cargo_start_draw;
    task->destructor = cargo_start_destroy;
}

/* pad_disable stops the pads as player 0 or 1 comes within NEAR. pad_enable
 * gives them back as player 0 is set down in the tunnel. cargo_start_destroy
 * gives them back if the task ends first */
static void cargo_start_run(struct task *task)
{
    struct task_work *work = task->work;
    struct task_force *force = task->force;
    Float speed;

    cargo_start_draw(task);
    if (edit_mode) {
        Float scale = MARKER_SCALE;

        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        njScale(NULL, scale, scale, scale);
        draw_tree(&marker_object);
        njPopMatrix(1);
    } else {
        switch (work->mode) {
        case START_WAIT:
            if (player_in_range(&work->pos, NEAR)) {
                work->flags |= WORLD_TESTED;
                work->mode = START_BOARD;
                if (pad_is_enabled()) {
                    pad_disable();
                }
            } else {
                work->flags &= ~WORLD_TESTED;
            }
            break;
        case START_BOARD:
            if (player_stands_on(0, task)) {
                work->timer = 0;
                VARS(work)->speed = 0.0F;
                work->mode = START_READY;
            }
            break;
        case START_READY:
            ++TASK_TIMER(work);
            if (TASK_TIMER(work) >= SOUND_STEPS) {
                sound_play_for(SE_TRAIN, (int)task, 1, 0, 2);
            }
            if (TASK_TIMER(work) > WAIT_STEPS) {
                work->timer = 0;
                work->mode = START_PULL;
            }
            break;
        case START_PULL:
            ++TASK_TIMER(work);
            sound_play_for(SE_TRAIN, (int)task, 1, 0, 2);
            if (TASK_TIMER(work) > FADE_STEPS) {
                work->timer = 0;
                work->mode = START_RIDE;
                task_create(TASK_WORK, 3, cargo_fade_start);
            }
            speed = VARS(work)->speed;
            if (speed < TOP_SPEED) {
                speed += ACCEL;
                if (speed >= TOP_SPEED) {
                    speed = TOP_SPEED;
                }
            }
            if (work->pos.y < VARS(work)->top) {
                work->pos.y += RISE_SPEED;
                if (work->pos.y > VARS(work)->top) {
                    work->pos.y = VARS(work)->top;
                }
            }
            VARS(work)->speed = speed;
            force->speed.x = -speed;
            work->pos.x -= speed;
            VARS(work)->world->pos[1] = work->pos.y;
            VARS(work)->world->pos[0] = work->pos.x;
            break;
        case START_RIDE:
            work->pos.x -= VARS(work)->speed;
            VARS(work)->world->pos[0] = work->pos.x;
            ++TASK_TIMER(work);
            if (TASK_TIMER(work) > RIDE_STEPS) {
                if (!pad_is_enabled()) {
                    pad_enable();
                }
                player_set_position(0, TUNNEL_X, TUNNEL_Y, TUNNEL_Z);
                work->mode = START_DONE;
            }
            break;
        case START_DONE:
            task_kill(task);
            break;
        }
    }
}

static void cargo_start_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (frame_step == 0) {
        njSetTexture(&hotshelter4_texlist);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        draw_tree(&cargo_car_object);
        njTranslate(NULL, 0.0F, 0.0F, CAR_SPACING);
        draw_tree(&cargo_car_object);
        njTranslate(NULL, 0.0F, 0.0F, CAR_SPACING);
        draw_tree(&cargo_car_object);
        njTranslate(NULL, 0.0F, 0.0F, CAR_SPACING);
        draw_tree(&cargo_container_b_object);
        draw_tree(&cargo_car_object);
        njTranslate(NULL, 0.0F, 0.0F, CAR_SPACING);
        draw_tree(&cargo_container_b_object);
        draw_tree(&cargo_car_object);
        njPopMatrix(1);
    }
}

static void cargo_start_destroy(struct task *task)
{
    struct task_work *work = task->work;

    if (!pad_is_enabled()) {
        pad_enable();
    }
    if (VARS(work)->world != NULL) {
        world_remove(task, VARS(work)->world);
        world_object_free(VARS(work)->world);
    }
}
