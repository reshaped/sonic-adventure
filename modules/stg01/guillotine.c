/* O BGIRO: a guillotine blade that drops between two posts, then rises half
 * as fast */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/model.h"
#include "effect/dust.h"
#include "object/common.h"
#include "object/placement.h"
#include "sound/sound.h"
#include "stg01/guillotine.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT guillotine_frame_object;
extern NJS_MODEL guillotine_blade_model;
extern NJS_MODEL guillotine_shadow_model;

/* how far the blade drops. at the bottom it holds HOLD + 1 steps */
#define DROP 38.0F
#define HOLD 30

/* the draw shows nothing unless a sphere of radius VIEW_RADIUS about the
 * blade is in view */
#define VIEW_RADIUS 20.0F

/* the shadow lies SHADOW_DEPTH below top */
#define SHADOW_DEPTH 46.0F

/* the dust comes in DUST_PAIRS pairs of puffs, along the blade's width. the
 * first pair is at its center, and each next pair DUST_GAP farther out
 * either side */
#define DUST_PAIRS 9
#define DUST_GAP 3

#define SE_GUILLOTINE 45

/* the blade's vars. TASK_TIMER(work) holds the cycle's angle in degrees, and
 * work->tick is 1 once a hold is over. work->scl.y holds how far the blade
 * has dropped, and work->scl.z is 1 until the first drop */
struct guillotine_vars {
    Uint8 held; /* the steps of the current hold so far */
    Float top; /* the blade's y at rest, where it starts */
};

#define VARS(work) ((struct guillotine_vars *)&(work)->vars)

/* in GUILLOTINE_END, the step only checks its range. no code sets it */
enum guillotine_mode { GUILLOTINE_INIT, GUILLOTINE_RUN, GUILLOTINE_END };
enum guillotine_posts_mode { POSTS_INIT, POSTS_RUN };

static void guillotine_move(struct task *task);
static void guillotine_draw(struct task *task);
static void guillotine_destroy(struct task *task);
static void guillotine_posts_step(struct task *task);
static void guillotine_dust(struct task *task);

static struct collision_shape guillotine_post_shapes[2] = {
    {SHAPE_CYLINDER,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {-29.0F, -27.0F, 0.0F},
     5.0F,
     27.0F,
     0.0F,
     {0, 0, 0},
     0,
     NULL},
    {SHAPE_CYLINDER,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {29.0F, -27.0F, 0.0F},
     5.0F,
     27.0F,
     0.0F,
     {0, 0, 0},
     0,
     NULL},
};

static struct collision_shape guillotine_blade_shape = {
    SHAPE_RECTANGLE | SHAPE_HURTABLE,
    ATTACK(2) | SHAPE_HURTS_PLAYERS,
    PUSH_ALL | PUSH_FIRM(PUSH_ALL),
    {0.0F, 0.0F, 0.0F},
    28.2F,
    10.0F,
    5.0F,
    {0, 0, 0},
    0,
    NULL};

/* params for a dust puff. no code reads them, but the image holds them */
/* clang-format off */
static struct dust_params guillotine_dust_params = {
    1.0F, 0.23F, 0.17F, 0.06F, 0.025F,
    {0.0F, 0.0F, 0.0F}, {0.0F, 0.0F, 0.0F}, {1.0F, 1.0F, 1.0F, 1.0F},
};
/* clang-format on */

void guillotine_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return;
    }
    switch (work->mode) {
    case GUILLOTINE_INIT:
        task_create_child(TASK_WORK, guillotine_posts_step, task);
        if (!placement_editing(task)) {
            collision_init(task, &guillotine_blade_shape, COLLISION_OBJECT, 1,
                           1);
        }
        task->destructor = guillotine_destroy;
        task->draw = guillotine_draw;
        work->mode = GUILLOTINE_RUN;
        VARS(work)->top = work->pos.y;
        work->timer = 0;
        work->scl.z = 1.0F;
        break;
    case GUILLOTINE_RUN:
        if (!placement_editing(task)) {
            guillotine_move(task);
            collision_register(work);
            object_clear_touch(work, NULL);
        }
        guillotine_draw(task);
        task_step_children(task);
        break;
    case GUILLOTINE_END:
        break;
    }
}

/* advance the cycle a step. the blade drops through the cycle's first
 * quarter, 2 degrees a step, and holds at the bottom. then it rises through
 * the second quarter and rests through the second half, 1 degree a step. a
 * hold at the bottom starts with dust and SE_GUILLOTINE. before the first
 * drop, the blade holds as long at the top, without them */
static void guillotine_move(struct task *task)
{
    struct task_work *work = task->work;
    int angle = TASK_TIMER(work);

    if (angle > 0 && angle < 90) {
        TASK_TIMER(work) += 2;
        work->tick = 0;
        VARS(work)->held = 0;
    } else if (!work->tick) {
        VARS(work)->held++;
        if (VARS(work)->held == 1 && work->scl.z == 0.0F) {
            guillotine_dust(task);
            sound_play(SE_GUILLOTINE, 0, 0, 0);
        }
        if (VARS(work)->held > HOLD) {
            work->tick = 1;
        }
    } else {
        work->scl.z = 0.0F;
        TASK_TIMER(work)++;
    }
    work->scl.y = njSin((Angle)(TASK_TIMER(work) * ANGLE_PER_DEGREE)) * DROP;
    if (work->scl.y < 0.0F) {
        work->scl.y = 0.0F;
    }
    if (TASK_TIMER(work) > 360) {
        TASK_TIMER(work) = 0;
    }
    work->pos.y = VARS(work)->top - work->scl.y;
}

/* draw the blade where it has dropped to, and the frame at top. the shadow
 * widens along z as the blade drops */
static void guillotine_draw(struct task *task)
{
    static const NJS_VECTOR guillotine_frame_offset = {0.0F, 0.0F, 0.0F};
    struct task_work *work = task->work;
    NJS_OBJECT *frame = &guillotine_frame_object;
    NJS_VECTOR offset;
    Angle ang;

    if (draw_in_view(work->pos.x, work->pos.y, work->pos.z, VIEW_RADIUS)) {
        texture_set_stage();
        njPushMatrix(NULL);
        njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
        TURN_VIA(njRotateY, work->ang.y, ang);
        draw_model(&guillotine_blade_model);
        njPushMatrix(NULL);
        njTranslate(NULL, frame->pos[0], frame->pos[1], frame->pos[2]);
        offset = guillotine_frame_offset;
        offset.y = work->scl.y;
        njTranslateV(NULL, &offset);
        draw_model(frame->model);
        njPopMatrix(1);
        njPopMatrix(1);
        njPushMatrix(NULL);
        njTranslate(NULL, work->pos.x, work->pos.y + work->scl.y - SHADOW_DEPTH,
                    work->pos.z);
        TURN_VIA(njRotateY, work->ang.y, ang);
        njScale(NULL, 1.0F, 1.0F, (0.0325F * work->scl.y) + 0.82F);
        draw_model(&guillotine_shadow_model);
        njPopMatrix(1);
    }
}

static void guillotine_destroy(struct task *task)
{
    task_destroy_children(task);
}

/* the step of the posts' task, a child of the blade's. the posts stay where
 * the blade starts, facing the same way */
static void guillotine_posts_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *blade = task->parent->work;

    switch (work->mode) {
    case POSTS_INIT:
        collision_init(task, guillotine_post_shapes, COLLISION_OBJECT, 1, 2);
        work->mode = POSTS_RUN;
        work->pos.x = blade->pos.x;
        work->pos.y = blade->pos.y;
        work->pos.z = blade->pos.z;
        work->ang.x = blade->ang.x;
        work->ang.y = blade->ang.y;
        work->ang.z = blade->ang.z;
        break;
    case POSTS_RUN:
        collision_register(work);
        break;
    }
}

/* puffs of dust at the bottom of the drop */
static void guillotine_dust(struct task *task)
{
    static const NJS_VECTOR guillotine_dust_speed = {0.0F, 0.0F, 0.0F};
    struct task_work *work = task->work;
    NJS_POINT3 turned;
    NJS_POINT3 pos;
    NJS_VECTOR speed;
    Angle ang;
    int i;

    speed = guillotine_dust_speed;
    for (i = 0; i < DUST_PAIRS * DUST_GAP; i += DUST_GAP) {
        pos.x = (Float)i;
        pos.y = 0.0F;
        pos.z = 0.0F;
        njPushMatrix(&_unit_matrix_);
        TURN_VIA(njRotateY, work->ang.y, ang);
        njCalcPoint(NULL, &pos, &turned);
        njPopMatrix(1);
        pos.y = VARS(work)->top - DROP;
        pos.x = turned.x + work->pos.x;
        pos.z = turned.z + work->pos.z;
        dust_create(&pos, &speed, 2.4F);
        pos.x = -turned.x + work->pos.x;
        pos.z = -turned.z + work->pos.z;
        dust_create(&pos, &speed, 2.4F);
    }
}
