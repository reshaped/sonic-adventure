/* O BROOBJ, a box of HakoAF's six pieces that an attack breaks. all but the
 * first piece fly apart, and fade */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "draw/model.h"
#include "draw/tree.h"
#include "effect/dust.h"
#include "enemy/enemy.h"
#include "object/placement.h"
#include "player/lockon.h"
#include "sound/sound.h"
#include "stg07/breakable.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT box_piece0_object;
extern NJS_OBJECT box_piece1_object;
extern NJS_OBJECT box_piece2_object;
extern NJS_OBJECT box_piece3_object;
extern NJS_OBJECT box_piece4_object;
extern NJS_OBJECT box_piece5_object;

struct breakable_vars {
    Float unk_00; /* the start sets it to 0, and nothing reads it */
    Float spin; /* it gains scl.x each step, and nothing reads it */
};

struct piece_vars {
    Uint8 unk_00[4];
    NJS_OBJECT *part;
    Float fade; /* its constant material's alpha: 0, falling as it fades */
};

struct piece_motion {
    Uint8 unk_00[4];
    NJS_VECTOR speed;
    Uint8 unk_10[24];
    Angle spin_x; /* its turn about x each step */
    Angle spin_y; /* its turn about y each step */
};

#define VARS(work) ((struct breakable_vars *)&(work)->vars)
#define PIECE(work) ((struct piece_vars *)&(work)->vars)
#define PIECE_MOTION(task) ((struct piece_motion *)(task)->motion)
/* the parts the pieces' task makes pieces of: all but the first of the
 * PIECES_COUNT from PIECES_FIRST */
#define PIECES_FIRST(work) ((work)->submode)
#define PIECES_COUNT(work) ((work)->tick)

enum breakable_mode { BREAKABLE_STAND, BREAKABLE_BROKEN };

/* the pieces' modes, 0 and 2 as the image stores them */
enum pieces_mode { PIECES_START = 0, PIECES_RUN = 2 };

/* a break plays SE_BREAK, and raises dust DUST_HEIGHT above the box at a
 * scale of DUST_SCALE */
#define SE_BREAK 64
#define DUST_HEIGHT 10.0F
#define DUST_SCALE 3.4F

/* placement_try_leave_by's squared range: the box runs while the camera
 * or player 0 is within 200 */
#define BREAKABLE_RANGE (200.0F * 200.0F)

/* it draws only while a sphere of this radius at its position is in view */
#define VIEW_RADIUS 10.0F

/* the pieces' task kills itself once its timer passes PIECE_LIFE. each
 * step, a piece's speed up falls by PIECE_GRAVITY. once a piece has flown
 * more than PIECE_FADE_AFTER steps, it fades by PIECE_FADE a step */
#define PIECE_LIFE 300
#define PIECE_GRAVITY 0.1F
#define PIECE_FADE 0.02F
#define PIECE_FADE_AFTER 20

static const NJS_VECTOR dust_speed = {0.0F, 1.0F, 0.0F};

#define BREAKABLE_PARTS 6

/* the box's parts, which it draws whole. a break makes pieces of all but
 * the first */
static NJS_OBJECT *ruin_breakable_parts[BREAKABLE_PARTS] = {
    &box_piece0_object, &box_piece1_object, &box_piece2_object,
    &box_piece3_object, &box_piece4_object, &box_piece5_object,
};

/* clang-format off */
static struct collision_shape ruin_breakable_shape = {
    SHAPE_SPHERE | SHAPE_HURTABLE, 0, PUSH_ALL | PUSH_FIRM(PUSH_ALL),
    {0.0F, 0.0F, 0.0F}, 12.0F, 10.0F, 0.0F, {0, 0, 0}, 0, NULL,
};
/* clang-format on */

static void ruin_breakable_pieces_step(struct task *task);
static void ruin_breakable_pieces_destroy(struct task *task);
static void ruin_breakable_run(struct task *task);
static void ruin_breakable_draw(struct task *task);
static void ruin_breakable_destroy(struct task *task);
static void ruin_breakable_piece_start(struct task *task);
static void ruin_breakable_piece_step(struct task *task);
static void ruin_breakable_piece_draw(struct task *task);
static void ruin_breakable_piece_destroy(struct task *task);

/* give the box an enemy's record, so that E-102 can lock on to it */
void ruin_breakable_step(struct task *task)
{
    struct task_work *work = task->work;

    enemy_alloc_record(task, work);
    work->mode = BREAKABLE_STAND;
    work->timer = 0;
    VARS(work)->unk_00 = 0.0F;
    if (!placement_editing(task)) {
        collision_init(task, &ruin_breakable_shape, COLLISION_ITEM, 1, 1);
    }
    task->step = ruin_breakable_run;
    task->draw = ruin_breakable_draw;
    task->destructor = ruin_breakable_destroy;
}

/* the step of the task that holds a break's pieces. its first step makes a
 * piece of each part it names, where the part sits in the box. later steps
 * step the pieces */
static void ruin_breakable_pieces_step(struct task *task)
{
    struct task_work *work = task->work;
    int i;
    int count;
    Float x;
    Float y;
    Float z;
    Angle ang;
    struct task *child;
    struct task_work *piece;
    NJS_OBJECT *part;

    switch (work->mode) {
    case PIECES_START:
        count = PIECES_COUNT(work);
        x = work->pos.x;
        y = work->pos.y;
        z = work->pos.z;
        ang = work->ang.y;
        for (i = 1; i < count; i++) {
            child = task_create_child(TASK_WORK | TASK_MOTION,
                                      ruin_breakable_piece_start, task);
            piece = child->work;
            part = ruin_breakable_parts[PIECES_FIRST(work) + i];
            PIECE(piece)->part = part;
            piece->ang.y = ang;
            piece->pos.x = x + part->pos[0];
            piece->pos.y = y + part->pos[1];
            piece->pos.z = z + part->pos[2];
        }
        work->timer = 0;
        task->destructor = ruin_breakable_pieces_destroy;
        work->mode = PIECES_RUN;
        break;
    case PIECES_RUN:
        TASK_TIMER(work)++;
        if (TASK_TIMER(work) > PIECE_LIFE) {
            task_kill(task);
        }
        task_step_children(task);
        break;
    }
}

static void ruin_breakable_pieces_destroy(struct task *task)
{
    task_destroy_children(task);
}

/* while the box stands, each hit enemy_take_hit reports breaks it. the
 * break raises dust, plays SE_BREAK and starts a task for the pieces. the
 * step after, placement_kill ends the box for good */
static void ruin_breakable_run(struct task *task)
{
    struct task_work *work = task->work;
    NJS_POINT3 pos;
    NJS_VECTOR speed;
    struct task *pieces;
    struct task_work *pieces_work;

    if (placement_try_leave_by(task, BREAKABLE_RANGE)) {
        return;
    }
    VARS(work)->spin += work->scl.x;
    if (placement_editing(task)) {
        ruin_breakable_draw(task);
    } else {
        switch (work->mode) {
        case BREAKABLE_STAND:
            if (enemy_take_hit(work, LOCKON_TARGET(task))) {
                pos.x = work->pos.x;
                pos.y = work->pos.y + DUST_HEIGHT;
                pos.z = work->pos.z;
                speed = dust_speed;
                dust_create(&pos, &speed, DUST_SCALE);
                work->mode = BREAKABLE_BROKEN;
                pieces = task_create(TASK_WORK | TASK_MOTION, 3,
                                     ruin_breakable_pieces_step);
                pieces_work = pieces->work;
                PIECES_COUNT(pieces_work) = BREAKABLE_PARTS;
                PIECES_FIRST(pieces_work) = 0;
                pieces_work->mode = PIECES_START;
                pieces_work->ang.y = work->ang.y;
                pieces_work->pos.x = work->pos.x;
                pieces_work->pos.y = work->pos.y;
                pieces_work->pos.z = work->pos.z;
                sound_play(SE_BREAK, 0, 0, 0);
            } else {
                collision_register(work);
            }
            ruin_breakable_draw(task);
            break;
        case BREAKABLE_BROKEN:
            placement_kill(task);
            break;
        }
    }
}

static void ruin_breakable_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (draw_in_view(work->pos.x, work->pos.y, work->pos.z, VIEW_RADIUS)) {
        texture_set_stage();
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateZ, work->ang.z, ang);
        TURN_VIA(njRotateX, work->ang.x, ang);
        TURN_VIA(njRotateY, work->ang.y, ang);
        draw_tree(ruin_breakable_parts[0]);
        draw_tree(ruin_breakable_parts[1]);
        draw_tree(ruin_breakable_parts[2]);
        draw_tree(ruin_breakable_parts[3]);
        draw_tree(ruin_breakable_parts[4]);
        draw_tree(ruin_breakable_parts[5]);
        njPopMatrix(1);
    }
}

static void ruin_breakable_destroy(struct task *task)
{
    struct task_work *work = task->work;
    struct lockon_target *record = LOCKON_TARGET(task);

    enemy_end(task, work, record);
}

static void ruin_breakable_piece_start(struct task *task)
{
    struct task_work *work = task->work;
    struct piece_motion *motion = PIECE_MOTION(task);

    work->timer = 0;
    PIECE(work)->fade = 0.0F;
    motion->speed.x = RANDOM(-0.5F, 0.5F) * 1.6F;
    motion->speed.y = RANDOM(2.0F, 3.5F);
    motion->speed.z = RANDOM(-0.5F, 0.5F) * 1.6F;
    motion->spin_y = RANDOM(-0.5F, 0.5F) * 2000.0F;
    motion->spin_x = RANDOM(-0.5F, 0.5F) * 3500.0F;
    task->step = ruin_breakable_piece_step;
    task->draw = ruin_breakable_piece_draw;
    task->destructor = ruin_breakable_piece_destroy;
}

static void ruin_breakable_piece_step(struct task *task)
{
    struct task_work *work = task->work;
    struct piece_motion *motion = PIECE_MOTION(task);

    work->pos.x += motion->speed.x;
    work->pos.y += motion->speed.y;
    work->pos.z += motion->speed.z;
    motion->speed.y -= PIECE_GRAVITY;
    work->ang.y += motion->spin_y;
    work->ang.x += motion->spin_x;
    if (TASK_TIMER(work) > PIECE_FADE_AFTER) {
        PIECE(work)->fade -= PIECE_FADE;
    } else {
        TASK_TIMER(work)++;
    }
    ruin_breakable_piece_draw(task);
}

static void ruin_breakable_piece_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateX, work->ang.x, ang);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_attr_save();
    draw_attr_set(0, NJD_FLAG_USE_ALPHA);
    draw_set_material(PIECE(work)->fade, 0.0F, 0.0F, 0.0F);
    draw_model(PIECE(work)->part->model);
    draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
    draw_attr_restore();
    njPopMatrix(1);
}

static void ruin_breakable_piece_destroy(struct task *task)
{
}
