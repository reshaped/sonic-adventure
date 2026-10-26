/* the Egg Hornet in the events: Eggman's flier, drawn with him in it, and
 * its jets */
#ifndef EVENT_HORNET_H
#define EVENT_HORNET_H
#include <ninja.h>
#include <sg_xpt.h>

#include "system/task.h"

/* the hornet's textures and Eggman's, from EV_EGGMOBLE1 and EV_EGGMAN_BODY,
 * which the event loads */
extern NJS_TEXLIST event_hornet_texlist;
extern NJS_TEXLIST event_eggman_body_texlist;

/* what the hornet's step does, by its work's mode */
enum hornet_mode {
    HORNET_HOVER, /* bob where it is */
    HORNET_FLY, /* fly to the flight's point, then hover */
    HORNET_TURN, /* turn by the flight's turn each step, then hover */
    HORNET_PLAY, /* play its action on to the last frame, then hover */
    HORNET_SCATTER, /* spread its parts apart, with no end */
    /* play its action back to the first frame, then hover. no function sets
     * it */
    HORNET_REWIND
};

/* what a hornet's flights and turns keep, which event_hornet_create
 * allocates from the heap. destroying the hornet's task frees it */
struct hornet_flight {
    Uint8 unk_00[12];
    /* event_hornet_fly_to clears them, and no code reads them */
    Float unk_0c[6];
    NJS_POINT3 to;
    NJS_POINT3 from;
    struct angles turn; /* added to the angles each step of a turn */
    /* counts a turn's steps down. the turn ends on the step that takes it
     * below 0 */
    Sint32 steps;
    Angle bank; /* the roll about z that steering builds as the hornet flies */
    Float speed; /* at either end of a flight, in units a step */
};

struct hornet_vars {
    Float bob; /* the bob's phase, in degrees */
    Float frame; /* the frame of its action that it draws */
    struct hornet_flight *flight;
};

#define HORNET(work) ((struct hornet_vars *)&(work)->vars)

/* the task of the last hornet event_hornet_create made, or null before it
 * and after event_hornet_kill */
extern struct task *event_hornet_task;

/* empty, and no source calls it */
void fun_0cb82d64(void);
/* make a hornet at x, y and z, turned by ang_x, ang_y and ang_z. it hovers,
 * with its jets in JETS_HOLD. event_hornet_task holds it. the functions
 * declared after event_hornet_kill act on it or its jets. call them only
 * between event_hornet_create and event_hornet_kill */
void event_hornet_create(Float x, Float y, Float z, Angle ang_x, Angle ang_y,
                         Angle ang_z);
/* destroy the hornet and its jets on the hornet's next step, and set
 * event_hornet_task to null. it does nothing while event_hornet_task is
 * null */
void event_hornet_kill(void);
/* fly to x, y and z, then hover there. the hornet steers toward the point as
 * it flies. it climbs or falls with the share of the way it has flown */
void event_hornet_fly_to(Float x, Float y, Float z);
/* play the hornet's action on to its last frame, then hover on that frame.
 * it plays a fifth of a frame a step */
void event_hornet_play(void);
/* put the hornet at x, y and z, turned by ang_x, ang_y and ang_z. its mode
 * and its flight stay as they are */
void event_hornet_place(Float x, Float y, Float z, Angle ang_x, Angle ang_y,
                        Angle ang_z);
/* turn the hornet toward angles x, y and z, then hover. each step adds the
 * difference from its angles at the call, divided by steps. the turn runs
 * steps + 1 steps. the difference isn't wrapped, so a turn can go the long
 * way round. steps must not be 0 */
void event_hornet_turn(Angle x, Angle y, Angle z, int steps);
/* set the jets' mode to JETS_OFF, JETS_ON, JETS_BOOST or JETS_HOLD */
void event_hornet_jets_off(void);
void event_hornet_jets_on(void);
void event_hornet_jets_boost(void);
void event_hornet_jets_hold(void);
/* set the hornet's speed at either end of a flight, in units a step. it
 * flies 1 faster halfway. a new hornet's is 0.1 */
void event_hornet_set_speed(Float speed);
/* spread the hornet's parts apart, with no end. each step moves every part
 * farther out along its offset from its parent. the steps change the model
 * every hornet draws, so a hornet made later draws as scattered */
void event_hornet_scatter(void);

#endif
