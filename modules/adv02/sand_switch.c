/* O SAND SWITCH button that opens the way to Sand Hill when a player stands on
 * it */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv02/sand_switch.h"
#include "adv02/texlists.h"
#include "collision/collision.h"
#include "debug/editor.h"
#include "debug/print.h"
#include "object/placement.h"
#include "player/player.h"
#include "progress/progress.h"
#include "system/task.h"
#include "system/util.h"

extern NJS_MODEL sand_switch_base_model;
extern NJS_MODEL sand_switch_button_model;

/* the task lives while the camera or player 0 is within 500 units.
 * placement_try_leave_by takes the range squared */
#define RANGE (500.0F * 500.0F)
/* the switch is drawn only while a player is within DRAW_RANGE */
#define DRAW_RANGE 500.0F

/* button_y once the button has sunk, and how far it sinks a step */
#define SUNK (-8.5F)
#define SINK_SPEED 0.4F

/* half the side of the box on the button. the box's center stands that far
 * above the button, so the box rests on it */
#define BOX_HALF 5.0F

/* the task's modes. for a character without a flag the switch is absent, with
 * no draw and no collision */
enum sand_switch_mode { SWITCH_INIT, SWITCH_PRESENT, SWITCH_ABSENT };

/* the button's states, in work->submode */
enum sand_switch_state { BUTTON_UP, BUTTON_SINKING, BUTTON_SUNK };

struct sand_switch_vars {
    Float button_y; /* the button's height, 0 while up and SUNK once sunk */
};

#define VARS(work) ((struct sand_switch_vars *)&(work)->vars)

static void sand_switch_init(struct task *task);
static void sand_switch_update(struct task *task);
static void sand_switch_draw(struct task *task);
static void sand_switch_destroy(struct task *task);
static void sand_switch_set_flag(Bool on);

/* a box resting on the button, which sand_switch_update moves down with it */
static struct collision_shape sand_switch_shape[1] = {
    {SHAPE_RECTANGLE | SHAPE_NO_PUSH,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, BOX_HALF, 0.0F},
     BOX_HALF,
     BOX_HALF,
     BOX_HALF,
     {0},
     0,
     NULL},
};

/* the event flag the switch sets for each character, or -1 for none. only Sonic
 * and Tails have one */
static int sand_switch_flags[CHARACTER_COUNT] = {
    FLAG_SONIC_MR_ENTRANCE_SAND_BOARD,
    -1,
    FLAG_TAILS_MR_ENTRANCE_SAND_BOARD,
    -1,
    -1,
    -1,
    -1,
    -1,
};

void sand_switch_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave_by(task, RANGE)) {
        return;
    }
    if (!placement_editing(task)) {
        switch (work->mode) {
        case SWITCH_INIT:
            sand_switch_init(task);
            break;
        case SWITCH_PRESENT:
            sand_switch_draw(task);
            sand_switch_update(task);
            break;
        }
    } else {
        switch (work->mode) {
        case SWITCH_INIT:
            sand_switch_init(task);
            break;
        case SWITCH_PRESENT:
            sand_switch_draw(task);
            sand_switch_update(task);
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Y), "-> ROTATE Y");
            break;
        }
    }
}

/* the button starts sunk if the flag is set */
static void sand_switch_init(struct task *task)
{
    struct task_work *work = task->work;

    if (sand_switch_flags[player_get_character(0)] >= 0) {
        task->draw = sand_switch_draw;
        task->destructor = sand_switch_destroy;
        collision_init(task, sand_switch_shape, COLLISION_OBJECT, 1, 1);
        /* short of the box, whose farthest corner lies 12.2 units from the
         * task */
        work->collision->radius = 10.0F;
        if (sand_switch_flag()) {
            VARS(work)->button_y = SUNK;
            work->submode = BUTTON_SUNK;
        } else {
            VARS(work)->button_y = 0.0F;
            work->submode = BUTTON_UP;
        }
        work->mode = SWITCH_PRESENT;
    } else {
        work->mode = SWITCH_ABSENT;
    }
}

/* when a player stands on the button, sink it, and set the flag once it is
 * down */
static void sand_switch_update(struct task *task)
{
    struct task_work *work = task->work;
    struct collision_work *collision = work->collision;

    switch (work->submode) {
    case BUTTON_UP:
        if ((work->flags & COLLISION_UNDER) &&
            collision->hit->kind == COLLISION_PLAYER) {
            work->submode = BUTTON_SINKING;
        }
        break;
    case BUTTON_SINKING:
        VARS(work)->button_y -= SINK_SPEED;
        if (VARS(work)->button_y < SUNK) {
            VARS(work)->button_y = SUNK;
            work->submode = BUTTON_SUNK;
            sand_switch_set_flag(TRUE);
        }
        break;
    }
    collision->shapes->center.y = VARS(work)->button_y + BOX_HALF;
    collision_register(work);
}

static void sand_switch_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (player_in_range(&work->pos, DRAW_RANGE) == 0) {
        return;
    }
    njSetTexture(&mrobj_texlist);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    njEasyDrawModel(&sand_switch_base_model);
    njTranslate(NULL, 0.0F, VARS(work)->button_y, 0.0F);
    njEasyDrawModel(&sand_switch_button_model);
    njPopMatrix(1);
}

static void sand_switch_destroy(struct task *task)
{
}

static void sand_switch_set_flag(Bool on)
{
    int flag = sand_switch_flags[player_get_character(0)];

    if (flag >= 0) {
        event_flags[flag] = on;
    }
}

Bool sand_switch_flag(void)
{
    int flag = sand_switch_flags[player_get_character(0)];

    if (flag >= 0) {
        return event_flags[flag];
    }
    return FALSE;
}
