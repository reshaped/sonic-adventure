/* WATERING CAN, a can that a racing Chao picks up and waters with */
#include <ninja.h>
#include <sg_xpt.h>

#include "al_main/models.h"
#include "al_race/deal.h"
#include "al_race/game.h"
#include "al_race/item.h"
#include "al_race/watering_can.h"
#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/model.h"
#include "object/placement.h"
#include "object/simple.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/task_desc.h"
#include "system/util.h"

/* the can's object, in AL_MAIN.PRS */
extern NJS_OBJECT dat_0ca4ec58;

/* the can's turn each step it waters */
#define WATERING_CAN_SPIN NJM_DEG_ANG(10)

static void watering_can_start(struct task *task, void *arg);
static void watering_can_step(struct task *task);
static void watering_can_draw(struct task *task);
static void watering_can_destroy(struct task *task);
static void watering_can_set_racer(struct task *task, int racer);

static struct collision_shape watering_can_shapes[1];

static const struct task_desc watering_can_desc = {
    sizeof(struct item), TASK_WORK,         watering_can_start,
    watering_can_step,   watering_can_draw, watering_can_destroy};
static const struct item_start watering_can_kind = {
    WATERING_CAN_KIND, &dat_0ca4ec58, {watering_can_shapes, 1, 40.0F}};
static const struct collision_kind watering_can_collision = {COLLISION_OBJECT,
                                                             1};

static struct collision_shape watering_can_shapes[1] = {
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

void watering_can_init(struct task *task)
{
    task_apply_desc(task, &watering_can_desc, (void *)&watering_can_kind);
}

int watering_can_tell(struct task *task, struct task *other, Sint8 command)
{
    struct item *item = ITEM(task);
    int heard = 0;

    if (item->owner == other) {
        switch (command) {
        case ITEM_LET_GO:
            item->owner = NULL;
            heard = 1;
            break;
        case WATERING_CAN_WATER:
            if (item->mode == ITEM_HELD) {
                item->mode = ITEM_IN_USE;
                heard = 1;
            }
            break;
        case WATERING_CAN_DONE:
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

static void watering_can_start(struct task *task, void *arg)
{
    struct item_start *start = arg;
    struct item *item;
    struct collision_list *list = &start->list;

    object_collision_init(task, list,
                          (struct collision_kind *)&watering_can_collision);
    item = ITEM(task);
    item->mode = ITEM_WAIT;
    item->kind = start->kind;
    item->racer = -1;
    item->model = start->object->model;
    item->list = list;
    item->owner = NULL;
}

/* @hack spelling: the turn in a helper, which SHC expands at its one call, in
 * watering_can_step, so the image holds no watering_can_spin of its own. the
 * expansion reads task->work again through R4 (MOV R13,R4 and MOV.L
 * @(32,R4),R4 at 0x0cb996a6); written in place, 25 of watering_can_step's 87
 * words differ */
static void watering_can_spin(struct task *task)
{
    struct task_work *work = task->work;

    work->ang.y += WATERING_CAN_SPIN;
}

static void watering_can_step(struct task *task)
{
    struct item *item;
    struct task_work *work;
    struct task *chao_task;
    Bool mine;

    watering_can_draw(task);
    work = task->work;
    item = ITEM(task);
    if (!placement_editing(task)) {
        switch (item->mode) {
        case ITEM_WAIT:
            if (deal_add(task, watering_can_set_racer, item->kind)) {
                item->mode = ITEM_LYING;
            }
            break;
        case ITEM_LYING:
            chao_task = item_hit_chao(work->collision);
            /* @hack redundant: TRUE && ahead of the lane test, which with mine
             * keeps the test's result in the frame at @R15 (MOV.L R0,@R15 at
             * 0x0cb9967a); as a plain test, 72 of 87 words differ */
            /* @hack redundant: mine, set by the lane test and never read,
             * which with TRUE && stores the result at @R15; as a plain test,
             * 72 of 87 words differ */
            if (TRUE && (mine = race_lane_of_task(chao_task) == item->racer)) {
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
            watering_can_spin(task);
            break;
        case ITEM_USED:
            break;
        }
    }
}

static void watering_can_draw(struct task *task)
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

static void watering_can_destroy(struct task *task)
{
    struct item *item = ITEM(task);

    item_tell_chao(item->owner, task, ITEM_LET_GO, item->kind);
}

static void watering_can_set_racer(struct task *task, int racer)
{
    struct item *item = ITEM(task);

    item->racer = racer;
}
