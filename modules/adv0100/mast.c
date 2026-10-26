/* O MAST platform that turns about y, and turns player 0 standing on it */
#include <ninja.h>

#include "adv0100/mast.h"
#include "collision/world.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "player/player.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT mast_object;
extern NJS_OBJECT mast_solid_object;

/* the mast's turn each step, a whole turn every 512 steps */
#define MAST_TURN 0x80

/* the surface's flags in the collision world */
#define SOLID_FLAGS (WORLD_HUGE | WORLD_MOVES | WORLD_SOLID)
/* player 0's distance within which the collision world tests the surface */
#define TEST_RANGE 200.0F

struct mast_vars {
    NJS_OBJECT *solid; /* the surface's copy in the collision world */
    NJS_OBJECT *from; /* the node whose model solid takes */
};

#define VARS(work) ((struct mast_vars *)&(work)->vars)

enum mast_mode { MAST_INIT, MAST_RUN };

static void mast_draw(struct task *task);
static void mast_destroy(struct task *task);
static void mast_draw_work(struct task_work *work);

/* add the surface to the collision world on the first step, then turn the mast
 * and its surface by MAST_TURN each step. the turn goes in player 0's force
 * record, so a player 0 standing on the mast turns with it. the record the
 * other players read holds no turn */
void egg_carrier_mast_step(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *solid;
    struct task_force *force;

    if (placement_try_leave(task)) {
        return;
    }
    switch (work->mode) {
    case MAST_INIT:
        VARS(work)->from = &mast_solid_object;
        VARS(work)->solid = solid = world_object_alloc();
        solid->evalflags = NJD_EVAL_BREAK;
        solid->scl[0] = 1.0F;
        solid->scl[1] = 1.0F;
        solid->scl[2] = 1.0F;
        solid->ang[0] = 0;
        solid->ang[1] = 0;
        solid->ang[2] = 0;
        solid->pos[0] = work->pos.x;
        solid->pos[1] = work->pos.y;
        solid->pos[2] = work->pos.z;
        solid->model = VARS(work)->from->model;
        solid->child = NULL;
        solid->sibling = NULL;
        world_add(SOLID_FLAGS, task, solid);
        task->destructor = mast_destroy;
        task->draw = mast_draw;
        work->mode = MAST_RUN;
        break;
    case MAST_RUN:
        force = task->force;
        work->ang.y += MAST_TURN;
        VARS(work)->solid->ang[1] = work->ang.y;
        force->turn.y = MAST_TURN;
        break;
    }
    work->flags &= ~WORLD_TESTED;
    if (njDistanceP2P(&player_work[0]->pos, &work->pos) < TEST_RANGE) {
        work->flags |= WORLD_TESTED;
    }
    mast_draw_work(task->work);
}

static void mast_draw(struct task *task)
{
    mast_draw_work(task->work);
}

static void mast_destroy(struct task *task)
{
    struct task_work *work = task->work;

    if (VARS(work)->solid != NULL) {
        world_remove(task, VARS(work)->solid);
        world_object_free(VARS(work)->solid);
    }
}

/* draw the mast at the surface's y angle */
static void mast_draw_work(struct task_work *work)
{
    Angle ang;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, VARS(work)->solid->ang[1], ang);
    draw_tree(&mast_object);
    njPopMatrix(1);
}
