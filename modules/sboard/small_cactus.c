/* SMALL CACTUS A and B, small cacti that burst at a touch. their pieces fly
 * on ahead of player 0, tumbling and falling, then fade */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/state.h"
#include "effect/bomb.h"
#include "object/placement.h"
#include "object/simple.h"
#include "player/player.h"
#include "sboard/small_cactus.h"
#include "sound/cue.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/task_desc.h"
#include "system/util.h"

/* a kind of small cactus, which init takes as arg */
struct small_cactus_kind {
    NJS_OBJECT *object;
    struct collision_list list;
};

/* @hack spelling: arg read through KIND at each use, which keeps arg on the
 * stack (MOV.L R5,@R15 at 0x0c9044c4); through a local, SHC keeps it in
 * R13, small_cactus_init runs 4 bytes short and 30 of its 31 words
 * differ */
#define KIND(arg) ((struct small_cactus_kind *)(arg))

/* a small cactus's record, in task->motion */
struct small_cactus_record {
    Sint8 mode;
    NJS_OBJECT *object;
    struct collision_list *list;
};

enum small_cactus_mode { SMALL_CACTUS_STAND, SMALL_CACTUS_BURST };

/* a piece's record, in task->motion */
struct small_cactus_piece {
    Sint8 mode;
    NJS_VECTOR speed;
    struct angles spin; /* what each step adds to work->ang */
    Sint16 life; /* the steps left before it fades */
    Float alpha;
};

enum small_cactus_piece_mode { PIECE_FLY, PIECE_FADE, PIECE_GONE };

/* a burst makes PIECES pieces, each with a life of PIECE_LIFE steps. once
 * a piece's life runs out, it fades: each step adds PIECE_FADE_OUT to its
 * alpha */
#define PIECES 5
#define PIECE_LIFE 120
#define PIECE_FADE_OUT (-0.01F)

/* PIECE_LIFT adds to a piece's starting speed.y, and each step adds
 * PIECE_GRAVITY to its speed.y. the image's lift is 0x3f6b851d, the second
 * float below 0.92F (0x0c9048a8). 0.92F would build another constant */
#define PIECE_LIFT 0.9199999F
#define PIECE_GRAVITY (-0.027222218F)

#define SE_SMALL_CACTUS_BURST 695

extern NJS_OBJECT small_cactus_a_object;
extern NJS_OBJECT small_cactus_b_object;
extern NJS_OBJECT small_cactus_piece_object;

static void small_cactus_init(struct task *task, void *arg);
static void small_cactus_run(struct task *task);
static void small_cactus_draw(struct task *task);
static void small_cactus_piece_init(struct task *task, void *arg);
static void small_cactus_piece_step(struct task *task);
static void small_cactus_piece_draw(struct task *task);

static const struct collision_kind small_cactus_collision = {COLLISION_OBJECT,
                                                             1};
static const struct task_desc small_cactus_desc = {
    sizeof(struct small_cactus_record),
    TASK_WORK,
    small_cactus_init,
    small_cactus_run,
    small_cactus_draw,
    NULL};
static const struct task_desc small_cactus_piece_desc = {
    sizeof(struct small_cactus_piece), TASK_WORK,
    small_cactus_piece_init,           small_cactus_piece_step,
    small_cactus_piece_draw,           NULL};

/* each kind's shape: a cylinder from the cactus's foot, 32 high for A and
 * 22 for B. it pushes every group, and none can push it */
static struct collision_shape small_cactus_a_shapes[1] = {
    {SHAPE_CYLINDER2,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 16.0F, 0.0F},
     10.0F,
     16.0F,
     0.0F,
     {0},
     0,
     NULL},
};
static struct small_cactus_kind small_cactus_a = {
    &small_cactus_a_object, {small_cactus_a_shapes, 1, 40.0F}};
static struct collision_shape small_cactus_b_shapes[1] = {
    {SHAPE_CYLINDER2,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 11.0F, 0.0F},
     9.0F,
     11.0F,
     0.0F,
     {0},
     0,
     NULL},
};
static struct small_cactus_kind small_cactus_b = {
    &small_cactus_b_object, {small_cactus_b_shapes, 1, 40.0F}};

void small_cactus_a_step(struct task *task)
{
    task_apply_desc(task, &small_cactus_desc, &small_cactus_a);
}

void small_cactus_b_step(struct task *task)
{
    task_apply_desc(task, &small_cactus_desc, &small_cactus_b);
}

/* give the cactus its shapes, unless the set editor holds it, and start it
 * standing */
static void small_cactus_init(struct task *task, void *arg)
{
    struct collision_list *list = &KIND(arg)->list;
    struct small_cactus_record *record;
    /* @hack spelling: the test through in_play, which SHC computes with MOVT
     * R4 in the branch's delay slot (0x0c9044ce); tested directly, the MOVT
     * goes, small_cactus_init runs 2 bytes short and 19 of its 31 words
     * differ */
    int in_play = !placement_editing(task);

    if (in_play) {
        object_collision_init(task, list, &small_cactus_collision);
    }
    record = (struct small_cactus_record *)task->motion;
    record->mode = SMALL_CACTUS_STAND;
    record->object = KIND(arg)->object;
    record->list = list;
}

/* a standing cactus draws itself and registers its shape. a touch bursts
 * it into PIECES pieces, which it then runs as its children. once they are
 * gone, the cactus destroys itself, and its placement makes no more until
 * placement_reset. while the set editor holds the cactus, it only draws */
static void small_cactus_run(struct task *task)
{
    struct small_cactus_record *record;
    struct task_work *work;
    NJS_POINT3 burst;
    NJS_POINT3 *pos;
    struct collision_work *collision;
    int i;

    if (placement_try_leave(task)) {
        return;
    }
    work = task->work;
    record = (struct small_cactus_record *)task->motion;
    if (placement_editing(task)) {
        small_cactus_draw(task);
        return;
    }
    switch (record->mode) {
    case SMALL_CACTUS_STAND:
        small_cactus_draw(task);
        collision = work->collision;
        /* @bug memory: a cactus the set editor held as it started has no
         * collision record. once the editor lets it go, this reads the
         * record's flags through null (MOV.W @(2,R4),R0 at 0x0c90453a) */
        if (!(collision->flags & COLLISION_HIT)) {
            collision_register(work);
            break;
        }
        collision->flags &= ~COLLISION_HIT;
        pos = &work->pos;
        burst.x = pos->x;
        burst.y = pos->y + 10.0F;
        burst.z = pos->z;
        bomb_create(&burst, 1.0F);
        i = 0;
        do {
            task_create_desc_child(task, &small_cactus_piece_desc, NULL);
        } while (++i < PIECES);
        sound_play_near(SE_SMALL_CACTUS_BURST, &work->pos, NULL);
        record->mode = SMALL_CACTUS_BURST;
        break;
    case SMALL_CACTUS_BURST:
        if (task->child == NULL) {
            placement_kill(task);
        } else {
            task_step_children(task);
        }
        break;
    }
}

/* draw the cactus on a frame's first step */
static void small_cactus_draw(struct task *task)
{
    struct task_work *work;
    struct small_cactus_record *record;

    if (frame_step != 0) {
        return;
    }
    work = task->work;
    record = (struct small_cactus_record *)task->motion;
    object_draw_tree(record->object, work, NULL);
}

/* start a piece at the edge of player 0's first collision sphere, along
 * his speed. it flies at 1.5 times his speed, plus PIECE_LIFT upward and a
 * random push of up to 0.5. it starts at a random angle. on each axis, it
 * spins up to 182 a step, about a degree */
static void small_cactus_piece_init(struct task *task, void *arg)
{
    struct task_work *work = task->work;
    struct small_cactus_piece *piece =
        (struct small_cactus_piece *)task->motion;
    NJS_VECTOR way;
    NJS_VECTOR scatter;
    NJS_VECTOR *speed;
    struct collision_shape *ball;
    NJS_POINT3 *center;
    NJS_POINT3 *pos;
    struct angles *ang;
    struct angles *spin;
    NJS_VECTOR *piece_speed;
    Float radius;
    Float strength;

    piece->mode = PIECE_FLY;
    piece->life = PIECE_LIFE;
    piece->alpha = 1.0F;
    speed = &player_motion[0]->speed;
    ball = player_work[0]->collision->shapes;
    radius = ball->radius;
    center = &ball->center;
    way = *speed;
    njUnitVector(&way);
    pos = &work->pos;
    pos->x = center->x + (radius * way.x);
    pos->y = center->y + (radius * way.y);
    pos->z = center->z + (radius * way.z);
    ang = &work->ang;
    ang->x = (Angle)RANDOM_ANGLE();
    ang->y = (Angle)RANDOM_ANGLE();
    ang->z = (Angle)RANDOM_ANGLE();
    scatter.x = rand() * RAND_SCALE;
    scatter.y = rand() * RAND_SCALE;
    scatter.z = rand() * RAND_SCALE;
    njUnitVector(&scatter);
    strength = (rand() * RAND_SCALE) - 0.5F;
    piece_speed = &piece->speed;
    piece_speed->x = (speed->x * 1.5F) + (scatter.x * strength);
    piece_speed->y = (speed->y * 1.5F) + (scatter.y * strength) + PIECE_LIFT;
    piece_speed->z = (speed->z * 1.5F) + (scatter.z * strength);
    spin = &piece->spin;
    spin->x = (Angle)(rand() * (182.0F * RAND_SCALE));
    spin->y = (Angle)(rand() * (182.0F * RAND_SCALE));
    spin->z = (Angle)(rand() * (182.0F * RAND_SCALE));
}

/* draw the piece, move and spin it, and add PIECE_GRAVITY to its speed.y.
 * once its life runs out, it fades. once its alpha reaches 0, it destroys
 * itself */
static void small_cactus_piece_step(struct task *task)
{
    struct task_work *work;
    struct small_cactus_piece *piece;
    NJS_VECTOR *speed;
    NJS_POINT3 *pos;
    struct angles *spin;
    struct angles *ang;

    small_cactus_piece_draw(task);
    work = task->work;
    piece = (struct small_cactus_piece *)task->motion;
    speed = &piece->speed;
    spin = &piece->spin;
    pos = &work->pos;
    pos->x += speed->x;
    pos->y += speed->y;
    pos->z += speed->z;
    ang = &work->ang;
    ang->x += spin->x;
    ang->y += spin->y;
    ang->z += spin->z;
    speed->y += PIECE_GRAVITY;
    switch (piece->mode) {
    case PIECE_FLY:
        if (piece->life > 0) {
            --piece->life;
        } else {
            piece->mode = PIECE_FADE;
        }
        break;
    case PIECE_FADE:
        if (piece->alpha > 0.0F) {
            piece->alpha += PIECE_FADE_OUT;
        } else {
            piece->alpha = 0.0F;
            piece->mode = PIECE_GONE;
        }
        break;
    case PIECE_GONE:
    default:
        task_kill(task);
        break;
    }
}

/* on a frame's first step, draw the piece at its alpha, without fog */
static void small_cactus_piece_draw(struct task *task)
{
    struct task_work *work;
    struct small_cactus_piece *piece;

    if (frame_step != 0) {
        return;
    }
    work = task->work;
    piece = (struct small_cactus_piece *)task->motion;
    draw_state_save();
    draw_translucent(piece->alpha);
    njFogDisable();
    object_draw_tree(&small_cactus_piece_object, work, NULL);
    njFogEnable();
    draw_state_restore();
}
