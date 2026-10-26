/* O GRAY BLD, O BROWN BLD and O RED BLD: the Perfect Chaos fight's buildings,
 * which blow apart */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "b_chaos7/building.h"
#include "b_chaos7/column.h"
#include "collision/collision.h"
#include "collision/world.h"
#include "debug/editor.h"
#include "draw/tree.h"
#include "effect/explosion.h"
#include "object/common.h"
#include "object/placement.h"
#include "player/player.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT gray_building_object;
extern NJS_OBJECT brown_building_object;
extern NJS_OBJECT red_building_object;
extern NJS_OBJECT gray_building_piece_0_object;
extern NJS_OBJECT gray_building_piece_1_object;
extern NJS_OBJECT gray_building_piece_2_object;
extern NJS_OBJECT gray_building_piece_3_object;
extern NJS_OBJECT gray_building_piece_4_object;
extern NJS_OBJECT gray_building_piece_5_object;
extern NJS_OBJECT gray_building_piece_6_object;
extern NJS_OBJECT brown_building_piece_0_object;
extern NJS_OBJECT brown_building_piece_1_object;
extern NJS_OBJECT brown_building_piece_2_object;
extern NJS_OBJECT brown_building_piece_3_object;
extern NJS_OBJECT brown_building_piece_4_object;
extern NJS_OBJECT brown_building_piece_5_object;
extern NJS_OBJECT brown_building_piece_6_object;
extern NJS_OBJECT red_building_piece_0_object;
extern NJS_OBJECT red_building_piece_1_object;
extern NJS_OBJECT red_building_piece_2_object;
extern NJS_OBJECT red_building_piece_3_object;
extern NJS_OBJECT red_building_piece_4_object;
extern NJS_OBJECT red_building_piece_5_object;
extern NJS_OBJECT red_building_piece_6_object;

/* a building or a piece destroys itself once neither the camera nor player 0
 * is within 4010. the range is squared, as placement_try_leave_by
 * takes it */
#define BUILDING_RANGE (4010.0F * 4010.0F)

/* a broken building destroys its pieces and kills its placement once its
 * timer passes BUILDING_PIECES_TIME. the timer counts a step from the break */
#define BUILDING_PIECES_TIME 180

/* a piece rises PIECE_UP to PIECE_UP + 1 a step at first. it then gains
 * PIECE_GRAVITY downward a step. its ang.x and ang.y gain PIECE_SPIN a step,
 * but the draw turns it about y alone */
#define PIECE_UP 0.4F
#define PIECE_GRAVITY 0.05F
#define PIECE_SPIN 0x10

/* the scl.x a piece gives the column it raises. the column's first step sets
 * it to 0 */
#define PIECE_COLUMN 3.0F

/* the building's row of building_kinds, which each kind's step sets before
 * building_step runs */
#define BUILDING_KIND(work) ((work)->tick)
enum building_kind_row { KIND_GRAY, KIND_BROWN, KIND_RED };

/* a piece's velocity, in units a step */
#define PIECE_SPEED(work) ((work)->scl)

/* nothing sets BUILDING_IDLE or PIECE_IDLE */
enum building_mode {
    BUILDING_INIT,
    BUILDING_STANDING,
    BUILDING_BROKEN,
    BUILDING_IDLE
};

enum piece_mode {
    PIECE_INIT,
    PIECE_BLOWN,
    PIECE_FALLING,
    PIECE_SPLASHED,
    PIECE_IDLE
};

struct building_kind {
    NJS_OBJECT *object;
    struct task_spec *pieces;
    int shape; /* index in building_shapes */
    /* the collision record's reach, and how near a player must be for the
     * collision world to test the building */
    Float radius;
};

static void building_piece_step(struct task *task);

/* the spec of a piece: a child with a work, which runs building_piece_step.
 * its vars.l[0] holds the piece's object */
/* clang-format off */
#define PIECE(object)                                                          \
    {building_piece_step, TASK_WORK, 0, {0, 0, 0}, {0.0F, 0.0F, 0.0F},         \
     (Sint32)(object)}
/* clang-format on */
#define PIECES_END {NULL, 0, 0, {0, 0, 0}, {0.0F, 0.0F, 0.0F}, 0}

static struct task_spec gray_building_pieces[8] = {
    PIECE(&gray_building_piece_0_object), PIECE(&gray_building_piece_1_object),
    PIECE(&gray_building_piece_2_object), PIECE(&gray_building_piece_3_object),
    PIECE(&gray_building_piece_4_object), PIECE(&gray_building_piece_5_object),
    PIECE(&gray_building_piece_6_object), PIECES_END,
};
static struct task_spec brown_building_pieces[8] = {
    PIECE(&brown_building_piece_0_object),
    PIECE(&brown_building_piece_1_object),
    PIECE(&brown_building_piece_2_object),
    PIECE(&brown_building_piece_3_object),
    PIECE(&brown_building_piece_4_object),
    PIECE(&brown_building_piece_5_object),
    PIECE(&brown_building_piece_6_object),
    PIECES_END,
};
static struct task_spec red_building_pieces[8] = {
    PIECE(&red_building_piece_0_object), PIECE(&red_building_piece_1_object),
    PIECE(&red_building_piece_2_object), PIECE(&red_building_piece_3_object),
    PIECE(&red_building_piece_4_object), PIECE(&red_building_piece_5_object),
    PIECE(&red_building_piece_6_object), PIECES_END,
};

static struct collision_shape building_shapes[3] = {
    {SHAPE_RECTANGLE | SHAPE_HURTABLE | SHAPE_IGNORE_PLAYERS,
     0,
     0,
     {0.0F, 70.65F, 0.0F},
     140.7F,
     70.65F,
     158.4F,
     {0},
     0,
     NULL},
    {SHAPE_RECTANGLE | SHAPE_HURTABLE | SHAPE_IGNORE_PLAYERS,
     0,
     0,
     {0.0F, 102.0F, 0.0F},
     250.5F,
     102.0F,
     226.4F,
     {0},
     0,
     NULL},
    {SHAPE_RECTANGLE | SHAPE_HURTABLE | SHAPE_IGNORE_PLAYERS,
     0,
     0,
     {0.0F, 140.2F, 0.0F},
     308.5F,
     140.2F,
     245.0F,
     {0},
     0,
     NULL},
};

/* the kinds, in building_kind_row's order */
static struct building_kind building_kinds[3] = {
    {&gray_building_object, gray_building_pieces, 0, 300.0F},
    {&brown_building_object, brown_building_pieces, 1, 455.0F},
    {&red_building_object, red_building_pieces, 2, 470.0F},
};

/* the destructor of a building and of each piece. a piece's world var holds
 * its own object, not one the world lent. world_remove changes nothing for a
 * task with none in the world, and world_object_free frees only a lent
 * object. so for a piece, the two do nothing */
static void building_destroy(struct task *task)
{
    struct task_work *work = task->work;

    if (WORLD_MODEL(work)->world != NULL) {
        world_remove(task, WORLD_MODEL(work)->world);
        world_object_free(WORLD_MODEL(work)->world);
    }
    task_destroy_children(task);
    task_kill(task);
}

static void building_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_tree(WORLD_MODEL(work)->model);
    njPopMatrix(1);
}

/* start a piece at a random place round the building and explode it. it flies
 * off and falls. once it drops below y 0, into the sea, it raises a column and
 * sinks on */
static void building_piece_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task *column;
    Float reach;
    Uint8 kind;

    if (placement_try_leave_by(task, BUILDING_RANGE)) {
        return;
    }
    switch (work->mode) {
    case PIECE_INIT:
        work->mode = PIECE_BLOWN;
        /* @bug logic: reads the piece's own tick, not its building's
         * (MOV.B @(3,R4),R0 at 0x0c91bb84), and nothing sets it. so every
         * building's pieces spread by the gray building's radius of 300, not
         * the brown's 455 or the red's 470 */
        kind = BUILDING_KIND(task->work);
        reach = building_kinds[kind].radius;
        /* the piece starts at the building's position. it moves off it by up
         * to half of reach either way along x and z. its y is set within half
         * of reach of 0. it flies off at under half a unit a step either way
         * along x and z */
        work->pos.x += ((rand() * RAND_SCALE) - 0.5F) * reach;
        work->pos.y = ((rand() * RAND_SCALE) - 0.5F) * reach;
        work->pos.z += ((rand() * RAND_SCALE) - 0.5F) * reach;
        PIECE_SPEED(work).x = (rand() * RAND_SCALE) - 0.5F;
        PIECE_SPEED(work).y = (rand() * RAND_SCALE) + PIECE_UP;
        PIECE_SPEED(work).z = (rand() * RAND_SCALE) - 0.5F;
        WORLD_MODEL(work)->model = WORLD_MODEL(work)->world;
        task->draw = building_draw;
        task->destructor = building_destroy;
        break;
    case PIECE_BLOWN:
        work->mode = PIECE_FALLING;
        explosion_create_at(work->pos.x, work->pos.y, work->pos.z, 2.0F);
        /* falls through, so the piece moves on the step it explodes */
    case PIECE_FALLING:
        if (work->pos.y < 0.0F) {
            work->mode = PIECE_SPLASHED;
            if ((column = task_create(TASK_WORK, 3, chaos7_column_step)) !=
                NULL) {
                column->work->pos = work->pos;
                column->work->pos.y = 0.0F;
                column->work->scl.x = PIECE_COLUMN;
            }
        }
        /* falls through, so the piece moves on the step it splashes */
    case PIECE_SPLASHED:
        work->pos.x += PIECE_SPEED(work).x;
        work->pos.y += PIECE_SPEED(work).y;
        work->pos.z += PIECE_SPEED(work).z;
        PIECE_SPEED(work).y -= PIECE_GRAVITY;
        work->ang.x += PIECE_SPIN;
        work->ang.y += PIECE_SPIN;
        building_draw(task);
        break;
    case PIECE_IDLE:
        break;
    }
}

/* in edit mode, draw the building and label its set fields. otherwise, add a
 * copy of it to the collision world. the world tests the copy only while a
 * player is within the kind's radius. once a COLLISION_ITEM record hurts the
 * building, it breaks into its pieces. Chaos's ball of light is this module's
 * only such record. once the building's timer passes BUILDING_PIECES_TIME, it
 * destroys its pieces and kills its placement */
static void building_step(struct task *task)
{
    struct task_work *work = task->work;
    Uint8 kind = BUILDING_KIND(work);
    struct building_kind *entry;
    struct task_spec *pieces;
    NJS_OBJECT *world;
    int shape;

    if (placement_try_leave_by(task, BUILDING_RANGE)) {
        return;
    }
    if (edit_mode) {
        WORLD_MODEL(work)->model = building_kinds[kind].object;
        building_draw(task);
        if (placement_editing(task)) {
            njPrintC(EDITOR_LABEL(EDITOR_ROW_ANG_X), ">");
            njPrintC(EDITOR_LABEL(EDITOR_ROW_ANG_Y), "> ROTATION");
            njPrintC(EDITOR_LABEL(EDITOR_ROW_ANG_Z), ">");
            njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_X), ">");
            njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_Y), ">");
            njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_Z), ">");
        }
    } else {
        switch (work->mode) {
        case BUILDING_INIT:
            work->mode = BUILDING_STANDING;
            work->scl.x = work->scl.y = work->scl.z = 0.0F;
            entry = &building_kinds[kind];
            WORLD_MODEL(work)->model = entry->object;
            task->destructor = building_destroy;
            task->draw = building_draw;
            shape = building_kinds[kind].shape;
            collision_init(task, &building_shapes[shape], COLLISION_OBJECT, 1,
                           1);
            work->collision->radius = entry->radius;
            world = world_object_alloc();
            WORLD_MODEL(work)->world = world;
            world->evalflags = WORLD_MODEL(work)->model->evalflags &
                               ~(NJD_EVAL_UNIT_POS | NJD_EVAL_UNIT_ANG);
            world->scl[0] = world->scl[1] = world->scl[2] = 1.0F;
            world->ang[0] = world->ang[2] = 0;
            world->ang[1] = work->ang.y;
            world->pos[0] = work->pos.x;
            world->pos[1] = work->pos.y;
            world->pos[2] = work->pos.z;
            world->model = WORLD_MODEL(work)->model->model;
            world->child = NULL;
            world->sibling = NULL;
            world_add(WORLD_LARGE, task, world);
            break;
        case BUILDING_STANDING:
            if ((work->flags & COLLISION_DAMAGED) &&
                work->collision->hit->kind == COLLISION_ITEM) {
                work->mode = BUILDING_BROKEN;
                pieces = building_kinds[kind].pieces;
                if (pieces != NULL) {
                    task_create_children(pieces, task);
                }
            }
            if (player_in_range(&work->pos, building_kinds[kind].radius)) {
                work->flags |= WORLD_TESTED;
            } else {
                work->flags &= ~WORLD_TESTED;
            }
            collision_register(work);
            building_draw(task);
            break;
        case BUILDING_BROKEN:
            work->flags &= ~WORLD_TESTED;
            task_step_children(task);
            /* falls through, so the wait counts from the break */
        case BUILDING_IDLE:
        default:
            if (TASK_TIMER(work)++ > BUILDING_PIECES_TIME) {
                if (WORLD_MODEL(work)->world != NULL) {
                    world_remove(task, WORLD_MODEL(work)->world);
                    world_object_free(WORLD_MODEL(work)->world);
                }
                task_destroy_children(task);
                placement_kill(task);
            }
            break;
        }
    }
}

void chaos7_gray_building_step(struct task *task)
{
    BUILDING_KIND(task->work) = KIND_GRAY;
    building_step(task);
}

void chaos7_brown_building_step(struct task *task)
{
    BUILDING_KIND(task->work) = KIND_BROWN;
    building_step(task);
}

void chaos7_red_building_step(struct task *task)
{
    BUILDING_KIND(task->work) = KIND_RED;
    building_step(task);
}
