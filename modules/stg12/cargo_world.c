/* the cargo tunnel's walls and track bed in the collision world. each keeps
 * to player 0's z */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/world.h"
#include "player/player.h"
#include "stage/stage.h"
#include "stg12/cargo_ride.h"
#include "stg12/cargo_world.h"
#include "system/task.h"

extern NJS_OBJECT cargo_walls_object;
extern NJS_OBJECT cargo_bed_object;

/* the walls and the bed stand at the train's height */
#define WORLD_Y 9990.0F

/* the bed's force for player 0 moves a player 0 on it BED_SPEED along z each
 * step. once player 0 has stayed on, it moves BED_FAST_SPEED and turns BED_TURN
 * about y each step. that lasts until player 0 leaves the bed */
#define BED_SPEED (-2.0F)
#define BED_FAST_SPEED (-5.0F)
#define BED_TURN 2000

/* as player 0 lands on the bed, the bed sets player 0's speed to this, straight
 * up */
#define BOUNCE_SPEED 1.8F
/* from player 0's landing, the bed looks for player 0 again once its count of
 * steps passes LANDED_STEPS. player 0 must then stand on it more than
 * STAY_STEPS steps in a row to speed it up */
#define LANDED_STEPS 60
#define STAY_STEPS 20

/* in work->submode: the walls' task or the bed's */
enum cargo_world_kind { CARGO_WALLS, CARGO_BED };

/* the bed's modes. the walls' task keeps mode 0 */
enum cargo_world_mode { BED_WAIT = 2, BED_LANDED, BED_CARRY, BED_FAST };

/* work->tick counts the steps player 0 stays on the bed. TASK_TIMER(work)
 * holds the stage and act the task started in, which nothing reads */
struct cargo_world_vars {
    NJS_OBJECT *object;
    Uint8 unk_04[4];
    int steps; /* the steps in BED_LANDED */
};

#define VARS(work) ((struct cargo_world_vars *)&(work)->vars)

static void cargo_world_init(struct task *task);
static void cargo_world_step(struct task *task);
static void cargo_world_destroy(struct task *task);

void cargo_world_start(void)
{
    struct task *task;

    task = task_create(TASK_WORK | TASK_FORCE, 3, cargo_world_init);
    task->work->submode = CARGO_WALLS;
    task = task_create(TASK_WORK | TASK_FORCE, 3, cargo_world_init);
    task->work->submode = CARGO_BED;
}

static void cargo_world_init(struct task *task)
{
    struct task_work *work = task->work;
    struct task_force *force = task->force;
    NJS_OBJECT *object;

    work->pos.y = WORLD_Y;
    work->pos.x = 0.0F;
    TASK_TIMER(work) = stage_get_stage_act();
    switch (work->submode) {
    case CARGO_WALLS:
        force->turn.x = 0;
        force->turn.y = 0;
        force->turn.z = 0;
        force->speed.x = 0.0F;
        force->speed.y = 0.0F;
        force->speed.z = 0.0F;
        VARS(work)->object = object = world_object_alloc();
        object->evalflags =
            NJD_EVAL_UNIT_SCL | NJD_EVAL_BREAK | NJD_EVAL_ZXY_ANG;
        object->scl[0] = 1.0F;
        object->scl[1] = 1.0F;
        object->scl[2] = 1.0F;
        object->ang[0] = 0;
        object->ang[1] = 0;
        object->ang[2] = 0;
        object->pos[0] = work->pos.x;
        object->pos[1] = work->pos.y;
        object->pos[2] = work->pos.z;
        object->model = cargo_walls_object.model;
        object->child = NULL;
        object->sibling = NULL;
        world_add(WORLD_LARGE | WORLD_MOVES | WORLD_SOLID, task, object);
        work->flags |= WORLD_TESTED;
        break;
    case CARGO_BED:
        force->turn.x = 0;
        force->turn.y = 0;
        force->turn.z = 0;
        force->speed.x = 0.0F;
        force->speed.y = 0.0F;
        force->speed.z = BED_SPEED;
        VARS(work)->object = object = world_object_alloc();
        object->evalflags =
            NJD_EVAL_UNIT_SCL | NJD_EVAL_BREAK | NJD_EVAL_ZXY_ANG;
        object->scl[0] = 1.0F;
        object->scl[1] = 1.0F;
        object->scl[2] = 1.0F;
        object->ang[0] = 0;
        object->ang[1] = 0;
        object->ang[2] = 0;
        object->pos[0] = work->pos.x;
        object->pos[1] = work->pos.y;
        object->pos[2] = work->pos.z;
        object->model = cargo_bed_object.model;
        object->child = NULL;
        object->sibling = NULL;
        world_add(WORLD_LARGE | WORLD_MOVES | WORLD_WATER, task, object);
        work->flags |= WORLD_TESTED;
        work->mode = BED_WAIT;
        break;
    }
    task->step = cargo_world_step;
    task->destructor = cargo_world_destroy;
}

static void cargo_world_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_force *force = task->force;
    NJS_POINT3 pos;
    Float z;

    if (!cargo_ride_on) {
        task_kill(task);
        return;
    }
    player_get_history(0, 0, &pos, NULL);
    z = pos.z;
    work->pos.z = z;
    VARS(work)->object->pos[2] = z;
    if (work->submode == CARGO_BED) {
        switch (work->mode) {
        case BED_WAIT:
            if (player_stands_on(0, task)) {
                work->tick = 0;
                VARS(work)->steps = 0;
                work->mode = BED_LANDED;
                player_set_speed(0, 0.0F, BOUNCE_SPEED, 0.0F);
            }
            break;
        case BED_LANDED:
            VARS(work)->steps++;
            if (VARS(work)->steps > LANDED_STEPS) {
                VARS(work)->steps = 0;
                work->mode = BED_CARRY;
            }
            break;
        case BED_CARRY:
            if (player_stands_on(0, task)) {
                work->tick++;
                if (work->tick > STAY_STEPS) {
                    work->mode = BED_FAST;
                    force->speed.z = BED_FAST_SPEED;
                    force->turn.y = BED_TURN;
                }
            } else {
                work->tick = 0;
                work->mode = BED_WAIT;
            }
            break;
        case BED_FAST:
            if (!player_stands_on(0, task)) {
                work->tick = 0;
                work->mode = BED_WAIT;
                force->speed.z = BED_SPEED;
                force->turn.y = 0;
            }
            break;
        }
    }
}

static void cargo_world_destroy(struct task *task)
{
    struct task_work *work = task->work;

    if (VARS(work)->object) {
        world_remove(task, VARS(work)->object);
        world_object_free(VARS(work)->object);
    }
}
