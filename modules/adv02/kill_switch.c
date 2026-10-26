/* the switch the object list names O KILL SWITCH: a player carries it, and
 * pressing it blows up the nearest monkey cage within reach */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv02/kill_switch.h"
#include "adv02/monkey_cage.h"
#include "adv02/texlists.h"
#include "collision/collision.h"
#include "debug/editor.h"
#include "debug/print.h"
#include "draw/model.h"
#include "event/script.h"
#include "object/common.h"
#include "object/placement.h"
#include "player/player.h"
#include "player/target.h"
#include "system/task.h"
#include "system/util.h"

#define TEXT_1_LENGTH 55
#define TEXT_2_LENGTH 14
#define TEXT_3_LENGTH 36
#include "adv02/kill_switch/text.inc"

extern NJS_MODEL kill_switch_base_model;
extern NJS_MODEL kill_switch_button_model;

/* placement_try_leave_by ends the task once the camera and player 0 are
 * both beyond 500, unless a player holds it */
#define RANGE (500.0F * 500.0F)

/* a switch that has lain alone this many steps kills itself. work->timer
 * counts them */
#define ALONE_STEPS 1800

#define RADIUS 50.0F /* the collision record's radius */

/* a pressed switch blows up the nearest cage only within this squared
 * distance */
#define CAGE_REACH (60.0F * 60.0F)
/* a squared distance beyond every cage, where the search for the nearest
 * starts */
#define FAR 100000000.0F

#define PRESS_DEPTH 1.0F /* how far the button sinks */
#define PRESS_SPEED 0.1F /* how far the button moves each step */

/* a switch that kill_switch_init doesn't start stays in KILL_SWITCH_IDLE,
 * where its step neither draws nor updates it */
enum kill_switch_mode { KILL_SWITCH_INIT, KILL_SWITCH_RUN, KILL_SWITCH_IDLE };

/* the switch's states, in work->tick */
enum kill_switch_state { SWITCH_ALONE, SWITCH_TOUCHED, SWITCH_HELD };

/* the button's states, in work->submode */
enum kill_switch_button {
    BUTTON_RISING,
    BUTTON_UP,
    BUTTON_PRESSING,
    BUTTON_DOWN
};

struct kill_switch_vars {
    Float press; /* the button's height: 0 when up, -PRESS_DEPTH when down */
};

#define VARS(work) ((struct kill_switch_vars *)&(work)->vars)

static void kill_switch_init(struct task *task);
static void kill_switch_reset(struct task *task);
static void kill_switch_update(struct task *task);
static void kill_switch_draw(struct task *task);
static void kill_switch_destroy(struct task *task);

/* the base's cylinder, then the button's */
static struct collision_shape kill_switch_shapes[2] = {
    {SHAPE_CYLINDER | SHAPE_HURTABLE | SHAPE_NO_PUSH,
     0,
     PUSH_FIRM(PUSH_ALL),
     {0.0F, 4.0F, 0.0F},
     5.0F,
     6.0F,
     0.0F,
     {0},
     0,
     NULL},
    {SHAPE_CYLINDER | SHAPE_HURTABLE | SHAPE_NO_PUSH,
     0,
     PUSH_FIRM(PUSH_ENEMIES | PUSH_OTHERS) | PUSH_ALL,
     {0.0F, 4.0F, 0.0F},
     3.0F,
     5.0F,
     0.0F,
     {0},
     0,
     NULL},
};

struct task_work *kill_switch_work = NULL;

static char *kill_switch_picked_up[3] = {
    TEXT_1,
    NULL,
    NULL,
};
static char *kill_switch_on[2] = {
    TEXT_2,
    NULL,
};
static char *kill_switch_again[2] = {
    TEXT_3,
    NULL,
};
static char *kill_switch_nothing[2] = {
    TEXT_3,
    NULL,
};

void kill_switch_step(struct task *task)
{
    struct task_work *work = task->work;

    if (!(work->flags & CARRY_HELD) && placement_try_leave_by(task, RANGE)) {
        return;
    }
    if (player_get_task(0)->step == NULL) {
        task->step = NULL;
        return;
    }
    if (!placement_editing(task)) {
        switch (work->mode) {
        case KILL_SWITCH_INIT:
            kill_switch_init(task);
            break;
        case KILL_SWITCH_RUN:
            kill_switch_draw(task);
            kill_switch_update(task);
            break;
        }
    } else {
        switch (work->mode) {
        case KILL_SWITCH_INIT:
            kill_switch_init(task);
            break;
        case KILL_SWITCH_RUN:
            kill_switch_reset(task);
            kill_switch_draw(task);
            kill_switch_update(task);
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Y), "-> ROTATE Y");
            break;
        }
    }
}

/* a switch runs only while no other is live and the set editor doesn't hold
 * it */
static void kill_switch_init(struct task *task)
{
    struct task_work *work = task->work;

    work->mode = KILL_SWITCH_IDLE;
    if (kill_switch_work == NULL && !placement_editing(task)) {
        kill_switch_work = work;
        task->draw = kill_switch_draw;
        task->destructor = kill_switch_destroy;
        collision_init(task, kill_switch_shapes, COLLISION_OBJECT, 1, 2);
        work->collision->radius = RADIUS;
        carry_set_kind(work, task->motion, CARRY_KIND_SMALL);
        kill_switch_reset(task);
        work->mode = KILL_SWITCH_RUN;
    }
}

static void kill_switch_reset(struct task *task)
{
    struct task_work *work;

    /* @hack spelling: work set inside the first store, which keeps it in R5,
     * out of task's R4, MOV.L @(32,R4),R5 at 0x0c90543a; set apart, work takes
     * R4 and 4 of kill_switch_reset's 8 words differ */
    VARS(work = task->work)->press = 0.0F;
    work->submode = BUTTON_RISING;
    work->tick = SWITCH_ALONE;
}

static void kill_switch_update(struct task *task)
{
    struct task_work *work = task->work;
    struct collision_work *collision = work->collision;
    struct task_work *cage;
    NJS_VECTOR away;
    Float distance;
    Float nearest_distance;
    int nearest;
    int i;

    if (!(work->flags & CARRY_HELD)) {
        if (work->tick == SWITCH_HELD) {
            player_set_held_kind(0, 0);
        }
        if ((collision->flags & COLLISION_HIT) &&
            collision->hit->kind == COLLISION_PLAYER) {
            if (work->tick != SWITCH_TOUCHED) {
                work->submode = BUTTON_PRESSING;
                work->tick = SWITCH_TOUCHED;
            }
        } else if (work->tick != SWITCH_ALONE) {
            TASK_TIMER(work) = 0;
            work->tick = SWITCH_ALONE;
        } else {
            TASK_TIMER(work)++;
            if (TASK_TIMER(work) >= ALONE_STEPS) {
                task_kill(task);
                return;
            }
        }
    } else if (work->tick != SWITCH_HELD) {
        message_show_timed(kill_switch_picked_up);
        work->submode = BUTTON_RISING;
        work->tick = SWITCH_HELD;
        player_set_held_kind(0, HELD_KILL_SWITCH);
    }
    if (work->submode == BUTTON_PRESSING) {
        VARS(work)->press -= PRESS_SPEED;
        if (VARS(work)->press < -PRESS_DEPTH) {
            VARS(work)->press = -PRESS_DEPTH;
            nearest_distance = FAR;
            for (i = 0; i < MONKEY_CAGES; i++) {
                cage = monkey_cages[i];
                if (cage != NULL) {
                    away = cage->pos;
                    njSubVector(&away, &work->pos);
                    distance = (away.x * away.x) + (away.y * away.y) +
                               (away.z * away.z);
                    if (distance < nearest_distance) {
                        nearest_distance = distance;
                        nearest = i;
                    }
                }
            }
            if (nearest_distance <= CAGE_REACH) {
                if (!monkey_cage_flag(nearest)) {
                    message_show_timed(kill_switch_on);
                    monkey_cage_set_flag(nearest, TRUE);
                } else {
                    message_show_timed(kill_switch_again);
                }
            } else {
                message_show_timed(kill_switch_nothing);
            }
            work->submode = BUTTON_DOWN;
        }
    } else if (work->submode == BUTTON_RISING) {
        VARS(work)->press += PRESS_SPEED;
        if (VARS(work)->press > 0.0F) {
            VARS(work)->press = 0.0F;
            work->submode = BUTTON_UP;
        }
    }
    work->flags |= CARRY_WALLS;
    carry_step_small(task);
}

static void kill_switch_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    njSetTexture(&mrobj_texlist);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_model(&kill_switch_base_model);
    njTranslate(NULL, 0.0F, VARS(work)->press, 0.0F);
    draw_model(&kill_switch_button_model);
    njPopMatrix(1);
}

static void kill_switch_destroy(struct task *task)
{
    kill_switch_work = NULL;
}
