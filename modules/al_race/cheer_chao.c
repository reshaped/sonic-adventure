/* CHEER CHAO, a Chao on the race course that cheers over and over */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "al_race/cheer_chao.h"
#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "object/placement.h"
#include "object/simple.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/task_desc.h"
#include "system/texture.h"
#include "system/util.h"

/* the Chao's model and its cheer */
extern NJS_ACTION dat_0cbd2780;

/* the Chao's state, in its task's motion */
struct cheer_chao {
    Float frame;
    Float last; /* the action's last frame */
    Float speed; /* the frames the cheer plays a step */
    NJS_ACTION *action;
};

#define CHEER_CHAO(task) ((struct cheer_chao *)(task)->motion)

/* cheer_chao_start's arg */
struct cheer_chao_start {
    Float speed;
    NJS_ACTION *action;
};

#define CHEER_CHAO_SCALE 1.3F

static void cheer_chao_start(struct task *task, void *arg);
static void cheer_chao_step(struct task *task);
static void cheer_chao_draw(struct task *task);

static struct collision_shape cheer_chao_shapes[1];

static const struct task_desc cheer_chao_desc = {
    sizeof(struct cheer_chao), TASK_WORK,       cheer_chao_start,
    cheer_chao_step,           cheer_chao_draw, NULL};
static const struct cheer_chao_start cheer_chao_kind = {0.5F, &dat_0cbd2780};
static const struct collision_list cheer_chao_list = {cheer_chao_shapes, 1,
                                                      40.0F};
static const struct collision_kind cheer_chao_collision = {COLLISION_OBJECT, 1};

static struct collision_shape cheer_chao_shapes[1] = {
    {SHAPE_SPHERE | SHAPE_IGNORE_OBJECTS | SHAPE_IGNORE_OTHERS,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 2.6496401F, 0.0F},
     2.6496401F,
     0.0F,
     0.0F,
     {0, 0, 0},
     0,
     NULL},
};

void cheer_chao_init(struct task *task)
{
    task_apply_desc(task, &cheer_chao_desc, (void *)&cheer_chao_kind);
}

static void cheer_chao_start(struct task *task, void *arg)
{
    NJS_ACTION *action;
    Float last;
    struct cheer_chao *chao;

    if (!placement_editing(task)) {
        object_collision_init(task, (struct collision_list *)&cheer_chao_list,
                              (struct collision_kind *)&cheer_chao_collision);
    }
    action = ((struct cheer_chao_start *)arg)->action;
    last = action->motion->nbFrame - 1.0F;
    chao = CHEER_CHAO(task);
    chao->frame = rand() * last * RAND_SCALE;
    chao->last = last;
    chao->speed = ((struct cheer_chao_start *)arg)->speed;
    chao->action = action;
}

static void cheer_chao_step(struct task *task)
{
    struct task_work *work;
    struct cheer_chao *chao;

    if (placement_far_from_camera(task)) {
        task_kill(task);
        return;
    }
    work = task->work;
    chao = CHEER_CHAO(task);
    chao->frame += chao->speed;
    if (chao->frame >= chao->last) {
        chao->frame -= chao->last;
    }
    if (!placement_editing(task)) {
        collision_register(work);
    }
    cheer_chao_draw(task);
}

static void cheer_chao_draw(struct task *task)
{
    if (frame_step == 0) {
        struct task_work *work = task->work;
        struct cheer_chao *chao = CHEER_CHAO(task);

        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        {
            struct angles *ang = &work->ang;
            Angle turn;

            TURN_VIA(njRotateY, ang->y, turn);
        }
        {
            /* @hack spelling: scale, CHEER_CHAO_SCALE in a local, which puts
             * ang at @(4,R15) and turn at @R15 (0x0cb95c9a, 0x0cb95ca2), as
             * the image does. with CHEER_CHAO_SCALE passed to njScale, the
             * two swap and 3 of 59 words differ */
            Float scale = CHEER_CHAO_SCALE;

            njScale(NULL, scale, scale, scale);
        }
        texture_set_stage();
        draw_action(chao->action, chao->frame);
        njPopMatrix(1);
    }
}
