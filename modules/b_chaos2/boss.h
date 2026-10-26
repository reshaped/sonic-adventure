/* Chaos 2, the boss: his modes, his actions, his record's bits and his
 * shapes */
#ifndef B_CHAOS2_BOSS_H
#define B_CHAOS2_BOSS_H
#include <ninja.h>
#include <sg_xpt.h>

#include "draw/motion.h"
#include "system/task.h"

/* the radius of the ball he gathers into. drawn this far up, the ball
 * rests on the ground */
#define BALL_RADIUS 19.590952F
/* the speed his leap, the ball's bounces and the burst's drops gain
 * downward each step */
#define GRAVITY 0.027222222F

/* his work->mode, which changes through boss_next_mode */
enum chaos2_boss_mode {
    BOSS_SETUP, /* his first step makes his record */
    BOSS_START, /* he waits while the camera swings round him */
    BOSS_CHASE, /* he walks after player 0 */
    BOSS_HIT, /* struck, he melts into the puddle */
    BOSS_GATHER, /* the puddle gathers into the ball */
    /* nothing in the module asks for it. chaos2_run_mode runs nothing in
     * it */
    BOSS_MODE_5,
    /* nothing in the module asks for it. chaos2_run_mode only sets
     * CHAOS2_BUSY in it */
    BOSS_MODE_6,
    BOSS_LEAP, /* he leaps to the far side of the hall from player 0 */
    BOSS_MELT, /* he melts into the puddle for an event */
    BOSS_FORM, /* he rises out of the puddle */
    BOSS_TO_BALL, /* his figure rounds into the ball */
    BOSS_FROM_BALL, /* the ball turns back into his figure */
    BOSS_STRETCH, /* he stretches his arm out at player 0 */
    BOSS_PUNCH, /* he punches at player 0 nearby */
    BOSS_BOUNCE, /* the ball leaps after player 0 */
    BOSS_SPREAD, /* he spreads the puddle over the floor */
    BOSS_BURST, /* the ball bursts into drops, which crawl back to him */
    BOSS_BARRIER, /* his barrier throws player 0 off */
    BOSS_WAIT, /* his shapes are off while an event has control */
    /* the mode chaos2_event_melt's melt returns to. chaos2_run_mode runs it
     * as BOSS_WAIT */
    BOSS_MODE_19
};

/* the submode in which a mode cleans up. a mode sets it as it ends. before
 * the step changes his mode, it sets MODE_DONE and runs the mode once
 * more, unless the mode has set it already */
#define MODE_DONE (-1)

/* his entries of chaos2_actions. the wind-up plays on into the stretch,
 * and the two stretch actions into each other. each punch is named for the
 * fist shape it strikes with */
enum chaos2_action {
    ACTION_STAND,
    ACTION_WALK,
    ACTION_HIT,
    ACTION_WIND_UP,
    ACTION_STRETCH,
    ACTION_STRETCHED,
    ACTION_PUNCH_6,
    ACTION_PUNCH_10,
    ACTION_BARRIER_UP,
    ACTION_BARRIER_DOWN,
    ACTION_LEAP,
    ACTION_LAND,
    ACTION_FEINT,
    /* his event's entry: ACTION_LAND's NJS_ACTION, at the speed
     * chaos2_event_action gives it */
    ACTION_EVENT
};

extern struct motion_entry chaos2_actions[];

/* bits of his record's flags, beside chaos.h's CHAOS_NO_BEADS and
 * CHAOS_BLINKED */
#define CHAOS2_BUSY 0x2 /* set while he may not raise his barrier */
#define CHAOS2_BARRIER 0x8 /* set while his barrier is up */
/* set as a hit ends. while it is set, chaos2_bounce's leaps change as
 * ball.h says. the bounce clears it at rest */
#define CHAOS2_STRUCK 0x100
/* set once a punch's SE_STRIKE has played, and cleared as the next punch
 * winds up */
#define CHAOS2_PUNCH_HEARD 0x200

/* bits of his record's figures, which chaos2_draw draws */
#define FIGURE_BODY 0x1
#define FIGURE_PUDDLE 0x2 /* the puddle, or his blobs once he is beaten */
#define FIGURE_STRETCH 0x8 /* the stretched arm */
#define FIGURE_BALL 0x10
#define FIGURE_ROUND 0x40 /* chaos2_round_object, placed as his body is */
/* his body melts or forms, so the draw deforms it and fades it */
#define FIGURE_MELTING 0x8000

/* BOSS_SHAPES counts his collision shapes. SHAPE_FIST_6 and SHAPE_FIST_10
 * index his fists, SHAPE_PUDDLE the puddle's cylinder, and SHAPE_CORE his
 * core */
#define BOSS_SHAPES 14
#define SHAPE_FIST_6 6
#define SHAPE_FIST_10 10
#define SHAPE_PUDDLE 12
#define SHAPE_CORE 13

struct chaos2_vars {
    Uint8 unk_00[4];
    /* a count of steps. the rest after a stretch or a punch, the spread's
     * hold and a rounding count it down. BOSS_BOUNCE counts its own steps
     * up in it, and a hit in the bounce sets it to the count that ends the
     * leaps */
    Sint32 timer;
};

#define CHAOS2_VARS(work) ((struct chaos2_vars *)&(work)->vars)

/* chaos2_set_shapes' rows, by the shapes each turns on */
enum chaos2_shapes {
    SHAPES_NONE,
    SHAPES_BODY, /* the eleven balls of his body, shapes 0 to 10 */
    SHAPES_PUDDLE, /* the puddle round his feet */
    SHAPES_CORE,
    SHAPES_MELTED, /* the puddle and the core */
    SHAPES_BARRIER /* the barrier, shape 11 */
};

/* turn each of work's BOSS_SHAPES shapes on or off by row, a chaos2_shapes.
 * SHAPES_MELTED also gives shape 0 the highest guard, and every other row
 * takes its guard away */
void chaos2_set_shapes(struct task_work *work, int row);
/* make his task, at level 2, with a work and a motion */
void chaos2_create(void);

#endif
