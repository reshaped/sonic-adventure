/* the Chaos Emerald the object list names O CHAOS EMERALD: a solid emerald in
 * one of seven colors */
#include <ninja.h>

#include "adv02/chaos_emerald.h"
#include "adv02/texlists.h"
#include "collision/world.h"
#include "debug/editor.h"
#include "debug/print.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "player/player.h"
#include "system/task.h"
#include "system/util.h"

/* each color's model, by the number the set editor's COLOR row shows */
extern NJS_OBJECT mystic_ruins_emerald_0_object;
extern NJS_OBJECT mystic_ruins_emerald_1_object;
extern NJS_OBJECT mystic_ruins_emerald_2_object;
extern NJS_OBJECT mystic_ruins_emerald_3_object;
extern NJS_OBJECT mystic_ruins_emerald_4_object;
extern NJS_OBJECT mystic_ruins_emerald_5_object;
extern NJS_OBJECT mystic_ruins_emerald_6_object;

#define EMERALDS 7

/* the square of the distance from the camera or player 0 within which the
 * emerald stays */
#define EMERALD_RANGE (500.0F * 500.0F)
/* the reach within which a player has the collision world test the
 * emerald */
#define EMERALD_NEAR 30.0F

enum chaos_emerald_mode { EMERALD_INIT, EMERALD_RUN };

/* the emerald's color, from 0 to EMERALDS - 1, kept in work->tick */
#define EMERALD(work) ((work)->tick)

struct emerald_vars {
    /* the copy of the color's model that the emerald puts in the collision
     * world */
    NJS_OBJECT *object;
};

#define VARS(work) ((struct emerald_vars *)&(work)->vars)

static void mystic_ruins_chaos_emerald_init(struct task *task);
static void mystic_ruins_chaos_emerald_reset(struct task *task);
static void mystic_ruins_chaos_emerald_add_to_world(struct task *task);
static void mystic_ruins_chaos_emerald_update(struct task *task);
static void mystic_ruins_chaos_emerald_draw(struct task *task);
static void mystic_ruins_chaos_emerald_destroy(struct task *task);
static void mystic_ruins_chaos_emerald_remove(struct task *task);

/* each color's model, as an action with no motion */
static NJS_ACTION mystic_ruins_chaos_emeralds[EMERALDS] = {
#include "adv02/chaos_emerald/mystic_ruins_chaos_emeralds.inc"
};

/* the emeralds placed and not yet destroyed */
static int mystic_ruins_chaos_emerald_count = 0;

/* while the set editor holds the emerald, each step puts its model in the
 * collision world again. the color and pose come from its work, where the
 * editor leaves them. the step then labels two of the editor's rows */
void mystic_ruins_chaos_emerald_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave_by(task, EMERALD_RANGE)) {
        return;
    }
    if (!placement_editing(task)) {
        switch (work->mode) {
        case EMERALD_INIT:
            mystic_ruins_chaos_emerald_init(task);
            break;
        case EMERALD_RUN:
            mystic_ruins_chaos_emerald_draw(task);
            mystic_ruins_chaos_emerald_update(task);
            break;
        }
    } else {
        switch (work->mode) {
        case EMERALD_INIT:
            mystic_ruins_chaos_emerald_init(task);
            break;
        case EMERALD_RUN:
            mystic_ruins_chaos_emerald_remove(task);
            mystic_ruins_chaos_emerald_reset(task);
            mystic_ruins_chaos_emerald_draw(task);
            mystic_ruins_chaos_emerald_update(task);
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Y), "-> ROTATE Y");
            debug_print(EDITOR_LABEL(EDITOR_ROW_SCL_X), "-> COLOR (0-%d)",
                        EMERALDS - 1);
            break;
        }
    }
}

static void mystic_ruins_chaos_emerald_init(struct task *task)
{
    struct task_work *work = task->work;
    int i;

    /* the first emerald placed runs a loop over the emeralds that does
     * nothing (0x0c90917a to 0x0c90917e) */
    if (mystic_ruins_chaos_emerald_count == 0) {
        for (i = 0; i < EMERALDS; i++) {
        }
    }
    mystic_ruins_chaos_emerald_count++;
    task->draw = mystic_ruins_chaos_emerald_draw;
    task->destructor = mystic_ruins_chaos_emerald_destroy;
    mystic_ruins_chaos_emerald_reset(task);
    work->mode = EMERALD_RUN;
}

/* scl.x picks the color, and a color past the last becomes the last. the
 * color's model then goes in the collision world */
static void mystic_ruins_chaos_emerald_reset(struct task *task)
{
    struct task_work *work = task->work;

    EMERALD(work) = work->scl.x;
    if (EMERALD(work) >= EMERALDS) {
        EMERALD(work) = EMERALDS - 1;
    }
    mystic_ruins_chaos_emerald_add_to_world(task);
}

static void mystic_ruins_chaos_emerald_add_to_world(struct task *task)
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
    from = mystic_ruins_chaos_emeralds[EMERALD(work)].object;
    VARS(work)->object = object = world_object_alloc();
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
    world_add(WORLD_MEDIUM, task, object);
}

static void mystic_ruins_chaos_emerald_update(struct task *task)
{
    struct task_work *work = task->work;

    if (player_in_range(&work->pos, EMERALD_NEAR)) {
        work->flags |= WORLD_TESTED;
    } else {
        work->flags &= ~WORLD_TESTED;
    }
}

static void mystic_ruins_chaos_emerald_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    njSetTexture(&mrobj_texlist);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_tree(mystic_ruins_chaos_emeralds[EMERALD(work)].object);
    njPopMatrix(1);
}

/* the last emerald destroyed runs the loop that does nothing again
 * (0x0c90933a to 0x0c90933e) */
static void mystic_ruins_chaos_emerald_destroy(struct task *task)
{
    int i;

    mystic_ruins_chaos_emerald_count--;
    if (mystic_ruins_chaos_emerald_count == 0) {
        for (i = 0; i < EMERALDS; i++) {
        }
    }
    mystic_ruins_chaos_emerald_remove(task);
}

static void mystic_ruins_chaos_emerald_remove(struct task *task)
{
    struct task_work *work = task->work;

    world_remove(task, VARS(work)->object);
    world_object_free(VARS(work)->object);
}
