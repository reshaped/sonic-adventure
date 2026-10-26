/* Chaos 6, the boss, and what the fight's other tasks read and set of him */
#ifndef B_CHAOS6_BOSS_H
#define B_CHAOS6_BOSS_H
#include <ninja.h>
#include <sg_xpt.h>

struct deform;
struct task_work;

/* his modes, as his work's mode holds them and boss_next_mode asks for
 * them */
enum chaos6_boss_mode {
    /* the mode he starts in: his first step makes his record and sets him
     * up */
    BOSS_SETUP = 0,
    BOSS_START = 1, /* he waits for the camera to swing round him */
    BOSS_CHASE = 2, /* he walks after player 0 */
    BOSS_STRIKE = 3, /* he strikes once */
    BOSS_JETS = 4, /* in his jets form, he fires the jets */
    BOSS_INHALE = 5, /* he breathes in */
    BOSS_LEAP = 6, /* he leaps toward player 0 */
    BOSS_LEAP_HOME = 7, /* he leaps to the fight's center */
    BOSS_MELT = 8, /* he melts into his puddle */
    BOSS_FORM = 9, /* he rises out of his puddle */
    BOSS_TRANSFORM = 10, /* from his body to his jets form */
    BOSS_UNTRANSFORM = 11, /* from his jets form to his body */
    BOSS_FROZEN = 12, /* a bomb at his mouth has frozen him */
    BOSS_DEFEATED = 16, /* he has broken: the stage is clear */
    /* he waits with his shapes off: from his setup until the fight begins,
     * and after an event's BOSS_FORM */
    BOSS_WAIT = 17,
    BOSS_MELTED = 18, /* he lies melted, after an event's BOSS_MELT */
    BOSS_WEAVE = 19, /* he zigzags toward player 0 */
    BOSS_MODE_20 = 20,
    BOSS_MODE_21 = 21,
    BOSS_FROG_HOOKED = 22, /* he stands once Big has hooked the frog */
    BOSS_RETREAT = 23 /* he backs away from player 0 */
};

/* the entries of his body's actions, boss.c's body_actions, that his modes
 * ask for */
enum body_entry {
    BODY_STAND = 0,
    BODY_WALK = 2,
    BODY_LEAP_START = 6, /* it plays on into BODY_LEAP */
    BODY_LEAP = 7, /* his leap leaves the ground as it starts */
    BODY_LAND = 10,
    BODY_INHALE_START = 11,
    BODY_INHALING = 12, /* chaos6_inhale_pull pulls while it plays */
    BODY_CATCH = 13, /* it plays on into BODY_THROW */
    BODY_THROW = 14,
    BODY_STRIKE = 15,
    BODY_FREEZE = 16,
    BODY_FROZEN_HIT = 17,
    BODY_INHALE_END = 36, /* his inhale's end, once it has caught nothing */
    BODY_FAST_WALK = 37 /* the walk, at twice BODY_WALK's speed */
};

/* set once Big has caught the frog. chaos6_begin clears it as Big's fight
 * begins */
extern Sint8 chaos6_frog_caught;
/* boss.c alone uses chaos6_deform, chaos6_shape_row, chaos6_inhale_due and
 * chaos6_defense_wait. they're declared here, in this order, because SHC
 * lays out bss by each name's first declaration */
/* the deformer that melts his body */
extern struct deform *chaos6_deform;
/* the row of shape_rows that chaos6_set_shapes last turned on */
extern Sint8 chaos6_shape_row;
/* set while his inhale's turn is due. chaos6_choose_attack alternates it:
 * on its turn he inhales, or below 3 health inhales or chases */
extern Sint8 chaos6_inhale_due;
/* the steps left until a hit can show his guard flash again */
extern Sint16 chaos6_defense_wait;
/* set from the step he lands a BOSS_LEAP leap until his next step begins */
extern Sint8 chaos6_landed;
/* the matrix of node 35, his tail, in his jets form's last pose, from the
 * node into the world. his step sets it while he's in that form */
extern NJS_MATRIX chaos6_tail_matrix;

/* start his task, at level 2, with a work and a motion */
void chaos6_create(void);
/* FALSE while he's in BOSS_START, BOSS_CHASE, BOSS_STRIKE or BOSS_WEAVE, and
 * TRUE in the others */
Bool chaos6_bombs_paused(void);
/* pull work toward his mouth, boss_points[0], while he's in BOSS_INHALE
 * and BODY_INHALING plays. a distance across is in x and z alone. the pull
 * moves work only within 60 degrees of the way he faces. work must also lie
 * nearer than 45 above or below his mouth, and nearer than 250 across.
 * returns TRUE in that mode and action if work began the call within 30 of
 * his mouth across, at any height, and FALSE otherwise */
Bool chaos6_inhale_pull(struct task_work *work);
/* ask for mode through boss_next_mode, unless he's in it already */
void chaos6_set_mode(int mode);

#endif
