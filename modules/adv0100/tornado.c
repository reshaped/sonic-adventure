/* O TORNADO2, Tails' plane, parked on the deck for a player to stand on */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv0100/egg_carrier.h"
#include "adv0100/texlists.h"
#include "adv0100/tornado.h"
#include "collision/world.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "player/player.h"
#include "progress/progress.h"
#include "stage/stage.h"
#include "system/task.h"
#include "system/util.h"

extern NJS_OBJECT tornado_object;
extern NJS_OBJECT tornado_solid_object;

/* the plane's range for placement_try_leave_by: 800, squared */
#define TORNADO_RANGE (800.0F * 800.0F)

#define SOLID_FLAGS (WORLD_MEDIUM | WORLD_SOLID)

/* the collision world tests the plane while player 0 is within
 * TESTED_RANGE */
#define TESTED_RANGE 40.0F

/* the flag each character's story keeps for the plane. while it is set,
 * the plane is off the deck */
#define FLAG_SONIC_EC_TORNADO 146
#define FLAG_TAILS_EC_TORNADO 209
#define FLAG_KNUCKLES_EC_TORNADO 270
#define FLAG_AMY_EC_TORNADO 332
#define FLAG_E102_EC_TORNADO 395
#define FLAG_BIG_EC_TORNADO 458

struct tornado_vars {
    NJS_OBJECT *solid; /* in the collision world */
    NJS_OBJECT *from; /* the object whose model solid takes */
};

#define VARS(work) ((struct tornado_vars *)&(work)->vars)

enum tornado_mode { TORNADO_INIT, TORNADO_RUN };

static void tornado_draw(struct task *task);
static void tornado_destroy(struct task *task);
static void tornado_draw_work(struct task_work *work);
static Bool tornado_gone(void);

/* put the plane's surface in the collision world, and draw the plane. the
 * step does nothing while tornado_gone is TRUE, or once the Egg Carrier has
 * sunk */
void egg_carrier_tornado_step(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *solid;

    if (placement_try_leave_by(task, TORNADO_RANGE)) {
        return;
    }
    if (tornado_gone()) {
        return;
    }
    if (egg_carrier_sunk) {
        return;
    }
    switch (work->mode) {
    case TORNADO_INIT:
        task->destructor = tornado_destroy;
        task->draw = tornado_draw;
        VARS(work)->from = &tornado_solid_object;
        VARS(work)->solid = solid = world_object_alloc();
        solid->evalflags = NJD_EVAL_BREAK;
        solid->scl[0] = 1.0F;
        solid->scl[1] = 1.0F;
        solid->scl[2] = 1.0F;
        solid->ang[0] = 0;
        solid->ang[1] = work->ang.y;
        solid->ang[2] = 0;
        solid->pos[0] = work->pos.x;
        solid->pos[1] = work->pos.y;
        solid->pos[2] = work->pos.z;
        solid->model = VARS(work)->from->model;
        solid->child = NULL;
        solid->sibling = NULL;
        world_add(SOLID_FLAGS, task, solid);
        work->mode = TORNADO_RUN;
        break;
    case TORNADO_RUN:
        break;
    }
    tornado_draw_work(work);
    work->flags &= ~WORLD_TESTED;
    if (njDistanceP2P(&player_work[0]->pos, &work->pos) < TESTED_RANGE) {
        work->flags |= WORLD_TESTED;
    }
}

static void tornado_draw(struct task *task)
{
    struct task_work *work = task->work;

    tornado_draw_work(work);
}

static void tornado_destroy(struct task *task)
{
    struct task_work *work = task->work;

    if (VARS(work)->solid != NULL) {
        world_remove(task, VARS(work)->solid);
        world_object_free(VARS(work)->solid);
    }
}

static void tornado_draw_work(struct task_work *work)
{
    njSetTexture(&ec_tornado_texlist);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateY, work->ang.y);
    draw_tree(&tornado_object);
    njPopMatrix(1);
}

/* TRUE while the current character's flag for the plane is set. FALSE for
 * Eggman and Tikal */
static Bool tornado_gone(void)
{
    switch (get_character()) {
    case CHARACTER_SONIC:
        return event_flag_check(FLAG_SONIC_EC_TORNADO);
    case CHARACTER_TAILS:
        return event_flag_check(FLAG_TAILS_EC_TORNADO);
    case CHARACTER_KNUCKLES:
        return event_flag_check(FLAG_KNUCKLES_EC_TORNADO);
    case CHARACTER_AMY:
        return event_flag_check(FLAG_AMY_EC_TORNADO);
    case CHARACTER_E102:
        return event_flag_check(FLAG_E102_EC_TORNADO);
    case CHARACTER_BIG:
        return event_flag_check(FLAG_BIG_EC_TORNADO);
    }
    return FALSE;
}
