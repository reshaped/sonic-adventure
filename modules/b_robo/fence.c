/* the fence round the deck: eight posts and the chains of light between
 * them. boxes along the fence push and hurt */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "b_robo/banks.h"
#include "b_robo/effects.h"
#include "b_robo/erobo.h"
#include "b_robo/fence.h"
#include "collision/collision.h"
#include "draw/tree.h"
#include "system/frame.h"
#include "system/math.h"
#include "system/task.h"
#include "system/util.h"

#define FENCE_POSTS 8
/* the chains from each post to the next */
#define CHAINS 3

/* a post's modes. POST_GROW and POST_FULL count down the wait
 * erobo_fence_stop sets. when it ends, the post moves to POST_SHRINK */
enum fence_post_mode {
    POST_START,
    POST_FALL, /* the post falls, and bounces to a stop on the deck */
    /* the post's chains widen 1 / GROW_STEPS of their full width a step.
     * they start once the next post is down too */
    POST_GROW,
    POST_FULL,
    /* the post's chains narrow 1 / SHRINK_STEPS of their full width a
     * step. lightning crackles about the post */
    POST_SHRINK,
    /* for SMOKE_STEPS, lightning crackles about the post, and puffs of
     * smoke come ever closer together */
    POST_SMOKE,
    POST_GONE /* fence_draw skips the post */
};

struct fence_post {
    NJS_POINT3 pos;
    Float fall_speed;
    Angle ang_y;
    /* where its chains meet it, set as it lands. the first CHAINS are where
     * its chains to the next post leave. the rest are where the previous
     * post's arrive */
    NJS_POINT3 ends[2 * CHAINS];
    /* erobo_chain_draw's frame for its first chain, from a random start.
     * it goes up by 1 on each frame's first step */
    int frame;
    /* its chains' width, as erobo_chain_draw's scale, from 0 to 1 */
    Float width;
    enum fence_post_mode mode;
    /* the steps left of its wait before POST_SHRINK, or of POST_SMOKE */
    int timer;
};

/* the fence's record, at its task's extra */
struct fence {
    struct fence_post posts[FENCE_POSTS];
    Bool active; /* fence_step registers the shapes while it is TRUE */
};

/* the radius of the ring the posts stand on, round the deck's middle. a
 * post faces a quarter turn, FACE_ALONG, on from its angle round the ring */
#define RADIUS 190.0F
#define FACE_ALONG 0x4000
/* the height a post's pos comes to rest at, 9.406 below DECK_FLOOR_Y */
#define LAND_Y 742.684F
/* a post starts at a random height, from START_Y up to START_SPREAD
 * higher */
#define START_Y (DECK_FLOOR_Y + 250.0F)
#define START_SPREAD 100.0F

/* a falling post's speed changes by GRAVITY each step, down to FALL_MOST.
 * a landing at BOUNCE_LEAST or faster bounces it back up, at BOUNCE times
 * its speed */
#define GRAVITY (-0.1F)
#define FALL_MOST (-3.0F)
#define BOUNCE_LEAST (-0.2F)
#define BOUNCE (-0.5F)

/* erobo_fence_stop gives each post a wait from WAIT_LEAST up to WAIT_MOST.
 * its int timer truncates the wait to 1 to 29 steps */
#define WAIT_LEAST 1.0F
#define WAIT_MOST 30.0F
/* a post's chains widen over GROW_STEPS and narrow over SHRINK_STEPS. its
 * smoke lasts SMOKE_STEPS */
#define GROW_STEPS 45.0F
#define SHRINK_STEPS 90.0F
#define SMOKE_STEPS 240
/* a post's lightning and puffs are made near the point this far above its
 * pos */
#define CRACKLE_HEIGHT 15.0F
/* a post's lightning strikes within LIGHTNING_RANGE of that point in x and
 * z. its puffs come within PUFF_RANGE, at a scale from PUFF_LEAST up to
 * PUFF_MOST */
#define LIGHTNING_RANGE 4.0F
#define PUFF_RANGE 6.0F
#define PUFF_LEAST 0.5F
#define PUFF_MOST 0.8F
/* in POST_SMOKE, a puff comes when timer is a multiple of timer /
 * PUFF_TAPER + PUFF_GAP. the puffs come about every 13 steps at first, and
 * every 2 at the end */
#define PUFF_TAPER 20
#define PUFF_GAP 2

/* the post's object, as an offset into the model bank */
#define POST_OBJECT 0x123d8
/* a post is drawn this many times its model's height */
#define POST_STRETCH 1.5F

/* where a post's chains meet it, as offsets from its pos before its turn.
 * the first CHAINS are where its chains to the next post leave. the rest
 * are where the previous post's arrive */
static const NJS_POINT3 chain_ends[2 * CHAINS] = {
    {4.94F, 20.399998F, 0.28F}, {4.94F, 15.3F, 0.28F},
    {4.94F, 9.975F, 0.28F},     {-4.94F, 20.399998F, 0.28F},
    {-4.94F, 15.3F, 0.28F},     {-4.94F, 9.975F, 0.28F},
};

/* between each post and the next stand a lower box and an upper box,
 * which shapes_place places. both push every group. the lower box hurts
 * players, enemies and objects at attack level 2. the upper box's attack
 * of 0 exceeds no guard, so it hurts nothing */
#define LOWER_SHAPE                                                            \
    {SHAPE_RECTANGLE | SHAPE_IN_WORLD | SHAPE_HURTABLE,                        \
     ATTACK(2) | SHAPE_HURTS_PLAYERS | SHAPE_HURTS_ENEMIES |                   \
         SHAPE_HURTS_OBJECTS,                                                  \
     PUSH_ALL,                                                                 \
     {0.0F, 0.0F, 0.0F},                                                       \
     100.0F,                                                                   \
     13.0F,                                                                    \
     1.0F,                                                                     \
     {0, 0, 0},                                                                \
     0,                                                                        \
     NULL}
#define UPPER_SHAPE                                                            \
    {SHAPE_RECTANGLE | SHAPE_IN_WORLD | SHAPE_HURTABLE,                        \
     SHAPE_HURTS_PLAYERS | SHAPE_HURTS_ENEMIES | SHAPE_HURTS_OBJECTS,          \
     PUSH_ALL,                                                                 \
     {0.0F, 0.0F, 0.0F},                                                       \
     100.0F,                                                                   \
     30.0F,                                                                    \
     5.0F,                                                                     \
     {0, 0, 0},                                                                \
     0,                                                                        \
     NULL}
static struct collision_shape fence_shapes[2 * FENCE_POSTS] = {
    LOWER_SHAPE, LOWER_SHAPE, LOWER_SHAPE, LOWER_SHAPE,
    LOWER_SHAPE, LOWER_SHAPE, LOWER_SHAPE, LOWER_SHAPE,
    UPPER_SHAPE, UPPER_SHAPE, UPPER_SHAPE, UPPER_SHAPE,
    UPPER_SHAPE, UPPER_SHAPE, UPPER_SHAPE, UPPER_SHAPE,
};
/* a lower box's center lies on the deck's floor, INSET from its halfway
 * point toward the deck's middle. an upper box's center lies UPPER_RISE
 * above the floor, at the halfway point */
#define INSET 5.0F
#define UPPER_RISE 30.0F

/* the fence's collision radius, which takes in every box round the deck */
#define FENCE_REACH 1000.0F

/* the fence's task, or NULL while there is none */
static struct task *fence_task = NULL;

static void fence_drop(struct task *task);
static void posts_raise(struct task *task);
static void shapes_place(struct task *task);
static void fence_destroy(struct task *task);
static void fence_step(struct task *task);
static void fence_draw(struct task *task);

void erobo_fence_stop(void)
{
    struct fence *fence;
    struct fence_post *post;
    int i;

    if (fence_task != NULL) {
        fence = fence_task->extra;
        fence->active = FALSE;
        post = fence->posts;
        for (i = 0; i < FENCE_POSTS; i++, post++) {
            post->timer = RANDOM(WAIT_LEAST, WAIT_MOST);
        }
    }
}

void erobo_fence_start(void)
{
    if (fence_task != NULL) {
        fence_task->step = fence_drop;
    }
}

void erobo_fence_create(void)
{
    struct task *task;
    struct task_work *work;
    struct fence *fence;

    if ((task = task_create(TASK_WORK, 4, NULL)) != NULL) {
        work = task->work;
        collision_init(task, fence_shapes, COLLISION_OBJECT, 1,
                       2 * FENCE_POSTS);
        /* @hack spelling: the record through task->work, which the image loads
         * again after collision_init (MOV.L @(32,R14),R3 at 0x0c90666a);
         * through work, 46 of 65 words differ */
        task->work->collision->radius = FENCE_REACH;
        fence = heap_alloc(sizeof(struct fence));
        if (fence == NULL) {
            task_destroy(task);
            return;
        }
        work->pos.x = DECK_MIDDLE_X;
        work->pos.y = DECK_FLOOR_Y;
        work->pos.z = DECK_MIDDLE_Z;
        task->extra = fence;
        posts_raise(task);
        shapes_place(task);
        task->destructor = fence_destroy;
        task->draw = fence_draw;
    }
    fence_task = task;
}

static void fence_drop(struct task *task)
{
    struct fence *fence = task->extra;

    fence->active = TRUE;
    posts_raise(task);
    task->step = fence_step;
}

/* spread the posts evenly round the ring at random heights, each facing
 * along the ring. each starts in POST_START, with its chains at width 0 */
static void posts_raise(struct task *task)
{
    struct fence *fence = task->extra;
    struct fence_post *post = fence->posts;
    int i;
    Angle ang = 0;
    Float sine;
    Float cosine;

    for (i = 0; i < FENCE_POSTS; i++, post++, ang += 0x10000 / FENCE_POSTS) {
        _builtin_fsca(ang, &sine, &cosine);
        post->pos.x = (cosine * RADIUS) + DECK_MIDDLE_X;
        post->pos.z = (sine * RADIUS) + DECK_MIDDLE_Z;
        post->pos.y = (rand() * (START_SPREAD * RAND_SCALE)) + START_Y;
        post->fall_speed = 0.0F;
        post->ang_y = ang + FACE_ALONG;
        post->frame = rand();
        post->width = 0.0F;
        post->mode = POST_START;
        post->timer = 0;
    }
}

/* put each pair of boxes halfway between a post and the next, turned along
 * the ring. the halfway point is that of chain_ends[0] at the post and
 * chain_ends[4] at the next. neither end is turned with its post. their x
 * offsets cancel, so the point lies 0.28 further along z than the posts'
 * midpoint */
static void shapes_place(struct task *task)
{
    struct fence *fence = task->extra;
    struct collision_shape *shape = task->work->collision->shapes;
    struct fence_post *post = fence->posts;
    struct fence_post *next = post + 1;
    struct collision_shape *upper;
    int i = 0;
    NJS_POINT3 end;
    NJS_POINT3 next_end;
    NJS_POINT3 sum;
    NJS_VECTOR out;

    do {
        if (next >= fence->posts + FENCE_POSTS) {
            next = fence->posts;
        }
        njPushMatrix(&_unit_matrix_);
        njPushMatrix(NULL);
        njTranslateV(NULL, &post->pos);
        njCalcPoint(NULL, (NJS_POINT3 *)&chain_ends[0], &end);
        njPopMatrix(1);
        njTranslateV(NULL, &next->pos);
        njCalcPoint(NULL, (NJS_POINT3 *)&chain_ends[4], &next_end);
        njPopMatrix(1);
        /* @hack spelling: end, next_end, sum and out through their addresses,
         * which has SHC work out a local's stack address again for each
         * member it reads or writes (MOV R15,R1 and ADD #44,R1 at
         * 0x0c9069b2, for next_end); as plain members, 59 of 205 words
         * differ */
        (&sum)->x = (&end)->x + (&next_end)->x;
        (&sum)->z = (&end)->z + (&next_end)->z;
        shape->center.x = (&sum)->x * 0.5F;
        shape->center.z = (&sum)->z * 0.5F;
        shape->center.y = DECK_FLOOR_Y;
        shape->ang.y = -((Uint16)(post->ang_y + next->ang_y) >> 1);
        upper = shape + FENCE_POSTS;
        upper->center.x = shape->center.x;
        upper->center.y = shape->center.y;
        upper->center.z = shape->center.z;
        out.x = DECK_MIDDLE_X - shape->center.x;
        out.z = DECK_MIDDLE_Z - shape->center.z;
        vector_unit_across(&out, &out);
        (&out)->x = (&out)->x * INSET;
        (&out)->z = (&out)->z * INSET;
        shape->center.x += (&out)->x;
        /* @hack spelling: the += written out, which reads out.z through R0
         * and R3 (FMOV.S @(R0,R3),FR3 at 0x0c906a72); with +=, SHC builds
         * its address in R1, and 48 of 205 words differ */
        shape->center.z = shape->center.z + (&out)->z;
        upper->center.y += UPPER_RISE;
        upper->ang.y = shape->ang.y;
        next++;
        post++;
        i++;
        shape++;
    } while (i < FENCE_POSTS);
    /* @hack redundant: tests of next, njTranslateV and njCalcPoint that
     * never run. they keep next in R13 (0x0c906948), njTranslateV's address
     * in R9 (0x0c906952) and njCalcPoint's in R10 (0x0c906940). without
     * them, next takes R12, R9 and R10 hold njPushMatrix's and njPopMatrix's
     * addresses, and 51 of 205 words differ */
    if (0) {
        if (next) {
        }
        /* NOLINTBEGIN(clang-diagnostic-pointer-bool-conversion) */
        if (njTranslateV) {
        }
        if (njCalcPoint) {
        }
        /* NOLINTEND(clang-diagnostic-pointer-bool-conversion) */
    }
}

static void fence_destroy(struct task *task)
{
    fence_task = NULL;
}

/* set the post's ends: chain_ends, turned with the post and moved to its
 * pos */
static void post_set_ends(struct fence_post *post)
{
    int i;

    njPushMatrix(&_unit_matrix_);
    njTranslateV(NULL, &post->pos);
    TURN(njRotateY, -post->ang_y);
    for (i = 0; i < 2 * CHAINS; i++) {
        njCalcPoint(NULL, (NJS_POINT3 *)&chain_ends[i], &post->ends[i]);
    }
    njPopMatrix(1);
}

/* run the post a step. next is the post its chains run to */
static void post_step(struct fence_post *post, struct fence_post *next)
{
    NJS_POINT3 pos;

    switch (post->mode) {
    case POST_START:
        post->mode = POST_FALL;
        break;
    case POST_FALL:
        post->fall_speed += GRAVITY;
        if (post->fall_speed <= FALL_MOST) {
            post->fall_speed = FALL_MOST;
        }
        post->pos.y += post->fall_speed;
        if (post->pos.y < LAND_Y) {
            post->pos.y = LAND_Y;
            if (post->fall_speed <= BOUNCE_LEAST) {
                post->fall_speed *= BOUNCE;
            } else {
                post->fall_speed = 0.0F;
                post_set_ends(post);
                post->mode = POST_GROW;
            }
        }
        break;
    case POST_GROW:
        if (next->mode >= POST_GROW) {
            if (post->width < 1.0F) {
                post->width += 1.0F / GROW_STEPS;
                if (post->width >= 1.0F) {
                    post->width = 1.0F;
                    post->mode = POST_FULL;
                }
            }
        }
        /* fall through */
    case POST_FULL:
        if (post->timer != 0) {
            post->timer--;
            if (post->timer == 0) {
                post->mode = POST_SHRINK;
            }
        }
        break;
    case POST_SHRINK:
        post->width -= 1.0F / SHRINK_STEPS;
        if (post->width <= 0.0F) {
            post->width = 0.0F;
            post->mode = POST_SMOKE;
            post->timer = SMOKE_STEPS;
        }
        break;
    case POST_SMOKE:
        if (--post->timer <= 0) {
            post->mode = POST_GONE;
        }
        if (post->timer % ((post->timer / PUFF_TAPER) + PUFF_GAP) == 0) {
            VECTOR_COPY(&pos, &post->pos);
            pos.y += CRACKLE_HEIGHT;
            erobo_bomb_create(&pos, PUFF_RANGE, RANDOM(PUFF_LEAST, PUFF_MOST));
        }
        break;
    case POST_GONE:
        break;
    }
    switch (post->mode) {
    case POST_SHRINK:
    case POST_SMOKE:
        if (!frame_step) {
            VECTOR_COPY(&pos, &post->pos);
            pos.y += CRACKLE_HEIGHT;
            erobo_lightning_create(&pos, LIGHTNING_RANGE);
        }
        break;
    /* @hack redundant: an empty default, which keeps the BF at 0x0c906dc0
     * branching to the load of frame_step below; without it, the BF skips
     * the second test of frame_step, and 7 of 242 words differ */
    default:
        break;
    }
    if (!frame_step) {
        post->frame++;
    }
}

static void fence_step(struct task *task)
{
    struct task_work *work = task->work;
    struct fence *fence = task->extra;
    struct fence_post *post = fence->posts;
    int i;

    for (i = 1; i < FENCE_POSTS; i++, post++) {
        post_step(post, post + 1);
    }
    post_step(post, fence->posts);
    if (fence->active) {
        collision_register(work);
    }
    fence_draw(task);
}

/* draw the post's chains to the next post, one link each */
static void chains_draw(struct fence_post *post, struct fence_post *next)
{
    int i;

    for (i = 0; i < CHAINS; i++) {
        erobo_chain_draw(&post->ends[i], &next->ends[i + CHAINS],
                         post->frame + i, 1, post->width);
    }
}

static void fence_draw(struct task *task)
{
    struct fence *fence = task->extra;
    struct fence_post *post;
    struct fence_post *next;
    int i;

    if (frame_step) {
        return;
    }
    njSetTexture(erobo_textures);
    post = fence->posts;
    for (i = 0; i < FENCE_POSTS; i++, post++) {
        if (post->mode != POST_GONE) {
            njPushMatrix(NULL);
            njTranslateV(NULL, &post->pos);
            TURN(njRotateY, -post->ang_y);
            njScale(NULL, 1.0F, POST_STRETCH, 1.0F);
            draw_tree((NJS_OBJECT *)(erobo_models + POST_OBJECT));
            njPopMatrix(1);
        }
    }
    post = fence->posts;
    next = post + 1;
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
    for (i = 1; i < FENCE_POSTS; i++, post++, next++) {
        if (post->mode >= POST_GROW) {
            chains_draw(post, next);
        }
    }
    next = fence->posts;
    if (post->mode >= POST_GROW) {
        chains_draw(post, next);
    }
}
