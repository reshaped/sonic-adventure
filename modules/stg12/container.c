/* O CONTAINER, a solid crate that blows apart once every target of its id is
 * shot down */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "debug/print.h"
#include "draw/model.h"
#include "draw/tree.h"
#include "effect/bomb.h"
#include "object/placement.h"
#include "object/target.h"
#include "stg12/container.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT shelter_container_object;
extern NJS_OBJECT shelter_container_pieces_object;

/* destroyed beyond 500 units of the camera and player 0 */
#define RANGE (500.0F * 500.0F)

/* the steps a piece flies before it ends */
#define PIECE_STEPS 240
/* bomb_create's scales: the blast as the crate breaks, and each puff a piece
 * trails every eighth step */
#define BLAST_SCALE 2.0F
#define PUFF_SCALE 0.5F

/* added to the y of each piece's offset before the offset becomes a unit
 * vector. it tilts the piece's flight upward */
#define PIECE_LIFT 20.0F
/* the most a piece turns about each axis each step */
#define PIECE_SPIN 1000.0F

enum shelter_container_mode {
    CONTAINER_INIT,
    CONTAINER_WHOLE,
    CONTAINER_BROKEN
};

enum shelter_container_piece_mode { PIECE_START, PIECE_FLY, PIECE_GONE };

struct shelter_container_vars {
    Sint32 target; /* scale x: the id of the targets that break the crate */
    Uint8 unk_04[4];
    /* scale y, in no_break[0]: while it is nonzero, the crate never breaks */
    /* @hack type: four bytes for a one-byte flag, which SHC reaches through its
     * address, ADD #16,R2 at 0x0c900d20 and 0x0c900e04. as one Sint8, 62 of
     * shelter_container_step's 88 words differ, as do 16 of
     * shelter_container_init's 32 */
    Sint8 no_break[4];
};

#define VARS(work) ((struct shelter_container_vars *)&(work)->vars)

/* TASK_TIMER(work) holds the steps the piece has left */
struct shelter_container_piece_vars {
    Uint16 spin_x;
    Uint16 spin_y;
    Sint16 spin_z;
    Uint8 unk_06[2];
    NJS_OBJECT *piece;
};

#define PIECE(work) ((struct shelter_container_piece_vars *)&(work)->vars)
#define PIECE_SPEED(work) ((work)->scl)

static void shelter_container_init(struct task *task);
static void shelter_container_draw(struct task *task);
static void shelter_container_destroy(struct task *task);
static void shelter_container_break_when_cleared(struct task *task);
static void shelter_container_break(struct task *task);
static void shelter_container_piece_draw(struct task *task);
static void shelter_container_piece_start(struct task_work *work,
                                          struct task_work *crate);
static void shelter_container_piece_move(struct task_work *work);
static void shelter_container_piece_destroy(struct task *task);
static void shelter_container_piece_step(struct task *task);

static struct collision_shape shelter_container_shape = {
    SHAPE_RECTANGLE,
    0,
    PUSH_ALL | PUSH_FIRM(PUSH_ALL),
    {0.0F, 20.0F, 0.0F},
    20.0F,
    20.0F,
    20.0F,
    {0},
    0,
    NULL,
};

/* a piece's launch speed, and the downward speed it gains each step */
static Float shelter_container_piece_speed = 3.0F;
static Float shelter_container_piece_fall = 0.05F;

void shelter_container_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave_by(task, RANGE)) {
        return;
    }
    if (!placement_editing(task)) {
        switch (work->mode) {
        case CONTAINER_INIT:
            shelter_container_init(task);
            break;
        case CONTAINER_WHOLE:
            shelter_container_draw(task);
            if (!VARS(work)->no_break[0]) {
                shelter_container_break_when_cleared(task);
            }
            collision_register(task->work);
            break;
        case CONTAINER_BROKEN:
            shelter_container_draw(task);
            if (task->child == NULL) {
                placement_kill(task);
            }
            break;
        }
    } else {
        switch (work->mode) {
        case CONTAINER_INIT:
            shelter_container_init(task);
            break;
        case CONTAINER_WHOLE:
            debug_print(EDITOR_LABEL(EDITOR_ROW_SCL_X), "-> DYNAMITE ID");
            debug_print(EDITOR_LABEL(EDITOR_ROW_SCL_Y), "-> NO BREAK FLAG");
            shelter_container_draw(task);
            shelter_container_destroy(task);
            break;
        }
    }
    task_step_children(task);
}

static void shelter_container_init(struct task *task)
{
    struct task_work *work = task->work;

    task->draw = shelter_container_draw;
    task->destructor = shelter_container_destroy;
    collision_init(task, &shelter_container_shape, COLLISION_OBJECT, 1, 1);
    VARS(work)->target = work->scl.x;
    VARS(work)->no_break[0] = work->scl.y;
    work->mode = CONTAINER_WHOLE;
}

static void shelter_container_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (frame_step == 0) {
        texture_set_stage();
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        switch (work->mode) {
        case CONTAINER_WHOLE:
            draw_tree(&shelter_container_object);
            break;
        case CONTAINER_BROKEN:
            break;
        }
        njPopMatrix(1);
    }
}

static void shelter_container_destroy(struct task *task)
{
    /* @hack redundant: a read of the work, which keeps the task passed in, MOV
     * R14,R4 at 0x0c900d7a, where SHC expands this in shelter_container_step;
     * without it, 19 of 88 words differ */
    (void)task->work;
}

static void shelter_container_break_when_cleared(struct task *task)
{
    struct task_work *work = task->work;

    if (target_cleared(VARS(work)->target)) {
        bomb_create(&work->pos, BLAST_SCALE);
        shelter_container_break(task);
        work->mode = CONTAINER_BROKEN;
    }
}

static void shelter_container_break(struct task *task)
{
    NJS_OBJECT *piece = shelter_container_pieces_object.child;
    struct task *child;

    while (piece != NULL) {
        child =
            task_create_child(TASK_WORK, shelter_container_piece_step, task);
        if (child != NULL) {
            PIECE(child->work)->piece = piece;
        }
        piece = piece->sibling;
    }
}

static void shelter_container_piece_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *piece;

    if (frame_step != 0) {
        return;
    }
    piece = PIECE(work)->piece;
    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    njRotateXYZ(NULL, work->ang.x, work->ang.y, work->ang.z);
    draw_model(piece->model);
    njPopMatrix(1);
}

/* start the piece at the crate's pos plus the piece's own offset. launch it
 * along that offset, raised by PIECE_LIFT. neither takes the crate's turn */
static void shelter_container_piece_start(struct task_work *work,
                                          struct task_work *crate)
{
    NJS_OBJECT *piece = PIECE(work)->piece;
    NJS_VECTOR out;

    work->pos.x = crate->pos.x + piece->pos[0];
    work->pos.y = crate->pos.y + piece->pos[1];
    work->pos.z = crate->pos.z + piece->pos[2];
    work->ang.x = piece->ang[0];
    work->ang.y = piece->ang[1];
    work->ang.z = piece->ang[2];
    out.x = piece->pos[0];
    out.y = piece->pos[1] + PIECE_LIFT;
    out.z = piece->pos[2];
    njUnitVector(&out);
    PIECE_SPEED(work).x = out.x * shelter_container_piece_speed;
    PIECE_SPEED(work).y = out.y * shelter_container_piece_speed;
    PIECE_SPEED(work).z = out.z * shelter_container_piece_speed;
    PIECE(work)->spin_x = RANDOM(-PIECE_SPIN, PIECE_SPIN);
    PIECE(work)->spin_y = RANDOM(-PIECE_SPIN, PIECE_SPIN);
    PIECE(work)->spin_z = RANDOM(-PIECE_SPIN, PIECE_SPIN);
}

static void shelter_container_piece_move(struct task_work *work)
{
    work->pos.x += PIECE_SPEED(work).x;
    work->pos.y += PIECE_SPEED(work).y;
    work->pos.z += PIECE_SPEED(work).z;
    PIECE_SPEED(work).y -= shelter_container_piece_fall;
    work->ang.x += PIECE(work)->spin_x;
    work->ang.y += PIECE(work)->spin_y;
    work->ang.z += PIECE(work)->spin_z;
    if (!(TASK_TIMER(work) & 7)) {
        bomb_create(&work->pos, PUFF_SCALE);
    }
    /* the timer is unsigned, so the test holds only when it reaches 0 */
    if (--TASK_TIMER(work) <= 0) {
        work->mode = PIECE_GONE;
    }
}

static void shelter_container_piece_destroy(struct task *task)
{
}

static void shelter_container_piece_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case PIECE_START:
        task->draw = shelter_container_piece_draw;
        task->destructor = shelter_container_piece_destroy;
        shelter_container_piece_start(work, task->parent->work);
        work->timer = PIECE_STEPS;
        work->mode = PIECE_FLY;
        break;
    case PIECE_FLY:
        shelter_container_piece_draw(task);
        shelter_container_piece_move(work);
        break;
    case PIECE_GONE:
        task_kill(task);
        break;
    }
}
