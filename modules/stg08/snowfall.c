/* O Dd, the snow that falls around player 0 on the board run */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "camera/camera.h"
#include "player/player.h"
#include "stg08/snowfall.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_TEXNAME obj_icecap_textures[100];

/* the snow starts once player 0 comes down to SNOW_START. it grows by a
 * flake a step to SNOW_FLAKES. after that, each step with player 0 below
 * SNOW_END allows a flake fewer. the snow ends at none */
#define SNOW_START (-6650.0F)
#define SNOW_END (-11800.0F)
#define SNOW_FLAKES 300
/* SNOW_X and SNOW_Z lie this far ahead of player 0, the way the camera
 * faces */
#define SNOW_AHEAD 50.0F

/* a flake starts at random in a box FLAKE_WIDTH square and FLAKE_HEIGHT
 * tall. the box centers on SNOW_X and SNOW_Z, and on player 0's height */
#define FLAKE_WIDTH 160.0F
#define FLAKE_HEIGHT 40.0F
/* a flake leaves sight at FLAKE_SIGHT or more either way of the camera's
 * heading. it also leaves more than FLAKE_BELOW below player 0, or
 * FLAKE_ABOVE above */
#define FLAKE_SIGHT 0x1e00
#define FLAKE_BELOW 30.0F
#define FLAKE_ABOVE 50.0F
/* a flake falls FLAKE_FALL a step. its sprite turns FLAKE_SWAY a step about
 * y, swinging between FLAKE_SWAY_MOST either way */
#define FLAKE_FALL 0.8F
#define FLAKE_SWAY 0x800
#define FLAKE_SWAY_MOST 0x2800

/* in a flake's work->flags: set while its sway turns back, toward
 * -FLAKE_SWAY_MOST */
#define FLAKE_BACK 0x100
/* the sway, which the sprite adds to the camera's heading */
#define FLAKE_SWAY_ANG(work) ((work)->ang.y)

struct snowfall_vars {
    Sint32 phase; /* SNOW_GROW, then SNOW_THIN */
    Sint32 count; /* flakes made, less one for each step of thinning */
    Sint32 most; /* flakes allowed. at 0, the snow ends */
};

#define VARS(work) ((struct snowfall_vars *)&(work)->vars)

enum snowfall_phase { SNOW_GROW, SNOW_THIN };

/* TRUE while snowfall_draw runs the flakes' steps, which then draw the
 * flakes without moving them */
#define SNOW_DRAWING(work) ((work)->mode)
/* the flakes due to end as they leave sight, rather than start again */
#define SNOW_RETIRE(work) ((work)->timer)
/* the point the flakes start around, in x and z */
#define SNOW_X(work) ((work)->scl.x)
#define SNOW_Z(work) ((work)->scl.y)

/* the flakes' sprite draws OBJ_ICECAP's snow_sprite, the only texture its
 * texlist names */
static NJS_TEXLIST snowfall_texlist = {
#include "stg08/snowfall/snowfall_texlist.inc"
};
static NJS_TEXANIM snowfall_texanim = {
#include "stg08/snowfall/snowfall_texanim.inc"
};
NJS_SPRITE snowfall_sprite = {
#include "stg08/snowfall/snowfall_sprite.inc"
};

/* put a flake at random in its box, its sway at 0 */
static void snowfall_flake_place(struct task *task)
{
    struct task_work *work = task->work;
    NJS_POINT3 *pos = &work->pos;
    struct task_work *snow = task->parent->work;

    pos->x = SNOW_X(snow) + (rand() * (FLAKE_WIDTH * RAND_SCALE)) -
             (FLAKE_WIDTH / 2);
    pos->y = player_work[0]->pos.y + (rand() * (FLAKE_HEIGHT * RAND_SCALE)) -
             (FLAKE_HEIGHT / 2);
    pos->z = SNOW_Z(snow) + (rand() * (FLAKE_WIDTH * RAND_SCALE)) -
             (FLAKE_WIDTH / 2);
    FLAKE_SWAY_ANG(work) = 0;
    /* the sway turns forward first. the store also sets every other flag
     * bit, and nothing reads a flake's other flag bits */
    work->flags = ~FLAKE_BACK;
}

/* a flake's step. unless SNOW_DRAWING, a flake out of sight starts again or
 * ends, and any other falls and sways. a flake that didn't start again or end
 * then draws, on a frame's first step only */
static void snowfall_flake_step(struct task *task)
{
    struct task_work *snow = task->parent->work;
    struct task_work *work = task->work;
    NJS_POINT3 *pos = &work->pos;
    struct task_work *camera = camera_work;
    struct angles *view = &camera->ang;
    int height;
    /* how far the flake lies off the camera's heading, seen from the camera.
     * as a Sint16, the bearing takes the shorter way round. off is the
     * bearing widened to an int */
    Sint16 bearing;
    int off;
    Sint16 sway;
    Angle ang;

    if (!SNOW_DRAWING(snow)) {
        height = player_work[0]->pos.y;
        bearing =
            (Angle)(atan2f(camera->pos.x - pos->x, camera->pos.z - pos->z) *
                    ANGLE_PER_RADIAN) -
            view->y;
        if ((off = bearing) >= FLAKE_SIGHT || off <= -FLAKE_SIGHT ||
            pos->y < height - FLAKE_BELOW || pos->y > height + FLAKE_ABOVE) {
            if (SNOW_RETIRE(snow)) {
                SNOW_RETIRE(snow)--;
                task_kill(task);
            } else {
                snowfall_flake_place(task);
            }
            return;
        }
        /* @hack redundant: a test of height and off that never runs, which
         * keeps the bearing in R5 from 0x0c90145e and off, its sign-extended
         * copy, in R4; without it, the two trade and 5 of 191 words differ */
        if (0) {
            if (height && off) {
            }
        }
        sway = FLAKE_SWAY_ANG(work);
        if (!(work->flags & FLAKE_BACK)) {
            sway += FLAKE_SWAY;
            if (sway >= FLAKE_SWAY_MOST) {
                /* FLAKE_BACK is clear here, so the add sets it */
                work->flags += FLAKE_BACK;
            }
        } else {
            sway -= FLAKE_SWAY;
            if (sway <= -FLAKE_SWAY_MOST) {
                /* FLAKE_BACK is set here, so the subtraction clears it */
                work->flags -= FLAKE_BACK;
            }
        }
        FLAKE_SWAY_ANG(work) = sway;
        pos->y -= FLAKE_FALL;
    }
    if (!frame_step) {
        njPushMatrix(NULL);
        njTranslateV(NULL, pos);
        TURN_VIA(njRotateY, view->y + FLAKE_SWAY_ANG(work), ang);
        TURN_VIA(njRotateX, view->x, ang);
        njDrawSprite3D(&snowfall_sprite, 0, NJD_SPRITE_ALPHA);
        njPopMatrix(1);
    }
}

static void snowfall_destroy(struct task *task)
{
    task_destroy_children(task);
}

static void snowfall_draw(struct task *task)
{
    SNOW_DRAWING(task->work) = TRUE;
    njFogDisable();
    texture_set_stage();
    task_step_children(task);
    njFogEnable();
}

static void snowfall_run(struct task *task)
{
    struct task_work *work = task->work;
    NJS_POINT3 *player = &player_work[0]->pos;
    Angle yaw = camera_work->ang.y;

    if (VARS(work)->most) {
        switch (VARS(work)->phase) {
        case SNOW_GROW:
            if (VARS(work)->count < VARS(work)->most) {
                snowfall_flake_place(
                    task_create_child(TASK_MOTION | TASK_WORK | TASK_FORCE,
                                      snowfall_flake_step, task));
                VARS(work)->count++;
            } else {
                VARS(work)->phase++;
            }
            break;
        case SNOW_THIN:
            if (player->y < SNOW_END) {
                VARS(work)->count--;
                VARS(work)->most--;
                if (VARS(work)->most < 0) {
                    VARS(work)->most = 0;
                }
                SNOW_RETIRE(work) = 1;
            }
            break;
        }
        SNOW_X(work) = (-njSin(yaw) * SNOW_AHEAD) + player->x;
        SNOW_Z(work) = (-njCos(yaw) * SNOW_AHEAD) + player->z;
        SNOW_DRAWING(work) = FALSE;
        njFogDisable();
        texture_set_stage();
        task_step_children(task);
        njFogEnable();
    } else {
        task_destroy(task);
    }
}

void snowfall_step(struct task *task)
{
    struct task_work *work = task->work;

    if (player_work[0]->pos.y > SNOW_START) {
        return;
    }
    VARS(work)->most = SNOW_FLAKES;
    /* @hack spelling: count and SNOW_RETIRE cleared in one chain, which makes
     * their 0 in R0 (MOV #0,R0 at 0x0c901754); as two statements, SHC makes
     * it in R6, reads count back for the phase, and 11 of 25 words differ */
    VARS(work)->count = SNOW_RETIRE(work) = 0;
    /* @hack spelling: the phase set from count, whose 0 SHC copies from R0
     * to R3 and stores from there (0x0c901758, 0x0c901762); set to
     * SNOW_GROW, the 0 is made in R6 and 12 of 25 words differ */
    VARS(work)->phase = VARS(work)->count;
    task->destructor = snowfall_destroy;
    task->step = snowfall_run;
    task->draw = snowfall_draw;
}
