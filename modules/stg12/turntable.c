/* O KAITENDAI, a turntable that carries player 0 around as it turns */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/world.h"
#include "debug/editor.h"
#include "debug/print.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "player/player.h"
#include "stg12/turntable.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT turntable_object;

/* destroyed beyond 1000 units of the camera and player 0 */
#define RANGE (1000.0F * 1000.0F)

#define DISC_FLAGS (WORLD_LARGE | WORLD_MOVES)

enum turntable_mode { TURNTABLE_INIT, TURNTABLE_RUN };

struct turntable_vars {
    Uint8 unk_00[8];
    NJS_OBJECT *disc;
};

#define VARS(work) ((struct turntable_vars *)&(work)->vars)
#define FORCE(task) ((struct task_force *)(task)->force)
/* the turn about y each step, the set entry's scale x */
#define TURN_STEP(work) ((work)->scl.x)

static void turntable_init(struct task *task);
static void turntable_turn(struct task *task);
static void turntable_draw(struct task *task);
static void turntable_destroy(struct task *task);

void turntable_step(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_try_leave_by(task, RANGE)) {
        if (!placement_editing(task)) {
            switch (work->mode) {
            case TURNTABLE_INIT:
                turntable_init(task);
                break;
            case TURNTABLE_RUN:
                turntable_draw(task);
                turntable_turn(task);
                break;
            }
        } else {
            switch (work->mode) {
            case TURNTABLE_INIT:
                turntable_init(task);
                break;
            case TURNTABLE_RUN:
                debug_print(EDITOR_LABEL(EDITOR_ROW_SCL_X), "-> ROT SPEED");
                turntable_draw(task);
                turntable_turn(task);
                break;
            }
        }
    }
}

static void turntable_init(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *disc;

    task->draw = turntable_draw;
    task->destructor = turntable_destroy;
    VARS(work)->disc = disc = world_object_alloc();
    disc->evalflags = turntable_object.evalflags;
    disc->scl[0] = 1.0F;
    disc->scl[1] = 1.0F;
    disc->scl[2] = 1.0F;
    disc->ang[0] = 0;
    disc->ang[1] = 0;
    disc->ang[2] = 0;
    disc->pos[0] = work->pos.x + turntable_object.pos[0];
    disc->pos[1] = work->pos.y + turntable_object.pos[1];
    disc->pos[2] = work->pos.z + turntable_object.pos[2];
    disc->model = turntable_object.model;
    disc->child = NULL;
    disc->sibling = NULL;
    world_add(DISC_FLAGS, task, disc);
    work->ang.x = 0;
    work->ang.y = 0;
    work->ang.z = 0;
    work->mode = TURNTABLE_RUN;
}

/* turn the disc TURN_STEP about y. player 0's force record carries player
 * 0, while it rides, through the same turn about work->pos. its speed is
 * the move that turn gives player 0's place. its turn is -TURN_STEP, since
 * a player's heading turns opposite to njRotateY. the record the other
 * players read stays zero */
static void turntable_turn(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *disc;
    Angle ang;
    NJS_POINT3 offset;
    NJS_POINT3 moved;

    work->ang.y += (Angle)TURN_STEP(work);
    disc = VARS(work)->disc;
    disc->ang[0] = 0;
    disc->ang[1] = work->ang.y;
    disc->ang[2] = 0;
    disc->pos[0] = work->pos.x + turntable_object.pos[0];
    disc->pos[1] = work->pos.y + turntable_object.pos[1];
    disc->pos[2] = work->pos.z + turntable_object.pos[2];
    offset.x = player_work[0]->pos.x - work->pos.x;
    offset.y = 0.0F;
    offset.z = player_work[0]->pos.z - work->pos.z;
    njPushMatrix(&_unit_matrix_);
    TURN_VIA(njRotateY, (Angle)TURN_STEP(work), ang);
    njCalcVector(NULL, &offset, &moved);
    njPopMatrix(1);
    njSubVector(&moved, &offset);
    FORCE(task)->speed = moved;
    FORCE(task)->turn.y = -(Angle)TURN_STEP(work);
}

static void turntable_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (frame_step == 0) {
        texture_set_stage();
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        draw_tree(&turntable_object);
        njPopMatrix(1);
    }
}

static void turntable_destroy(struct task *task)
{
    struct task_work *work = task->work;

    world_remove(task, VARS(work)->disc);
    world_object_free(VARS(work)->disc);
}
