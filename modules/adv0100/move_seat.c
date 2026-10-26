/* O MOVESEAT seat on a spring that bounces three times when its cushion is
 * touched */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv0100/move_seat.h"
#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "draw/model.h"
#include "object/placement.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

/* the base. its child is the spring, and the spring's sibling the cushion */
extern NJS_OBJECT move_seat_object;

/* the index in move_seat_shapes of the cushion, whose touch starts a bounce */
#define CUSHION 1
/* the bounce's phase gain each step, and the phase at which it ends, three
 * whole turns on */
#define BOUNCE_STEP 0x400
#define BOUNCE_END 0x30000
#define LEVEL_STEP 0.1F /* the change in LEVEL each step */
/* the cushion's height above its node's place at rest */
#define CUSHION_REST 1.0F
#define CUSHION_SAG 1.5F /* half the depth the bounce lowers the cushion by */
/* the farthest the bounce rocks the cushion, a sixteenth of a turn */
#define CUSHION_ROCK 4096.0F

/* in work->flags while the seat bounces */
#define SEAT_BOUNCING 0x2
/* rises to 1 while the seat bounces, and falls back to 0 after. nothing in this
 * image reads it */
#define LEVEL(work) ((work)->scl.x)

struct move_seat_vars {
    Uint8 unk_00[8];
    Angle bounce;
};

#define VARS(work) ((struct move_seat_vars *)&(work)->vars)

enum move_seat_mode {
    SEAT_INIT,
    SEAT_WAIT,
    SEAT_BOUNCE,
    SEAT_STILL /* no code switches to it */
};

static void move_seat_destroy(struct task *task);
static void move_seat_draw(struct task *task);
static void move_seat_draw_work(struct task *task, struct task_work *work);

/* a box for the seat and a sphere for the cushion */
static struct collision_shape move_seat_shapes[] = {
    {SHAPE_RECTANGLE | SHAPE_NO_PUSH,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 2.0F, 0.0F},
     15.0F,
     4.0F,
     10.0F,
     {0},
     0,
     NULL},
    {SHAPE_SPHERE | SHAPE_NO_PUSH,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 15.0F, 0.0F},
     8.0F,
     2.0F,
     0.0F,
     {0},
     0,
     NULL},
};

/* a sprite frame the image keeps but nothing reads */
static NJS_TEXANIM move_seat_texanim = {
#include "adv0100/move_seat/move_seat_texanim.inc"
};

void egg_carrier_move_seat_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return;
    }
    switch (work->mode) {
    case SEAT_INIT:
        if (!placement_editing(task)) {
            collision_init(task, move_seat_shapes, COLLISION_OBJECT, 1, 2);
        }
        task->destructor = move_seat_destroy;
        task->draw = move_seat_draw;
        work->mode = SEAT_WAIT;
        break;
    case SEAT_WAIT:
        if (work->collision->flags & COLLISION_HIT) {
            if (work->collision->own_shape == CUSHION) {
                work->mode = SEAT_BOUNCE;
                work->flags |= SEAT_BOUNCING;
                VARS(work)->bounce = 0;
            }
        }
        break;
    case SEAT_BOUNCE:
        VARS(work)->bounce += BOUNCE_STEP;
        if (VARS(work)->bounce == BOUNCE_END) {
            work->mode = SEAT_WAIT;
            VARS(work)->bounce = 0;
            work->flags &= ~SEAT_BOUNCING;
        }
        break;
    case SEAT_STILL:
        break;
    }
    if (work->flags & SEAT_BOUNCING) {
        LEVEL(work) += LEVEL_STEP;
        if (LEVEL(work) >= 1.0F) {
            LEVEL(work) = 1.0F;
        }
    } else {
        /* @hack spelling: -LEVEL_STEP added, a constant of its own, which SHC
         * loads through R1 and FPUL (MOV.L at 0x0c926bfc), as the rise loads
         * 0.1 (0x0c926be4); with -=, SHC loads 0.1 once into FR4 for both, the
         * function runs 8 bytes short and 46 of 123 words differ */
        LEVEL(work) += -LEVEL_STEP;
        if (LEVEL(work) <= 0.0F) {
            LEVEL(work) = 0.0F;
        }
    }
    move_seat_draw_work(task, work);
    if (!placement_editing(task)) {
        collision_register(work);
    }
}

static void move_seat_destroy(struct task *task)
{
}

static void move_seat_draw(struct task *task)
{
    struct task_work *work = task->work;

    move_seat_draw_work(task, work);
}

/* draw the base, then the spring and the cushion, its children. the bounce
 * lowers the cushion and rocks it about z either way, as CUSHION_SAG and
 * CUSHION_ROCK say */
static void move_seat_draw_work(struct task *task, struct task_work *work)
{
    Float height = ((njSin(VARS(work)->bounce + 0x4000) - 1.0F) * CUSHION_SAG) +
                   CUSHION_REST;
    Angle rock = (Angle)(njSin(VARS(work)->bounce) * CUSHION_ROCK);
    NJS_OBJECT *object = &move_seat_object;
    Angle ang;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    njTranslateV(NULL, (NJS_VECTOR *)object->pos);
    draw_model(object->model);
    object = object->child;
    njPushMatrix(NULL);
    njTranslateV(NULL, (NJS_VECTOR *)object->pos);
    draw_model(object->model);
    njPopMatrix(1);
    object = object->sibling;
    njPushMatrix(NULL);
    njTranslateV(NULL, (NJS_VECTOR *)object->pos);
    njTranslate(NULL, 0.0F, height, 0.0F);
    TURN(njRotateZ, rock);
    draw_model(object->model);
    njPopMatrix(1);
    njPopMatrix(1);
    draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
}
