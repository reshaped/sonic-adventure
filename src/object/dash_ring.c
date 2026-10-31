/* the dash ring, CMN_DRING or O DUSH RING in the object lists. a player
 * who passes through it is carried on toward its target */
#include <ninja.h>
#include <sg_xpt.h>

#include "debug/editor.h"
#include "draw/model.h"
#include "draw/tree.h"
#include "object/dash_ring.h"
#include "object/helpers.h"
#include "object/models.h"
#include "object/placement.h"
#include "player/player.h"
#include "sound/sound.h"
#include "stage/stage.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

/* squared ranges: the ring's, for placement_try_leave_by, and the one
 * within which a carrier takes its player */
#define RING_RANGE (1500.0F * 1500.0F)
#define TAKE_RANGE (30.0F * 30.0F)

/* how far a carrier carries its player: NO_TARGET_DISTANCE without a
 * target, else the target's distance, up to MAX_DISTANCE */
#define NO_TARGET_DISTANCE 400.0F
#define MAX_DISTANCE 1500.0F

/* a carry's phase runs to a quarter turn by PHASE_STEP a step. the
 * player's distance along the carry is its length times the phase's sine,
 * so each step moves the player less. the carry ends early once a step
 * would move it less than LEAST_MOVE */
#define PHASE_STEP 0x200
#define LEAST_MOVE 3.0F

#define SE_DASH_RING 732

/* the ring's work->scl is its target, or all zero for none */
struct dash_ring_vars {
    NJS_OBJECT *object;
};

#define VARS(work) ((struct dash_ring_vars *)&(work)->vars)

/* the ring makes a carrier for each of players 0 and 1: a child task that
 * keeps the angles the ring had then. a carrier's work->scl.x is how far it
 * carries its player. scl.y is the z, in the carrier's frame, that the
 * carry last moved the player to, and 0 between carries */
struct carrier_vars {
    struct task *player_task; /* the player's task, set as a carry starts */
    Angle phase; /* how far along the carry, up to a quarter turn */
    Sint32 player; /* the player it waits for and carries */
};

#define CARRIER(work) ((struct carrier_vars *)&(work)->vars)

enum dash_ring_mode { DASH_RING_INIT, DASH_RING_RUN };
enum carrier_mode { CARRIER_WAIT, CARRIER_CARRY };

static Bool dash_ring_start(struct task *task);
static Float dash_ring_aim(struct task *task);
static Bool dash_ring_add_carrier(struct task *task, int player,
                                  Float distance);
static void dash_ring_carrier_step(struct task *task);
static Bool dash_ring_carry(struct task *task);
static void dash_ring_draw(struct task *task);
static void dash_ring_destroy(struct task *task);

void dash_ring_step(struct task *task)
{
    struct task_work *work = task->work;
    Float distance;

    if (placement_editing(task)) {
        njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_X), "Target Position");
    }
    if (placement_try_leave_by(task, RING_RANGE)) {
        return;
    }
    switch (work->mode) {
    case DASH_RING_INIT:
        if (!dash_ring_start(task)) {
            task_kill(task);
            return;
        }
        break;
    case DASH_RING_RUN:
        task_step_children(task);
        break;
    }
    object_step_draw(task);
    if (edit_mode != 1) {
        return;
    }
    /* while the set editor is open, an arrow from the ring shows the carry */
    distance = dash_ring_aim(task);
    scratch_vector.x = 0.0F;
    scratch_vector.y = 0.0F;
    scratch_vector.z = -distance;
    njPushMatrix(&_unit_matrix_);
    TURN(njRotateX, work->ang.x);
    TURN(njRotateY, work->ang.y);
    TURN(njRotateZ, work->ang.z);
    njCalcVector(NULL, &scratch_vector, &scratch_vector2);
    njPopMatrix(1);
    arrow_draw(&work->pos, &scratch_vector2);
}

/* aim the ring, and make its two carriers. returns FALSE when a carrier
 * can't be made, else TRUE */
static Bool dash_ring_start(struct task *task)
{
    struct task_work *work = task->work;
    Float distance;

    work->mode = DASH_RING_RUN;
    VARS(work)->object = &dash_ring_object;
    task->draw = dash_ring_draw;
    task->destructor = dash_ring_destroy;
    distance = dash_ring_aim(task);
    if (!dash_ring_add_carrier(task, 0, distance) ||
        !dash_ring_add_carrier(task, 1, distance)) {
        return FALSE;
    }
    return TRUE;
}

/* turn the ring so its -z points at its target, and return the carry's
 * length. without a target, the ring keeps its angles */
static Float dash_ring_aim(struct task *task)
{
    struct task_work *work = task->work;
    Float distance;

    if (work->scl.x == 0.0F && work->scl.y == 0.0F && work->scl.z == 0.0F) {
        return NO_TARGET_DISTANCE;
    } else {
        vector_between(&work->scl, &work->pos, &scratch_vector);
        vector_aim_z(&scratch_vector, &work->ang.x, &work->ang.y);
        distance = njScalor(&scratch_vector);
        if (distance > MAX_DISTANCE) {
            distance = MAX_DISTANCE;
        }
        return distance;
    }
}

static Bool dash_ring_add_carrier(struct task *task, int player, Float distance)
{
    struct task_work *ring = task->work;
    struct task *child;
    struct task_work *work;

    child = task_create_child(TASK_WORK, dash_ring_carrier_step, task);
    if (child == NULL) {
        return FALSE;
    }
    work = child->work;
    work->pos = ring->pos;
    CARRIER(work)->player = player;
    work->scl.x = distance;
    return TRUE;
}

static void dash_ring_carrier_step(struct task *task)
{
    struct task_work *work = task->work;
    int player;
    struct task_motion *motion;
    struct player *record;
    int near;

    switch (work->mode) {
    case CARRIER_WAIT:
        near = players_near(&work->pos, TAKE_RANGE);
#ifdef GCC
        /* GCC can't see that players_near returns 0 to 3, the cases below */
        player = 0;
#endif
        switch (near) {
        case 0:
            return;
        case 1:
            player = 0;
            break;
        case 2:
            player = 1;
            break;
        case 3:
            player = CARRIER(work)->player;
            break;
        }
        if (player != CARRIER(work)->player) {
            return;
        }
        motion = player_motion[player];
        record = player_get(player);
        work->mode = CARRIER_CARRY;
        CARRIER(work)->player_task = player_get_task(player);
        player_clear_speed(motion, record);
        sound_play(SE_DASH_RING, 0, 0, 0);
        switch (get_character()) {
        case CHARACTER_TAILS:
            player_set_mode(player, PLAYER_MODE_HOVER);
            break;
        case CHARACTER_KNUCKLES:
            player_set_mode(player, PLAYER_MODE_STILL);
            break;
        }
        break;
    case CARRIER_CARRY:
        if (!dash_ring_carry(task)) {
            return;
        }
        work->mode = CARRIER_WAIT;
        player_set_mode(CARRIER(work)->player, PLAYER_MODE_RESET);
        work->scl.y = 0.0F;
        CARRIER(work)->phase = 0;
        if (get_character() == CHARACTER_TAILS) {
            player_set_mode(0, PLAYER_MODE_FLY);
        }
        break;
    }
}

/* move the player one step along the carry, and set its speed to its move
 * from its history's previous entry. returns TRUE, with the player left
 * where it is, once the step would move it less than LEAST_MOVE or the
 * phase passes a quarter turn. else FALSE */
static Bool dash_ring_carry(struct task *task)
{
    struct task_work *work = task->work;
    struct task *player_task = CARRIER(work)->player_task;
    Angle phase = CARRIER(work)->phase;
    struct task_work *carried = player_task->work;
    int player = CARRIER(work)->player;
    Float z;
    NJS_POINT3 pos;

    phase += PHASE_STEP;
    z = -(work->scl.x * njSin(phase));
    CARRIER(work)->phase = phase;
    scratch_vector.x = 0.0F;
    scratch_vector.y = 0.0F;
    scratch_vector.z = z;
    njPushMatrix(&_unit_matrix_);
    TURN(njRotateX, work->ang.x);
    TURN(njRotateY, work->ang.y);
    njCalcVector(NULL, &scratch_vector, &scratch_vector2);
    njPopMatrix(1);
    if (z - work->scl.y > -LEAST_MOVE || (Uint16)phase > NJM_DEG_ANG(90)) {
        return TRUE;
    }
    work->scl.y = z;
    player_set_position(player, work->pos.x + scratch_vector2.x,
                        work->pos.y + scratch_vector2.y,
                        work->pos.z + scratch_vector2.z);
    player_get_history(player, 1, &pos, NULL);
    player_motion[player]->speed.x = carried->pos.x - pos.x;
    player_motion[player]->speed.y = carried->pos.y - pos.y;
    player_motion[player]->speed.z = carried->pos.z - pos.z;
    return FALSE;
}

static void dash_ring_draw(struct task *task)
{
    struct task_work *work = task->work;

    if (!draw_in_view(work->pos.x, work->pos.y, work->pos.z, 100.0F)) {
        return;
    }
    texture_set_regular();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateX, work->ang.x);
    TURN(njRotateY, work->ang.y);
    TURN(njRotateZ, work->ang.z);
    draw_tree(&dash_ring_object);
    njPopMatrix(1);
}

static void dash_ring_destroy(struct task *task)
{
}
