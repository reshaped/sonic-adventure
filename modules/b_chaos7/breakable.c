/* the Perfect Chaos fight's cars and walls. the set editor names them
 * O RED CAR, O YELLOW CAR, O BLUE CAR and O WAll A to O WAll C. a car or
 * wall blows apart once hurt, but O WAll C can't be hurt */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "b_chaos7/breakable.h"
#include "b_chaos7/column.h"
#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/tree.h"
#include "effect/explosion.h"
#include "effect/splash.h"
#include "object/common.h"
#include "object/placement.h"
#include "object/ring.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT red_car_object;
extern NJS_OBJECT yellow_car_object;
extern NJS_OBJECT blue_car_object;
extern NJS_OBJECT wall_a_object;
extern NJS_OBJECT wall_b_object;
extern NJS_OBJECT wall_c_object;
extern NJS_OBJECT red_car_piece_0_object;
extern NJS_OBJECT red_car_piece_1_object;
extern NJS_OBJECT red_car_piece_2_object;
extern NJS_OBJECT red_car_piece_3_object;
extern NJS_OBJECT red_car_piece_4_object;
extern NJS_OBJECT red_car_piece_5_object;
extern NJS_OBJECT yellow_car_piece_0_object;
extern NJS_OBJECT yellow_car_piece_1_object;
extern NJS_OBJECT yellow_car_piece_2_object;
extern NJS_OBJECT yellow_car_piece_3_object;
extern NJS_OBJECT yellow_car_piece_4_object;
extern NJS_OBJECT yellow_car_piece_5_object;
extern NJS_OBJECT blue_car_piece_0_object;
extern NJS_OBJECT blue_car_piece_1_object;
extern NJS_OBJECT blue_car_piece_2_object;
extern NJS_OBJECT blue_car_piece_3_object;
extern NJS_OBJECT blue_car_piece_4_object;
extern NJS_OBJECT blue_car_piece_5_object;
extern NJS_OBJECT wall_a_piece_0_object;
extern NJS_OBJECT wall_a_piece_1_object;
extern NJS_OBJECT wall_a_piece_2_object;
extern NJS_OBJECT wall_a_piece_3_object;
extern NJS_OBJECT wall_a_piece_4_object;
extern NJS_OBJECT wall_a_piece_5_object;
extern NJS_OBJECT wall_b_piece_0_object;
extern NJS_OBJECT wall_b_piece_1_object;
extern NJS_OBJECT wall_b_piece_2_object;
extern NJS_OBJECT wall_b_piece_3_object;
extern NJS_OBJECT wall_b_piece_4_object;
extern NJS_OBJECT wall_b_piece_5_object;

/* a car, wall or piece lasts while the camera or player 0 is within 4010 of
 * it. placement_try_leave_by takes the range squared */
#define BREAKABLE_RANGE (4010.0F * 4010.0F)

/* a broken car or wall counts its steps. once the count passes
 * BREAKABLE_PIECES_TIME, it destroys its pieces and kills its placement */
#define BREAKABLE_PIECES_TIME 180

/* a piece's speed starts at random, from -PIECE_ACROSS up to PIECE_ACROSS
 * along x and z, and from PIECE_UP up to PIECE_UP + 1 along y. each step
 * its speed along y drops by PIECE_GRAVITY */
#define PIECE_ACROSS 0.5F
#define PIECE_UP 0.4F
#define PIECE_GRAVITY 0.05F
/* each step a piece's ang.x and ang.y grow by PIECE_SPIN. breakable_draw
 * turns by ang.z and ang.y alone, so the piece shows its spin about y
 * only */
#define PIECE_SPIN 0x100

/* the scl.x a piece gives the column it raises, which the column reads as
 * its lower part's share of its full height. the column's first step sets
 * it to 0 before it reads it */
#define PIECE_COLUMN 1.0F

/* a car's or wall's kind, its index in breakable_kinds, in its work's
 * tick */
#define BREAKABLE_KIND(work) ((work)->tick)

/* a piece's speed, in its work's scl */
#define PIECE_SPEED(work) ((work)->scl)

/* a car's or wall's modes. BREAKABLE_IDLE shares BREAKABLE_BROKEN's case,
 * and no step sets it */
enum breakable_mode {
    BREAKABLE_INIT,
    BREAKABLE_STANDING,
    BREAKABLE_BROKEN,
    BREAKABLE_IDLE
};

enum piece_mode { PIECE_INIT, PIECE_FLYING, PIECE_SPLASHED };

/* the kinds' indexes in breakable_kinds, in the order of their steps */
enum { RED_CAR, YELLOW_CAR, BLUE_CAR, WALL_A, WALL_B, WALL_C, BREAKABLE_KINDS };

/* the shapes' indexes in breakable_shapes */
enum { CAR_SHAPE, WALL_A_SHAPE, WALL_B_SHAPE, WALL_C_SHAPE, BREAKABLE_SHAPES };

struct breakable_kind {
    NJS_OBJECT *object;
    struct task_spec *pieces; /* the pieces it breaks into, or null for none */
    int shape; /* its shape's index in breakable_shapes */
    /* its collision record's radius, in place of the one collision_init
     * finds */
    Float radius;
};

static void breakable_piece_step(struct task *task);

/* the speed of the drop a piece splashes up */
static const NJS_VECTOR breakable_piece_rise = {0.0F, 1.0F, 0.0F};

/* the cars' box, then each wall's. O WAll C's alone isn't SHAPE_HURTABLE,
 * and its levels hold GUARD(3), the highest guard */
static struct collision_shape breakable_shapes[BREAKABLE_SHAPES] = {
    {SHAPE_RECTANGLE2 | SHAPE_HURTABLE,
     0,
     0,
     {0.0F, 0.0F, 0.0F},
     13.51F,
     8.657F,
     26.82F,
     {0},
     0,
     NULL},
    {SHAPE_RECTANGLE2 | SHAPE_HURTABLE,
     0,
     PUSH_FIRM(PUSH_ALL) | PUSH_ALL,
     {0.0F, 0.0F, 0.0F},
     37.88F,
     35.25F,
     5.365F,
     {0},
     0,
     NULL},
    {SHAPE_RECTANGLE2 | SHAPE_HURTABLE,
     0,
     PUSH_FIRM(PUSH_ALL) | PUSH_ALL,
     {0.0F, 0.0F, 0.0F},
     16.38F,
     18.81F,
     1.876F,
     {0},
     0,
     NULL},
    {SHAPE_RECTANGLE2,
     GUARD(3),
     PUSH_FIRM(PUSH_ALL) | PUSH_ALL,
     {0.0F, 0.0F, 0.0F},
     11.26F,
     8.956F,
     1.945F,
     {0},
     0,
     NULL},
};

/* a piece's spec for task_create_children: a child with a work, whose
 * vars.l[0] holds its object */
/* clang-format off */
#define PIECE(object)                                                          \
    {breakable_piece_step, TASK_WORK, 0, {0, 0, 0}, {0.0F, 0.0F, 0.0F},        \
     (Sint32)(object)}
/* clang-format on */
#define PIECES_END {NULL, 0, 0, {0, 0, 0}, {0.0F, 0.0F, 0.0F}, 0}

static struct task_spec red_car_pieces[7] = {
    PIECE(&red_car_piece_0_object),
    PIECE(&red_car_piece_1_object),
    PIECE(&red_car_piece_2_object),
    PIECE(&red_car_piece_3_object),
    PIECE(&red_car_piece_4_object),
    PIECE(&red_car_piece_5_object),
    PIECES_END,
};
static struct task_spec yellow_car_pieces[7] = {
    PIECE(&yellow_car_piece_0_object),
    PIECE(&yellow_car_piece_1_object),
    PIECE(&yellow_car_piece_2_object),
    PIECE(&yellow_car_piece_3_object),
    PIECE(&yellow_car_piece_4_object),
    PIECE(&yellow_car_piece_5_object),
    PIECES_END,
};
static struct task_spec blue_car_pieces[7] = {
    PIECE(&blue_car_piece_0_object),
    PIECE(&blue_car_piece_1_object),
    PIECE(&blue_car_piece_2_object),
    PIECE(&blue_car_piece_3_object),
    PIECE(&blue_car_piece_4_object),
    PIECE(&blue_car_piece_5_object),
    PIECES_END,
};
static struct task_spec wall_a_pieces[7] = {
    PIECE(&wall_a_piece_0_object),
    PIECE(&wall_a_piece_1_object),
    PIECE(&wall_a_piece_2_object),
    PIECE(&wall_a_piece_3_object),
    PIECE(&wall_a_piece_4_object),
    PIECE(&wall_a_piece_5_object),
    PIECES_END,
};
static struct task_spec wall_b_pieces[7] = {
    PIECE(&wall_b_piece_0_object),
    PIECE(&wall_b_piece_1_object),
    PIECE(&wall_b_piece_2_object),
    PIECE(&wall_b_piece_3_object),
    PIECE(&wall_b_piece_4_object),
    PIECE(&wall_b_piece_5_object),
    PIECES_END,
};

/* the kinds, by their indexes. O WAll C never breaks, and has no pieces */
static struct breakable_kind breakable_kinds[BREAKABLE_KINDS] = {
    {&red_car_object, red_car_pieces, CAR_SHAPE, 27.48F},
    {&yellow_car_object, yellow_car_pieces, CAR_SHAPE, 27.48F},
    {&blue_car_object, blue_car_pieces, CAR_SHAPE, 27.48F},
    {&wall_a_object, wall_a_pieces, WALL_A_SHAPE, 38.01F},
    {&wall_b_object, wall_b_pieces, WALL_B_SHAPE, 16.99F},
    {&wall_c_object, NULL, WALL_C_SHAPE, 11.36F},
};

static void breakable_destroy(struct task *task)
{
    task_destroy_children(task);
    task_kill(task);
}

/* draw a car, wall or piece, turned about z and y */
static void breakable_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_tree(MODEL(work)->object);
    njPopMatrix(1);
}

/* a piece's step. the first gives it a random speed. it then flies, falls
 * and spins. as it first drops below the sea's surface, at y 0, it splashes
 * and raises a column there, and it sinks on */
static void breakable_piece_step(struct task *task)
{
    struct task_work *work = task->work;
    NJS_VECTOR rise;
    struct task *column;
    /* @hack spelling: rand in a local, called through it, which puts rand's
     * address ahead of RAND_SCALE in the pool (0x0c91b74c, 0x0c91b750).
     * called by name, the two trade and 2 of 152 words differ */
    int (*random)(void);

    if (placement_try_leave_by(task, BREAKABLE_RANGE)) {
        return;
    }
    switch (work->mode) {
    case PIECE_INIT:
        work->mode = PIECE_FLYING;
        random = rand;
        PIECE_SPEED(work).x = (RAND_SCALE * random()) - PIECE_ACROSS;
        PIECE_SPEED(work).y = (RAND_SCALE * random()) + PIECE_UP;
        PIECE_SPEED(work).z = (RAND_SCALE * random()) - PIECE_ACROSS;
        task->draw = breakable_draw;
        task->destructor = breakable_destroy;
        break;
    case PIECE_FLYING:
        if (work->pos.y < 0.0F) {
            rise = breakable_piece_rise;
            work->mode = PIECE_SPLASHED;
            if ((column = task_create(TASK_WORK, 3, chaos7_column_step)) !=
                NULL) {
                column->work->pos = work->pos;
                column->work->pos.y = 0.0F;
                column->work->scl.x = PIECE_COLUMN;
            }
            splash_create(&work->pos, &rise, 1.0F);
        }
        /* falls through, so the piece moves on the step it splashes */
    case PIECE_SPLASHED:
        work->pos.x += PIECE_SPEED(work).x;
        work->pos.y += PIECE_SPEED(work).y;
        work->pos.z += PIECE_SPEED(work).z;
        PIECE_SPEED(work).y -= PIECE_GRAVITY;
        work->ang.x += PIECE_SPIN;
        work->ang.y += PIECE_SPIN;
        breakable_draw(task);
        break;
    }
}

/* a car's or wall's step, for the kind in BREAKABLE_KIND. in the set editor
 * it draws the kind's object. while the editor holds it, it labels ang.y and
 * ang.z its rotation. otherwise it stands until it is hurt, and then
 * explodes into its pieces. it scatters a ring when the record it touched is
 * a player's */
static void breakable_step(struct task *task)
{
    struct task_work *work = task->work;
    Uint8 kind = BREAKABLE_KIND(work);
    struct breakable_kind *entry;
    struct task_spec *pieces;
    int shape;

    if (placement_try_leave_by(task, BREAKABLE_RANGE)) {
        return;
    }
    if (edit_mode) {
        MODEL(work)->object = breakable_kinds[kind].object;
        breakable_draw(task);
        if (placement_editing(task)) {
            njPrintC(EDITOR_LABEL(EDITOR_ROW_ANG_X), ">");
            njPrintC(EDITOR_LABEL(EDITOR_ROW_ANG_Y), "> ROTATION");
            njPrintC(EDITOR_LABEL(EDITOR_ROW_ANG_Z), "> ROTATION");
            njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_X), ">");
            njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_Y), ">");
            njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_Z), ">");
        }
    } else {
        switch (work->mode) {
        case BREAKABLE_INIT:
            work->mode = BREAKABLE_STANDING;
            work->scl.x = work->scl.y = work->scl.z = 0.0F;
            entry = &breakable_kinds[kind];
            MODEL(work)->object = entry->object;
            task->destructor = breakable_destroy;
            task->draw = breakable_draw;
            shape = breakable_kinds[kind].shape;
            collision_init(task, &breakable_shapes[shape], COLLISION_OBJECT, 1,
                           1);
            work->collision->radius = entry->radius;
            break;
        case BREAKABLE_STANDING:
            if (work->flags & COLLISION_DAMAGED) {
                work->mode = BREAKABLE_BROKEN;
                explosion_create_at(work->pos.x, work->pos.y, work->pos.z,
                                    2.0F);
                entry = &breakable_kinds[kind];
                pieces = entry->pieces;
                /* @hack spelling: an empty if with the call in its else, which
                 * loads task_create_children's address into R2 and the
                 * collision record into R3 (0x0c91b978, 0x0c91b97e). as a
                 * plain if, the two trade and 5 of 201 words differ */
                if (pieces == NULL) {
                } else {
                    task_create_children(pieces, task);
                }
                if (work->collision->hit->kind == COLLISION_PLAYER) {
                    ring_scatter(work->pos.x, work->pos.y, work->pos.z, 1);
                }
            }
            collision_register(work);
            breakable_draw(task);
            break;
        case BREAKABLE_BROKEN:
        case BREAKABLE_IDLE:
            task_step_children(task);
            /* fall through */
        default:
            if (TASK_TIMER(work)++ > BREAKABLE_PIECES_TIME) {
                task_destroy_children(task);
                placement_kill(task);
            }
            break;
        }
    }
}

void chaos7_red_car_step(struct task *task)
{
    BREAKABLE_KIND(task->work) = RED_CAR;
    breakable_step(task);
}

void chaos7_yellow_car_step(struct task *task)
{
    BREAKABLE_KIND(task->work) = YELLOW_CAR;
    breakable_step(task);
}

void chaos7_blue_car_step(struct task *task)
{
    BREAKABLE_KIND(task->work) = BLUE_CAR;
    breakable_step(task);
}

void chaos7_wall_a_step(struct task *task)
{
    BREAKABLE_KIND(task->work) = WALL_A;
    breakable_step(task);
}

void chaos7_wall_b_step(struct task *task)
{
    BREAKABLE_KIND(task->work) = WALL_B;
    breakable_step(task);
}

void chaos7_wall_c_step(struct task *task)
{
    BREAKABLE_KIND(task->work) = WALL_C;
    breakable_step(task);
}
