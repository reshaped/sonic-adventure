/* the snake statues: HebiZou, HebiZouL and HebiZouR, each with a fire above
 * it. an enemy's hit breaks all but the base into nine flying pieces */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/model.h"
#include "object/common.h"
#include "object/placement.h"
#include "stg07/fire.h"
#include "stg07/snake_statue.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_MODEL snake_statue_model;
extern NJS_MODEL snake_statue_base_model;
extern NJS_MODEL snake_statue_piece0_model;
extern NJS_MODEL snake_statue_piece1_model;
extern NJS_MODEL snake_statue_piece2_model;
extern NJS_MODEL snake_statue_piece3_model;
extern NJS_MODEL snake_statue_piece4_model;
extern NJS_MODEL snake_statue_piece5_model;
extern NJS_MODEL snake_statue_piece6_model;
extern NJS_MODEL snake_statue_piece7_model;
extern NJS_MODEL snake_statue_piece8_model;

/* the statue's kind, in its work's mode. a round statue's pieces fly out
 * all round, and a left or right statue's to one side */
enum snake_statue_kind { STATUE_ROUND, STATUE_LEFT, STATUE_RIGHT };

struct snake_statue_vars {
    Uint8 unk_00[4];
    Angle spin; /* the pieces' turn about y so far */
};

#define VARS(work) ((struct snake_statue_vars *)&(work)->vars)

/* in the work's scale. STATUE_SPREAD is how far out the pieces have flown,
 * and STATUE_HEIGHT how high. the height gains twice STATUE_RISE each step */
#define STATUE_SPREAD(work) ((work)->scl.x)
#define STATUE_RISE(work) ((work)->scl.y)
#define STATUE_HEIGHT(work) ((work)->scl.z)

/* in the work's flags: the pieces have fallen away, and only the base
 * draws */
#define STATUE_GONE 0x100

/* each step, the pieces fly PIECE_SPREAD farther out and turn PIECE_SPIN.
 * their rise starts at PIECE_RISE and loses PIECE_GRAVITY. they are gone
 * once it drops to GONE_RISE */
#define PIECE_SPREAD 1.0F
#define PIECE_SPIN 0x400
#define PIECE_RISE 3.0F
#define PIECE_GRAVITY 0.2F
#define GONE_RISE (-10.0F)

/* a piece shrinks by PIECE_SHRINK of its size for each unit of
 * STATUE_SPREAD. a left or right statue's pieces fly SIDE_SPREAD times as
 * far */
#define PIECE_SHRINK 0.02F
#define SIDE_SPREAD 3.0F

/* the fire burns FIRE_HEIGHT above the statue, and FIRE_AHEAD toward its y
 * angle */
#define FIRE_AHEAD 2.0F
#define FIRE_HEIGHT 10.0F

/* clang-format off */
static struct collision_shape snake_statue_shape = {
    SHAPE_CAPSULE, 0, PUSH_ALL | PUSH_FIRM(PUSH_ALL), {0.0F, 5.0F, 0.0F},
    4.0F, 5.0F, 0.0F, {0}, 0, NULL,
};
/* clang-format on */

static void snake_statue_destroy(struct task *task)
{
    task_destroy_children(task);
}

/* draw model, a piece at x, y and z in the statue, flown out toward ang */
static void snake_statue_draw_piece(struct task_work *work, Angle ang, Float x,
                                    Float y, Float z, NJS_MODEL *model)
{
    Float dx;
    Float dz;
    Float scale;
    Angle spin;

    if (work->mode == STATUE_ROUND) {
        dx = njSin(ang) * STATUE_SPREAD(work);
        dz = njCos(ang) * STATUE_SPREAD(work);
    } else {
        dx = njSin(ang) * STATUE_SPREAD(work) * SIDE_SPREAD;
        dz = njCos(ang) * STATUE_SPREAD(work) * SIDE_SPREAD;
    }
    scale = 1.0F - (STATUE_SPREAD(work) * PIECE_SHRINK);
    njPushMatrix(NULL);
    njTranslate(NULL, x + dx, y + STATUE_HEIGHT(work), z + dz);
    TURN_VIA(njRotateY, VARS(work)->spin, spin);
    njScale(NULL, scale, scale, scale);
    draw_model(model);
    njPopMatrix(1);
}

static void snake_statue_draw_pieces(struct task_work *work)
{
    switch (work->mode) {
    case STATUE_LEFT:
        snake_statue_draw_piece(work, 0x4000, 0.0F, 2.5F, 0.3F,
                                &snake_statue_piece0_model);
        snake_statue_draw_piece(work, 0x4800, 0.0F, 5.0F, 1.5F,
                                &snake_statue_piece1_model);
        snake_statue_draw_piece(work, 0x3800, 0.0F, 6.4F, 0.5F,
                                &snake_statue_piece2_model);
        snake_statue_draw_piece(work, 0x5000, 0.0F, 8.8F, -0.65F,
                                &snake_statue_piece3_model);
        snake_statue_draw_piece(work, 0x3000, 0.0F, 10.0F, -0.3F,
                                &snake_statue_piece4_model);
        snake_statue_draw_piece(work, 0x4400, 0.0F, 12.42F, 1.669999F,
                                &snake_statue_piece5_model);
        snake_statue_draw_piece(work, 0x3c00, 0.0F, 12.5F, 2.0F,
                                &snake_statue_piece6_model);
        snake_statue_draw_piece(work, 0x4c00, 0.0F, 12.35F, 3.899999F,
                                &snake_statue_piece7_model);
        snake_statue_draw_piece(work, 0x3400, 0.0F, 9.860002F, 1.0F,
                                &snake_statue_piece8_model);
        break;
    case STATUE_RIGHT:
        snake_statue_draw_piece(work, 0xc000, 0.0F, 2.5F, 0.3F,
                                &snake_statue_piece0_model);
        snake_statue_draw_piece(work, 0xc800, 0.0F, 5.0F, 1.5F,
                                &snake_statue_piece1_model);
        snake_statue_draw_piece(work, 0xb800, 0.0F, 6.4F, 0.5F,
                                &snake_statue_piece2_model);
        snake_statue_draw_piece(work, 0xd000, 0.0F, 8.8F, -0.65F,
                                &snake_statue_piece3_model);
        snake_statue_draw_piece(work, 0xb000, 0.0F, 10.0F, -0.3F,
                                &snake_statue_piece4_model);
        snake_statue_draw_piece(work, 0xc400, 0.0F, 12.42F, 1.669999F,
                                &snake_statue_piece5_model);
        snake_statue_draw_piece(work, 0xbc00, 0.0F, 12.5F, 2.0F,
                                &snake_statue_piece6_model);
        snake_statue_draw_piece(work, 0xcc00, 0.0F, 12.35F, 3.899999F,
                                &snake_statue_piece7_model);
        snake_statue_draw_piece(work, 0xb400, 0.0F, 9.860002F, 1.0F,
                                &snake_statue_piece8_model);
        break;
    default:
        snake_statue_draw_piece(work, 0, 0.0F, 2.5F, 0.3F,
                                &snake_statue_piece0_model);
        snake_statue_draw_piece(work, 0x8000, 0.0F, 5.0F, 1.5F,
                                &snake_statue_piece1_model);
        snake_statue_draw_piece(work, 0x4000, 0.0F, 6.4F, 0.5F,
                                &snake_statue_piece2_model);
        snake_statue_draw_piece(work, 0xc000, 0.0F, 8.8F, -0.65F,
                                &snake_statue_piece3_model);
        snake_statue_draw_piece(work, 0x2000, 0.0F, 10.0F, -0.3F,
                                &snake_statue_piece4_model);
        snake_statue_draw_piece(work, 0xa000, 0.0F, 12.42F, 1.669999F,
                                &snake_statue_piece5_model);
        snake_statue_draw_piece(work, 0x6000, 0.0F, 12.5F, 2.0F,
                                &snake_statue_piece6_model);
        snake_statue_draw_piece(work, 0xe000, 0.0F, 12.35F, 3.899999F,
                                &snake_statue_piece7_model);
        snake_statue_draw_piece(work, 0x1000, 0.0F, 9.860002F, 1.0F,
                                &snake_statue_piece8_model);
        break;
    }
}

static void snake_statue_draw_broken(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_model(&snake_statue_base_model);
    if (!(work->flags & STATUE_GONE)) {
        snake_statue_draw_pieces(work);
    }
    njPopMatrix(1);
}

/* the broken statue's step. the pieces fly out and turn while they rise
 * and then fall */
static void snake_statue_break(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return;
    }
    snake_statue_draw_broken(task);
    if (work->flags & STATUE_GONE) {
        return;
    }
    if (STATUE_RISE(work) <= GONE_RISE) {
        work->flags += STATUE_GONE;
    } else {
        STATUE_SPREAD(work) += PIECE_SPREAD;
        VARS(work)->spin += PIECE_SPIN;
        STATUE_RISE(work) -= PIECE_GRAVITY;
        STATUE_HEIGHT(work) += STATUE_RISE(work) * 2.0F;
    }
}

/* the whole statue's step and draw. it steps its fire until an enemy's hit
 * breaks the statue */
static void snake_statue_wait(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (placement_try_leave(task)) {
        return;
    }
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    TURN_VIA(njRotateY, work->ang.y, ang);
    texture_set_stage();
    draw_model(&snake_statue_model);
    njPopMatrix(1);
    task_step_children(task);
    if (work->collision->flags & COLLISION_HIT) {
        if (work->collision->hit->kind == COLLISION_ENEMY) {
            task->step = snake_statue_break;
            task->draw = snake_statue_draw_broken;
        }
    }
    if (!placement_editing(task)) {
        collision_register(work);
        object_clear_touch(work, NULL);
    }
}

static void snake_statue_init(struct task *task)
{
    struct task_work *work = task->work;
    struct task *fire;

    collision_init(task, &snake_statue_shape, COLLISION_OBJECT, 1, 1);
    task->step = snake_statue_wait;
    task->draw = snake_statue_wait;
    task->destructor = snake_statue_destroy;
    STATUE_HEIGHT(work) = 0.0F;
    STATUE_RISE(work) = PIECE_RISE;
    if (!placement_editing(task)) {
        fire = task_create_child(TASK_WORK, ruin_fire_step, task);
        fire->work->pos.x = work->pos.x + (njSin(work->ang.y) * FIRE_AHEAD);
        fire->work->pos.y = work->pos.y + FIRE_HEIGHT;
        fire->work->pos.z = work->pos.z + (njCos(work->ang.y) * FIRE_AHEAD);
    }
    work->flags &= ~STATUE_GONE;
}

void snake_statue_step(struct task *task)
{
    struct task_work *work = task->work;

    work->mode = STATUE_ROUND;
    snake_statue_init(task);
}

void snake_statue_l_step(struct task *task)
{
    struct task_work *work = task->work;

    work->mode = STATUE_LEFT;
    work->ang.y = 0;
    snake_statue_init(task);
}

void snake_statue_r_step(struct task *task)
{
    struct task_work *work = task->work;

    work->mode = STATUE_RIGHT;
    work->ang.y = 0x8000;
    snake_statue_init(task);
}
