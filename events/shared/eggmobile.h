/* an event's Egg Mobile: Eggman in his craft, which bobs, flies and turns.
 * the calls act on the last craft created */
#ifndef EVENT_EGGMOBILE_H
#define EVENT_EGGMOBILE_H
#include <ninja.h>
#include <sg_xpt.h>

#include "system/task.h"

/* what the craft does, by its work's mode. a move or a turn ends in
 * EGGMOBILE_HOVER */
enum eggmobile_mode {
    EGGMOBILE_HOVER, /* it bobs where it is */
    EGGMOBILE_FLY, /* it moves to a point, bobbing */
    EGGMOBILE_SLIDE, /* it moves to a point without bobbing */
    EGGMOBILE_TURN, /* it turns, bobbing, and its jets go off at the end */
    EGGMOBILE_FLY_TURN, /* EGGMOBILE_FLY and EGGMOBILE_TURN at once */
    EGGMOBILE_STILL /* it neither moves nor bobs */
};

/* what its jets do, by its work's submode */
enum eggmobile_jets {
    EGGMOBILE_JETS_OFF,
    EGGMOBILE_JETS_ALL, /* all three jets glow */
    EGGMOBILE_JETS_MINUS_Z, /* its -z jet glows */
    EGGMOBILE_JETS_PLUS_Z, /* its +z jet glows */
    EGGMOBILE_JETS_GLARE, /* a glare shines back from its nozzle */
    /* six puffs of smoke from its nozzle, five steps apart, then off */
    EGGMOBILE_JETS_PUFF
};

/* create the craft at a place and angles, unless task_create fails. its
 * first step allocates its records: a move record, an event motion and an
 * event record. that step also sets EGGMOBILE_HOVER with its jets off. wait
 * it out before the calls that set or move the craft. its event record has
 * neither EVENT_GROUND nor EVENT_SLOPE */
void event_eggmobile_create(Float x, Float y, Float z, Angle ang_x, Angle ang_y,
                            Angle ang_z);
/* kill the craft, if there is one */
void event_eggmobile_kill(void);
/* set the craft's mode. event_eggmobile_set_jets sets its jets, an enum
 * eggmobile_jets value it takes as an Sint8. each does nothing without a
 * craft */
void event_eggmobile_set_mode(enum eggmobile_mode mode);
void event_eggmobile_set_jets(Sint8 jets);
/* move the craft to x, y and z, 1 / steps of the way a step. the move lasts
 * that many steps. event_eggmobile_fly_to bobs it on the way, and
 * event_eggmobile_slide_to doesn't. the calls from here to set_bob need a
 * craft: they don't check for one */
void event_eggmobile_fly_to(Float x, Float y, Float z, int steps);
void event_eggmobile_slide_to(Float x, Float y, Float z, int steps);
/* turn the craft by x / steps, y / steps and z / steps a step. the turn lasts
 * that many steps. each division truncates, so the turn can fall short of the
 * angles. with quiet 0, a positive y turns the -z jet on, and a negative y the
 * +z one. the jets go off as the turn ends, quiet or not */
void event_eggmobile_turn(Angle x, Angle y, Angle z, int steps, Sint8 quiet);
/* event_eggmobile_fly_to and event_eggmobile_turn at once */
void event_eggmobile_fly_turn(Float x, Float y, Float z, Angle ang_x,
                              Angle ang_y, Angle ang_z, int steps, Sint8 quiet);
/* set the bob's speed, an angle a step, and its height. a value below 0
 * keeps that one as it was */
void event_eggmobile_set_bob(Angle speed, Float height);
/* the last craft's task, or null before the first create and after
 * event_eggmobile_kill */
struct task *event_eggmobile_get_task(void);

#endif
