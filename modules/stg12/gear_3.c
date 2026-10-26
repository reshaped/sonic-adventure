/* O HAGURUMA3: a gear that turns about z, the other way while its switch
 * is on. it carries player 0 round with it */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/world.h"
#include "debug/editor.h"
#include "debug/print.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "object/switch.h"
#include "player/player.h"
#include "sound/sound.h"
#include "stg12/gear_3.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT gear_3_object;
extern NJS_OBJECT gear_3_world_object;

/* destroyed beyond 1000 units of the camera and player 0 */
#define RANGE (1000.0F * 1000.0F)

#define GEAR_FLAGS (WORLD_LARGE | WORLD_MOVES | WORLD_GRIP_HIGH)

/* the collision world tests the gear only while player 0 or 1 is within
 * NEAR of it */
#define NEAR 148.0F

#define SE_GEAR 321

enum gear_3_mode { GEAR_INIT, GEAR_RUN };

struct gear_3_vars {
    Uint8 switch_id;
    Uint8 unk_01[7];
    NJS_OBJECT *world;
};

#define VARS(work) ((struct gear_3_vars *)&(work)->vars)
#define FORCE(task) ((struct task_force *)(task)->force)
/* the angle the gear turns each step */
#define TURN_STEP(work) ((work)->scl.x)

static void gear_3_init(struct task *task);
static void gear_3_turn(struct task *task);
static void gear_3_draw(struct task *task);
static void gear_3_destroy(struct task *task);

void gear_3_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave_by(task, RANGE)) {
        return;
    }
    if (!placement_editing(task)) {
        switch (work->mode) {
        case GEAR_INIT:
            gear_3_init(task);
            break;
        case GEAR_RUN:
            gear_3_draw(task);
            gear_3_turn(task);
            sound_play_at_for(SE_GEAR, (int)task, 1, 0, work->pos.x,
                              work->pos.y, work->pos.z, 2);
            break;
        /* @hack redundant: an empty default, which leaves the pop of
         * sound_play_at_for's stack argument, ADD #4,R15 at 0x0c903e78,
         * ahead of the BRA rather than in its delay slot; without it, 3 of 93
         * words differ */
        default:;
        }
    } else {
        switch (work->mode) {
        case GEAR_INIT:
            gear_3_init(task);
            break;
        case GEAR_RUN:
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_X), "-> No Use");
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Y), "-> No Use");
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Z), "-> No Use");
            debug_print(EDITOR_LABEL(EDITOR_ROW_SCL_X), "-> ROTSPEED");
            debug_print(EDITOR_LABEL(EDITOR_ROW_SCL_Y), "-> SWITCH ID");
            gear_3_draw(task);
            gear_3_turn(task);
            break;
        }
    }
}

static void gear_3_init(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *world;

    task->draw = gear_3_draw;
    task->destructor = gear_3_destroy;
    VARS(work)->world = world = world_object_alloc();
    world->evalflags = gear_3_world_object.evalflags;
    world->scl[0] = 1.0F;
    world->scl[1] = 1.0F;
    world->scl[2] = 1.0F;
    world->ang[0] = 0;
    world->ang[1] = 0;
    world->ang[2] = 0;
    world->pos[0] = work->pos.x + gear_3_world_object.pos[0];
    world->pos[1] = work->pos.y + gear_3_world_object.pos[1];
    world->pos[2] = work->pos.z + gear_3_world_object.pos[2];
    world->model = gear_3_world_object.model;
    world->child = NULL;
    world->sibling = NULL;
    world_add(GEAR_FLAGS, task, world);
    work->ang.x = 0;
    work->ang.y = 0;
    work->ang.z = 0;
    VARS(work)->switch_id = work->scl.y;
    work->mode = GEAR_RUN;
}

/* turn the gear a step, the other way while its switch is on. the gear's
 * force carries player 0 through the same turn about the gear's axis */
static void gear_3_turn(struct task *task)
{
    struct task_work *work = task->work;
    int turn;
    NJS_OBJECT *world;
    NJS_POINT3 offset;
    NJS_POINT3 moved;

    if (player_in_range(&work->pos, NEAR)) {
        work->flags |= WORLD_TESTED;
    } else {
        work->flags &= ~WORLD_TESTED;
    }
    if (!switch_is_on(VARS(work)->switch_id)) {
        turn = (int)TURN_STEP(work);
    } else {
        turn = -(int)TURN_STEP(work);
    }
    work->ang.z += turn;
    world = VARS(work)->world;
    world->ang[0] = 0;
    world->ang[1] = 0;
    world->ang[2] = work->ang.z;
    world->pos[0] = work->pos.x + gear_3_world_object.pos[0];
    world->pos[1] = work->pos.y + gear_3_world_object.pos[1];
    world->pos[2] = work->pos.z + gear_3_world_object.pos[2];
    offset.x = player_work[0]->pos.x - work->pos.x;
    offset.y = player_work[0]->pos.y - work->pos.y;
    offset.z = 0.0F;
    njPushMatrix(&_unit_matrix_);
    TURN(njRotateZ, turn);
    njCalcVector(NULL, &offset, &moved);
    njPopMatrix(1);
    njSubVector(&moved, &offset);
    FORCE(task)->speed = moved;
}

static void gear_3_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (frame_step == 0) {
        texture_set_stage();
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateZ, work->ang.z, ang);
        draw_tree(&gear_3_object);
        njPopMatrix(1);
    }
}

static void gear_3_destroy(struct task *task)
{
    struct task_work *work = task->work;

    world_remove(task, VARS(work)->world);
    world_object_free(VARS(work)->world);
}
