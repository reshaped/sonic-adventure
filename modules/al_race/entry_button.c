/* ENTRY BUTTON, a button on a base of its own, which sinks under player 0
 * and opens the race window */
#include <ninja.h>
#include <sg_xpt.h>

#include "al_race/entry_button.h"
#include "al_race/game.h"
#include "collision/collision.h"
#include "collision/world.h"
#include "debug/editor.h"
#include "draw/model.h"
#include "draw/state.h"
#include "object/placement.h"
#include "object/simple.h"
#include "player/player.h"
#include "player/reset.h"
#include "sound/cue.h"
#include "system/frame.h"
#include "system/pad.h"
#include "system/task.h"
#include "system/task_desc.h"
#include "system/texture.h"
#include "system/util.h"

/* the button and its base */
extern NJS_OBJECT dat_0cbca5c8;
extern NJS_OBJECT dat_0cbca18c;

struct entry_button_objects {
    NJS_OBJECT *button;
    NJS_OBJECT *base;
};

#define OBJECTS(arg) ((struct entry_button_objects *)(arg))

/* the button's record, at its task's motion */
struct entry_button {
    Sint8 mode; /* an enum entry_button_mode */
    /* TRUE from the step it's down until it rises. the button draws ignoring
     * the light while this is set */
    Sint8 down;
    Float top; /* its height when up, where its base stays */
    Float bottom; /* its height when down */
    Float y; /* the height the next step puts it at */
    Float reach; /* its model's diameter */
    NJS_OBJECT *world; /* its copy in the collision world */
    NJS_MODEL *button;
    NJS_MODEL *base;
};

#define BUTTON(task) ((struct entry_button *)(task)->motion)

/* the base's record, at its task's motion */
struct entry_base {
    NJS_OBJECT *world;
    Float reach;
};

#define BASE(task) ((struct entry_base *)(task)->motion)

/* the object the button hands its base's task */
struct entry_base_arg {
    NJS_OBJECT *object;
};

enum entry_button_mode {
    BUTTON_UP, /* until player 0 stands on it, when it holds player 0 still */
    BUTTON_SINKING, /* it sinks BUTTON_SPEED a step, down to bottom */
    /* for one step, which plays SE_BUTTON and calls race_open_race_window */
    BUTTON_DOWN,
    /* until player 0's task has its step and its draw again, which the race
     * window's opening suspends. the button then puts player 0 at its start,
     * lets pad 0 drive it again, and rises at once */
    BUTTON_WAIT
};

/* how far the button sinks, and how far each step */
#define BUTTON_DEPTH 0.7F
#define BUTTON_SPEED 0.1F

#define SE_BUTTON 2

static void entry_button_start(struct task *task, void *arg);
static void entry_button_step(struct task *task);
static void entry_button_draw(struct task *task);
static void entry_button_destroy(struct task *task);
static void entry_base_start(struct task *task, void *arg);
static void entry_base_step(struct task *task);
static void entry_base_destroy(struct task *task);

static struct collision_shape entry_button_shapes[1];

static const struct task_desc entry_button_desc = {
    sizeof(struct entry_button), TASK_WORK | TASK_FORCE, entry_button_start,
    entry_button_step,           entry_button_draw,      entry_button_destroy};
static const struct entry_button_objects entry_button_objects = {&dat_0cbca5c8,
                                                                 &dat_0cbca18c};
static const struct task_desc entry_base_desc = {sizeof(struct entry_base),
                                                 TASK_WORK | TASK_FORCE,
                                                 entry_base_start,
                                                 entry_base_step,
                                                 NULL,
                                                 entry_base_destroy};
static const struct collision_list entry_button_list = {entry_button_shapes, 1,
                                                        40.0F};
static const struct collision_kind entry_button_collision = {COLLISION_OBJECT,
                                                             1};

static struct collision_shape entry_button_shapes[1] = {
    {SHAPE_CYLINDER | SHAPE_IGNORE_OBJECTS | SHAPE_IGNORE_OTHERS,
     0,
     PUSH_FIRM(PUSH_ALL),
     {0.0F, 3.5F, 0.0F},
     8.0F,
     3.5F,
     0.0F,
     {0, 0, 0},
     0,
     NULL},
};

void entry_button_init(struct task *task)
{
    task_apply_desc(task, &entry_button_desc, (void *)&entry_button_objects);
}

static void entry_button_start(struct task *task, void *arg)
{
    NJS_OBJECT *object = OBJECTS(arg)->button;
    NJS_OBJECT *world = object_world_copy(object);
    struct entry_button *button = BUTTON(task);
    NJS_OBJECT *base;
    struct entry_base_arg base_arg;
    struct task *child;
    struct task_work *work;
    NJS_MODEL *model;
    Float y;

    button->world = world;
    if (world == NULL) {
        task_kill(task);
        return;
    }
    base = OBJECTS(arg)->base;
    base_arg.object = base;
    child = task_create_desc_child(task, &entry_base_desc, &base_arg);
    if (child == NULL) {
        task_kill(task);
    } else {
        model = object->model;
        work = task->work;
        if (!placement_editing(task)) {
            object_collision_init(task, &entry_button_list,
                                  &entry_button_collision);
        }
        object_world_place(world, work, 1.0F);
        object_world_add_sized(task, world, WORLD_MOVES | WORLD_SOLID, 1.0F);
        object_force_clear(task->force);
        y = work->pos.y;
        button->mode = BUTTON_UP;
        button->down = FALSE;
        button->top = y;
        button->bottom = y - BUTTON_DEPTH;
        button->y = y;
        button->reach = model->r * 2.0F;
        button->button = model;
        button->base = base->model;
    }
}

static void entry_button_step(struct task *task)
{
    struct entry_button *button;
    struct task_work *work;
    struct task_force *force;
    struct player *player;
    struct task *player_task;
    Float y;

    if (placement_try_leave(task)) {
        return;
    }
    if (!placement_editing(task)) {
        button = BUTTON(task);
        work = task->work;
        force = task->force;
        work->pos.y = button->y;
        switch (button->mode) {
        case BUTTON_UP:
            player = player_get(0);
            if ((work->collision->flags & COLLISION_HIT) &&
                player->ground_task == task) {
                player_hold(0);
                button->mode = BUTTON_SINKING;
            }
            collision_register(work);
            break;
        case BUTTON_SINKING: {
            Float from = work->pos.y;
            Float bottom = button->bottom;

            y = from - BUTTON_SPEED;
            if (y <= bottom) {
                y = bottom;
                button->mode = BUTTON_DOWN;
            }
            button->y = y;
            force->speed.y = y - from;
            break;
        }
        case BUTTON_DOWN:
            sound_play_near(SE_BUTTON, NULL, NULL);
            object_force_clear((struct task_motion *)force);
            race_open_race_window();
            button->down = TRUE;
            button->mode = BUTTON_WAIT;
            break;
        /* @hack redundant: a default beside BUTTON_WAIT's case, which never
         * runs: the step and its init set button->mode only to the four modes
         * the cases name. the switch's BRA at 0x0cb96df8 sends any other mode
         * to the wait's code, as the image does. without it, the BRA goes past
         * the switch, and 1 of 149 words differ */
        case BUTTON_WAIT:
        default:
            player_task = player_get_task(0);
            if (player_task != NULL && player_task->step != NULL &&
                player_task->draw != NULL) {
                player_place_start(0);
                pad_enable_player(0);
                button->down = FALSE;
                button->y = button->top;
                button->mode = BUTTON_UP;
            }
            break;
        }
        object_world_place(button->world, work, 1.0F);
        object_world_test_radius(work, button->reach);
    }
    entry_button_draw(task);
    task_step_children(task);
}

static void entry_button_draw(struct task *task)
{
    struct task_work *work;
    struct entry_button *button;
    struct angles *ang;
    NJS_POINT3 *pos;
    Angle turn;
    Uint32 clear;
    Uint32 set;

    if (frame_step != 0) {
        return;
    }
    work = task->work;
    button = BUTTON(task);
    pos = &work->pos;
    ang = &work->ang;
    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, pos);
    TURN_VIA(njRotateY, ang->y, turn);
    draw_state_save();
    if (button->down) {
        clear = 0;
        set = NJD_FLAG_IGNORE_LIGHT;
    } else {
        clear = NJD_FLAG_IGNORE_LIGHT;
        set = 0;
    }
    draw_constant_attr(clear, set);
    draw_model(button->button);
    draw_state_restore();
    njTranslate(NULL, 0.0F, button->top - pos->y, 0.0F);
    draw_model(button->base);
    njPopMatrix(1);
}

static void entry_button_destroy(struct task *task)
{
    struct entry_button *button = BUTTON(task);

    object_world_free(task, button->world);
}

static void entry_base_start(struct task *task, void *arg)
{
    NJS_OBJECT *object = ((struct entry_base_arg *)arg)->object;
    NJS_OBJECT *world = object_world_copy(object);
    struct entry_base *base = BASE(task);

    base->world = world;
    if (world == NULL) {
        task_kill(task);
    } else {
        object_world_place(world, task->work, 1.0F);
        object_world_add_sized(task, world, WORLD_SOLID, 1.0F);
        object_force_clear(task->force);
        base->reach = object->model->r * 2.0F;
    }
}

static void entry_base_step(struct task *task)
{
    struct task_work *work;
    struct entry_base *base;

    if (!placement_editing(task)) {
        work = task->work;
        base = BASE(task);
        object_world_test_radius(work, base->reach);
    }
}

static void entry_base_destroy(struct task *task)
{
    struct entry_base *base = BASE(task);

    object_world_free(task, base->world);
}
