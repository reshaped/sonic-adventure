/* Chaos 4, the boss. his work's vars point to his struct chaos4, and his
 * task's extra holds his struct chaos */
#ifndef B_CHAOS4_BOSS_H
#define B_CHAOS4_BOSS_H
#include <ninja.h>
#include <sg_xpt.h>

#include "system/task.h"

/* his work's modes, which a mode's step asks for through boss_next_mode.
 * BOSS_INTRO leads to BOSS_WAIT, and BOSS_WAIT to BOSS_SWIM. BOSS_SWIM
 * leads to the mode choose_attack picks, from BOSS_LEAP to BOSS_SURFACE.
 * each of those leads back to BOSS_SWIM as it ends, but a hit in
 * BOSS_SURFACE leads to BOSS_HIT. BOSS_HIT leads back to BOSS_SWIM too, but
 * for the hit that beats him */
enum chaos4_boss_mode {
    BOSS_WAIT = 1,
    BOSS_SWIM = 2,
    BOSS_LEAP = 3,
    BOSS_SICKLE = 4,
    BOSS_SWEEP = 5,
    BOSS_SPLIT = 6,
    BOSS_SURFACE = 7,
    BOSS_INTRO = 8,
    BOSS_HIT = 9
};

/* the submode a mode's step sets as the mode ends. as his step leaves a
 * mode, it runs the mode's step once more at MODE_DONE, unless the submode
 * is MODE_DONE still. the modes from BOSS_SWIM to BOSS_SURFACE, and
 * BOSS_HIT, set submode 0 in the same step, so they run once more.
 * BOSS_INTRO, BOSS_WAIT and a hit in BOSS_SURFACE leave it at MODE_DONE */
#define MODE_DONE (-1)

/* the actions of his motion, by their index in chaos4_actions. a return to
 * ACTION_REST ends the leap, the sickle and the sweep */
enum chaos4_action {
    ACTION_RISE = 0, /* the sweep's and the surfacing's rise */
    ACTION_REST = 3, /* he rests and swims in it, paced by his speed */
    ACTION_LASH = 4, /* the sickle's lash */
    ACTION_LEAP = 5,
    ACTION_REACH = 6, /* the sweep's reach, which holds its last frame */
    ACTION_BACK = 7, /* the sweep's draw back */
    /* ACTION_LEAP's NJS_ACTION, played once into ACTION_RISE */
    ACTION_INTRO = 8
};

/* at LAST_HEALTH or less, the next hit beats him */
#define LAST_HEALTH 1.0F

/* the sound of his body, a piece of him or a drop falling into the swamp */
#define SE_SPLASH 0x16e
/* the sound of his lash, in the sickle and as the sweep's rise starts */
#define SE_LASH 0x165

/* his record's move. MOVE_HOW's two bits say how his speed moves him each
 * step. MOVE_STILL leaves him in place, and MOVE_DRIFT moves him by his
 * speed. MOVE_TOWARD damps his speed and pulls it toward his struct chaos's
 * target. MOVE_TOWARD_ACROSS damps it the same, but pulls it in x and z
 * alone. MOVE_FALLS pulls his speed down by gravity while he is at or above
 * the swamp's surface */
#define MOVE_HOW 0x3
#define MOVE_STILL 0
#define MOVE_DRIFT 1
#define MOVE_TOWARD_ACROSS 2
#define MOVE_TOWARD 3
#define MOVE_FALLS 0x4

/* his record's turn: what chaos4_turn turns him toward, three tenths of
 * the way each step. TURN_HEADING turns him to his record's heading, and
 * TURN_PLAYER to player 0 */
enum chaos4_turn { TURN_NONE, TURN_HEADING, TURN_PLAYER };

/* his record's draw: the bits of him his draw shows */
#define DRAW_BODY 0x1 /* his body, in his motion's pose */
/* the blob he forms back from, at his work's scale, unless DRAW_BODY is
 * set too */
#define DRAW_BLOB 0x2
#define DRAW_ARM 0x4 /* his arm as the sweep stretches it out */

/* the arm in his sweep has a spring at each joint but the hand's, the
 * sixth */
#define SWEEP_SPRINGS 5

/* his arm in the sweep, whose springs chaos4_sweep_draw moves once a
 * frame. each spring's joint stands off its rest by turn, and turn changes
 * by spin each frame. stretch scales the arm along its length. a kick runs
 * down the joints. kick is the joint it has reached, and kick_timer counts
 * down there each frame. on the frame the count passes below 0, the kick
 * adds to that joint's spin and moves on. both start at 0 with his record,
 * and nothing resets them, so the kick runs once a fight */
struct chaos4_arm {
    struct angles turn[SWEEP_SPRINGS];
    struct angles spin[SWEEP_SPRINGS];
    Float stretch;
    /* stiffness and damping are set as he starts, to sweep.c's SPRING_PULL
     * and SPRING_KEEP, and read nowhere: the springs use those constants */
    Float stiffness;
    Float damping;
    Sint16 kick_timer;
    Sint16 kick;
};

/* his record, which his work's vars point to. his step allocates it,
 * zeroed, and his destructor frees it */
struct chaos4 {
    Uint8 unk_00[0x10];
    Angle heading; /* the angle TURN_HEADING turns him to */
    Uint8 unk_14[4];
    NJS_VECTOR speed; /* how far he moves a step */
    Uint8 unk_24[2];
    Uint8 move;
    Uint8 turn;
    Uint8 unk_28[0xc];
    struct chaos4_arm arm;
    Uint8 unk_bc[0xc];
    Uint8 draw;
    /* the legs of his swim left before an attack. it starts at 0 with his
     * record. each leg counts it down as it starts, and the leg that takes it
     * to 0 or below stops half way. an attack follows that leg, and the count
     * starts over at boss.c's LEGS. his first swim is thus one half leg */
    Sint8 legs_left;
    Uint8 unk_ca[0x32];
    /* choose_attack's calls since he last surfaced. the call that takes
     * it to boss.c's SURFACE_COUNT, or to SURFACE_COUNT_LAST at LAST_HEALTH
     * or less, sends him to BOSS_SURFACE and starts it over */
    Sint8 attack_count;
    Uint8 unk_fd[3];
};

struct chaos4_vars {
    Uint8 unk_00[2];
    Uint16 leg_timer; /* the steps left in his swim's leg */
    Sint32 timer; /* a count of steps, which each mode keeps as it needs */
    struct chaos4 *record;
};

#define CHAOS4_VARS(work) ((struct chaos4_vars *)&(work)->vars)

/* the count of his collision shapes, and the two rows of
 * chaos4_shape_rows, by which chaos4_set_shapes turns them all off or all
 * on */
#define CHAOS4_SHAPES 7
#define SHAPES_OFF 0
#define SHAPES_ON 1

/* his work's pos and scl, from his first step on. they dangle once his
 * task is destroyed: nothing clears them */
extern NJS_POINT3 *chaos4_pos;
extern NJS_POINT3 *chaos4_scale;
/* the arm's node in his body's model. the sweep hides it while it draws
 * an arm of its own */
extern NJS_OBJECT chaos4_body_arm;

/* start his task, at level 1. its first step sets up the shared Chaos
 * state and his ribbon, and starts the swamp and its pads. player 0 must
 * exist by then. the module must stay loaded while his task or a task he
 * starts runs */
void chaos4_create(void);

/* turn the first CHAOS4_SHAPES shapes of work's collision record on or
 * off, by row SHAPES_ON or SHAPES_OFF. the record must hold that many */
void chaos4_set_shapes(struct task_work *work, int row);

#endif
