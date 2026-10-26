/* an event's props. each is a task in its slot of event_props, with its
 * record, struct prop, from the heap */
#include <ninja.h>
#include <sg_xpt.h>

#include "draw/draw.h"
#include "draw/model.h"
#include "draw/tree.h"
#include "event/event.h"
#include "shared/prop.h"
#include "system/task.h"
#include "system/util.h"

/* a prop's work mode: still, or playing its action */
#define PROP_STILL 0
#define PROP_PLAYING 1

/* a prop's record, which its destructor frees */
struct prop {
    NJS_OBJECT *object; /* drawn while action is -1 */
    NJS_ACTION *actions[EVENT_PROP_ACTIONS];
    Sint32 unk_2c; /* the create clears it, and nothing reads it */
    NJS_TEXLIST *texlist;
    Float speed; /* the action's frames advanced each step */
    Sint32 slot; /* the create sets it, and nothing reads it */
    Sint32 action; /* the index in actions of the one drawn, or -1 */
    Uint8 unk_40[4];
};

struct prop_vars {
    Float frame;
    /* the plays left, the one under way included. at 0 or below, the action
     * plays without end */
    Sint32 count;
    struct prop *prop;
};

#define VARS(work) ((struct prop_vars *)&(work)->vars)

/* the initializer puts the slots in data, where the image has them. without
 * one, SHC puts them in bss */
static struct task *event_props[EVENT_PROPS] = {NULL};

static void event_prop_init(struct task *task);
static void event_prop_step(struct task *task);
static void event_prop_draw(struct task *task);
static void event_prop_destroy(struct task *task);

void event_prop_create(Float x, Float y, Float z, Angle ang_x, Angle ang_y,
                       Angle ang_z, Float scl_x, Float scl_y, Float scl_z,
                       NJS_OBJECT *object, NJS_TEXLIST *texlist, int slot)
{
    struct task_work *work;
    struct prop *prop;
    int i;

    event_props[slot] = task_create(TASK_WORK, 5, event_prop_init);
    prop = heap_alloc(sizeof(struct prop));
    work = event_props[slot]->work;
    VARS(work)->prop = prop;
    work->mode = PROP_STILL;
    work->pos.x = x;
    work->pos.y = y;
    work->pos.z = z;
    work->ang.x = ang_x;
    work->ang.y = ang_y;
    work->ang.z = ang_z;
    work->scl.x = scl_x;
    work->scl.y = scl_y;
    work->scl.z = scl_z;
    VARS(work)->frame = 0.0F;
    prop->object = object;
    prop->texlist = texlist;
    prop->slot = slot;
    prop->action = -1;
    prop->unk_2c = 0;
    for (i = 0; i < EVENT_PROP_ACTIONS; i++) {
        prop->actions[i] = NULL;
    }
    event_alloc(work);
}

void event_prop_set_action(int slot, NJS_ACTION *action, int number)
{
    struct task_work *work = event_props[slot]->work;
    struct prop *prop = VARS(work)->prop;

    prop->actions[number] = action;
}

void event_prop_play(int slot, int number, Float speed, int count)
{
    struct task_work *work = event_props[slot]->work;
    struct prop *prop = VARS(work)->prop;

    prop->action = number;
    prop->speed = speed;
    VARS(work)->count = count;
    VARS(work)->frame = 0.0F;
    work->mode = PROP_PLAYING;
}

void event_prop_stop(int slot)
{
    struct task_work *work = event_props[slot]->work;
    struct prop *prop = VARS(work)->prop;

    work->mode = PROP_STILL;
    prop->action = -1;
}

struct task *event_prop_get(int slot)
{
    return event_props[slot];
}

void event_prop_kill(int slot)
{
    if (event_props[slot] != NULL) {
        task_kill(event_props[slot]);
        event_props[slot] = NULL;
    }
}

static void event_prop_init(struct task *task)
{
    task->step = event_prop_step;
    task->draw = event_prop_draw;
    task->destructor = event_prop_destroy;
}

/* advance a playing action by speed. at the action's end, its frame goes
 * back to 0. the prop stands still there once its plays are done. then run
 * the prop's paths, and draw it */
static void event_prop_step(struct task *task)
{
    struct task_work *work = task->work;
    struct prop *prop = VARS(work)->prop;

    switch (work->mode) {
    case PROP_STILL:
        break;
    case PROP_PLAYING:
        VARS(work)->frame += prop->speed;
        if (VARS(work)->frame >=
            (Float)prop->actions[prop->action]->motion->nbFrame) {
            VARS(work)->frame = 0.0F;
            if (VARS(work)->count >= 0) {
                VARS(work)->count--;
                if (VARS(work)->count == 0) {
                    work->mode = PROP_STILL;
                }
            }
        }
    }
    if (work->event->paths != NULL) {
        event_paths_step(work);
    }
    event_prop_draw(task);
}

static void event_prop_draw(struct task *task)
{
    struct task_work *work = task->work;
    struct prop *prop = VARS(work)->prop;
    Angle ang;

    njSetTexture(prop->texlist);
    draw_set_palette(DRAW_PLAYER_PALETTE);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    TURN_VIA(njRotateY, work->ang.y, ang);
    njScaleV(NULL, &work->scl);
    if (prop->action >= 0) {
        draw_action(prop->actions[prop->action], VARS(work)->frame);
    } else {
        draw_tree(prop->object);
    }
    njPopMatrix(1);
    draw_set_palette(0);
}

static void event_prop_destroy(struct task *task)
{
    struct task_work *work = task->work;
    struct prop *prop = VARS(work)->prop;

    if (prop != NULL) {
        heap_free(prop);
    }
}
