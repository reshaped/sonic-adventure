/* the Egg Hornet's flight: it crosses the arena, fires, and dives */
#ifndef B_EGM1_FLIGHT_H
#define B_EGM1_FLIGHT_H
#include <ninja.h>
#include <sg_xpt.h>

#include "b_egm1/egg_hornet.h"

struct task;
struct task_work;

/* the flight's states, in the record's state */
enum flight_state {
    FLIGHT_START, /* takes up a hover where it is, to fire or to dive */
    FLIGHT_FIRE, /* fires a volley, or at the round's start crosses first */
    FLIGHT_RELOAD, /* waits to fire again, until the volleys are fired */
    FLIGHT_CHOOSE, /* waits, then chooses the third to cross to */
    FLIGHT_TURN, /* waits for the turn's action, then turns half round */
    FLIGHT_BOOST, /* waits for the jets to boost */
    FLIGHT_CROSS, /* crosses to another third in an arc */
    FLIGHT_BRAKE, /* slows, and turns to face player 0 */
    FLIGHT_DIVE_WAIT, /* waits for the rock to end */
    FLIGHT_DIVE_POSE, /* waits for the dive's action to hold */
    FLIGHT_DIVE_SHOUT, /* shouts and shows the barrier */
    FLIGHT_DIVE_AIM, /* waits for player 0 to be on the ground */
    FLIGHT_DIVE, /* dives with the barrier showing, until 0.4 of the way */
    FLIGHT_DIVING,
    FLIGHT_LAND,
    FLIGHT_STUCK, /* sways, stuck in the ground */
    FLIGHT_CLIMB_START,
    FLIGHT_CLIMB
};

/* the arena's thirds, by x: ZONE_UPPER past 1008, ZONE_LOWER short of 752,
 * and ZONE_MIDDLE between */
enum flight_zone { ZONE_UPPER, ZONE_MIDDLE, ZONE_LOWER };

/* the y angle at which work faces player 0, from -0xc000 to 0x4000 */
Angle egg_hornet_face_player(struct task_work *work);
/* the same angle, from 0 to 0xffff. only flight.c calls it. SHC expands both
 * its calls, in egg_hornet_flight_step. the image also holds a copy at
 * 0x0c90277c that nothing calls. it isn't static, since SHC would write no
 * copy of a static it expands at every call */
Angle egg_hornet_face_player_unsigned(struct task_work *work);
/* TRUE from the dive's start until the climb ends, while the mode is
 * EGG_HORNET_FIGHT or EGG_HORNET_BEATEN */
Bool egg_hornet_diving(struct task *task);
/* start the flight over, at FLIGHT_START. the round it starts crosses
 * before it fires. work goes unread */
void egg_hornet_flight_reset(struct task *task, struct task_work *work);
/* move the boss one step along its flight. task's extra is its record */
void egg_hornet_flight_step(struct task *task, struct task_work *work);

#endif
