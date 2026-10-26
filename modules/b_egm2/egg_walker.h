/* the Egg Walker: Eggman's machine on four legs, the boss of the stage */
#ifndef B_EGM2_EGG_WALKER_H
#define B_EGM2_EGG_WALKER_H
#include <ninja.h>
#include <sg_xpt.h>

#include "draw/motion.h"

struct task;

/* the boss's work->mode. egg_walker_step changes it to boss_next_mode, at
 * submode 0, once the two differ */
enum egg_walker_mode {
    EGG_WALKER_INIT,
    EGG_WALKER_INTRO, /* until the camera has swung round it */
    EGG_WALKER_WALK, /* toward its target, picking its attacks */
    EGG_WALKER_IDLE,
    EGG_WALKER_JUMP,
    EGG_WALKER_CANNONS, /* the far attack */
    EGG_WALKER_FLAMES, /* the middle one */
    EGG_WALKER_BOMBS,
    EGG_WALKER_CLOSE, /* the attack for a player close by */
    EGG_WALKER_DEFEAT
};

/* the entries of egg_walker_actions the boss's modes ask for */
#define EGG_WALKER_ACTION_STAND 0
#define EGG_WALKER_ACTION_WALK_LOWER_Z 1
#define EGG_WALKER_ACTION_WALK_HIGHER_Z 2
#define EGG_WALKER_ACTION_JUMP 5
/* the boss's actions as it falls and as it rises again. _01 follows a last
 * stomp by leg 0 or 1, and _23 one by leg 2 or 3 */
#define EGG_WALKER_ACTION_FALL_01 6
#define EGG_WALKER_ACTION_RISE_01 7
#define EGG_WALKER_ACTION_FALL_23 8
#define EGG_WALKER_ACTION_RISE_23 9

/* struct egg_walker's walk. WALK_START picks the way and the pace in one
 * step. the walk then goes toward lower or higher z until it stops */
enum egg_walker_walk { WALK_STOPPED, WALK_START, WALK_LOWER_Z, WALK_HIGHER_Z };

/* struct egg_walker's jump: where the boss jumps to. JUMP_TO_END is the
 * end of the walk farther from it. JUMP_TO_RANDOM is one of jump.c's three
 * spots, picked at random, and a jump to the player follows it. the record
 * starts at JUMP_TO_END, and no step sets JUMP_TO_RANDOM */
enum egg_walker_jump { JUMP_TO_END, JUMP_TO_RANDOM, JUMP_TO_PLAYER };

/* the y the boss stands at. a walk stops once part 0 passes WALK_Z_MIN or
 * WALK_Z_MAX, and WALK_Z_MIDDLE is halfway between */
#define STAND_Y (-1.36F)
#define WALK_Z_MIN 1000.0F
#define WALK_Z_MAX 1400.0F
#define WALK_Z_MIDDLE 1200.0F

#define LEGS 4

/* the boss's record, at its task's extra. it begins with its struct
 * motion, as motion_create requires */
struct egg_walker {
    struct motion motion;
    /* the turret's yaw on the body, and the yaw it turns toward */
    Angle turret_yaw;
    Angle turret_aim;
    /* the cannon's pitch, and the pitch it turns toward */
    Angle cannon_pitch;
    Angle cannon_aim;
    Uint8 unk_48[0xc];
    Float cannon_speed; /* the next missile's */
    /* the steps before the flames give the turret and the nozzle new aims */
    Sint32 turret_hold;
    Sint32 nozzle_hold;
    /* the nozzle's pitch, and the pitch it turns toward */
    Angle nozzle_pitch;
    Angle nozzle_aim;
    /* the turret's sweeps and the nozzle's tips so far. each new aim counts
     * one up first. the count's low bit then picks the side. an even count
     * sweeps or tips one way, and an odd count the other */
    Uint8 turret_sweeps;
    Uint8 nozzle_tips;
    Uint8 unk_6a[2];
    NJS_VECTOR velocity; /* through the air, as it jumps */
    NJS_POINT3 target; /* where it walks or jumps to */
    /* the frames of the walk each step plays. the bombs drop more often
     * at a higher pace */
    Float pace;
    Uint8 walk; /* an egg_walker_walk */
    Uint8 jump; /* an egg_walker_jump */
    /* while alternate_target_x is set, alternate_target moves the target's
     * x across the street as the boss walks. last_alternate_z is the boss's
     * z when the close attack set the flag, and at each move after. the
     * first step that finds the walk stopped clears the flag */
    Uint8 alternate_target_x;
    Uint8 unk_8b;
    Float last_alternate_z;
    Uint8 legs[LEGS]; /* each leg's state, LEG_NONE to LEG_FADE */
    Uint8 unk_94[0x104];
    Float health; /* the hits it has left */
};

/* the states of struct egg_walker's legs. the close attack selects the
 * legs it stomps with. a selected leg glows once it has stomped. a hit on
 * the glow turns the leg to LEG_FADE, whose glow shrinks and gives off
 * lightning */
#define LEG_NONE 0
#define LEG_GLOW 1
#define LEG_SELECTED 2
#define LEG_FADE 3

/* work->vars of the boss, which its modes count in */
struct egg_walker_vars {
    Sint32 count;
    Sint32 timer;
};

#define EGG_WALKER_VARS(work) ((struct egg_walker_vars *)&(work)->vars)

/* turn, a change of angle mod 0x10000, as the short way round: from -0x7fff
 * to 0x8000 */
#define SHORT_TURN(turn) ((turn) <= 0x8000 ? (turn) : (turn) - 0x10000)

/* the submode a mode of the boss sets as it ends. the mode asks for the
 * next one then, and egg_walker_step changes to it in the same step */
#define MODE_DONE (-1)

/* the sound of a bomb puff: at a hit on the cockpit, and about the beaten
 * boss */
#define SE_WALKER_BLAST 0x1e0

/* make the boss's task at level 1, or none when no task is free. the
 * first step that can allocate the boss's record sets the fight up. each
 * step until then tries again. a draw before then, while the game is
 * paused, reads through a null extra */
void egg_walker_create(void);
/* 1 when none of the boss's legs is LEG_GLOW or LEG_SELECTED, else 0.
 * task is the boss's */
int egg_walker_legs_spent(struct task *task);
/* aim the turret at player 0 from part 0, and turn it 0.06 of the short
 * way there. task is the boss's */
void egg_walker_aim_turret(struct task *task);

#endif
