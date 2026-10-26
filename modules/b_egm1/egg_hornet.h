/* the Egg Hornet: its modes, its actions and its record */
#ifndef B_EGM1_EGG_HORNET_H
#define B_EGM1_EGG_HORNET_H
#include <ninja.h>
#include <sg_xpt.h>

#include "b_egm1/barrier.h"
#include "b_egm1/util.h"
#include "draw/motion.h"

struct task;

/* the boss's modes, in its work's mode */
enum egg_hornet_mode {
    EGG_HORNET_WAIT, /* until the camera has swung round to the boss */
    EGG_HORNET_FIGHT,
    EGG_HORNET_BEATEN, /* until its flight returns to FLIGHT_START */
    EGG_HORNET_WRECK, /* until the wreck is done, with player 0 on the ground */
    EGG_HORNET_CLEAR, /* clears the stage, in one step */
    EGG_HORNET_WRECKED /* the wreck goes on bobbing */
};

/* indexes in egg_hornet_actions. ACTION_DIVE and each lean hold their last
 * frame. ACTION_CLIMB plays the dive's action backward, as each _END plays
 * its lean's. those, the turns and the shudder then go to ACTION_IDLE,
 * which plays over and over. entry 3 holds its last frame, and entry 4
 * plays entry 3's action backward. the fight asks for neither. Eggman plays
 * his own action for each turn and the shudder */
#define ACTION_IDLE 0
#define ACTION_TURN_1 1
#define ACTION_TURN_2 2
#define ACTION_DIVE 5
#define ACTION_CLIMB 6
#define ACTION_LEAN_1 7
#define ACTION_LEAN_1_END 8
#define ACTION_LEAN_2 9
#define ACTION_LEAN_2_END 10
#define ACTION_SHUDDER 11

extern struct motion_entry egg_hornet_actions[];
/* Eggman's model, sitting still, and his actions for the boss's two turns
 * and its shudder */
extern NJS_OBJECT eggman_object;
extern NJS_ACTION eggman_turn_1_action;
extern NJS_ACTION eggman_turn_2_action;
extern NJS_ACTION eggman_shudder_action;

/* the boss's record, which its task's extra holds */
struct egg_hornet {
    struct motion motion;
    /* an enum flight_state, or from EGG_HORNET_WRECK on, an enum
     * wreck_state */
    Sint32 state;
    Sint32 to_zone; /* the enum flight_zone a cross goes to */
    Sint32 from_zone; /* the enum flight_zone a cross leaves */
    /* @hack type: an array of one curve, which flight.c reaches through its
     * address. SHC then forms that address, the record plus 68, for its uses
     * (0x0c902902, 0x0c902904). as one curve, flight.c's code is 64 bytes
     * shorter, and 1313 of the 1329 words of the six functions that use it
     * differ */
    struct curve path[1];
    /* the distance the cross moved the boss last step. the brake moves the
     * boss on at that speed, and slows it */
    Float speed;
    /* how far through the cross, the brake, the dive or the climb it is,
     * from 0 to 1 */
    Float time;
    Float time_step; /* time's change each step of the dive or the climb */
    Float hover_y; /* the height it bobs about */
    Angle bob; /* the phase of its bob */
    Angle pitch; /* work's x angle as the dive starts and the climb ends */
    Angle return_pitch; /* work's x angle as the climb starts */
    /* work's y angle as a cross, the brake or the climb starts. the boss
     * turns from it, and sways about it while stuck */
    Angle heading;
    Float radius; /* half the cross's chord: the arc's radius at its ends */
    Angle arc_start; /* arc at the cross's start */
    Angle arc; /* the cross's angle round the path's middle */
    Angle arc_step; /* arc's change each step */
    /* how far the rock after a brake has run, up to half a turn. the
     * brake's end sets it to 1, and it is 0 with no rock */
    Angle sway;
    Angle tilt_phase; /* the phase of the rock */
    Angle tilt; /* the turn about x the rock adds */
    Angle bank_amount; /* the bank at the cross's middle */
    Angle bank; /* the turn about z the cross adds */
    struct barrier barrier;
    Sint32 timer; /* the steps left of the wreck's waits */
    /* in the flight, the steps left of a wait, or the steps stuck so far.
     * in the wreck, the shudders left, then the steps since the burst */
    Sint16 count;
    /* where the boss is in its round. it is -1 before the first cross, and
     * 0 until the volleys are fired. it is 1 from then until the dive */
    Sint8 round_phase;
    Sint8 volleys; /* the volleys fired in this hover */
    Sint8 volley_limit; /* the volleys of each round: 1, and 1 more each hit */
    /* TRUE once the wreck has burst the boss. the draw then shows only the
     * Egg Mobile and Eggman */
    Sint8 burst;
    Sint8 humming; /* TRUE until the wreck stops the hum */
    Sint8 health;
    Sint8 fell; /* TRUE once player 0 has fallen and been downed */
    Sint8 hinted; /* TRUE once Tikal's hint has shown */
};

/* create the boss at its start, facing player 0, and turn player 0 to face
 * it. create its jets, and start the camera's swing round to it. player 0
 * must exist. with no room for the boss's record, the task still runs, and
 * reads the record through null */
void egg_hornet_create(void);
/* request voice, one of Eggman's lines, unless Tikal's hint is showing */
void egg_hornet_voice(int voice);
/* move the current matrix to the boss's position, and turn it by the
 * boss's angles, tilt and bank. then scale it by 2 */
void egg_hornet_place(struct task *task);

#endif
