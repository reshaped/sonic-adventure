/* O S2SAKU fence of the second shop, which comes down while the egg stand is
 * empty */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv00/egg_stand.h"
#include "adv00/shop2_fence.h"
#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/tree.h"
#include "object/common.h"
#include "object/placement.h"
#include "progress/progress.h"
#include "sound/sound.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

/* the top part, with the middle part as its child and the bottom part as the
 * child's sibling */
extern NJS_OBJECT shop2_fence_object;

enum shop2_fence_mode {
    FENCE_INIT,
    FENCE_UP,
    FENCE_BOTTOM_DOWN,
    FENCE_MIDDLE_DOWN,
    FENCE_TOP_DOWN,
    FENCE_DOWN,
    FENCE_BOTTOM_UP,
    FENCE_MIDDLE_UP,
    FENCE_TOP_UP
};

/* each moving part's travel at each step */
#define FENCE_SPEED 0.5F

/* sounds of the parts coming down and going up, and of the top part landing */
#define SE_FENCE_DOWN 0x217
#define SE_FENCE_UP 0x218
#define SE_FENCE_LANDED 0x216

/* boxes of the top, middle and bottom parts, all at the top while the fence is
 * up */
/* clang-format off */
static struct collision_shape shop2_fence_shapes[] = {
    {SHAPE_RECTANGLE, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL,
     {-0.1875F, 35.25F, 0.0F}, 0.9375F, 5.25F, 18.25F, {0}, 0, NULL},
    {SHAPE_RECTANGLE, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL,
     {-0.1875F, 35.25F, 0.0F}, 0.9375F, 5.25F, 18.25F, {0}, 0, NULL},
    {SHAPE_RECTANGLE, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL,
     {-0.1875F, 35.25F, 0.0F}, 0.9375F, 5.25F, 18.25F, {0}, 0, NULL},
};
/* clang-format on */

static void shop2_fence_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    texture_set_stage();
    draw_tree(MODEL(work)->object);
    njPopMatrix(1);
}

static void shop2_fence_destroy(struct task *task)
{
    task_kill(task);
}

/* in the set editor, draw the fence as it is. otherwise the parts come down
 * one after another, bottom first, while the stand is empty. they go back up
 * in the same order while an egg is on it. the fence registers no collision
 * while it is up */
void shop2_fence_step(struct task *task)
{
    struct task_work *work = task->work;
    struct collision_work *collision = work->collision;
    Bool egg;

    if (placement_try_leave(task)) {
        return;
    }
    if (edit_mode) {
        if (!MODEL(work)->object) {
            MODEL(work)->object = &shop2_fence_object;
        }
        shop2_fence_draw(task);
    } else {
        switch (work->mode) {
        case FENCE_INIT:
            work->mode = FENCE_UP;
            if (!MODEL(work)->object) {
                MODEL(work)->object = &shop2_fence_object;
            }
            collision_init(task, shop2_fence_shapes, COLLISION_OBJECT, 1, 3);
            task->destructor = shop2_fence_destroy;
            task->draw = shop2_fence_draw;
            MODEL(work)->object->pos[1] = 30.0F;
            MODEL(work)->object->child->pos[1] = 0.0F;
            MODEL(work)->object->child->sibling->pos[1] = 0.0F;
            break;
        case FENCE_UP:
            /* @hack spelling: the flag read inside the test, which loads it
             * into R5 (MOV.B @R4,R5 at 0x0c904132). read before the test, it
             * takes R4 and 3 of 491 words differ */
            /* @hack redundant: the empty else, which has this case branch to
             * FENCE_TOP_DOWN's tail call of the draw (BRA 0x0c904320 at
             * 0x0c90413a). without it, the case branches to FENCE_DOWN's at
             * 0x0c904372, and 12 of 491 words differ */
            if (egg = event_flags[FLAG_EGG_ON_STAND], !egg) {
                work->mode = FENCE_BOTTOM_DOWN;
            } else {
            }
            shop2_fence_draw(task);
            break;
        case FENCE_BOTTOM_DOWN:
            if ((MODEL(work)->object->child->sibling->pos[1] -= FENCE_SPEED) <=
                -10.0F) {
                work->mode = FENCE_MIDDLE_DOWN;
            }
            collision->shapes[2].center.y -= FENCE_SPEED;
            egg = event_flags[FLAG_EGG_ON_STAND];
            if (egg) {
                work->mode = FENCE_TOP_UP;
            }
            collision_register(work);
            shop2_fence_draw(task);
            sound_play_for(SE_FENCE_DOWN, (int)task, 1, 0, 2);
            break;
            /* @hack redundant: a second break, which has FENCE_MIDDLE_DOWN load
             * the fence's object into R3 and its middle part into R1
             * (0x0c9041ec, 0x0c9041ee). without it, the two trade and 6 of
             * 491 words differ */
            break;
        case FENCE_MIDDLE_DOWN:
            if ((MODEL(work)->object->child->sibling->pos[1] -= FENCE_SPEED) <=
                -20.0F) {
                work->mode = FENCE_TOP_DOWN;
            }
            MODEL(work)->object->child->pos[1] -= FENCE_SPEED;
            collision->shapes[2].center.y -= FENCE_SPEED;
            collision->shapes[1].center.y -= FENCE_SPEED;
            if (MODEL(work)->object->child->pos[1] <= -10.0F) {
                MODEL(work)->object->child->pos[1] = -10.0F;
                collision->shapes[1].center.y = 25.25F;
                /* @hack redundant: a store of 25.25 that never runs, which
                 * keeps 25.25 in FR12 through the switch (FMOV.S @R0,FR12 at
                 * 0x0c90408e). without it, FR12 takes 30.0, and the pools
                 * move with the loads shop2_fence_draw and
                 * shop2_fence_destroy make from them; 381 of 491 words
                 * differ */
                if (0) {
                    collision->shapes[1].center.y = 25.25F;
                }
            }
            egg = event_flags[FLAG_EGG_ON_STAND];
            if (egg) {
                work->mode = FENCE_MIDDLE_UP;
            }
            collision_register(work);
            shop2_fence_draw(task);
            sound_play_for(SE_FENCE_DOWN, (int)task, 1, 0, 2);
            break;
        case FENCE_TOP_DOWN:
            if ((MODEL(work)->object->pos[1] -= FENCE_SPEED) <= 20.0F) {
                work->mode = FENCE_DOWN;
                sound_play(SE_FENCE_LANDED, 0, 0, 0);
            } else {
                sound_play_for(SE_FENCE_DOWN, (int)task, 1, 0, 2);
            }
            collision->shapes[2].center.y -= FENCE_SPEED;
            collision->shapes[1].center.y -= FENCE_SPEED;
            collision->shapes[0].center.y -= FENCE_SPEED;
            if (MODEL(work)->object->pos[1] <= 20.0F) {
                MODEL(work)->object->pos[1] = 20.0F;
                MODEL(work)->object->child->pos[1] = -10.0F;
                MODEL(work)->object->child->sibling->pos[1] = -20.0F;
                collision->shapes[0].center.y = 25.25F;
                collision->shapes[1].center.y = 15.25F;
                collision->shapes[2].center.y = 5.25F;
            }
            egg = event_flags[FLAG_EGG_ON_STAND];
            /* @hack spelling: the empty branch for an empty stand. with it, the
             * set editor's draw and FENCE_UP's each jump to this case's tail
             * call of the draw (BRA 0x0c904320 at 0x0c90407e and
             * 0x0c90413a). with the test if (egg) and no empty branch, they
             * jump to FENCE_DOWN's at 0x0c904372, and 18 of 491 words
             * differ */
            if (!egg) {
            } else {
                work->mode = FENCE_BOTTOM_UP;
            }
            collision_register(work);
            shop2_fence_draw(task);
            break;
            /* @hack redundant: a second break, which gives this case a tail
             * call of the draw of its own, at 0x0c904320, where the set
             * editor's draw and FENCE_UP's branch too. without it, all three
             * branch to FENCE_DOWN's at 0x0c904372, and 30 of 491 words differ
             */
            break;
            /* @hack redundant: a return after the breaks, which has FENCE_DOWN
             * store FENCE_BOTTOM_UP from R2 (MOV #6,R2 at 0x0c90436a).
             * without it, R1 takes it and 2 of 491 words differ */
            return;
        case FENCE_DOWN:
            /* @hack spelling: the flag read inside the test, which loads it
             * into R5 (MOV.B @R4,R5 at 0x0c904360). read before the test, it
             * takes R4 and 3 of 491 words differ */
            if (egg = event_flags[FLAG_EGG_ON_STAND], egg) {
                work->mode = FENCE_BOTTOM_UP;
            }
            collision_register(work);
            shop2_fence_draw(task);
            break;
        case FENCE_BOTTOM_UP:
            if ((MODEL(work)->object->child->sibling->pos[1] += FENCE_SPEED) >=
                -10.0F) {
                work->mode = FENCE_MIDDLE_UP;
            }
            collision->shapes[2].center.y += FENCE_SPEED;
            collision_register(work);
            shop2_fence_draw(task);
            egg = event_flags[FLAG_EGG_ON_STAND];
            /* @hack spelling: the empty branch for an egg on the stand, which
             * stores FENCE_TOP_DOWN from R3 (MOV #4,R3 at 0x0c9043be). as
             * if (!egg), R2 takes it and 9 of 491 words differ */
            if (egg) {
            } else {
                work->mode = FENCE_TOP_DOWN;
            }
            sound_play_for(SE_FENCE_UP, (int)task, 1, 0, 2);
            break;
        case FENCE_MIDDLE_UP:
            if ((MODEL(work)->object->child->sibling->pos[1] += FENCE_SPEED) >=
                0.0F) {
                work->mode = FENCE_TOP_UP;
            }
            MODEL(work)->object->child->pos[1] += FENCE_SPEED;
            collision->shapes[2].center.y += FENCE_SPEED;
            collision->shapes[1].center.y += FENCE_SPEED;
            if (MODEL(work)->object->child->pos[1] >= 0.0F) {
                MODEL(work)->object->child->pos[1] = 0.0F;
                collision->shapes[1].center.y = 25.25F;
            }
            egg = event_flags[FLAG_EGG_ON_STAND];
            if (!egg) {
                work->mode = FENCE_MIDDLE_DOWN;
            }
            collision_register(work);
            shop2_fence_draw(task);
            sound_play_for(SE_FENCE_UP, (int)task, 1, 0, 2);
            break;
            /* @hack redundant: a second break, which has FENCE_TOP_UP load the
             * fence's object into R3 (MOV.L @(8,R14),R3 at 0x0c9044a0).
             * without it, R1 takes it and 3 of 491 words differ */
            break;
        case FENCE_TOP_UP:
            if ((MODEL(work)->object->pos[1] += FENCE_SPEED) >= 30.0F) {
                work->mode = FENCE_UP;
            }
            collision->shapes[2].center.y += FENCE_SPEED;
            collision->shapes[1].center.y += FENCE_SPEED;
            collision->shapes[0].center.y += FENCE_SPEED;
            if (MODEL(work)->object->pos[1] >= 30.0F) {
                MODEL(work)->object->pos[1] = 30.0F;
                MODEL(work)->object->child->pos[1] = 0.0F;
                MODEL(work)->object->child->sibling->pos[1] = 0.0F;
                collision->shapes[0].center.y = 35.25F;
                collision->shapes[1].center.y = 35.25F;
                collision->shapes[2].center.y = 35.25F;
            }
            egg = event_flags[FLAG_EGG_ON_STAND];
            if (!egg) {
                work->mode = FENCE_BOTTOM_DOWN;
            }
            collision_register(work);
            shop2_fence_draw(task);
            sound_play_for(SE_FENCE_UP, (int)task, 1, 0, 2);
            break;
        }
    }
}
