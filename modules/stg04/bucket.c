/* O BAKETU bucket that breaks in two when hit */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "collision/ground.h"
#include "debug/editor.h"
#include "draw/model.h"
#include "object/common.h"
#include "object/placement.h"
#include "player/player.h"
#include "stg04/bucket.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_MODEL bucket_piece_1_model;
extern NJS_MODEL bucket_piece_2_model;
extern NJS_OBJECT bucket_object; /* its model is the second piece */

/* the least ground speed of player 0 counted when the bucket breaks */
#define BREAK_SPEED_LEAST 0.5F

/* a piece is drawn PIECE_LIFT above its position. ground whose normal's y is
 * under NORMAL_Y_LEAST is too steep for a piece. over it, a piece takes
 * PIECE_NO_GROUND as the ground's height */
#define PIECE_LIFT 3.0F
#define NORMAL_Y_LEAST 0.7F
#define PIECE_NO_GROUND (-10000.0F)

/* a piece's slide speed starts at SLIDE_PER_SPEED times player 0's ground
 * speed. its rise speed starts at RISE_PER_SPEED times it */
#define SLIDE_PER_SPEED 2.0F
#define RISE_PER_SPEED 3.0F

/* a flying piece moves up SPEED_SHARE of its rise speed a step. it moves
 * across SPEED_SHARE of its slide speed. it moves FALL_STEP less up for each
 * step it has flown. that count starts over whenever the piece is below the
 * ground. its rise speed loses RISE_LOSS a step, and its slide speed
 * SLIDE_LOSS */
#define SPEED_SHARE 0.25F
#define FALL_STEP 0.0625F
#define RISE_LOSS 0.012F
#define SLIDE_LOSS 0.01F

/* piece 1's tip limit past the slope and its slide off its heading, in
 * degrees, as bucket_piece_fly says. a piece tipped past its limit is set to
 * TIP_REST past the slope */
#define TIP_PAST 90.0F
#define TIP_REST NJM_DEG_ANG(92)
#define SLIDE_SWERVE 20.0F

enum bucket_mode { BUCKET_INIT = 0, BUCKET_STAND = 1, BUCKET_BROKEN = 3 };
enum piece_mode {
    PIECE_INIT = 0,
    PIECE_FLY = 1,
    PIECE_GONE = 3 /* this file never sets it */
};

struct piece_vars {
    NJS_MODEL *model;
    Sint16 spin; /* turn about y, in degrees */
};

#define PIECE(work) ((struct piece_vars *)&(work)->vars)
/* a piece's work->tick is TRUE once its rise speed runs out. work->scl holds
 * its slide speed, its index, 0 or 1, and its rise speed */
#define SETTLED(work) ((work)->tick)
#define SLIDE_SPEED(work) ((work)->scl.x)
#define PIECE_INDEX(work) ((work)->scl.y)
#define RISE_SPEED(work) ((work)->scl.z)

static void bucket_break(struct task *task);
static void bucket_draw(struct task *task);
static void bucket_destroy(struct task *task);
static void bucket_piece_step(struct task *task);
static void bucket_piece_draw(struct task_work *work);
static void bucket_piece_fly(struct task_work *work);

static struct collision_shape bucket_shape = {
    SHAPE_CYLINDER, 0, 0, {0.0F, 0.0F, 0.0F}, 5.0F, 12.0F, 0.0F, {0}, 0, NULL,
};

void bucket_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return;
    }
    switch (work->mode) {
    case BUCKET_INIT:
        MODEL(work)->object = &bucket_object;
        if (!placement_editing(task)) {
            collision_init(task, &bucket_shape, COLLISION_OBJECT, 1, 1);
        }
        task->destructor = bucket_destroy;
        task->draw = bucket_draw;
        work->mode = BUCKET_STAND;
        break;
    case BUCKET_STAND:
        if (!placement_editing(task)) {
            bucket_break(task);
            collision_register(work);
            object_clear_touch(work, NULL);
        }
        bucket_draw(task);
        break;
    case BUCKET_BROKEN:
        break;
    }
}

/* a hit breaks the bucket into its two pieces, which fly away from player 0.
 * player 0's ground speed counts as BREAK_SPEED_LEAST at the least */
static void bucket_break(struct task *task)
{
    struct task_work *work = task->work;
    struct task *player_task;
    struct task_work *player;
    struct task *piece;
    Float speed;
    int i;

    /* @hack redundant: the else after the return, which calls
     * player_ground_speed through R2 (0x0c903b28, 0x0c903b2e); without it, R3
     * takes the call and R2 the collision record, and 4 of 132 words differ */
    if ((player_task = player_get_task(0)) == NULL) {
        return;
    } else {
        player = player_task->work;
        /* @bug uninitialized: player_ground_speed leaves speed unset for
         * player 0 off the ground, and its FALSE goes unread. a hit then
         * reads speed's frame slot unwritten (FMOV.S @R15,FR3 at
         * 0x0c903b4a) */
#ifdef GCC
        /* repairs the unset speed the tag names: 0, a still player's ground
         * speed, breaks the bucket at BREAK_SPEED_LEAST */
        speed = 0.0F;
#endif
        player_ground_speed(0, &speed);
        if (work->collision->flags & COLLISION_HIT) {
            work->collision->flags &= ~COLLISION_HIT;
            if (speed < BREAK_SPEED_LEAST) {
                speed = BREAK_SPEED_LEAST;
            }
            work->mode = BUCKET_BROKEN;
            task->step = task_step_children;
            {
                NJS_MODEL *models[2] = {&bucket_piece_1_model,
                                        &bucket_piece_2_model};

                for (i = 0; i < 2; i++) {
                    if ((piece = task_create_child(TASK_WORK, bucket_piece_step,
                                                   task)) != NULL) {
                        piece->work->ang.y =
                            (Angle)(atan2f(work->pos.x - player->pos.x,
                                           work->pos.z - player->pos.z) *
                                    ANGLE_PER_RADIAN);
                        SLIDE_SPEED(piece->work) = speed * SLIDE_PER_SPEED;
                        PIECE_INDEX(piece->work) = (Float)i;
                        RISE_SPEED(piece->work) = speed * RISE_PER_SPEED;
                        PIECE(piece->work)->model = models[i];
                        piece->work->pos = work->pos;
                    }
                }
            }
        }
    }
}

static void bucket_draw(struct task *task)
{
    struct task_work *work = task->work;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
    draw_model(&bucket_piece_2_model);
    draw_model(&bucket_piece_1_model);
    njPopMatrix(1);
}

static void bucket_destroy(struct task *task)
{
}

static void bucket_piece_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case PIECE_INIT:
        work->mode = PIECE_FLY;
        work->timer = 0;
        break;
    case PIECE_FLY:
        bucket_piece_fly(work);
        break;
    case PIECE_GONE:
        task_kill(task);
        break;
    default:
        work->mode = PIECE_INIT;
        break;
    }
    if (!placement_try_leave(task)) {
        bucket_piece_draw(work);
    }
}

static void bucket_piece_draw(struct task_work *work)
{
    NJS_MODEL *model = PIECE(work)->model;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x, work->pos.y + PIECE_LIFT, work->pos.z);
    TURN(njRotateY, (Angle)(PIECE(work)->spin * ANGLE_PER_DEGREE));
    TURN(njRotateZ, work->ang.z);
    TURN(njRotateX, work->ang.x);
    draw_model(model);
    njPopMatrix(1);
}

/* a flying piece's step. until its rise speed runs out, the piece flies. as
 * it flies, it tips about x by PIECE_INDEX(work) + 1 degrees a step. it then
 * rests on the ground. its tip's limit is PIECE_INDEX(work) times TIP_PAST
 * degrees past the slope. once the piece tips past that, it is set to
 * TIP_REST past the slope. piece 1 also spins a degree a step about y while it
 * flies. it slides SLIDE_SWERVE degrees off its heading, where piece 0 slides
 * along it */
static void bucket_piece_fly(struct task_work *work)
{
    struct angles slope;
    NJS_VECTOR up;
    NJS_VECTOR normal;
    Float ground;

    ground = ground_below(work->pos.x, work->pos.y, work->pos.z, &slope);
    up.x = 0.0F;
    up.y = 1.0F;
    up.z = 0.0F;
    njPushMatrix(&_unit_matrix_);
    TURN(njRotateZ, slope.z);
    TURN(njRotateX, slope.x);
    njCalcVector(NULL, &up, &normal);
    njPopMatrix(1);
    if (normal.y < NORMAL_Y_LEAST) {
        ground = PIECE_NO_GROUND;
    }
    if (SETTLED(work) != TRUE) {
        work->pos.y +=
            (RISE_SPEED(work) * SPEED_SHARE) - (TASK_TIMER(work) * FALL_STEP);
        RISE_SPEED(work) -= RISE_LOSS;
        work->ang.x += (Angle)((PIECE_INDEX(work) + 1.0F) * ANGLE_PER_DEGREE);
        if (work->ang.x > slope.x + (Angle)(PIECE_INDEX(work) *
                                            (ANGLE_PER_DEGREE * TIP_PAST))) {
            work->ang.x = slope.x + TIP_REST;
        }
        if (RISE_SPEED(work) < 0.0F) {
            SETTLED(work) = TRUE;
        }
        if (PIECE_INDEX(work) != 0.0F) {
            PIECE(work)->spin++;
        }
        if (work->pos.y < ground) {
            work->timer = 0;
        }
    } else {
        work->pos.y = ground;
        /* @hack redundant: an empty test of the piece's index, which keeps its
         * FCMP/EQ at 0x0c903faa; without it, 74 of 218 words differ */
        if (PIECE_INDEX(work) == 0.0F) {
        }
    }
    work->pos.x +=
        SLIDE_SPEED(work) *
        njSin(work->ang.y +
              (Angle)(PIECE_INDEX(work) * (ANGLE_PER_DEGREE * SLIDE_SWERVE))) *
        SPEED_SHARE;
    work->pos.z +=
        SLIDE_SPEED(work) *
        njCos(work->ang.y +
              (Angle)(PIECE_INDEX(work) * (ANGLE_PER_DEGREE * SLIDE_SWERVE))) *
        SPEED_SHARE;
    SLIDE_SPEED(work) -= SLIDE_LOSS;
    if (SLIDE_SPEED(work) <= 0.0F) {
        SLIDE_SPEED(work) = 0.0F;
    }
    work->timer++;
}
