/* O BRIDGE, a bridge that rises out of the way while FLAG_BRIDGE_UP is set */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv0130/bridge.h"
#include "collision/world.h"
#include "draw/draw.h"
#include "object/placement.h"
#include "player/player.h"
#include "progress/progress.h"
#include "sound/sound.h"
#include "system/task.h"
#include "system/texture.h"

extern NJS_ACTION bridge_down_action;
extern NJS_ACTION bridge_down2_action;
extern NJS_ACTION bridge_up_action;
extern NJS_ACTION bridge_up2_action;
extern NJS_ACTION bridge_up3_action;
extern NJS_ACTION bridge_up4_action;
extern NJS_OBJECT bridge_solid_object;

/* the actions' frame at which the bridge stands up, and the frames a step
 * it rises and falls by */
#define BRIDGE_TOP 99.99F
#define BRIDGE_SPEED 0.5F

/* in work->flags, beside WORLD_TESTED. BRIDGE_SHOW_DOWN draws the down
 * actions, and BRIDGE_SHOW_UP the up actions. both are set while the bridge
 * moves */
#define BRIDGE_SHOW_DOWN 0x400
#define BRIDGE_SHOW_UP 0x800

/* the collision world tests the surface while the down actions draw and
 * player 0's y lies between TESTED_LOW and TESTED_HIGH */
#define TESTED_LOW 60.0F
#define TESTED_HIGH 90.0F

#define SE_BRIDGE 706

struct bridge_vars {
    NJS_OBJECT *solid; /* in the collision world */
    NJS_OBJECT *object; /* the object whose place and model solid takes */
    Float frame; /* the actions' frame: 0 down, BRIDGE_TOP up */
};

#define VARS(work) ((struct bridge_vars *)&(work)->vars)

enum bridge_mode {
    BRIDGE_INIT,
    BRIDGE_DOWN,
    BRIDGE_UP,
    BRIDGE_RISE,
    BRIDGE_FALL
};

static void egg_carrier_bridge_destroy(struct task *task);
static void egg_carrier_bridge_draw(struct task *task);
static void bridge_solid_init(struct task *task, struct task_work *work);

/* raise the bridge once FLAG_BRIDGE_UP is set, and lower it once the flag is
 * clear, each with SE_BRIDGE. the step tests the flag only while the bridge
 * stands up or down, so a bridge that starts to move goes all the way */
void egg_carrier_bridge_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return;
    }
    switch (work->mode) {
    case BRIDGE_INIT:
        bridge_solid_init(task, work);
        work->flags |= BRIDGE_SHOW_DOWN;
        work->flags &= ~BRIDGE_SHOW_UP;
        task->destructor = egg_carrier_bridge_destroy;
        task->draw = egg_carrier_bridge_draw;
        work->mode = BRIDGE_DOWN;
        break;
    case BRIDGE_DOWN:
        if (event_flags[FLAG_BRIDGE_UP] == 1) {
            sound_play(SE_BRIDGE, 0, 0, 0);
            work->mode = BRIDGE_RISE;
            work->flags |= BRIDGE_SHOW_UP;
        }
        break;
    case BRIDGE_UP:
        if (event_flags[FLAG_BRIDGE_UP] == 0) {
            sound_play(SE_BRIDGE, 0, 0, 0);
            work->flags |= BRIDGE_SHOW_DOWN;
            work->mode = BRIDGE_FALL;
        }
        break;
    case BRIDGE_RISE:
        VARS(work)->frame += BRIDGE_SPEED;
        if (VARS(work)->frame > BRIDGE_TOP) {
            VARS(work)->frame = BRIDGE_TOP;
            work->mode = BRIDGE_UP;
            work->flags &= ~BRIDGE_SHOW_DOWN;
            work->timer = 0;
        }
        break;
    case BRIDGE_FALL:
        /* @hack spelling: the speed negated and added, -0.5 in the pool at
         * 0x0c9229e8 and an FADD at 0x0c92294a; subtracted, 134 of 161 words
         * differ */
        VARS(work)->frame += -BRIDGE_SPEED;
        if (VARS(work)->frame < 0.0F) {
            VARS(work)->frame = 0.0F;
            work->mode = BRIDGE_DOWN;
            work->flags &= ~BRIDGE_SHOW_UP;
            work->timer = 0;
        }
        break;
    }
    work->flags &= ~WORLD_TESTED;
    if ((work->flags & BRIDGE_SHOW_DOWN) &&
        player_work[0]->pos.y < TESTED_HIGH &&
        player_work[0]->pos.y > TESTED_LOW) {
        work->flags |= WORLD_TESTED;
    }
    egg_carrier_bridge_draw(task);
}

static void egg_carrier_bridge_destroy(struct task *task)
{
    struct task_work *work = task->work;

    if (VARS(work)->solid != NULL) {
        world_remove(task, VARS(work)->solid);
        world_object_free(VARS(work)->solid);
    }
}

static void egg_carrier_bridge_draw(struct task *task)
{
    struct task_work *work = task->work;

    texture_set_stage();
    if (work->flags & BRIDGE_SHOW_DOWN) {
        draw_action(&bridge_down_action, VARS(work)->frame);
        draw_action(&bridge_down2_action, VARS(work)->frame);
    }
    if (work->flags & BRIDGE_SHOW_UP) {
        draw_action(&bridge_up_action, VARS(work)->frame);
        draw_action(&bridge_up2_action, VARS(work)->frame);
        draw_action(&bridge_up3_action, VARS(work)->frame);
        draw_action(&bridge_up4_action, VARS(work)->frame);
    }
}

static void bridge_solid_init(struct task *task, struct task_work *work)
{
    NJS_OBJECT *solid;

    VARS(work)->object = &bridge_solid_object;
    solid = world_object_alloc();
    VARS(work)->solid = solid;
    solid->evalflags = NJD_EVAL_BREAK;
    solid->scl[0] = 1.0F;
    solid->scl[1] = 1.0F;
    solid->scl[2] = 1.0F;
    solid->ang[0] = 0;
    /* the surface turns about y by the object's x angle, not its y angle.
     * both are 0 in bridge_solid_object */
    solid->ang[1] = VARS(work)->object->ang[0];
    solid->ang[2] = 0;
    solid->pos[0] = VARS(work)->object->pos[0];
    solid->pos[1] = VARS(work)->object->pos[1];
    solid->pos[2] = VARS(work)->object->pos[2];
    solid->model = VARS(work)->object->model;
    solid->child = NULL;
    solid->sibling = NULL;
    world_add(WORLD_SOLID, task, solid);
    work->pos = VARS(work)->object->model->center;
}
