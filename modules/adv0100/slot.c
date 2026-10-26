/* O SLOT, a slot machine that pulls its own lever, spins its drum to a stop
 * and plays again */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv0100/slot.h"
#include "collision/collision.h"
#include "debug/editor.h"
#include "debug/print.h"
#include "draw/model.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_MODEL slot_model;
extern NJS_MODEL slot_drum_model;
extern NJS_MODEL slot_lever_model;

struct slot_vars {
    Uint16 lever; /* the lever's turn about x */
    Uint16 drum; /* the drum's turn about x */
    Sint16 spring; /* the lever's turn each step as it springs */
};

#define VARS(work) ((struct slot_vars *)&(work)->vars)

enum slot_mode { SLOT_INIT, SLOT_RUN };
/* the phases of a play, in work->submode */
enum slot_play {
    PLAY_WAIT, /* work->timer counts down to 0 */
    PLAY_PULL, /* the lever goes down */
    PLAY_SPIN, /* the drum spins, slowing */
    PLAY_SETTLE, /* the drum settles onto the next of its faces */
    PLAY_RELEASE, /* the lever comes up */
    PLAY_SPRING /* the lever springs about LEVER_REST */
};

/* the lever's turn at rest */
#define LEVER_REST 0x1000
/* the pull and the release each swing the lever over LEVER_STEPS steps,
 * along a cosine of LEVER_SWING about LEVER_MIDDLE. the pull's angle turns
 * PULL_TURN a step from 0, which takes the lever from LEVER_REST to just
 * short of LEVER_REST + 2 * LEVER_SWING, where the release's first step sets
 * it. the release's angle turns RELEASE_TURN a step, which stops the lever
 * short of rest. the spring then settles it over SPRING_STEPS */
#define LEVER_SWING 4096.0F
#define LEVER_MIDDLE (LEVER_REST + LEVER_SWING)
#define LEVER_STEPS 16
#define PULL_TURN 0x800
#define RELEASE_TURN 0x700
#define SPRING_STEPS 80
/* each step the spring adds SPRING_PULL times the lever's turn back to
 * LEVER_REST to its speed, then keeps SPRING_DAMPING of the speed */
#define SPRING_PULL 3.0F
#define SPRING_DAMPING 0.83F

/* the drum's faces lie DRUM_FACE apart from DRUM_FIRST_FACE, eight to a
 * turn. its spin starts at DRUM_SPIN a step and slows by DRUM_SLOWING a step
 * to DRUM_SETTLE, the speed at which it settles onto the next face */
#define DRUM_FACE 0x2000
#define DRUM_FIRST_FACE 0x1000
#define DRUM_SPIN 0x1800
#define DRUM_SLOWING 16
#define DRUM_SETTLE 0x100

/* the steps each wait after the first counts down */
#define WAIT_STEPS 16

static void slot_init(struct task *task);
static void slot_play(struct task *task);
static void slot_draw(struct task *task);
static void slot_begin(struct task *task);
static void slot_destroy(struct task *task);

/* a box for the machine and a cylinder for its stool */
static struct collision_shape slot_shapes[] = {
    {SHAPE_RECTANGLE,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {2.5F, 39.5F, 3.0F},
     20.75F,
     20.0F,
     15.0F,
     {0},
     0,
     NULL},
    {SHAPE_CYLINDER,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 10.0F, 0.0F},
     12.0F,
     10.0F,
     0.0F,
     {0},
     0,
     NULL},
};

/* draw the machine and play it. the first wait counts down scale x steps, and
 * each later one WAIT_STEPS. while the set editor holds the task, also label
 * the editor's angle rows and its scale x row */
void egg_carrier_slot_step(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_editing(task)) {
        switch (work->mode) {
        case SLOT_INIT:
            slot_init(task);
            break;
        case SLOT_RUN:
            slot_draw(task);
            slot_play(task);
            break;
        }
    } else {
        switch (work->mode) {
        case SLOT_INIT:
            slot_init(task);
            break;
        case SLOT_RUN:
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_X), "-> ROTATE X");
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Y), "-> ROTATE Y");
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Z), "-> ROTATE Z");
            debug_print(EDITOR_LABEL(EDITOR_ROW_SCL_X),
                        "-> INITIAL WAIT VALUE");
            slot_draw(task);
            slot_play(task);
            break;
        }
    }
}

static void slot_init(struct task *task)
{
    struct task_work *work = task->work;

    task->draw = slot_draw;
    task->destructor = slot_destroy;
    slot_begin(task);
    work->mode = SLOT_RUN;
}

static void slot_play(struct task *task)
{
    struct task_work *work = task->work;
    int turn;

    collision_register(work);
    switch (work->submode) {
    case PLAY_WAIT:
        /* @hack spelling: <= 0, which for the unsigned timer is == 0, keeps
         * the image's CMP/PL R0 at 0x0c9276a0; as == 0, 11 of 213 words
         * differ */
        if (TASK_TIMER(work) <= 0) {
            work->submode = PLAY_PULL;
            TASK_TIMER(work) = 0;
        } else {
            TASK_TIMER(work)--;
        }
        break;
    case PLAY_PULL:
        VARS(work)->lever =
            (int)((-njCos(TASK_TIMER(work) * PULL_TURN) * LEVER_SWING) +
                  LEVER_MIDDLE);
        TASK_TIMER(work)++;
        if (TASK_TIMER(work) >= LEVER_STEPS) {
            work->submode = PLAY_SPIN;
            TASK_TIMER(work) = DRUM_SPIN;
        }
        break;
    case PLAY_SPIN:
        /* work->timer holds the drum's speed */
        VARS(work)->drum += TASK_TIMER(work);
        TASK_TIMER(work) -= DRUM_SLOWING;
        if (TASK_TIMER(work) == DRUM_SETTLE) {
            work->submode = PLAY_SETTLE;
            TASK_TIMER(work) = 0;
        }
        break;
    case PLAY_SETTLE:
        /* turn is first how far the drum has turned past its last face, and
         * then how far it has left to turn to the next */
        turn = (VARS(work)->drum - DRUM_FIRST_FACE) & (DRUM_FACE - 1);
        /* @hack spelling: the turn negated, then DRUM_FACE added, which keeps
         * the image's NEG at 0x0c927754; as one subtraction, 4 of 213 words
         * differ */
        turn = -turn;
        turn += DRUM_FACE;
        if (turn <= DRUM_SETTLE) {
            VARS(work)->drum += turn;
            work->submode = PLAY_RELEASE;
            TASK_TIMER(work) = 0;
        } else {
            VARS(work)->drum += DRUM_SETTLE;
        }
        break;
    case PLAY_RELEASE:
        VARS(work)->lever =
            (int)((njCos(TASK_TIMER(work) * RELEASE_TURN) * LEVER_SWING) +
                  LEVER_MIDDLE);
        TASK_TIMER(work)++;
        if (TASK_TIMER(work) >= LEVER_STEPS) {
            /* the spring starts at the release's speed as it ends: the slope
             * of LEVER_SWING cos(RELEASE_TURN t) at t = LEVER_STEPS.
             * RELEASE_TURN, 0x700 of a turn's 0x10000, is 7 pi / 128 radians.
             * so the slope is -sin(RELEASE_TURN t) times 224 pi */
            VARS(work)->spring = (int)(-njSin(LEVER_STEPS * RELEASE_TURN) *
                                       (224.0F * 3.141592F));
            work->submode = PLAY_SPRING;
            TASK_TIMER(work) = 0;
        }
        break;
    case PLAY_SPRING:
        turn = LEVER_REST - VARS(work)->lever;
        if (turn > 0x8000) {
            turn -= 0x10000;
        } else if (turn < -0x8000) {
            turn += 0x10000;
        }
        VARS(work)->spring += turn * SPRING_PULL;
        VARS(work)->spring *= SPRING_DAMPING;
        VARS(work)->lever += VARS(work)->spring;
        TASK_TIMER(work)++;
        if (TASK_TIMER(work) >= SPRING_STEPS) {
            work->submode = PLAY_WAIT;
            TASK_TIMER(work) = WAIT_STEPS;
        }
        break;
    }
}

static void slot_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_model(&slot_model);
    njPushMatrix(NULL);
    njTranslate(NULL, 0.5F, 48.0F, 13.0F);
    TURN_VIA(njRotateX, VARS(work)->drum, ang);
    draw_model(&slot_drum_model);
    njPopMatrix(1);
    njPushMatrix(NULL);
    njTranslate(NULL, 0.0F, 31.0F, 0.0F);
    TURN_VIA(njRotateX, VARS(work)->lever, ang);
    draw_model(&slot_lever_model);
    njPopMatrix(1);
    njPopMatrix(1);
}

/* set up the machine's collision and its first wait, and stand the drum on
 * its first face. the lever starts at 0, below LEVER_REST, until the first
 * pull */
static void slot_begin(struct task *task)
{
    struct task_work *work = task->work;

    collision_init(task, slot_shapes, COLLISION_OBJECT, 1, 2);
    TASK_TIMER(work) = (int)work->scl.x;
    /* @bug logic: the clamp of a negative first wait tests the timer
     * unsigned, EXTU.W and CMP/PZ at 0x0c927a02 and 0x0c927a04, so it never
     * clamps. a scale x of -n waits 65536 - n steps */
    if (TASK_TIMER(work) < 0) {
        TASK_TIMER(work) = 0;
    }
    VARS(work)->lever = 0;
    VARS(work)->drum = DRUM_FIRST_FACE;
    work->submode = PLAY_WAIT;
}

static void slot_destroy(struct task *task)
{
}
