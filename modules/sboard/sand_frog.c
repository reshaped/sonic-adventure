/* O SFROG: Froggy in a bubble, bobbing as the bubble turns. player 0 within
 * his reach clears the stage. the set editor draws a marker scaled as the
 * reach is */
#include <ninja.h>

#include "animal/barrier.h"
#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "player/big_kaeru_textures.h"
#include "player/player.h"
#include "progress/progress.h"
#include "sboard/sand_frog.h"
#include "stage/stage.h"
#include "system/task.h"
#include "system/util.h"

enum sand_frog_mode {
    SAND_FROG_INIT = 0,
    SAND_FROG_WAIT = 2,
    SAND_FROG_DONE = 3
};

/* Froggy floats SAND_FROG_LIFT above his position, and bobs SAND_FROG_BOB
 * up and down from there. his reach is SAND_FROG_REACH times (scl.x + 1) */
#define SAND_FROG_LIFT 10.0F
#define SAND_FROG_BOB 2.5F
#define SAND_FROG_REACH 14.0F

/* the bob's phase, in degrees, and its height */
#define SAND_FROG_PHASE(work) TASK_TIMER(work)
#define SAND_FROG_HEIGHT(work) ((work)->scl.y)

/* Froggy's model. the bubble is a barrier's, al_barria_object */
extern NJS_OBJECT big_kaeru_object;

static void sand_frog_draw(struct task *task);
static void sand_frog_bob(struct task *task);
static void sand_frog_draw_reach(struct task *task);
static void sand_frog_clear_if_reached(struct task *task);
static void sand_frog_show(struct task *task);

void sand_frog_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return;
    }
    switch (work->mode) {
    case SAND_FROG_INIT:
        work->mode = SAND_FROG_WAIT;
        task->draw = sand_frog_draw;
        break;
    case SAND_FROG_WAIT:
        if (!placement_editing(task)) {
            sand_frog_clear_if_reached(task);
        }
        if (placement_editing(task)) {
            njPrintC(NJM_LOCATION(17, 19), "<- SCALE");
            sand_frog_draw_reach(task);
        }
        sand_frog_bob(task);
        sand_frog_show(task);
        break;
    case SAND_FROG_DONE:
        break;
    }
}

/* draw Froggy, raised by SAND_FROG_LIFT and the bob, and turned by
 * work->ang. the bubble round him turns about y by the bob's phase too */
static void sand_frog_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object;
    Angle ang;

    njSetTexture(&big_kaeru_texlist);
    njPushMatrix(NULL);
    object = &big_kaeru_object;
    njTranslate(NULL, work->pos.x,
                work->pos.y + SAND_FROG_HEIGHT(work) + SAND_FROG_LIFT,
                work->pos.z);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_tree(object);
    njSetTexture(&al_barria_texlist);
    TURN_VIA(njRotateY, (Angle)(SAND_FROG_PHASE(work) * ANGLE_PER_DEGREE), ang);
    draw_tree(&al_barria_object);
    njPopMatrix(1);
}

/* advance the bob's phase a degree, and set its height from the phase */
static void sand_frog_bob(struct task *task)
{
    struct task_work *work = task->work;

    SAND_FROG_PHASE(work)++;
    SAND_FROG_HEIGHT(work) =
        njSin((Angle)(SAND_FROG_PHASE(work) * ANGLE_PER_DEGREE)) *
        SAND_FROG_BOB;
}

/* draw marker_object, a sphere of radius 10, at Froggy's position. it is
 * scaled by scl.x + 1, as the reach is */
static void sand_frog_draw_reach(struct task *task)
{
    struct task_work *work = task->work;
    /* @hack type: the scale as a vector's x, which keeps it on the stack
     * across njTranslate (FMOV.S FR2,@R15 at 0x0c904194); as a Float, SHC
     * keeps it in FR15, the function runs 10 bytes short, and 40 of 41 words
     * differ */
    NJS_POINT3 scale;

    scale.x = work->scl.x + 1.0F;
    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
    njScale(NULL, scale.x, scale.x, scale.x);
    draw_tree(&marker_object);
    njPopMatrix(1);
}

/* clear the stage if player 0 is within Froggy's reach. the step then does
 * nothing more, in SAND_FROG_DONE */
static void sand_frog_clear_if_reached(struct task *task)
{
    struct task_work *work = task->work;

    if (player_in_range(&work->pos, (work->scl.x + 1.0F) * SAND_FROG_REACH) ==
        1) {
        stage_clear();
        work->mode = SAND_FROG_DONE;
    }
}

/* draw Froggy unless Tails has cleared Sand Hill. the task's draw, which
 * runs while the game is paused, draws him either way */
static void sand_frog_show(struct task *task)
{
    if (!event_flag_check(FLAG_TAILS_CLEAR_SAND_BOARD)) {
        sand_frog_draw(task);
    }
}
