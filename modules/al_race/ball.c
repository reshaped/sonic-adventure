/* the balls of a race of kind 0, one to a lane, which each lane's Chao
 * pushes toward its lane's target */
#include <ninja.h>
#include <sg_xpt.h>

#include "al_main/models.h"
#include "al_race/ball.h"
#include "al_race/game.h"
#include "al_race/item.h"
#include "al_race/racer.h"
#include "al_race/scenery.h"
#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/model.h"
#include "object/placement.h"
#include "object/simple.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/task_desc.h"
#include "system/util.h"

/* the ball's radius, which its model, a unit sphere, is scaled to */
#define BALL_RADIUS 2.7F

/* a ball's modes. BALL_LYING waits for its lane's Chao to touch it.
 * BALL_CLAIMED waits while that Chao turns to it. BALL_PUSHED keeps the
 * ball at its offset from the Chao. BALL_STOPPED leaves the ball where the
 * push ended */
enum ball_mode { BALL_LYING, BALL_CLAIMED, BALL_PUSHED, BALL_STOPPED };

/* a ball's state, in its task's motion */
struct ball {
    Sint8 mode;
    Sint8 lane; /* the lane whose racer's Chao can claim it */
    Uint8 unk_02[2];
    NJS_MODEL *model;
    struct collision_list *list;
    struct task *owner; /* the task of the Chao that claimed it, or null */
    NJS_VECTOR offset; /* its position less its owner's as the push starts */
};

#define BALL(task) ((struct ball *)(task)->motion)

/* ball_start's arg: the ball's lane, its object, its collision list and
 * where it starts */
struct ball_start {
    Sint8 lane;
    NJS_OBJECT *object;
    struct collision_list list;
    const NJS_POINT3 *pos;
};

static void ball_start(struct task *task, void *arg);
static void ball_step(struct task *task);
static void ball_draw(struct task *task);
static void ball_destroy(struct task *task);
static void ball_keep_offset(struct task *task);
static void ball_follow(struct task *task);

static struct collision_shape ball_shapes[1];

static const NJS_POINT3 ball_places[RACE_LANES] = {
#include "al_race/ball/ball_places.inc"
};
static const struct task_desc ball_desc = {sizeof(struct ball), TASK_WORK,
                                           ball_start,          ball_step,
                                           ball_draw,           ball_destroy};
static const struct angles ball_no_turn = {0, 0, 0};
static const NJS_VECTOR ball_unit = {1.0F, 1.0F, 1.0F};
static const struct collision_kind ball_collision = {COLLISION_OBJECT, 1};

static struct collision_shape ball_shapes[1] = {
    {SHAPE_SPHERE,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 0.0F, 0.0F},
     BALL_RADIUS,
     0.0F,
     0.0F,
     {0, 0, 0},
     0,
     NULL},
};
/* every ball's start record. ball_create sets lane and pos for each ball in
 * turn. ball_start, which scenery_add runs, reads them at once */
static struct ball_start ball_kind = {
    0, &alife_ball_object, {ball_shapes, 1, 40.0F}, NULL};

void ball_create(void)
{
    int i;
    const NJS_POINT3 *pos;

    for (i = 0, pos = ball_places; i < RACE_LANES; i++, pos++) {
        /* @hack type: lane stored through &ball_kind, and pos, its last
         * member, through a byte offset from its start. each store then
         * loads its own address from the pool, as the image's do
         * (0x0cb986d2, 0x0cb986d4). as ball_kind.lane and ball_kind.pos,
         * SHC keeps ball_kind's address in R14 instead. the function runs 2
         * bytes short, and 32 of 35 words differ */
        (&ball_kind)->lane = i;
        *(const NJS_POINT3 **)((char *)&ball_kind + sizeof ball_kind -
                               sizeof(NJS_POINT3 *)) = pos;
        scenery_add(&ball_desc, &ball_kind);
    }
}

int ball_tell(struct task *task, struct task *other, Sint8 command)
{
    struct ball *ball = BALL(task);
    int heard = 0;

    if (ball->owner == other) {
        switch (command) {
        case ITEM_LET_GO:
            ball->owner = NULL;
            heard = 1;
            break;
        case BALL_PUSH:
            if (ball->mode == BALL_CLAIMED) {
                ball_keep_offset(task);
                ball->mode = BALL_PUSHED;
                heard = 1;
            }
            break;
        case BALL_STOP:
            if (ball->mode == BALL_PUSHED) {
                ball->owner = NULL;
                ball->mode = BALL_STOPPED;
                heard = 1;
            }
            break;
        }
    }
    return heard;
}

int ball_get_place(NJS_POINT3 *pos, int lane)
{
    int found;

    if (pos != NULL && lane >= 0 && lane < RACE_LANES) {
        *pos = ball_places[lane];
        found = 1;
    } else {
        found = 0;
    }
    return found;
}

static void ball_start(struct task *task, void *arg)
{
    /* @hack spelling: object, task under a second name, which keeps task in
     * R13 and start in R12 (0x0cb987c4, 0x0cb987c8), as the image does.
     * through task alone, the two swap and 9 of 59 words differ */
    struct task *object = task;
    struct ball_start *start = arg;
    struct collision_list *list = &start->list;
    struct task_work *work = object->work;
    struct ball *ball = BALL(object);
    Bool active;

    work->pos = *start->pos;
    work->ang = ball_no_turn;
    work->scl = ball_unit;
    active = !placement_editing(object);
    if (active) {
        object_collision_init(object, list, &ball_collision);
    }
    ball->mode = BALL_LYING;
    ball->lane = start->lane;
    ball->model = start->object->model;
    ball->list = list;
    ball->owner = NULL;
}

static void ball_step(struct task *task)
{
    struct ball *ball;
    struct task_work *work;
    struct task *chao_task;
    Bool same_lane;

    ball_draw(task);
    work = task->work;
    ball = BALL(task);
    if (!placement_editing(task)) {
        /* @hack spelling: the default ahead of the cases, which loads
         * item_hit_chao's address into R3 and race_lane_of_task's into R2
         * (0x0cb9886a, 0x0cb98870), as the image does. after the cases, the
         * two swap and 4 of 75 words differ */
        switch (ball->mode) {
        default:
            break;
        case BALL_LYING:
            chao_task = item_hit_chao(work->collision);
            /* @hack redundant: same_lane, which nothing reads, set in a test
             * that opens with TRUE &&. SHC stores the comparison's result in
             * same_lane, at @R15 (0x0cb98884), as the image does. as a plain
             * test of the lane, the code after the function starts 32 bytes
             * earlier, and 65 of 75 words differ */
            if (TRUE &&
                (same_lane = race_lane_of_task(chao_task) == ball->lane)) {
                if (item_tell_chao(chao_task, task, ITEM_PUSH, BALL_KIND)) {
                    ball->owner = chao_task;
                    ball->mode = BALL_CLAIMED;
                }
            }
            /* fallthrough */
        case BALL_CLAIMED:
            collision_register(work);
            break;
        case BALL_PUSHED:
            ball_follow(task);
            break;
        case BALL_STOPPED:
            break;
        }
    }
}

static void ball_draw(struct task *task)
{
    struct ball *ball;
    struct task_work *work;
    struct angles *ang;
    Angle turn;
    Float scale;

    if (frame_step == 0 && !placement_far_from_camera(task)) {
        work = task->work;
        ball = BALL(task);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        ang = &work->ang;
        TURN_VIA(njRotateZ, ang->z, turn);
        /* @hack spelling: scale, BALL_RADIUS in a local, which puts ball at
         * @R15 and ang at @(4,R15) (0x0cb988ee, 0x0cb98904), as the image
         * does. with BALL_RADIUS passed to njScale, the two swap and 3 of 67
         * words differ */
        scale = BALL_RADIUS;
        njScale(NULL, scale, scale, scale);
        njSetTexture(&alife_chao_texlist);
        draw_model(ball->model);
        njPopMatrix(1);
    }
}

static void ball_destroy(struct task *task)
{
    struct ball *ball = BALL(task);

    item_tell_chao(ball->owner, task, ITEM_LET_GO, BALL_KIND);
}

/* store the ball's position less its owner's in offset, which ball_follow
 * then keeps */
static void ball_keep_offset(struct task *task)
{
    struct ball *ball = BALL(task);
    NJS_POINT3 *pos = &task->work->pos;
    NJS_POINT3 *owner_pos = &ball->owner->work->pos;
    NJS_VECTOR *offset = &ball->offset;

    VECTOR_DIFF(offset, pos, owner_pos);
}

/* while the ball has an owner, move it to its offset from the owner, and
 * roll it about z */
static void ball_follow(struct task *task)
{
    struct ball *ball = BALL(task);
    struct task *owner = ball->owner;
    struct task_work *work = task->work;
    NJS_POINT3 *pos;
    NJS_POINT3 *owner_pos;
    NJS_VECTOR *offset;
    Float last_x;

    if (owner != NULL) {
        owner_pos = &owner->work->pos;
        offset = &ball->offset;
        pos = &work->pos;
        last_x = pos->x;
        VECTOR_SUM(pos, owner_pos, offset);
        /* @bug logic: ang.z takes the turn of a ball of BALL_RADIUS
         * rolled to pos.x, less last_x, the ball's x before the move. last_x
         * is a distance, which the FSUB at 0x0cb98a50 takes from the angle
         * with no conversion */
        work->ang.z =
            -(pos->x * (65536.0F / (2.0F * NJD_PI * BALL_RADIUS))) - last_x;
    }
}
