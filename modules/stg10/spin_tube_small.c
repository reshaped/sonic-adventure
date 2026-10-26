/* O SPIN_TUBES small spinning tube that carries player 0 around */
#include <ninja.h>

#include "collision/world.h"
#include "debug/editor.h"
#include "debug/print.h"
#include "draw/model.h"
#include "object/placement.h"
#include "player/player.h"
#include "sound/sound.h"
#include "stg10/spin_tube_small.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_MODEL spin_tube_small_frame_model;
extern NJS_MODEL spin_tube_small_inner_model_0;
extern NJS_MODEL spin_tube_small_inner_model_1;
extern NJS_MODEL spin_tube_small_lining_model;

/* destroyed beyond 1000 units of the camera and player 0 */
#define RANGE (1000.0F * 1000.0F)
/* the collision world tests the lining only while a player is this near */
#define NEAR 280.0F

/* the inner tube's turn about z each step, which work->timer adds up */
#define TURN_STEP 0x200

/* the lining's flags in the collision world: ground that players grip more
 * than most */
#define LINING_FLAGS WORLD_GRIP_HIGH

/* the tube's sound, which plays from its first step until it is destroyed */
#define SE_TUBE 286

struct tube_vars {
    NJS_OBJECT *object; /* the lining's object in the collision world */
};

#define VARS(work) ((struct tube_vars *)&(work)->vars)

enum tube_mode { TUBE_INIT, TUBE_RUN };

static void spin_tube_small_init(struct task *task);
static void spin_tube_small_add(struct task *task);
static void spin_tube_small_turn(struct task *task);
static void spin_tube_small_draw(struct task *task);
static void spin_tube_small_destroy(struct task *task);
static void spin_tube_small_remove(struct task *task);

void spin_tube_small_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave_by(task, RANGE)) {
        return;
    }
    if (!placement_editing(task)) {
        switch (work->mode) {
        case TUBE_INIT:
            spin_tube_small_init(task);
            break;
        case TUBE_RUN:
            spin_tube_small_draw(task);
            spin_tube_small_turn(task);
            break;
        }
    } else {
        switch (work->mode) {
        case TUBE_INIT:
            spin_tube_small_init(task);
            break;
        case TUBE_RUN:
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_X), "-> ROTATE X");
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Y), "-> ROTATE Y");
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Z), "-> ROTATE Z");
            spin_tube_small_draw(task);
            spin_tube_small_turn(task);
            break;
        }
    }
}

static void spin_tube_small_init(struct task *task)
{
    struct task_work *work = task->work;

    task->draw = spin_tube_small_draw;
    task->destructor = spin_tube_small_destroy;
    sound_play_held(SE_TUBE, (int)task, 1, 0);
    spin_tube_small_add(task);
    work->mode = TUBE_RUN;
}

/* put the lining in the collision world at the tube's position and angles.
 * clear the force that carries the tube's riders, and the inner tube's turn */
static void spin_tube_small_add(struct task *task)
{
    struct task_work *work = task->work;
    struct task_force *force = task->force;
    NJS_OBJECT *object;

    force->turn.x = 0;
    force->turn.y = 0;
    force->turn.z = 0;
    force->speed.x = 0.0F;
    force->speed.y = 0.0F;
    force->speed.z = 0.0F;
    VARS(work)->object = object = world_object_alloc();
    object->evalflags = 0;
    object->scl[0] = 1.0F;
    object->scl[1] = 1.0F;
    object->scl[2] = 1.0F;
    object->ang[0] = work->ang.x;
    object->ang[1] = work->ang.y;
    object->ang[2] = work->ang.z;
    object->pos[0] = work->pos.x;
    object->pos[1] = work->pos.y;
    object->pos[2] = work->pos.z;
    object->model = &spin_tube_small_lining_model;
    object->child = NULL;
    object->sibling = NULL;
    world_add(LINING_FLAGS, task, object);
    /* @hack spelling: the timer reached through task->work, not work, which
     * loads the work again (MOV.L @(32,R12),R3 at 0x0c9061c2); through
     * work, the function runs 2 bytes short and 8 of 64 words differ */
    task->work->timer = 0;
}

/* turn the inner tube a step. force->speed, which carries the riders, becomes
 * the move this turn gives player 0's latest recorded position */
static void spin_tube_small_turn(struct task *task)
{
    struct task_work *work = task->work;
    struct task_force *force = task->force;
    NJS_POINT3 pos;
    NJS_POINT3 local;
    NJS_VECTOR turned;
    Angle ang;

    if (player_in_range(&work->pos, NEAR)) {
        work->flags |= WORLD_TESTED;
    } else {
        work->flags &= ~WORLD_TESTED;
    }
    work->timer += TURN_STEP;
    player_get_history(0, 0, &pos, NULL);
    njPushMatrix(&_unit_matrix_);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    TURN_VIA(njRotateY, work->ang.y, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    njPushMatrix(NULL);
    njInvertMatrix(NULL);
    njCalcPoint(NULL, &pos, &local);
    njPopMatrix(1);
    njPushMatrix(&_unit_matrix_);
    njRotateZ(NULL, TURN_STEP);
    njCalcVector(NULL, &local, &turned);
    njPopMatrix(1);
    njSubVector(&turned, &local);
    njCalcVector(NULL, &turned, &force->speed);
    njPopMatrix(1);
}

static void spin_tube_small_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (frame_step == 0) {
        texture_set_stage();
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateZ, work->ang.z, ang);
        TURN_VIA(njRotateY, work->ang.y, ang);
        TURN_VIA(njRotateX, work->ang.x, ang);
        draw_model(&spin_tube_small_frame_model);
        TURN_VIA(njRotateZ, TASK_TIMER(work), ang);
        draw_model(&spin_tube_small_inner_model_0);
        draw_model(&spin_tube_small_inner_model_1);
        njPopMatrix(1);
    }
}

static void spin_tube_small_destroy(struct task *task)
{
    sound_stop_id((int)task);
    spin_tube_small_remove(task);
}

static void spin_tube_small_remove(struct task *task)
{
    struct task_work *work = task->work;

    world_remove(task, VARS(work)->object);
    world_object_free(VARS(work)->object);
}
