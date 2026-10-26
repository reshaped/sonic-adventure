/* the Egg Hornet's jets: flames at its three nozzles that grow, shrink and
 * stretch as it boosts */
#ifndef B_EGM1_JETS_H
#define B_EGM1_JETS_H
#include <ninja.h>
#include <sg_xpt.h>

struct task;

/* the flames' textures and the glows', which eggmobile1_pvms loads as
 * egm1jet and egm1jetb */
extern NJS_TEXLIST egm1jet_texlist;
extern NJS_TEXLIST egm1jetb_texlist;

/* what each step of the jets does to their size and stretch, by mode.
 * stretch moves only each flame model's first point, between its short
 * shape and its long one */
enum jets_mode {
    JETS_OFF, /* stretch back to 0, then shrink to nothing */
    /* below full size, grow unstretched. at full size, stretch back to 0 */
    JETS_ON,
    JETS_BOOST, /* grow to full size, then stretch to 1 */
    JETS_HOLD /* stay as they are */
};

/* make jets for the boss *hornet holds, at full size and unstretched in
 * JETS_HOLD. the boss's task must outlive them, with a struct egg_hornet in
 * its extra. they change the flame models' points and textures in place,
 * and put the points back as they end. their record's and points' heap
 * blocks are unchecked. egg_hornet_jets_set_mode and egg_hornet_jets_done_steps
 * act on the jets made last: call them only between egg_hornet_jets_create
 * and egg_hornet_jets_kill */
void egg_hornet_jets_create(struct task **hornet);
/* end the jets made last on their next step */
void egg_hornet_jets_kill(void);
/* does nothing. eggmobile1_start and viewer_boss_init each call it after
 * fun_0c900bc0 */
void fun_0c904188(void);
/* set the jets' mode, and restart the count egg_hornet_jets_done_steps returns
 */
void egg_hornet_jets_set_mode(enum jets_mode mode);
/* the steps since the jets came to what their mode asks. it stays 0 in
 * JETS_HOLD */
Uint32 egg_hornet_jets_done_steps(void);

#endif
