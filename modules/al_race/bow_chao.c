/* BOW CHAO, a Chao in the race hall that bows as the hall's door starts to
 * open from shut, and as something touches it */
#include <ninja.h>
#include <sg_xpt.h>

#include "al_race/bow_chao.h"
#include "al_race/door.h"
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

/* the Chao's model and its bow */
extern NJS_ACTION dat_0cbd2ee4;

/* the Chao's state, in its task's motion */
struct bow_chao {
    int mode;
    Float frame;
    Float last; /* the action's last frame */
    Float speed; /* the frames the bow plays a step */
    NJS_ACTION *action;
};

#define BOW_CHAO(task) ((struct bow_chao *)(task)->motion)

/* bow_chao_start's arg */
struct bow_chao_start {
    Float speed;
    NJS_ACTION *action;
};

/* the Chao's modes. BOW_WAIT waits for the door or a touch. BOW_BOWING
 * plays the bow once, from frame 0 to its last, then waits again */
enum bow_chao_mode { BOW_WAIT, BOW_BOWING };

#define BOW_CHAO_SCALE 2.0F

static void bow_chao_start(struct task *task, void *arg);
static void bow_chao_step(struct task *task);
static void bow_chao_draw(struct task *task);

static struct collision_shape bow_chao_shapes[1];

static const struct task_desc bow_chao_desc = {
    sizeof(struct bow_chao), TASK_WORK,     bow_chao_start,
    bow_chao_step,           bow_chao_draw, NULL};
static const struct bow_chao_start bow_chao_kind = {0.22F, &dat_0cbd2ee4};
static const struct collision_list bow_chao_list = {bow_chao_shapes, 1, 40.0F};
static const struct collision_kind bow_chao_collision = {COLLISION_OBJECT, 1};

static struct collision_shape bow_chao_shapes[1] = {
    {SHAPE_SPHERE | SHAPE_IGNORE_OBJECTS | SHAPE_IGNORE_OTHERS,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 4.0763698F, 0.0F},
     4.0763698F,
     0.0F,
     0.0F,
     {0, 0, 0},
     0,
     NULL},
};

void bow_chao_init(struct task *task)
{
    task_apply_desc(task, &bow_chao_desc, (void *)&bow_chao_kind);
}

static void bow_chao_start(struct task *task, void *arg)
{
    struct bow_chao *chao;
    NJS_ACTION *action;

    if (!placement_editing(task)) {
        object_collision_init(task, (struct collision_list *)&bow_chao_list,
                              (struct collision_kind *)&bow_chao_collision);
    }
    chao = BOW_CHAO(task);
    action = ((struct bow_chao_start *)arg)->action;
    chao->mode = BOW_WAIT;
    chao->frame = 0.0F;
    chao->last = action->motion->nbFrame - 1.0F;
    chao->speed = ((struct bow_chao_start *)arg)->speed;
    chao->action = action;
}

static void bow_chao_step(struct task *task)
{
    struct bow_chao *chao;
    struct task_work *work;

    if (!placement_try_leave(task)) {
        work = task->work;
        chao = BOW_CHAO(task);
        if (!placement_editing(task)) {
            switch (chao->mode) {
            case BOW_WAIT:
                if (door_opening() ||
                    (work->collision->flags & COLLISION_HIT)) {
                    chao->mode = BOW_BOWING;
                }
                break;
            case BOW_BOWING:
                chao->frame += chao->speed;
                if (chao->frame >= chao->last) {
                    chao->frame = 0.0F;
                    chao->mode = BOW_WAIT;
                }
                break;
            }
            collision_register(work);
        }
        bow_chao_draw(task);
    }
}

static void bow_chao_draw(struct task *task)
{
    if (frame_step == 0) {
        struct task_work *work = task->work;
        struct bow_chao *chao = BOW_CHAO(task);

        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        {
            struct angles *ang = &work->ang;
            Angle turn;

            TURN_VIA(njRotateY, ang->y, turn);
        }
        {
            /* @hack spelling: scale, BOW_CHAO_SCALE in a local, which puts
             * ang at @(4,R15) and turn at @R15 (0x0cb94e4a, 0x0cb94e52), as
             * the image does. with BOW_CHAO_SCALE passed to njScale, the two
             * swap and 3 of 60 words differ */
            Float scale = BOW_CHAO_SCALE;

            njScale(NULL, scale, scale, scale);
        }
        texture_set_stage();
        draw_action(chao->action, chao->frame);
        njPopMatrix(1);
    }
}
