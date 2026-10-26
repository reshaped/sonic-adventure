/* the Egg Hornet's jets in the events: flames that grow, shrink and stretch
 * as the event asks. each flame has a glow */
#ifndef EVENT_JETS_H
#define EVENT_JETS_H
#include <ninja.h>
#include <sg_xpt.h>

#include "system/task.h"

/* what each step of the jets does to their size and stretch, by mode.
 * stretch moves only each flame model's first point, between its short shape
 * and its long one. the other points keep their short shape */
enum jets_mode {
    JETS_OFF, /* stretch back to 0, then shrink to nothing */
    /* below full size, grow unstretched. at full size, stretch back to 0 */
    JETS_ON,
    JETS_BOOST, /* grow to full size, then stretch to 1 */
    JETS_HOLD /* stay as they are */
};

/* the flames' textures and the glows', which the event loads from egm1jet and
 * egm1jetb */
extern NJS_TEXLIST event_jet_texlist;
extern NJS_TEXLIST event_jet_glow_texlist;

/* make jets for the hornet *hornet holds, at full size in JETS_HOLD. they
 * draw at that hornet only while event_hornet_task holds a hornet with a
 * flight, which banks them. they change the flame models' points and
 * textures in place. destroying them puts the points back.
 * event_jets_set_mode and event_jets_done_steps act on the jets made last.
 * call them only between event_jets_create and event_jets_kill */
void event_jets_create(struct task **hornet);
/* destroy the jets made last, at once */
void event_jets_kill(void);
/* empty. event_hornet_create calls it just before event_jets_create */
void fun_0cb83e9c(void);
/* set the jets' mode, and restart the count event_jets_done_steps
 * returns */
void event_jets_set_mode(enum jets_mode mode);
/* the steps since the jets came to what their mode asks. it stays 0 in
 * JETS_HOLD */
int event_jets_done_steps(void);

#endif
