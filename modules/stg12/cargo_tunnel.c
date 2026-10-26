/* the cargo ride's tunnel walls, drawn scrolling past the train */
#include <ninja.h>
#include <sg_xpt.h>

#include "draw/tree.h"
#include "player/player.h"
#include "stage/stage.h"
#include "stg12/cargo_ride.h"
#include "stg12/cargo_tunnel.h"
#include "stg12/texlists.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/util.h"

extern NJS_OBJECT cargo_tunnel_a_object;
extern NJS_OBJECT cargo_tunnel_b_object;

#define TUNNEL_Y 10000.0F

/* the walls are PAIRS pairs of pieces, A then B, each PIECE_LENGTH long. the
 * first piece stands AHEAD plus the scroll along z from the tunnel's pos. each
 * later piece stands PIECE_LENGTH further along -z */
#define PIECE_LENGTH 260.0F
#define PAIRS 10
#define AHEAD (PIECE_LENGTH * 9)
#define PAIR_LENGTH (PIECE_LENGTH * 2)

/* the walls scroll back SCROLL_SPEED a step, while the train runs at most 5
 * a step. so the train seems faster than it is */
#define SCROLL_SPEED 20.0F
/* once the ride is ending, the scroll slows by SLOW_DOWN a step to
 * END_SPEED */
#define END_SPEED 2.0F
#define SLOW_DOWN 0.1F

/* the tunnel counts its steps in TUNNEL_RUN until the count passes RUN_STEPS,
 * then stays in TUNNEL_COUNTED. nothing else reads the mode */
#define RUN_STEPS 600
enum cargo_tunnel_mode { TUNNEL_RUN = 2, TUNNEL_COUNTED = 3 };

/* TASK_TIMER(work) holds the stage and act the tunnel started in, which
 * nothing reads */
struct cargo_tunnel_vars {
    Float scroll;
    Float speed;
    int steps; /* counted in TUNNEL_RUN */
};

#define VARS(work) ((struct cargo_tunnel_vars *)&(work)->vars)

static void cargo_tunnel_step(struct task *task);
static void cargo_tunnel_draw(struct task *task);

void cargo_tunnel_start(struct task *task)
{
    struct task_work *work = task->work;

    work->ang.x = 0;
    work->ang.z = 0;
    work->pos.x = 0.0F;
    work->pos.y = TUNNEL_Y;
    TASK_TIMER(work) = stage_get_stage_act();
    VARS(work)->scroll = 0.0F;
    VARS(work)->speed = SCROLL_SPEED;
    work->mode = TUNNEL_RUN;
    task->step = cargo_tunnel_step;
    task->draw = cargo_tunnel_draw;
}

/* the tunnel moves along z by whole pairs, so that it stays within a pair of
 * player 0. a whole pair leaves each piece where one of its kind was */
static void cargo_tunnel_step(struct task *task)
{
    struct task_work *work = task->work;
    NJS_POINT3 pos;
    Float pairs;

    if (!cargo_ride_on) {
        task_kill(task);
        return;
    }
    player_get_history(0, 0, &pos, NULL);
    /* @hack spelling: the difference, its scale and its round-off in three
     * statements, which keeps the difference in FR4, njRoundOff's argument,
     * FSUB FR3,FR4 at 0x0c9124fc. as one statement, SHC subtracts into FR2 and
     * copies it to FR4, and 31 of 92 words differ */
    pairs = pos.z - work->pos.z;
    pairs *= 1.0F / PAIR_LENGTH;
    pairs = njRoundOff(pairs);
    work->pos.z += pairs * PAIR_LENGTH;
    if (cargo_ride_ending) {
        VARS(work)->speed -= SLOW_DOWN;
        if (VARS(work)->speed <= END_SPEED) {
            VARS(work)->speed = END_SPEED;
        }
    } else if (work->mode == TUNNEL_RUN) {
        ++VARS(work)->steps;
        if (VARS(work)->steps > RUN_STEPS) {
            VARS(work)->steps = 0;
            work->mode = TUNNEL_COUNTED;
        }
    }
    VARS(work)->scroll -= VARS(work)->speed;
    if (VARS(work)->scroll <= PAIR_LENGTH) {
        VARS(work)->scroll += PAIR_LENGTH;
    }
    cargo_tunnel_draw(task);
}

static void cargo_tunnel_draw(struct task *task)
{
    struct task_work *work = task->work;
    Float ahead;
    Angle ang;
    int i;

    if (frame_step == 0) {
        /* @hack spelling: ahead assigned before the texture is set, which
         * stores it on the stack in the slot of njSetTexture's call, at
         * 0x0c912622. assigned at its use, 91 of 102 words differ */
        ahead = AHEAD;
        njSetTexture(&hotshelter4_texlist);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        /* @hack redundant: the scroll times 1, which SHC adds to ahead in one
         * FMAC, at 0x0c91264c. as a plain sum, 67 of 102 words differ */
        njTranslate(NULL, 0.0F, 0.0F, (VARS(work)->scroll * 1.0F) + ahead);
        for (i = 0; i < PAIRS; ++i) {
            draw_tree(&cargo_tunnel_a_object);
            njTranslate(NULL, 0.0F, 0.0F, -PIECE_LENGTH);
            draw_tree(&cargo_tunnel_b_object);
            njTranslate(NULL, 0.0F, 0.0F, -PIECE_LENGTH);
        }
        njPopMatrix(1);
    }
}
