/* Chaos 0, the boss: his modes and his record's bits, and what fight.c
 * decides for him */
#ifndef B_CHAOS0_BOSS_H
#define B_CHAOS0_BOSS_H
#include <ninja.h>
#include <sg_xpt.h>

#include "chaos/chaos.h"
#include "draw/motion.h"
#include "system/task.h"

/* his work->mode, which changes through boss_next_mode. chaos0_run_mode
 * runs no mode for the numbers the enum skips */
enum chaos0_boss_mode {
    BOSS_SETUP = 0, /* his first step makes his record */
    BOSS_START = 1, /* he waits for the camera's swing round him to end */
    BOSS_WALK = 2, /* he walks after player 0 */
    BOSS_JUMP = 4, /* he leaps as chaos0_vars.jump says */
    BOSS_PUNCH = 6, /* he stretches his arm out at player 0 */
    BOSS_HIT = 7, /* struck, he melts into the puddle */
    BOSS_MELT = 8, /* he melts into the puddle for an event */
    BOSS_FORM = 9, /* he rises out of the puddle */
    BOSS_SPIN = 11, /* he spins with his arms stretched out */
    BOSS_ON_POLE = 12, /* he punches from a pole while player 0 comes near */
    BOSS_FLOW_AFTER = 15, /* as the puddle, he flows to where player 0 was */
    /* as the puddle, he flows to the far side of the center from player 0 */
    BOSS_FLOW_ACROSS = 16,
    /* a barrier round him throws player 0 off. nothing in the module asks
     * for it */
    BOSS_BARRIER = 18,
    BOSS_FALL = 19, /* struck, he is thrown back from player 0 and falls */
    BOSS_WAIT = 20, /* his shapes are off while an event has control */
    /* the mode chaos0_event_melt's melt returns to. chaos0_run_mode runs
     * it as BOSS_WAIT */
    BOSS_MODE_21 = 21,
    /* nothing in the module asks for it. his camera follows him in it */
    BOSS_MODE_24 = 24
};

/* the submode in which a mode cleans up. a mode sets it as it ends. before
 * the step changes his mode, it sets MODE_DONE and runs the mode once
 * more, unless the mode has set it already */
#define MODE_DONE (-1)

/* chaos0_vars.jump, where BOSS_JUMP leaps to. boss.c's jump_heights gives
 * each value's peak above his start. 0 aims where JUMP_AT_PLAYER does. the
 * jump lands on a pole when JUMP_POLE's bit is set, as it is in
 * JUMP_POLE_AGAIN */
#define JUMP_AT_PLAYER 1 /* halfway to player 0, onto the ground there */
/* onto the pole nearest player 0, with five punches to make from it. a
 * pole that shares an x or a z with his start sends him halfway to player
 * 0 instead */
#define JUMP_POLE 2
#define JUMP_POLE_AGAIN 3 /* the same, with the punches he has left */
#define JUMP_ACROSS 4 /* 60 from the fight's center, across from player 0 */

/* above this height, he is up on a pole. it lies between the ground and
 * y 130, where a pole jump lands */
#define ON_POLE_HEIGHT 60.0F

/* the height above his work's position at which his draw and
 * chaos0_arm_pose put his figure's frame */
#define ORIGIN_HEIGHT 9.5F

struct chaos0_vars {
    Uint8 jump;
    Uint8 unk_01[3];
    union {
        /* BOSS_JUMP's steps flown so far, and the steps its flight takes */
        Sint16 flight[2];
        /* the steps BOSS_FLOW_AFTER has run, or the steps BOSS_FALL's
         * throw has left */
        Sint32 steps;
    } count;
    Uint8 unk_08[4];
};

#define CHAOS0_VARS(work) ((struct chaos0_vars *)&(work)->vars)

/* bits of his record's flags, beside chaos.h's CHAOS_NO_BEADS and
 * CHAOS_BLINKED */
/* set as he walks on the ground and as a jump to the ground ends. it is
 * cleared as he falls off the ground, climbs or jumps. chaos0_attack_due
 * finds an attack only while it is set, and spots mark his way only then */
#define CHAOS0_CAN_ATTACK 0x1
/* set from each strike of his punch until the next wind-up ends or the
 * punch does */
#define CHAOS0_STRIKING 0x2
#define CHAOS0_BARRIER 0x8 /* set while his barrier is up */

/* bits of his record's figures, which chaos0_draw draws */
#define FIGURE_BODY 0x1
#define FIGURE_PUDDLE 0x2 /* the puddle, or his blobs once he is beaten */
/* nothing in the module sets it. the draw hides the beads under it, as
 * under FIGURE_PUDDLE */
#define FIGURE_4 0x4
#define FIGURE_STRETCH 0x8 /* the stretched arm */
/* his body melts or forms, so the draw deforms it */
#define FIGURE_MELTING 0x8000

/* his entries of chaos0_actions */
enum chaos0_action {
    ACTION_STAND = 0,
    ACTION_ON_POLE = 1,
    ACTION_WALK = 2,
    ACTION_WIND_UP = 3,
    ACTION_STRETCH = 4,
    ACTION_STRETCHED = 5,
    ACTION_LEAP = 6,
    ACTION_FALL = 7,
    ACTION_LAND = 9,
    ACTION_LAND_POLE = 10,
    ACTION_HIT = 11,
    ACTION_THROWN = 12,
    ACTION_BARRIER_UP = 14,
    ACTION_BARRIER_DOWN = 15,
    ACTION_SPIN = 16,
    /* the entry his events play. it holds ACTION_BARRIER_UP's NJS_ACTION,
     * at the speed chaos0_event_action sets */
    ACTION_EVENT = 17
};

extern struct motion_entry chaos0_actions[];

/* the stretched arm's figure, which chaos0_arm_pose draws. its arm node
 * stretches along x. dat_0c9262bc and dat_0c926514, two nodes under it at
 * the hand, shrink by as much, so they keep their size */
extern NJS_OBJECT chaos0_stretch_object;
extern NJS_OBJECT chaos0_stretch_arm;
extern NJS_OBJECT dat_0c9262bc;
extern NJS_OBJECT dat_0c926514;

/* his hands' points in the world, as chaos0_arm_pose last found them, and
 * as it found them the call before. chaos0_hands[1] is the punching arm's
 * hand, or the stretched arm's while it shows */
/* @hack type: chaos0_hands declared without its bound of 2; with it,
 * chaos0_spin's code is 64 bytes longer and 174 of its 289 words differ */
extern NJS_POINT3 chaos0_hands[];
extern NJS_POINT3 chaos0_hands_last[];

/* BOSS_SHAPES counts his collision shapes. SHAPE_LOWER, SHAPE_HAND and
 * SHAPE_AROUND index his body's lower sphere, the stretched hand and the
 * cylinder round him */
#define BOSS_SHAPES 5
#define SHAPE_LOWER 1
#define SHAPE_HAND 3
#define SHAPE_AROUND 4

/* chaos0_set_shapes' rows, by the shapes each turns on. BOSS_SPIN widens
 * SHAPE_AROUND as his arms spread */
enum chaos0_row {
    SHAPES_FORMED, /* his body's two spheres */
    SHAPES_PUNCH, /* his body's two spheres and the stretched hand */
    SHAPES_SPIN, /* his body's two spheres and the cylinder */
    SHAPES_BARRIER, /* his body's lower sphere and the guard round him */
    SHAPES_FLAT, /* the cylinder alone, flattened to the puddle */
    SHAPES_MELTING, /* his body's lower sphere */
    SHAPES_NONE
};

/* TRUE when he attacks, and otherwise FALSE. he attacks only while chaos's
 * flags hold CHAOS0_CAN_ATTACK: with player 0 nearer than 30, or nearer
 * than range once boss_attack_checks is past 180. each call adds 1 to the
 * count. a call that finds player 0 in range with the count past 180 sets
 * it back to 0, as the start of each BOSS_WALK does. an attack asks for
 * BOSS_PUNCH unless that is his mode already. player is unread */
Bool chaos0_attack_due(struct task_work *work, struct chaos *chaos,
                       struct task_work *player, Float range);
/* run after each step's mode while the fight is on, to choose his next
 * move by his health. in BOSS_WALK, he asks chaos0_attack_due for an
 * attack, with a range of 80 at 3 and of 120 at 2 or 1. at 3, an attack is
 * two punches, and he walks on after them. at 2 or 1, it is three, and he
 * spins after them. as a spin ends, he walks with player 0 farther than
 * 50, and otherwise jumps across. at 1, every second spin ends in a jump
 * to a pole instead. motion is unread */
void chaos0_choose_attack(struct task_work *work, struct task_motion *motion,
                          struct chaos *record);
/* in his draw, on his frame's matrix: find his hands' points, keeping the
 * last ones in chaos0_hands_last, and turn SHAPE_HAND off. with
 * FIGURE_BODY and FIGURE_STRETCH both set, draw the stretched arm instead.
 * then only its hand, chaos0_hands[1], is found, and SHAPE_HAND turns on
 * there. the stretch sets the record's facing too */
void chaos0_arm_pose(struct task_work *work, struct chaos *record);
/* TRUE if an attack hurt him, as COLLISION_DAMAGED in work's flags says,
 * and otherwise FALSE. a hurt throws player 0 back and clears MOTION_ENDED
 * and MOTION_HOLDING. it asks for BOSS_FALL in any mode's submode 4, such
 * as his punch's strike, or at ON_POLE_HEIGHT or higher. otherwise it asks
 * for BOSS_HIT, and sets boss_return_mode to BOSS_FLOW_ACROSS. chaos is
 * unread */
Bool chaos0_take_hit(struct task_work *work, struct chaos *chaos);
/* turn each of his shapes on or off by row, a chaos0_row, and keep row. a
 * row below 0 takes the kept one. SHAPES_SPIN and SHAPES_FLAT resize
 * SHAPE_AROUND too */
void chaos0_set_shapes(struct task_work *work, int row);
/* make his task, at level 2, with a work and a motion */
void chaos0_create(void);

#endif
