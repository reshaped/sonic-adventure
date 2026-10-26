/* BUGLE, a bugle the deal gives a lane's racer, whose Chao takes it and
 * blows it */
#include <ninja.h>
#include <sg_xpt.h>

#include "al_main/models.h"
#include "al_race/bugle.h"
#include "al_race/deal.h"
#include "al_race/game.h"
#include "al_race/item.h"
#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/model.h"
#include "object/placement.h"
#include "object/simple.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/task_desc.h"
#include "system/util.h"

/* the bugle's object, in AL_MAIN.PRS */
extern NJS_OBJECT dat_0ca4f790;

/* the bugle's turn each step as it is blown, ten degrees */
#define BUGLE_SPIN 1820

static void bugle_start(struct task *task, void *arg);
static void bugle_step(struct task *task);
static void bugle_draw(struct task *task);
static void bugle_destroy(struct task *task);
static void bugle_set_racer(struct task *task, int racer);

static struct collision_shape bugle_shapes[1];

static const struct task_desc bugle_desc = {sizeof(struct item), TASK_WORK,
                                            bugle_start,         bugle_step,
                                            bugle_draw,          bugle_destroy};
static const struct item_start bugle_kind = {
    BUGLE_KIND, &dat_0ca4f790, {bugle_shapes, 1, 40.0F}};
static const struct collision_kind bugle_collision = {COLLISION_OBJECT, 1};

static struct collision_shape bugle_shapes[1] = {
    {SHAPE_SPHERE | SHAPE_IGNORE_PLAYERS | SHAPE_IGNORE_OBJECTS |
         SHAPE_IGNORE_OTHERS,
     0,
     PUSH_FIRM(PUSH_ALL),
     {0.0F, 0.0F, 0.0F},
     3.0F,
     0.0F,
     0.0F,
     {0, 0, 0},
     0,
     NULL},
};

void bugle_init(struct task *task)
{
    task_apply_desc(task, &bugle_desc, (void *)&bugle_kind);
}

int bugle_tell(struct task *task, struct task *other, Sint8 command)
{
    struct item *item = ITEM(task);
    int heard = 0;

    if (item->owner == other) {
        switch (command) {
        case ITEM_LET_GO:
            item->owner = NULL;
            heard = 1;
            break;
        case BUGLE_BLOW:
            if (item->mode == ITEM_HELD) {
                item->mode = ITEM_IN_USE;
                heard = 1;
            }
            break;
        case BUGLE_DONE:
            if (item->mode == ITEM_IN_USE) {
                item->owner = NULL;
                item->mode = ITEM_USED;
                heard = 1;
            }
            break;
        }
    }
    return heard;
}

static void bugle_start(struct task *task, void *arg)
{
    struct item_start *start = arg;
    struct item *item;
    struct collision_list *list = &start->list;

    object_collision_init(task, list,
                          (struct collision_kind *)&bugle_collision);
    item = ITEM(task);
    item->mode = ITEM_WAIT;
    item->kind = start->kind;
    item->racer = -1;
    item->model = start->object->model;
    item->list = list;
    item->owner = NULL;
}

/* turn a blown bugle */
/* @hack spelling: the turn in a helper, which SHC inlines into bugle_step, so
 * the image holds no code of its own for it. the expansion reads task->work
 * again through R4 (MOV R13,R4 and MOV.L @(32,R4),R4 at 0x0cb95026); written
 * in place, 25 of bugle_step's 87 words differ */
static void bugle_spin(struct task *task)
{
    struct task_work *work = task->work;

    work->ang.y += BUGLE_SPIN;
}

static void bugle_step(struct task *task)
{
    struct item *item;
    struct task_work *work;
    struct task *chao_task;
    Bool same_lane;

    bugle_draw(task);
    work = task->work;
    item = ITEM(task);
    if (!placement_editing(task)) {
        switch (item->mode) {
        case ITEM_WAIT:
            if (deal_add(task, bugle_set_racer, item->kind)) {
                item->mode = ITEM_LYING;
            }
            break;
        case ITEM_LYING:
            chao_task = item_hit_chao(work->collision);
            /* @hack redundant: same_lane, which nothing reads, set in a test
             * that opens with TRUE &&. SHC stores the comparison's result in
             * same_lane, at @R15 (0x0cb94ffa), as the image does. as a plain
             * test of the lane, 72 of 87 words differ */
            if (TRUE &&
                (same_lane = race_lane_of_task(chao_task) == item->racer)) {
                if (item_tell_chao(chao_task, task, ITEM_TAKE, item->kind)) {
                    item->owner = chao_task;
                    item->mode = ITEM_HELD;
                }
            }
            collision_register(work);
            break;
        case ITEM_HELD:
            break;
        case ITEM_IN_USE:
            bugle_spin(task);
            break;
        case ITEM_USED:
            break;
        }
    }
}

static void bugle_draw(struct task *task)
{
    struct item *item;
    struct task_work *work;
    struct angles *ang;
    Angle turn;

    if (frame_step == 0 && !placement_far_from_camera(task)) {
        work = task->work;
        item = ITEM(task);
        ang = &work->ang;
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateZ, ang->z, turn);
        TURN_VIA(njRotateX, ang->x, turn);
        TURN_VIA(njRotateY, ang->y, turn);
        njSetTexture(&alife_chao_texlist);
        draw_model(item->model);
        njPopMatrix(1);
    }
}

static void bugle_destroy(struct task *task)
{
    struct item *item = ITEM(task);

    item_tell_chao(item->owner, task, ITEM_LET_GO, item->kind);
}

static void bugle_set_racer(struct task *task, int racer)
{
    struct item *item = ITEM(task);

    item->racer = racer;
}
