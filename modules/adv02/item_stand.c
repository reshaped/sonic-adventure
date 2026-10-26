/* the stand the object list names O ITEM STAND. it holds a turning item, which
 * a player's touch fades away. the item is back five seconds after the touch */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv02/item_stand.h"
#include "adv02/texlists.h"
#include "collision/collision.h"
#include "collision/world.h"
#include "debug/editor.h"
#include "debug/print.h"
#include "draw/draw.h"
#include "draw/model.h"
#include "object/models.h"
#include "object/placement.h"
#include "player/player.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

/* @hack type: declared an array, not the one NJS_OBJECT it is, so SHC pools
 * its address once, at 0x0c903b98, and reads pos and model through it, ADD
 * #8,R13 at 0x0c903a42 and MOV.L @(4,R4),R4 at 0x0c903a56; declared as one,
 * 144 of item_stand_draw's 178 words differ */
extern NJS_OBJECT item_stand_object[];
extern NJS_OBJECT item_stand_glow_object;
/* the item's place: it has no model, and its position is where the item
 * turns */
/* @hack type: declared an array, not the one NJS_OBJECT it is, so SHC pools
 * its address at 0x0c903ba8 and reaches pos through it, ADD #8,R11 at
 * 0x0c903a84; declared as one, 112 of item_stand_draw's 178 words differ */
extern NJS_OBJECT item_stand_item_object[];

/* placement_try_leave_by ends the task once the camera and player 0 are
 * both beyond 500 */
#define RANGE (500.0F * 500.0F)

/* the steps the item takes to fade after a touch, and then the steps it stays
 * gone. work->timer counts them */
#define FADE_STEPS 120
#define GONE_STEPS 180

/* the collision world tests the stand only while a player is this near */
#define NEAR 50.0F

#define RADIUS 50.0F /* the collision record's radius */
#define STAND_FLAGS WORLD_SMALL

#define ITEMS 2

enum item_stand_mode { ITEM_STAND_INIT, ITEM_STAND_RUN };

/* the item's states, in work->submode */
enum item_stand_state { ITEM_SHOWN, ITEM_FADING, ITEM_GONE };

struct item_stand_vars {
    NJS_OBJECT *stand; /* the stand's copy in the collision world */
    Sint16 spin; /* the item's y angle */
};

#define VARS(work) ((struct item_stand_vars *)&(work)->vars)
#define ITEM(work) ((work)->scl.x)
#define SPIN_SPEED(work) ((work)->scl.y)

struct item_stand_item {
    NJS_MODEL *model;
    Float radius; /* its collision sphere's radius */
};

static void item_stand_init(struct task *task);
static void item_stand_reset(struct task *task);
static void item_stand_add_stand(struct task *task);
static void item_stand_update(struct task *task);
static void item_stand_draw(struct task *task);
static void item_stand_destroy(struct task *task);

static struct item_stand_item item_stand_items[ITEMS] = {
    {&switch_base_model, 10.0F},
    {&switch_button_model, 5.0F},
};

/* reset sets the center and radius from the item */
static struct collision_shape item_stand_shape[1] = {
    {SHAPE_SPHERE,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 0.0F, 0.0F},
     5.0F,
     0.0F,
     0.0F,
     {0},
     0,
     NULL},
};

/* while the set editor holds the stand, each step rereads the item from scl.x
 * and rebuilds the stand's world object, so it follows the edit */
void item_stand_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave_by(task, RANGE)) {
        return;
    }
    if (!placement_editing(task)) {
        switch (work->mode) {
        case ITEM_STAND_INIT:
            item_stand_init(task);
            break;
        case ITEM_STAND_RUN:
            item_stand_draw(task);
            item_stand_update(task);
            break;
        }
    } else {
        switch (work->mode) {
        case ITEM_STAND_INIT:
            item_stand_init(task);
            break;
        case ITEM_STAND_RUN:
            item_stand_destroy(task);
            item_stand_reset(task);
            item_stand_draw(task);
            item_stand_update(task);
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Y), "-> ROTATE Y");
            debug_print(EDITOR_LABEL(EDITOR_ROW_SCL_X), "-> ITEM NO(0-%d)",
                        ITEMS - 1);
            debug_print(EDITOR_LABEL(EDITOR_ROW_SCL_Y), "-> ITEM ROUND SPEED");
            break;
        }
    }
}

static void item_stand_init(struct task *task)
{
    struct task_work *work = task->work;

    task->draw = item_stand_draw;
    task->destructor = item_stand_destroy;
    VARS(work)->spin = 0;
    collision_init(task, item_stand_shape, COLLISION_OBJECT, 1, 1);
    work->collision->radius = RADIUS;
    item_stand_reset(task);
    work->mode = ITEM_STAND_RUN;
}

static void item_stand_reset(struct task *task)
{
    struct task_work *work = task->work;

    ITEM(work) = (int)ITEM(work);
    if (ITEM(work) >= ITEMS) {
        ITEM(work) = ITEMS - 1;
    } else if (ITEM(work) < 0.0F) {
        ITEM(work) = 0.0F;
    }
    item_stand_add_stand(task);
    work->collision->shapes->center.x =
        item_stand_object->pos[0] + item_stand_item_object->pos[0];
    work->collision->shapes->center.y =
        item_stand_object->pos[1] + item_stand_item_object->pos[1];
    work->collision->shapes->center.z =
        item_stand_object->pos[2] + item_stand_item_object->pos[2];
    work->collision->shapes->radius = item_stand_items[(int)ITEM(work)].radius;
    work->submode = ITEM_SHOWN;
}

static void item_stand_add_stand(struct task *task)
{
    struct task_work *work = task->work;
    struct task_force *force = task->force;
    NJS_OBJECT *object;
    NJS_OBJECT *from;

    force->turn.x = 0;
    force->turn.y = 0;
    force->turn.z = 0;
    force->speed.x = 0.0F;
    force->speed.y = 0.0F;
    force->speed.z = 0.0F;
    from = item_stand_object;
    VARS(work)->stand = object = world_object_alloc();
    object->evalflags = from->evalflags;
    object->scl[0] = 1.0F;
    object->scl[1] = 1.0F;
    object->scl[2] = 1.0F;
    object->ang[0] = 0;
    object->ang[1] = work->ang.y;
    object->ang[2] = 0;
    object->pos[0] = from->pos[0] + work->pos.x;
    object->pos[1] = from->pos[1] + work->pos.y;
    object->pos[2] = from->pos[2] + work->pos.z;
    object->model = from->model;
    object->child = NULL;
    object->sibling = NULL;
    world_add(STAND_FLAGS, task, object);
}

static void item_stand_update(struct task *task)
{
    struct task_work *work = task->work;
    struct collision_work *collision = work->collision;

    if ((collision->flags & COLLISION_HIT) &&
        collision->hit->kind == COLLISION_PLAYER &&
        work->submode == ITEM_SHOWN) {
        work->submode = ITEM_FADING;
        TASK_TIMER(work) = 0;
    }
    if (work->submode == ITEM_FADING) {
        TASK_TIMER(work)++;
        if (TASK_TIMER(work) >= FADE_STEPS) {
            work->submode = ITEM_GONE;
            TASK_TIMER(work) = 0;
        }
    } else if (work->submode == ITEM_GONE) {
        TASK_TIMER(work)++;
        if (TASK_TIMER(work) >= GONE_STEPS) {
            work->submode = ITEM_SHOWN;
        }
    }
    if (player_in_range(&work->pos, NEAR)) {
        work->flags |= WORLD_TESTED;
    } else {
        work->flags &= ~WORLD_TESTED;
    }
    if (work->submode != ITEM_GONE) {
        collision_register(work);
    }
    VARS(work)->spin += SPIN_SPEED(work);
}

static void item_stand_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    njSetTexture(&mrobj_texlist);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    njTranslate(NULL, item_stand_object->pos[0], item_stand_object->pos[1],
                item_stand_object->pos[2]);
    draw_model(item_stand_object->model);
    if (work->submode == ITEM_SHOWN) {
        njPushMatrix(NULL);
        njTranslate(NULL, item_stand_glow_object.pos[0],
                    item_stand_glow_object.pos[1],
                    item_stand_glow_object.pos[2]);
        draw_model(item_stand_glow_object.model);
        njPopMatrix(1);
        njTranslate(NULL, item_stand_item_object->pos[0],
                    item_stand_item_object->pos[1],
                    item_stand_item_object->pos[2]);
        TURN_VIA(njRotateY, VARS(work)->spin, ang);
        texture_set_regular();
        draw_model(item_stand_items[(int)ITEM(work)].model);
    } else if (work->submode == ITEM_FADING) {
        Float fade = TASK_TIMER(work) * (1.0F / FADE_STEPS);
        Float alpha = -(fade * fade * fade);

        draw_set_material(alpha, 0.0F, 0.0F, 0.0F);
        njPushMatrix(NULL);
        njTranslate(NULL, item_stand_glow_object.pos[0],
                    item_stand_glow_object.pos[1],
                    item_stand_glow_object.pos[2]);
        draw_model(item_stand_glow_object.model);
        njPopMatrix(1);
        njTranslate(NULL, item_stand_item_object->pos[0],
                    item_stand_item_object->pos[1],
                    item_stand_item_object->pos[2]);
        TURN_VIA(njRotateY, VARS(work)->spin, ang);
        texture_set_regular();
        draw_model(item_stand_items[(int)ITEM(work)].model);
        draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
    }
    njPopMatrix(1);
}

static void item_stand_destroy(struct task *task)
{
    struct task_work *work = task->work;

    world_remove(task, VARS(work)->stand);
    world_object_free(VARS(work)->stand);
}
